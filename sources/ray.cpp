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

#include "session.hpp"
#include "globals.hpp"
#include "buffer_object.hpp"
#include "kernel.hpp"
#include "work_range.hpp"
#include "source.h"
#include "map.hpp"
#include "init.hpp"
#include "camera.hpp"

cl::session *session = nullptr;

typedef map<cl::buffer_object*> buffer_map;
typedef map<cl::kernel*> kernel_map;

buffer_map buffers;
kernel_map kernels;

#define PRINT_TIME

void insert_kernel(kernel_map &m, cl::kernel *k)
{
	m.insert(k->get_name(),k);
}

static unsigned ceil_pow2_exp(unsigned num)
{
	int i;
	for(i = 0; (num-1)>>i > 0; ++i) {}
	return i;
}

int rayInit(int w, int h)
{
	cl_int err;
	int i;
	
	width = w;
	height = h;
	
	session = new cl::session();
	
	cl_context context = session->get_context().get_cl_context();
	cl_command_queue command_queue = session->get_queue().get_cl_command_queue();
	
	cl_image = __get_image(session->get_context().get_cl_context());
	
	screen_size = width*height;
	buffer_size = 1 << ceil_pow2_exp(screen_size*MAX_CHILD_RAYS);
	
	buffers.insert("ray_data",     new cl::buffer_object(context,RAY_SIZE*buffer_size));
	buffers.insert("hit_data",     new cl::buffer_object(context,HIT_SIZE*buffer_size));
	buffers.insert("cam_fdata",    new cl::buffer_object(context,CAM_SIZE));
	buffers.insert("color_buffer", new cl::buffer_object(context,sizeof(int)*3*screen_size));
	buffers.insert("accum_buffer", new cl::buffer_object(context,sizeof(float)*3*screen_size));
	buffers.insert("hit_info",     new cl::buffer_object(context,HIT_INFO_SIZE*buffer_size));
	buffers.insert("scan_buf",     new cl::buffer_object(context,2*sizeof(int)*buffer_size));
	buffers.insert("ray_count",    new cl::buffer_object(context,3*sizeof(int)));
	buffers.insert("random",       new cl::buffer_object(context,sizeof(int)*buffer_size));
	buffers.insert("shapes",       new cl::buffer_object(context,SHAPE_BUFFER_SIZE));
	
	unsigned seed = 0;
	unsigned *random_buffer = (unsigned*)malloc(sizeof(unsigned)*buffer_size);
	for(i = 0; i < int(buffer_size); ++i)
	{
		random_buffer[i] = (seed = 3942082377*seed + 1234567);
	}
	
	clEnqueueWriteBuffer(command_queue,buffers["random"]->get_cl_mem(),CL_TRUE,0,sizeof(unsigned int)*buffer_size,random_buffer,0,NULL,NULL);
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
	
	const float shape_coord[4*6*3] = {
	  0.0,-1.0,-2.0,sqrt(3.0)/2.0,0.5,-2.0,-sqrt(3.0)/2.0,0.5,-2.0,
	  sqrt(3.0)/2.0,-0.5,0.0,0.0,1.0,0.0,-sqrt(3.0)/2.0,-0.5,0.0,
	  0,1,-2,sqrt(3.0)/2,2.5,-1,-sqrt(3.0)/2,2.5,-1,
	  sqrt(3.0)/2,1.5,-1,0,3,-2,-sqrt(3.0)/2,1.5,-1,
	  0,-4,-3,2*sqrt(3.0),2,-3,-2*sqrt(3.0),2,-3,
	  2*sqrt(3.0),-2,-1,0,4,-1,-2*sqrt(3.0),-2,-1,
	  0.0,-0.5,2.0,sqrt(3.0)/4.0,0.25,2.0,-sqrt(3.0)/4.0,0.25,2.0,
	  sqrt(3.0)/4.0,-0.25,1.5,0.0,0.5,1.5,-sqrt(3.0)/4.0,-0.25,1.5
	};
	clEnqueueWriteBuffer(command_queue,buffers["shapes"]->get_cl_mem(),CL_TRUE,0,sizeof(float)*4*6*3,shape_coord,0,NULL,NULL);
	clFlush(command_queue);
	
	// Create a program from the kernel source
	size_t source_size;
	char *source = __loadSource("cl/kernel.cl",&source_size);;
	program = clCreateProgramWithSource(context, 1, (const char **)&source, (const size_t *)&source_size, &err);
	if(err != CL_SUCCESS) {fprintf(stderr,"clCreateProgramWithSource: failed\n"); return -3;}
	__freeSource(source);
	
	
	// Build the program
	cl_device_id device_id = session->get_device_id();
	err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if(err != CL_SUCCESS)
	{
		__printKernelCompilationInfo(program,device_id);
		return -1;
	}
	
	// Create the OpenCL kernel
	insert_kernel(kernels,new cl::kernel(program,"start"));
	insert_kernel(kernels,new cl::kernel(program,"intersect"));
	insert_kernel(kernels,new cl::kernel(program,"produce"));
	insert_kernel(kernels,new cl::kernel(program,"draw"));
	insert_kernel(kernels,new cl::kernel(program,"clear"));
	insert_kernel(kernels,new cl::kernel(program,"prepare"));
	insert_kernel(kernels,new cl::kernel(program,"sweep_up"));
	insert_kernel(kernels,new cl::kernel(program,"sweep_down"));
	insert_kernel(kernels,new cl::kernel(program,"expand"));
	
	for(std::pair<const std::string,cl::kernel *> &p : kernels)
	{
		p.second->bind_queue(session->get_queue());
	}
	
	return 0;
}

void rayDispose()
{
	// cl_uint err;
	session->get_queue().flush();
	
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
	
	delete session;
	
	glDeleteTextures(1,&gl_texture_id);
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

static void __printExecTime(cl::kernel *k)
{
	printf("%s exec time: %0.3f ms\n",k->get_name(),k->measure_time()/1000000.0);
}

int rayRender()
{
	cl_command_queue command_queue = session->get_queue().get_cl_command_queue();
	
	// Create buffers
	float cam_array[CAM_SIZE/sizeof(float)] =
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
	
	cl::work_range range2d = {width,height};
	size_t work_size;
	//size_t global_work_size[2] = {width,height};
	//size_t local_work_size[2] = {8,8};
	
	// start
	kernels["start"]->evaluate(
	  range2d,
	  buffers["ray_data"],
	  buffers["cam_fdata"],
	  buffers["random"]
	);
#ifdef PRINT_TIME
	__printExecTime(kernels["start"]);
#endif // PRINT_TIME
	clFlush(command_queue);
	
	unsigned int ray_count = width*height;
	unsigned int rc2[2] = {ray_count,0};
	
	uint sdr = 0;//(1 - samples)*(1 - (int)samples > 0);
	int depth = 3 - sdr;
	for(int i = 0; i < depth; ++i)
	{
		cl::work_range range1d = {ray_count};
		
		work_size = ray_count;
		
		// intersect
		kernels["intersect"]->evaluate(
		  range1d,
			buffers["shapes"],
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
			
			cl::buffer_object *scan_buf = buffers["scan_buf"];
			
			dev = 0;
			int pow2_exp = ceil_pow2_exp(ray_count);
			unsigned int work_size_pow2 = 1 << pow2_exp;
			
			cl::work_range range1d_pow2 = {work_size_pow2};
			
			kernels["prepare"]->evaluate(
				range1d_pow2,
				buffers["hit_info"],
				scan_buf,
				int(work_size),
				int(work_size_pow2)
			);
#ifdef PRINT_TIME
			__printExecTime(kernels["prepare"]);
#endif // PRINT_TIME
			clFlush(command_queue);
			
			// sweep up
			for(; dev < pow2_exp; ++dev)
			{
				range1d_pow2 = {1u << (pow2_exp - dev - 1)};
				
				kernels["sweep_up"]->evaluate(
					range1d_pow2,
					scan_buf,
					buffers["ray_count"],
					int(dev),
					int(work_size_pow2)
				);
#ifdef PRINT_TIME
				__printExecTime(kernels["sweep_up"]);
#endif // PRINT_TIME
				clFlush(command_queue);
			}
			
			--dev;
			
			// sweep down
			for(; dev >= 0; --dev)
			{
				range1d_pow2 = {1u << (pow2_exp - dev - 1)};
				
				kernels["sweep_down"]->evaluate(
					range1d_pow2,
					scan_buf,
					int(dev),
					int(work_size_pow2)
				);
#ifdef PRINT_TIME
				__printExecTime(kernels["sweep_down"]);
#endif // PRINT_TIME
				clFlush(command_queue);
			}
			
			buffers["ray_count"]->load_data(command_queue,&ray_count,sizeof(int));
			buffers["ray_count"]->load_data(command_queue,rc2,sizeof(int),2*sizeof(int));
			
			unsigned int mfactor = 0;
			if(rc2[1] != 0)
			{
				mfactor = (buffer_size - rc2[0])/rc2[1];
				unsigned mdif = 1;
				/*
				unsigned shift = (2 - samples)*(2 - (int)samples > 0);
				switch(i)
				{
				case 0:
					mdif = 4>>shift;
					break;
				case 1:
					mdif = 2>>shift;
					break;
				default:
					mdif = 1>>shift;
					break;
				}
				*/
				if(mdif <= 0)
				{
					mdif = 1;
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
				scan_buf,
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
		  buffers["random"]
		);
#ifdef PRINT_TIME
		__printExecTime(kernels["produce"]);
#endif // PRINT_TIME
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
#ifdef PRINT_TIME
	__printExecTime(kernels["draw"]);
#endif // PRINT_TIME
#ifdef RAY_GL
	clEnqueueReleaseGLObjects(command_queue, 1, &cl_image, 0, 0, 0);
#endif
	clFlush(command_queue);
	
	kernels["clear"]->evaluate(
	  range2d,
	  buffers["color_buffer"]
	);
	
	return 0;
}

#ifdef RAY_GL
GLuint rayGetGLTexture()
{
	return gl_texture_id;
}
#endif
