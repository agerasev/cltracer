#pragma once

#include <CL/cl.h>

#include "globals.hpp"
#include "exception.hpp"
#include "buffer_object.hpp"
#include "work_range.hpp"

#include <cassert>
#include <string>

class kernel
{
private:
	cl_kernel kern;
	std::string name;
	cl_uint arg_count;
	cl_event event;
	
	void set_arg(size_t pos, cl_mem mem) throw(cl_exception)
	{
		cl_int ret;
		ret = clSetKernelArg(kern,pos,sizeof(cl_mem),reinterpret_cast<void*>(&mem));
		if(ret != CL_SUCCESS)
		{
			throw cl_exception("clSetKernelArg",ret);
		}
	}
	
	template <typename ... Args>
	static void unroll_args(kernel *self, size_t count, buffer_object *buf_obj, Args ... args) throw(cl_exception)
	{
		self->set_arg(count,buf_obj->get_cl_mem());
		unroll_args(self,count+1,args...);
	}
	
	static void unroll_args(kernel *self, size_t count, buffer_object *buf_obj) throw(cl_exception)
	{
		self->set_arg(count,buf_obj->get_cl_mem());
		assert(count + 1 == self->arg_count);
	}
	
	template <typename ... Args>
	static void unroll_args(kernel *self, size_t count, cl_mem mem, Args ... args) throw(cl_exception)
	{
		self->set_arg(count,mem);
		unroll_args(self,count+1,args...);
	}
	
	static void unroll_args(kernel *self, size_t count, cl_mem mem) throw(cl_exception)
	{
		self->set_arg(count,mem);
		assert(count + 1 == self->arg_count);
	}
	
public:
	kernel(const std::string &kernel_name) throw(cl_exception)
	  : name(kernel_name)
	{
		cl_int ret;
		
		kern = clCreateKernel(program,name.data(),&ret);
		if(ret != CL_SUCCESS) 
		{
			throw cl_exception("clCreateKernel",ret);
		}
		
		ret = clGetKernelInfo(kern,CL_KERNEL_NUM_ARGS,sizeof(cl_uint),&arg_count,NULL);
		if(ret != CL_SUCCESS) 
		{
			throw cl_exception("clGetKernelInfo",ret);
		}
	}
	
	virtual ~kernel() throw(cl_exception)
	{
		cl_int ret;
		ret = clReleaseKernel(kern);
		if(ret != CL_SUCCESS)
		{
			throw cl_exception("clReleaseKernel",ret);
		}
	}
	
	const char *get_name() const
	{
		return name.data();
	}
	
	cl_kernel get_cl_kernel() const
	{
		return kern;
	}
	
	cl_event get_cl_event() const
	{
		return event;
	}
	
	template <typename ... Args>
	void evaluate(const work_range &range, Args ... args) throw(cl_exception)
	{
		cl_int ret;
		unroll_args(this,0,args...);
		ret = 
		  clEnqueueNDRangeKernel(
		    command_queue,kern,
		    range.get_dim(),range.get_offset(),
				range.get_global_size(),range.get_local_size(),
		    0,NULL,&event
		  );
		if(ret != CL_SUCCESS)
		{
			throw cl_exception("clEnqueueNDRangeKernel",ret);
		}
	}
};
