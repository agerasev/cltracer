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
	
	void init(std::initializer_list<size_t> size_list)
	{
		dim = size_list.size();
		offset = new size_t[dim];
		global_size = new size_t[dim];
		local_size = new size_t[dim];
		size_t lsize = 1;
		switch(dim)
		{
		case 1:
			lsize = 64;
			break;
		case 2:
			lsize = 8;
			break;
		case 3:
			lsize = 4;
			break;
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
	
	void deinit()
	{
		delete[] offset;
		delete[] global_size;
		delete[] local_size;
	}
	
public:
	work_range(std::initializer_list<size_t> size_list)
	{
		init(size_list);
	}
	
	virtual ~work_range()
	{
		deinit();
	}
	
	work_range &operator = (std::initializer_list<size_t> size_list)
	{
		deinit();
		init(size_list);
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
