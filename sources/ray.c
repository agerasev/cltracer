#include "ray.h"

#include <CL/cl.h>
#ifdef RAY_GL
#include <GL/glew.h>
#include <CL/cl_gl.h>
#ifdef __gnu_linux__
#include <GL/glx.h>
#endif
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static char *__loadSource(const char *fn, size_t *lenght)
{
	FILE *fp;
	size_t size;
	char *source = NULL;
	
	fp = fopen(fn,"r");
	if(!fp)
	{
			fprintf(stderr,"Failed to load shader: %s\n",fn);
			return NULL;
	}
	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	source = (char*)malloc(sizeof(char)*(size+1));
	fread(source,1,size,fp);
	source[size] = '\0';
	*lenght = size + 1;
	fclose(fp);
	return source;
}

static void __freeSource(char *source)
{
	free(source);
}

static void __printKernelCompilationInfo(cl_program program, cl_device_id device_id)
{
	fprintf(stderr,"clBuildProgram failed\n");
	size_t length;
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &length);
	char *buffer = malloc(sizeof(char)*length);
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, length, buffer, NULL);
	fprintf(stderr,"CL_PROGRAM_BUILD_LOG: \n%s\n",buffer);
	free(buffer);
}

#define CAM_FSIZE 13

#define RAY_FSIZE 9
#define RAY_ISIZE 2

#define HIT_FSIZE 12
#define HIT_ISIZE 3

#define MAX_CHILD_RAYS 2

static cl_platform_id platform_id = 0;
static cl_device_id device_id = 0;

static cl_context context;
static cl_command_queue command_queue;

#ifdef RAY_GL
static GLuint gl_texture_id;
#endif
static cl_mem cl_image;
static cl_mem cam_fdata;
static cl_mem ray_fdata, ray_idata;
static cl_mem hit_fdata, hit_idata;
static cl_mem color_buffer;

static cl_program program;
static cl_kernel start, intersect, produce, draw;

static int width, height;

static float cam_pos[3] = {0.0f,0.0f,0.0f};
static float cam_ori[9] = 
{
  1,0,0,
  0,0,1,
  0,1,0
};
static float cam_fov = 1.0f;

static void __get_platform_and_device()
{
	// Get platform and device information
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_DEFAULT, 1, 
					&device_id, &ret_num_devices);
}

static cl_context __get_context()
{
#ifdef RAY_GL
#ifdef __gnu_linux__
	// Create CL context properties, add GLX context & handle to DC
	cl_context_properties properties[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), // GLX Context
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), // GLX Display
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id, // OpenCL platform
		0
	};

	// Find CL capable devices in the current GL context
	cl_device_id devices[32]; size_t size;
	void *func = clGetExtensionFunctionAddress("clGetGLContextInfoKHR");
	((cl_int(*)(const cl_context_properties*,cl_gl_context_info,size_t,void*,size_t*))func)
	    (properties, CL_DEVICES_FOR_GL_CONTEXT_KHR, 32*sizeof(cl_device_id), devices, &size);
	
	// Create a context using the supported devices
	int count = size / sizeof(cl_device_id);
	return clCreateContext(properties, count, devices, NULL, 0, 0);
#endif // __gnu_linux__
#ifdef WIN32
	// Use WGL context
	return clCreateContext( NULL, 1, &device_id, NULL, NULL, &err);
#endif // WIN32
#else
	// Create an OpenCL context
	return clCreateContext( NULL, 1, &device_id, NULL, NULL, &err);
#endif // CLR_GL
}

static cl_mem __get_image()
{
	cl_int err;
#ifdef RAY_GL
	// Create shared image 
	GLenum 
	  gl_texture_target = GL_TEXTURE_2D,
	  gl_texture_internal = GL_RGBA32F,
	  gl_texture_format = GL_RGBA,
	  gl_texture_type = GL_FLOAT;
	
	// Create a texture in OpenGL and allocate space
	glGenTextures(1, &gl_texture_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(gl_texture_target, gl_texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(gl_texture_target, 0, gl_texture_internal, width, height, 0, gl_texture_format, gl_texture_type, NULL);
	glBindTexture(gl_texture_target, 0);
	
	// Create a reference mem object in OpenCL from GL texture
	return clCreateFromGLTexture2D(context, CL_MEM_READ_WRITE, gl_texture_target, 0, gl_texture_id, &err);
	if (!cl_image || err != CL_SUCCESS)
	{
		fprintf(stderr,"Failed to create OpenGL texture reference! %d\n", err);
		return -1;
	}
#else // RAY_GL
	// create cl_image here
	return clCreateImage2D(context,CL_MEM_READ_WRITE,...,width,height,1,NULL,&err);
#endif // RAY_GL
}

int rayInit(int w, int h)
{
	cl_int err;
	
	width = w;
	height = h;
	
	__get_platform_and_device();
	context = __get_context();
	
	// Create a command queue
	command_queue = clCreateCommandQueue(context, device_id, 0, &err);
	
	cl_image = __get_image();
	
	size_t buf_size = width*height;
	ray_fdata = clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(float)*RAY_FSIZE*buf_size,NULL,&err);
	ray_idata = clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(int)*RAY_ISIZE*buf_size,NULL,&err);
	hit_fdata = clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(float)*HIT_FSIZE*buf_size,NULL,&err);
	hit_idata = clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(int)*HIT_ISIZE*buf_size,NULL,&err);
	cam_fdata = clCreateBuffer(context,CL_MEM_READ_ONLY,sizeof(float)*CAM_FSIZE,NULL,&err);
	color_buffer = clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(unsigned int)*3*buf_size,NULL,&err);
	
	// Create a program from the kernel source
	size_t source_size;
	char *source = __loadSource("cl/ray_tracer_kernel.cl",&source_size);
	program = clCreateProgramWithSource(context, 1, (const char **)&source, (const size_t *)&source_size, &err);
	__freeSource(source);
	
	// Build the program
	err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if(err != CL_SUCCESS)
	{
		__printKernelCompilationInfo(program,device_id);
	}
	
	// Create the OpenCL kernel
	start = clCreateKernel(program, "start", &err);
	intersect = clCreateKernel(program, "intersect", &err);
	produce = clCreateKernel(program, "produce", &err);
	draw  = clCreateKernel(program, "draw" , &err);
		
	return 0;
}

void rayDispose()
{
	cl_uint err;
	clFlush(command_queue);
	clFinish(command_queue);
	
	clReleaseKernel(start);
	clReleaseKernel(intersect);
	clReleaseKernel(produce);
	clReleaseKernel(draw);
	
	clReleaseProgram(program);
	
	clReleaseMemObject(cam_fdata);
	clReleaseMemObject(ray_fdata);
	clReleaseMemObject(ray_idata);
	clReleaseMemObject(hit_fdata);
	clReleaseMemObject(hit_idata);
	clReleaseMemObject(color_buffer);
	clReleaseMemObject(cl_image);
	
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);
	glDeleteTextures(1,&gl_texture_id);
}

void raySetFov(float yfov)
{
	cam_fov = yfov;
}

void raySetSize(int w, int h)
{
	
}

void raySetPos(const float *pos)
{
	cam_pos[0] = pos[0];
	cam_pos[1] = pos[1];
	cam_pos[2] = pos[2];
}

void raySetOri(const float *ang)
{
	float yaw = ang[0];
	float pitch = ang[1];
	
	cam_ori[0] = cos(yaw);
	cam_ori[1] = sin(yaw);
	cam_ori[2] = 0.0f;
	
	cam_ori[3] = sin(yaw)*sin(pitch);
	cam_ori[4] = -cos(yaw)*sin(pitch);
	cam_ori[5] = cos(pitch);
	
	cam_ori[6] = -sin(yaw)*cos(pitch);
	cam_ori[7] = cos(yaw)*cos(pitch);
	cam_ori[8] = sin(pitch);
}


void rayLoadGeometry(const float *geom, long size)
{
	
}

void rayLoadInstance(const float *map, const unsigned *index, long size)
{
	
}

int rayRender()
{
	// Create buffers
	float cam_array[CAM_FSIZE] =
	{
	  cam_pos[0], cam_pos[1], cam_pos[2],
	  cam_ori[0], cam_ori[1], cam_ori[2],
	  cam_ori[3], cam_ori[4], cam_ori[5],
	  cam_ori[6], cam_ori[7], cam_ori[8],
	  cam_fov
	};
	clEnqueueWriteBuffer(command_queue,cam_fdata,CL_TRUE,0,sizeof(float)*CAM_FSIZE, cam_array, 0, NULL, NULL);
	
	// clFlush(command_queue);
	
	
	size_t global_work_size[2] = {width,height};
	size_t local_work_size[2] = {8,8};
	
	
	// start
	clSetKernelArg(start, 0, sizeof(cl_mem), (void*)&ray_fdata);
	clSetKernelArg(start, 1, sizeof(cl_mem), (void*)&ray_idata);
	clSetKernelArg(start, 2, sizeof(cl_mem), (void*)&cam_fdata);
	
	clEnqueueNDRangeKernel(command_queue,start,2,NULL,global_work_size,local_work_size,0,NULL,NULL);
	
	// clFlush(command_queue);
	
	
	size_t global_size = width*height;
	size_t local_size = 64;
	
	int i, depth = 4;
	for(i = 0; i < depth; ++i)
	{
		// intersect
		clSetKernelArg(intersect, 0, sizeof(cl_mem), (void*)&ray_fdata);
		clSetKernelArg(intersect, 1, sizeof(cl_mem), (void*)&ray_idata);
		clSetKernelArg(intersect, 2, sizeof(cl_mem), (void*)&hit_fdata);
		clSetKernelArg(intersect, 3, sizeof(cl_mem), (void*)&hit_idata);
		
		clEnqueueNDRangeKernel(command_queue,intersect,1,NULL,&global_size,&local_size,0,NULL,NULL);
		
		// clFlush(command_queue);
		
		
		// produce
		clSetKernelArg(produce, 0, sizeof(cl_mem), (void*)&hit_fdata);
		clSetKernelArg(produce, 1, sizeof(cl_mem), (void*)&hit_idata);
		clSetKernelArg(produce, 2, sizeof(cl_mem), (void*)&ray_fdata);
		clSetKernelArg(produce, 3, sizeof(cl_mem), (void*)&ray_idata);
		clSetKernelArg(produce, 4, sizeof(cl_mem), (void*)&color_buffer);
		
		clEnqueueNDRangeKernel(command_queue,produce,1,NULL,&global_size,&local_size,0,NULL,NULL);
		
		// clFlush(command_queue);
	}
	
	
	// draw
	clSetKernelArg(draw, 0, sizeof(cl_mem), (void*)&color_buffer);
	clSetKernelArg(draw, 1, sizeof(cl_mem), (void*)&cl_image);
	
#ifdef RAY_GL
	clEnqueueAcquireGLObjects(command_queue, 1, &cl_image, 0, 0, 0);
#endif
	clEnqueueNDRangeKernel(command_queue,draw,2,NULL,global_work_size,local_work_size,0,NULL,NULL);
#ifdef RAY_GL
	clEnqueueReleaseGLObjects(command_queue, 1, &cl_image, 0, 0, 0);
#endif
	
	clFlush(command_queue);
	
	return 0;
}

#ifdef RAY_GL
GLuint rayGetGLTexture()
{
	return gl_texture_id;
}
#endif
