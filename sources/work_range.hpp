#pragma once

#include <CL/cl.h>
#include <cmath>

#include <initializer_list>

class work_range
{
private:
	size_t dim;
	size_t *offset;
	size_t *global_size;
	size_t *local_size;
public:
	work_range(std::initializer_list<size_t> size_list)
	{
		dim = size_list.size();
		offset = new size_t[dim];
		global_size = new size_t[dim];
		local_size = new size_t[dim];
		size_t lsize = 8;
		if(dim == 1)
		{
			lsize = 64;
		}
		auto list_iter = size_list.begin();
		for(unsigned i = 0; i < dim; ++i)
		{
			offset[i] = 0;
			local_size[i] = lsize;
			global_size[i] = size_t(ceil(double(*list_iter)/local_size[i]))*local_size[i];
			++list_iter;
		}
	}
	
	virtual ~work_range()
	{
		delete[] offset;
		delete[] global_size;
		delete[] local_size;
	}
	
	size_t get_dim() const
	{
		return dim;
	}
	
	const size_t *get_offset() const
	{
		return offset;
	}
	
	const size_t *get_global_size() const
	{
		return global_size;
	}
	
	const size_t *get_local_size() const
	{
		return local_size;
	}
};
