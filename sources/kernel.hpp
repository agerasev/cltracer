#pragma once

#include <CL/cl.h>

#include "globals.hpp"
#include "exception.hpp"
#include "buffer_object.hpp"
#include "work_range.hpp"

#include <cstdio>
#include <string>

class kernel
{
private:
	cl_kernel kern;
	std::string name;
	cl_uint arg_count;
	cl_event event;
	
	void set_mem_arg(cl_uint pos, cl_mem mem) throw(cl_exception)
	{
		cl_int ret;
		ret = clSetKernelArg(kern,pos,sizeof(cl_mem),reinterpret_cast<void*>(&mem));
		if(ret != CL_SUCCESS)
		{
			throw cl_exception("clSetKernelArg",ret);
		}
	}
	
	template <typename T>
	void set_var_arg(cl_uint pos, T var) throw(cl_exception)
	{
		cl_int ret;
		ret = clSetKernelArg(kern,pos,sizeof(T),reinterpret_cast<void*>(&var));
		if(ret != CL_SUCCESS)
		{
			throw cl_exception("clSetKernelArg",ret);
		}
	}
	
	void check_args_count(cl_uint count) throw(exception)
	{
		if(count != arg_count)
		{
			char strbuf[0x100];
			snprintf(strbuf,0xff,"kernel '%s' takes %u arguments, %u given",name.data(),arg_count,count);
			throw(exception(strbuf));
		}
	}
	
	template <typename ... Args>
	static void unroll_args(kernel *self, cl_uint count, buffer_object *buf_obj, Args ... args) throw(exception)
	{
		self->set_mem_arg(count,buf_obj->get_cl_mem());
		unroll_args(self,count+1,args...);
	}
	
	template <typename ... Args>
	static void unroll_args(kernel *self, cl_uint count, cl_mem mem, Args ... args) throw(exception)
	{
		self->set_mem_arg(count,mem);
		unroll_args(self,count+1,args...);
	}
	
	template <typename T, typename ... Args>
	static void unroll_args(kernel *self, cl_uint count, T var, Args ... args) throw(exception)
	{
		self->set_var_arg(count,var);
		unroll_args(self,count+1,args...);
	}
	
	static void unroll_args(kernel *self, cl_uint count, buffer_object *buf_obj) throw(exception)
	{
		self->set_mem_arg(count,buf_obj->get_cl_mem());
		self->check_args_count(count+1);
	}
	
	static void unroll_args(kernel *self, cl_uint count, cl_mem mem) throw(exception)
	{
		self->set_mem_arg(count,mem);
		self->check_args_count(count+1);
	}
	
	template <typename T>
	static void unroll_args(kernel *self, cl_uint count, T var) throw(exception)
	{
		self->set_var_arg(count,var);
		self->check_args_count(count+1);
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
	void evaluate(const work_range &range, Args ... args) throw(exception)
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
