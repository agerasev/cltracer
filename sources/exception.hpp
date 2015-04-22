#pragma once

#include <CL/cl.h>

#include <string>
#include <exception>

#include "utility.hpp"

class cl_exception : public std::exception
{
private:
	cl_int ret;
	std::string func;
	std::string msg;
public:
	cl_exception(const char *func_name, cl_int error_code) noexcept
	  : ret(error_code), func(func_name)
	{
		msg = (func + " : ") + get_code_name(ret);
	}
	cl_int get_code() const noexcept
	{
		return ret;
	}
	const char *get_func_name() noexcept
	{
		return func.data();
	}
	virtual const char *what() const noexcept override
	{
		return msg.data();
	}
};
