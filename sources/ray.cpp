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

#include <string>
#include <map>

#include "globals.hpp"
#include "buffer_object.hpp"
#include "kernel.hpp"
#include "work_range.hpp"

typedef std::map<std::string,buffer_object*> buffer_map;
typedef std::map<std::string,kernel*> kernel_map;

buffer_map buffers;
kernel_map kernels;

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
	char *buffer = (char*)malloc(sizeof(char)*length);
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, length, buffer, NULL);
	fprintf(stderr,"CL_PROGRAM_BUILD_LOG: \n%s\n",buffer);
	free(buffer);
}

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
		return (cl_mem)-1;
	}
#else // RAY_GL
	// create cl_image here
	return clCreateImage2D(context,CL_MEM_READ_WRITE,...,width,height,1,NULL,&err);
#endif // RAY_GL
}

int rayInit(int w, int h)
{
	cl_int err;
	int i;
	
	width = w;
	height = h;
	
	__get_platform_and_device();
	context = __get_context();
	
	// Create a command queue
	command_queue = clCreateCommandQueue(context, device_id, 0, &err);
	
	cl_image = __get_image();
	
	size_t screen_size = width*height;
	size_t buffer_size = screen_size*MAX_CHILD_RAYS;
	
	buffers.insert(buffer_map::value_type("ray_fdata",new buffer_object(sizeof(float)*RAY_FSIZE*buffer_size)));
	buffers.insert(buffer_map::value_type("ray_idata",new buffer_object(sizeof(int)*RAY_ISIZE*buffer_size)));
	buffers.insert(buffer_map::value_type("hit_fdata",new buffer_object(sizeof(float)*HIT_FSIZE*buffer_size)));
	buffers.insert(buffer_map::value_type("hit_idata",new buffer_object(sizeof(int)*HIT_ISIZE*buffer_size)));
	buffers.insert(buffer_map::value_type("cam_fdata",new buffer_object(sizeof(float)*CAM_FSIZE)));
	buffers.insert(buffer_map::value_type("color_buffer",new buffer_object(sizeof(unsigned int)*3*screen_size)));
	buffers.insert(buffer_map::value_type("accum_buffer",new buffer_object(sizeof(float)*3*screen_size)));
	buffers.insert(buffer_map::value_type("hit_info",new buffer_object(sizeof(unsigned int)*HIT_INFO_SIZE*buffer_size)));
	buffers.insert(buffer_map::value_type("cl_ray_count",new buffer_object(2*sizeof(unsigned int))));
	buffers.insert(buffer_map::value_type("pitch",new buffer_object(sizeof(unsigned int))));
	buffers.insert(buffer_map::value_type("work_size",new buffer_object(sizeof(unsigned int))));
	buffers.insert(buffer_map::value_type("number",new buffer_object(sizeof(unsigned int))));
	buffers.insert(buffer_map::value_type("factor",new buffer_object(sizeof(float))));
	buffers.insert(buffer_map::value_type("cl_random",new buffer_object(sizeof(unsigned int)*buffer_size)));
	
	/*
	ray_fdata = buffers["ray_fdata"]->get_cl_mem();
	ray_idata = buffers["ray_idata"]->get_cl_mem();
	hit_fdata = buffers["hit_fdata"]->get_cl_mem();
	hit_idata = buffers["hit_idata"]->get_cl_mem();
	cam_fdata = buffers["cam_fdata"]->get_cl_mem();
	color_buffer = buffers["color_buffer"]->get_cl_mem();
	accum_buffer = buffers["accum_buffer"]->get_cl_mem();
	hit_info = buffers["hit_info"]->get_cl_mem();
	cl_ray_count = buffers["cl_ray_count"]->get_cl_mem();
	pitch = buffers["pitch"]->get_cl_mem();
	work_size = buffers["work_size"]->get_cl_mem();
	number = buffers["number"]->get_cl_mem();
	factor = buffers["factor"]->get_cl_mem();
	cl_random = buffers["cl_random"]->get_cl_mem();
	*/
	
	unsigned seed = 0;
	unsigned *random_buffer = (unsigned*)malloc(sizeof(unsigned)*buffer_size);
	for(i = 0; i < buffer_size; ++i)
	{
		random_buffer[i] = (seed = 3942082377*seed + 1234567);
	}
	
	clEnqueueWriteBuffer(command_queue,buffers["cl_random"]->get_cl_mem(),CL_TRUE,0,sizeof(unsigned int)*buffer_size,random_buffer,0,NULL,NULL);
	clFlush(command_queue);
	free(random_buffer);
	
	float *accum_buffer_data = (float*)malloc(sizeof(float)*3*screen_size);
	for(i = 0; i < 3*screen_size; ++i)
	{
		accum_buffer_data[i] = 0.0f;
	}
	clEnqueueWriteBuffer(command_queue,buffers["accum_buffer"]->get_cl_mem(),CL_TRUE,0,sizeof(float)*3*screen_size,accum_buffer_data,0,NULL,NULL);
	clFlush(command_queue);
	free(accum_buffer_data);
	
	// Create a program from the kernel source
	size_t source_size;
	char *source = __loadSource("cl/kernel.cl",&source_size);;
	program = clCreateProgramWithSource(context, 1, (const char **)&source, (const size_t *)&source_size, &err);
	if(err != CL_SUCCESS) {fprintf(stderr,"clCreateProgramWithSource: failed\n"); return -3;}
	__freeSource(source);
	
	
	// Build the program
	err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if(err != CL_SUCCESS)
	{
		__printKernelCompilationInfo(program,device_id);
		return -1;
	}
	
	// Create the OpenCL kernel
	kernels.insert(kernel_map::value_type("start",new kernel("start")));
	kernels.insert(kernel_map::value_type("intersect",new kernel("intersect")));
	kernels.insert(kernel_map::value_type("produce",new kernel("produce")));
	kernels.insert(kernel_map::value_type("draw",new kernel("draw")));
	kernels.insert(kernel_map::value_type("compact",new kernel("compact")));
	
	/*
	start = kernels["start"]->get_cl_kernel();
	intersect = kernels["intersect"]->get_cl_kernel();
	produce = kernels["produce"]->get_cl_kernel();
	draw = kernels["draw"]->get_cl_kernel();
	compact = kernels["compact"]->get_cl_kernel();
	*/
		
	return 0;
}

void rayDispose()
{
	// cl_uint err;
	clFlush(command_queue);
	clFinish(command_queue);
	
	for(auto pair : kernels)
	{
		delete pair.second;
	}
	
	clReleaseProgram(program);
	
	for(auto pair : buffers)
	{
		delete pair.second;
	}
	clReleaseMemObject(cl_image);
	
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);
	glDeleteTextures(1,&gl_texture_id);
}

void raySetFov(float yfov)
{
	cam_fov = yfov;
}

void raySetDof(float rad, float dof)
{
	cam_rad = rad;
	cam_dof = dof;
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


void rayLoadGeometry(const float *geom, size_t size)
{
	
}

void rayLoadInstance(const float *map, const unsigned *index, long size)
{
	
}

void rayClear()
{
	samples = 0;
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
	  cam_fov, cam_rad, cam_dof
	};
	clEnqueueWriteBuffer(command_queue,buffers["cam_fdata"]->get_cl_mem(),CL_TRUE,0,buffers["cam_fdata"]->get_size(),cam_array,0,NULL,NULL);
	clFlush(command_queue);
	
	clEnqueueWriteBuffer(command_queue,buffers["pitch"]->get_cl_mem(),CL_TRUE,0,buffers["pitch"]->get_size(),&width,0,NULL,NULL);
	
	work_range range2d = {width,height};
	//size_t global_work_size[2] = {width,height};
	//size_t local_work_size[2] = {8,8};
	
	// start
	kernels["start"]->evaluate(
	  range2d,
	  buffers["ray_fdata"],
	  buffers["ray_idata"],
	  buffers["cam_fdata"],
	  buffers["cl_random"]
	);
	clFlush(command_queue);
	
	unsigned int ray_count = width*height;
	
	int i, depth = 16;
	for(i = 0; i < depth; ++i)
	{
		work_range range1d = {ray_count};
		//size_t local_size = 64;
		//size_t global_size = local_size*((int)floor((float)ray_count/local_size) + 1);
		
		clEnqueueWriteBuffer(command_queue,buffers["work_size"]->get_cl_mem(),CL_TRUE,0,buffers["work_size"]->get_size(),&ray_count,0,NULL,NULL);
		
		// intersect
		kernels["intersect"]->evaluate(
		  range1d,
		  buffers["ray_fdata"],
		  buffers["ray_idata"],
		  buffers["hit_fdata"],
		  buffers["hit_idata"],
		  buffers["hit_info"],
		  buffers["work_size"]
		);
		clFlush(command_queue);
		
		// compact
		int dev;
		for(dev = 0; (1<<dev) < ray_count || (dev%2); ++dev)
		{
			clEnqueueWriteBuffer(command_queue,buffers["number"]->get_cl_mem(),CL_TRUE,0,buffers["number"]->get_size(),&dev,0,NULL,NULL);
			kernels["compact"]->evaluate(
			  range1d,
			  buffers["hit_info"],
			  buffers["cl_ray_count"],
			  buffers["number"],
			  buffers["work_size"]
			);
			clFlush(command_queue);
		}
		
		clEnqueueReadBuffer(command_queue,buffers["cl_ray_count"]->get_cl_mem(),CL_TRUE,0,sizeof(unsigned int),&ray_count,0,NULL,NULL);
		// fprintf(stdout,"ray_count: %d\n",ray_count);
		
		// produce
		kernels["produce"]->evaluate(
		  range1d,
		  buffers["hit_fdata"],
		  buffers["hit_idata"],
		  buffers["ray_fdata"],
		  buffers["ray_idata"],
		  buffers["hit_info"],
		  buffers["color_buffer"],
		  buffers["pitch"],
		  buffers["work_size"],
		  buffers["cl_random"]
		);
		clFlush(command_queue);
		
		if(ray_count == 0)
		{
			break;
		}
	}
	
	++samples;
	float mul = 1.0/samples;
	clEnqueueWriteBuffer(command_queue,buffers["factor"]->get_cl_mem(),CL_TRUE,0,buffers["factor"]->get_size(),&mul,0,NULL,NULL);
	
#ifdef RAY_GL
	clEnqueueAcquireGLObjects(command_queue, 1, &cl_image, 0, 0, 0);
#endif
	
	kernels["draw"]->evaluate(
	  range2d,
	  buffers["color_buffer"],
	  buffers["accum_buffer"],
	  buffers["factor"],
	  cl_image
	);
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
