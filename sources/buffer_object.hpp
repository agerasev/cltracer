#pragma once

#include <string>
#include <exception>

#include "exception.hpp"

class buffer_object
{
private:
	cl_mem mem;
	size_t size;
	
public:
	buffer_object(size_t buffer_size) throw(cl_exception)
	  : size(buffer_size)
	{
		cl_int ret;
		mem = clCreateBuffer(context,CL_MEM_READ_WRITE,size,NULL,&ret);
		if(ret != CL_SUCCESS)
		{
			throw cl_exception("clCreateBuffer",ret);
		}
	}
	virtual ~buffer_object() throw(cl_exception)
	{
		cl_int ret;
		ret = clReleaseMemObject(mem);
		if(ret != CL_SUCCESS)
		{
			throw cl_exception("clReleaseMemObject",ret);
		}
	}
	
	cl_mem get_cl_mem() const noexcept
	{
		return mem;
	}
	
	size_t get_size() const noexcept
	{
		return size;
	}
	
	void load_data(cl_command_queue queue, void *data, size_t offset, size_t length) const throw(cl_exception)
	{
		cl_int ret;
		ret = clEnqueueReadBuffer(queue,mem,CL_TRUE,offset,length,data,0,NULL,NULL);
		if(ret != CL_SUCCESS)
		{
			throw cl_exception("clEnqueueReadBuffer",ret);
		}
	}
	
	void load_data(cl_command_queue queue, void *data, size_t length) const throw(cl_exception)
	{
		load_data(queue,data,0,length);
	}
	
	void load_data(cl_command_queue queue, void *data) const throw(cl_exception)
	{
		load_data(queue,data,size);
	}
	
	void store_data(cl_command_queue queue, void *data, size_t offset, size_t length) throw(cl_exception)
	{
		cl_int ret;
		ret = clEnqueueWriteBuffer(queue,mem,CL_TRUE,offset,length,data,0,NULL,NULL);
		if(ret != CL_SUCCESS)
		{
			throw cl_exception("clEnqueueWriteBuffer",ret);
		}
	}
	
	void store_data(cl_command_queue queue, void *data, size_t length) throw(cl_exception)
	{
		store_data(queue,data,0,length);
	}
	
	void store_data(cl_command_queue queue, void *data) throw(cl_exception)
	{
		store_data(queue,data,size);
	}
};
