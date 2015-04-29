#pragma once

#include <CL/cl.h>

#include <string>
#include <exception>

#include "utility.hpp"

class exception : public std::exception
{
private:
	std::string msg;
	
public:
	exception(const char *message) noexcept
	  : msg(message)
	{
		
	}
	
	const char *get_message() const noexcept
	{
		return msg.data();
	}
	
	virtual const char *what() const noexcept override
	{
		return msg.data();
	}
};

class cl_exception : public exception
{
private:
	cl_int ret;
	std::string func;
public:
	cl_exception(const char *func_name, cl_int error_code) noexcept
	  : exception(((std::string(func_name) + " : ") + get_code_name(error_code)).data()), 
	    ret(error_code), func(func_name)
	{
		
	}
	
	cl_int get_code() const noexcept
	{
		return ret;
	}
	
	const char *get_func_name() noexcept
	{
		return func.data();
	}
};
