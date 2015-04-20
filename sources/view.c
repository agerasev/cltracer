#include "view.h"

#include <CL/cl.h>
#include <GL/glew.h>
#include <CL/cl_gl.h>

#ifdef __gnu_linux__
#include <GL/glx.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define RAY_DATA_SIZE   9
#define RAY_ORIGIN_SIZE 2
#define CAM_SIZE 13

#define MAX_SOURCE_SIZE (0x100000)

static cl_platform_id platform_id = 0;
static cl_device_id device_id = 0;

static cl_context context;
static cl_command_queue command_queue;

static GLuint gl_texture_id;
static cl_mem cl_image;
static cl_mem ray_data, ray_origin, cam_data;

static cl_program program;
static cl_kernel trace, start;

static int width, height;

static float cam_pos[3] = {0.0f,0.0f,0.0f};
static float cam_ori[9] = 
{
  1,0,0,
  0,0,1,
  0,1,0
};
static float cam_fov = 1.0f;

int initView(int w, int h)
{
	width = w;
	height = h;
	// Load the kernel source code into the array source_str
	FILE *fp;
	char *source_str;
	size_t source_size;
	
	fp = fopen("cl/ray_tracer_kernel.cl","r");
	if (!fp) {
			fprintf(stderr, "Failed to load kernel.\n");
			return -1;
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);
	
	// Get platform and device information
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_DEFAULT, 1, 
					&device_id, &ret_num_devices);
	
	// Create an OpenCL context
	// cl_context context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
	// Create an OpenCL context
	// cl_context context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
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
	context = clCreateContext(properties, count, devices, NULL, 0, 0);
#endif
	
	// Create a command queue
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
	
	float *data = malloc(4*sizeof(float)*width*height);
	int ix, iy;
	for(iy = 0; iy < height; ++iy)
	{
		for(ix = 0; ix < width; ++ix)
		{
			data[4*(ix + iy*width) + 0] = 0.0f;
			data[4*(ix + iy*width) + 1] = 1.0f;
			data[4*(ix + iy*width) + 2] = 0.0f;
			data[4*(ix + iy*width) + 3] = 1.0f;
		}
	}
	
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
	glTexImage2D(gl_texture_target, 0, gl_texture_internal, width, height, 0, gl_texture_format, gl_texture_type, data);
	glBindTexture(gl_texture_target, 0);
	
	// Create a reference mem object in OpenCL from GL texture
	cl_int err;
	cl_image = clCreateFromGLTexture2D(context, CL_MEM_READ_WRITE, gl_texture_target, 0, gl_texture_id, &err);
	if (!cl_image || err != CL_SUCCESS)
	{
		fprintf(stderr,"Failed to create OpenGL texture reference! %d\n", err);
		return -1;
	}
	
	ray_data = clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(float)*RAY_DATA_SIZE*width*height,NULL,&err);
	ray_origin = clCreateBuffer(context,CL_MEM_READ_WRITE,sizeof(int)*RAY_ORIGIN_SIZE*width*height,NULL,&err);
	cam_data = clCreateBuffer(context,CL_MEM_READ_ONLY,sizeof(float)*CAM_SIZE,NULL,&err);
	
	// Create a program from the kernel source
	program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
	
	// Build the program
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if(ret != CL_SUCCESS)
	{
		fprintf(stderr,"clBuildProgram failed\n");
		size_t length;
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &length);
		char *buffer = malloc(sizeof(char)*length);
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, length, buffer, NULL);
		fprintf(stderr,"CL_PROGRAM_BUILD_LOG: \n%s\n",buffer);
		free(buffer);
	}
	
	// Create the OpenCL kernel
	start = clCreateKernel(program, "start", &ret);
	trace = clCreateKernel(program, "trace", &ret);
		
	return 0;
}

void disposeView()
{
	cl_uint ret;
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	
	ret = clReleaseKernel(start);
	ret = clReleaseKernel(trace);
	
	ret = clReleaseProgram(program);
	
	ret = clReleaseMemObject(cam_data);
	ret = clReleaseMemObject(ray_data);
	ret = clReleaseMemObject(ray_origin);
	ret = clReleaseMemObject(cl_image);
	
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
	glDeleteTextures(1,&gl_texture_id);
}

void setFov(float yfov)
{
	cam_fov = yfov;
}

void setSize(int w, int h)
{
	
}

void setCamPos(const float *pos)
{
	cam_pos[0] = pos[0];
	cam_pos[1] = pos[1];
	cam_pos[2] = pos[2];
}

void setCamOri(const float *ang)
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


void loadGeometry(const float *geom, long size)
{
	
}

void loadInstance(const float *map, const unsigned *index, long size)
{
	
}

void render()
{
	cl_uint ret;
	
	// Create buffers
	float cam_array[CAM_SIZE] =
	{
	  cam_pos[0], cam_pos[1], cam_pos[2],
	  cam_ori[0], cam_ori[1], cam_ori[2],
	  cam_ori[3], cam_ori[4], cam_ori[5],
	  cam_ori[6], cam_ori[7], cam_ori[8],
	  cam_fov
	};
	ret = clEnqueueWriteBuffer(command_queue,cam_data,CL_TRUE,0,sizeof(float)*CAM_SIZE, cam_array, 0, NULL, NULL);
	
	clFlush(command_queue);
	
	// Set the arguments of the kernel
	ret = clSetKernelArg(start, 0, sizeof(cl_mem), (void*)&ray_data);
	ret = clSetKernelArg(start, 1, sizeof(cl_mem), (void*)&ray_origin);
	ret = clSetKernelArg(start, 2, sizeof(cl_mem), (void*)&cam_data);
	
	// Execute the OpenCL kernel on the list
	size_t global_work_size[2] = {width,height};
	size_t local_work_size[2] = {8,8};
	
	clEnqueueNDRangeKernel(command_queue,start,2,NULL,global_work_size,local_work_size,0,NULL,NULL);
	
	clFlush(command_queue);
	
	// Print data
	/*
	int *buf = malloc(2*sizeof(int)*width*height);
	ret = clEnqueueReadBuffer(command_queue, ray_origin, CL_TRUE, 0, 2*sizeof(int)*width*height, buf, 0, NULL, NULL);
	int i;
	for(i = 0; i < width*height; ++i)
	{
		fprintf(stdout,"(%d,%d)\n",buf[2*i],buf[2*i + 1]);
	}
	free(buf);
	*/
	
	// Set the arguments of the kernel
	ret = clSetKernelArg(trace, 0, sizeof(cl_mem), (void*)&ray_data);
	ret = clSetKernelArg(trace, 1, sizeof(cl_mem), (void*)&ray_origin);
	ret = clSetKernelArg(trace, 2, sizeof(cl_mem), (void*)&cl_image);
	
	size_t global_size = width*height;
	size_t local_size = 64;
	
	clEnqueueAcquireGLObjects(command_queue, 1, &cl_image, 0, 0, 0);
	clEnqueueNDRangeKernel(command_queue,trace,1,NULL,&global_size,&local_size,0,NULL,NULL);
	clEnqueueReleaseGLObjects(command_queue, 1, &cl_image, 0, 0, 0);
	
	clFlush(command_queue);
}

GLuint getTexture()
{
	return gl_texture_id;
}
