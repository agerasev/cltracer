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

#include "globals.hpp"
#include "buffer_object.hpp"
#include "kernel.hpp"
#include "work_range.hpp"
#include "source.hpp"
#include "map.hpp"

typedef map<buffer_object*> buffer_map;
typedef map<kernel*> kernel_map;

buffer_map buffers;
kernel_map kernels;

void insert_kernel(kernel_map &m, kernel *k)
{
	m.insert(k->get_name(),k);
}

static unsigned l2pow(unsigned num)
{
	int i;
	for(i = 0; (num-1)>>i > 0; ++i) {}
	return i;
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
	return clCreateImage2D(context,CL_MEM_READ_WRITE,NULL,width,height,1,NULL,&err); //???
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
	command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);
	
	cl_image = __get_image();
	
	screen_size = width*height;
	buffer_size = 1<<l2pow(screen_size*MAX_CHILD_RAYS);
	
	buffers.insert("ray_data",     new buffer_object(RAY_SIZE*buffer_size));
	buffers.insert("hit_data",     new buffer_object(HIT_SIZE*buffer_size));
	buffers.insert("cam_fdata",    new buffer_object(sizeof(float)*CAM_SIZE));
	buffers.insert("color_buffer", new buffer_object(sizeof(int)*3*screen_size));
	buffers.insert("accum_buffer", new buffer_object(sizeof(float)*3*screen_size));
	buffers.insert("hit_info",     new buffer_object(sizeof(int)*HIT_INFO_SIZE*buffer_size));
	buffers.insert("cl_ray_count", new buffer_object(3*sizeof(int)));
	buffers.insert("cl_random",    new buffer_object(sizeof(int)*buffer_size));
	
	unsigned seed = 0;
	unsigned *random_buffer = (unsigned*)malloc(sizeof(unsigned)*buffer_size);
	for(i = 0; i < int(buffer_size); ++i)
	{
		random_buffer[i] = (seed = 3942082377*seed + 1234567);
	}
	
	clEnqueueWriteBuffer(command_queue,buffers["cl_random"]->get_cl_mem(),CL_TRUE,0,sizeof(unsigned int)*buffer_size,random_buffer,0,NULL,NULL);
	clFlush(command_queue);
	free(random_buffer);
	
	float *accum_buffer_data = (float*)malloc(sizeof(float)*3*screen_size);
	for(i = 0; i < int(3*screen_size); ++i)
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
	insert_kernel(kernels,new kernel("start"));
	insert_kernel(kernels,new kernel("intersect"));
	insert_kernel(kernels,new kernel("produce"));
	insert_kernel(kernels,new kernel("draw"));
	insert_kernel(kernels,new kernel("compact"));
	insert_kernel(kernels,new kernel("sweep_up"));
	insert_kernel(kernels,new kernel("sweep_down"));
	insert_kernel(kernels,new kernel("expand"));
	
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

static void __move_cam_pos()
{
	cam_pre_pos[0] = cam_pos[0];
	cam_pre_pos[1] = cam_pos[1];
	cam_pre_pos[2] = cam_pos[2];
}

void raySetPos(const float *pos)
{
	__move_cam_pos();
	
	cam_pos[0] = pos[0];
	cam_pos[1] = pos[1];
	cam_pos[2] = pos[2];
}

static void __move_cam_ori()
{
	cam_pre_ori[0] = cam_ori[0];
	cam_pre_ori[1] = cam_ori[1];
	cam_pre_ori[2] = cam_ori[2];
	
	cam_pre_ori[3] = cam_ori[3];
	cam_pre_ori[4] = cam_ori[4];
	cam_pre_ori[5] = cam_ori[5];
	
	cam_pre_ori[6] = cam_ori[6];
	cam_pre_ori[7] = cam_ori[7];
	cam_pre_ori[8] = cam_ori[8];
}

void raySetOri(float yaw, float pitch)
{
	__move_cam_ori();
	
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

void rayUpdateMotion()
{
	__move_cam_pos();
	__move_cam_ori();
	rayClear();
}

static cl_ulong __measureTime(cl_event event)
{
	cl_ulong time_start, time_end;
	
	clWaitForEvents(1,&event);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
	return time_end - time_start;
}

static void __printExecTime(kernel *k)
{
	printf("%s exec time: %0.3f ms\n",k->get_name(),(__measureTime(k->get_cl_event())/1000000.0));
}

#define NAIVE_SCAN
#define PRINT_TIME

int rayRender()
{
	// Create buffers
	float cam_array[CAM_SIZE] =
	{
	  cam_pos[0], cam_pos[1], cam_pos[2],
	  cam_ori[0], cam_ori[1], cam_ori[2],
	  cam_ori[3], cam_ori[4], cam_ori[5],
	  cam_ori[6], cam_ori[7], cam_ori[8],
	  cam_pre_pos[0], cam_pre_pos[1], cam_pre_pos[2],
	  cam_pre_ori[0], cam_pre_ori[1], cam_pre_ori[2],
	  cam_pre_ori[3], cam_pre_ori[4], cam_pre_ori[5],
	  cam_pre_ori[6], cam_pre_ori[7], cam_pre_ori[8],
	  cam_fov, cam_rad, cam_dof
	};
	buffers["cam_fdata"]->store_data(command_queue,cam_array);
	
	work_range range2d = {width,height};
	size_t work_size;
	//size_t global_work_size[2] = {width,height};
	//size_t local_work_size[2] = {8,8};
	
	// start
	kernels["start"]->evaluate(
	  range2d,
	  buffers["ray_data"],
	  buffers["cam_fdata"],
	  buffers["cl_random"]
	);
#ifdef PRINT_TIME
	__printExecTime(kernels["start"]);
#endif // PRINT_TIME
	clFlush(command_queue);
	
	unsigned int ray_count = width*height;
	unsigned int rc2[2] = {ray_count,0};
	
	uint sdr = (1 - samples)*(1 - (int)samples > 0);
	int i, depth = 4 - sdr;
	for(i = 0; i < depth; ++i)
	{
		work_range range1d = {ray_count};
		
		work_size = ray_count;
		
		// intersect
		kernels["intersect"]->evaluate(
		  range1d,
		  buffers["ray_data"],
		  buffers["hit_data"],
		  buffers["hit_info"],
		  int(work_size)
		);
#ifdef PRINT_TIME
		__printExecTime(kernels["intersect"]);
#endif // PRINT_TIME
		clFlush(command_queue);
		
		if(i < depth - 1)
		{
			int dev;
			
#ifdef NAIVE_SCAN
			// compact
			for(dev = 0; (1<<dev) < int(ray_count) || (dev%2); ++dev)
			{
				kernels["compact"]->evaluate(
					range1d,
					buffers["hit_info"],
					buffers["cl_ray_count"],
					int(dev),
					int(work_size)
				);
#ifdef PRINT_TIME
				__printExecTime(kernels["compact"]);
#endif
				clFlush(command_queue);
			}
#else // NAIVE_SCAN
			dev = 0;
			unsigned int r2p = 8;//1<<l2pow(ray_count);
			clEnqueueWriteBuffer(command_queue,buffers["work_size"]->get_cl_mem(),CL_TRUE,0,buffers["work_size"]->get_size(),&r2p,0,NULL,NULL);
			
			// fprintf(stdout,"ray_count: %d, r2p: %d\n",ray_count,r2p);
			
			// sweep up
			for(; (1<<dev) < r2p; ++dev)
			{
				work_range range1dp2 = {r2p>>(dev+1)};
				clEnqueueWriteBuffer(command_queue,buffers["number"]->get_cl_mem(),CL_TRUE,0,buffers["number"]->get_size(),&dev,0,NULL,NULL);
				kernels["sweep_up"]->evaluate(
					range1dp2,
					buffers["hit_info"],
					buffers["cl_ray_count"],
					buffers["number"],
					buffers["work_size"]
				);
				__printExecTime(kernels["sweep_up"]);
				clFlush(command_queue);
				
				int buf[64*HIT_INFO_SIZE/sizeof(int)];
				clEnqueueReadBuffer(command_queue,buffers["hit_info"]->get_cl_mem(),CL_TRUE,0,64*HIT_INFO_SIZE*sizeof(int),buf,0,NULL,NULL);
				int j;
				for(j = 0; j < 64; ++j)
				{
					int *tmp = buf + HIT_INFO_SIZE*j;
					fprintf(stdout,"%d ",tmp[4]);
				}
				for(j = 0; j < 64; ++j)
				{
					int *tmp = buf + HIT_INFO_SIZE*j;
					fprintf(stdout,"%d ",tmp[6]);
				}
			}
			
			// return -1;
			
			// sweep down
			unsigned zeros[2] = {0,0};
			clEnqueueWriteBuffer(command_queue,buffers["hit_info"]->get_cl_mem(),CL_TRUE,HIT_INFO_SIZE*(r2p-1) + 2*(dev%2) + 4,2,&zeros,0,NULL,NULL);
			for(; dev >= 0; --dev)
			{
				work_range range1dp2 = {r2p>>(dev+1)};
				clEnqueueWriteBuffer(command_queue,buffers["number"]->get_cl_mem(),CL_TRUE,0,buffers["number"]->get_size(),&dev,0,NULL,NULL);
				kernels["sweep_down"]->evaluate(
					range1dp2,
					buffers["hit_info"],
					buffers["cl_ray_count"],
					buffers["number"],
					buffers["work_size"]
				);
				__printExecTime(kernels["sweep_down"]);
				clFlush(command_queue);
			}
			
			clEnqueueWriteBuffer(command_queue,buffers["work_size"]->get_cl_mem(),CL_TRUE,0,buffers["work_size"]->get_size(),&ray_count,0,NULL,NULL);
#endif // NAIVE_SCAN
			
			buffers["cl_ray_count"]->load_data(command_queue,&ray_count,sizeof(int));
			buffers["cl_ray_count"]->load_data(command_queue,rc2,sizeof(int),2*sizeof(int));
			
			unsigned int mfactor = 0;
			if(rc2[1] != 0)
			{
				mfactor = (buffer_size - rc2[0])/rc2[1];
				unsigned mdif = 1;
				unsigned shift = (2 - samples)*(2 - (int)samples > 0);
				switch(i)
				{
				case 0:
					mdif = 8>>shift;
					break;
				case 1:
					mdif = 4>>shift;
					break;
				case 2:
					mdif = 2>>shift;
					break;
				default:
					mdif = 1>>shift;
					break;
				}
				if(mfactor > mdif)
				{
					mfactor = mdif;
				}
			}
			ray_count = rc2[0] + mfactor*rc2[1];
			
			kernels["expand"]->evaluate(
				range1d,
				buffers["hit_info"],
				int(mfactor),
				int(work_size)
			);
#ifdef PRINT_TIME
			__printExecTime(kernels["expand"]);
#endif // PRINT_TIME
			clFlush(command_queue);
		}
		
		// fprintf(stdout,"ray_count: %d, rc2: {%d,%d}\n",ray_count,rc2[0],rc2[1]);
		// return 0;
		
		// produce
		kernels["produce"]->evaluate(
		  range1d,
		  buffers["hit_data"],
		  buffers["ray_data"],
		  buffers["hit_info"],
		  buffers["color_buffer"],
		  int(width),
		  int(work_size),
		  buffers["cl_random"]
		);
		__printExecTime(kernels["produce"]);
		clFlush(command_queue);
		
		if(ray_count == 0)
		{
			break;
		}
	}
	
	++samples;
	uint csmp = samples - (samples < 3)*(samples - 1);
	float mul = 1.0/csmp;
	
#ifdef RAY_GL
	clEnqueueAcquireGLObjects(command_queue, 1, &cl_image, 0, 0, 0);
#endif
	
	kernels["draw"]->evaluate(
	  range2d,
	  buffers["color_buffer"],
	  buffers["accum_buffer"],
	  float(mul),
	  cl_image
	);
	__printExecTime(kernels["draw"]);
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
