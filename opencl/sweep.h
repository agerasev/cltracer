#pragma once

#include <opencl.h>

#include "def/hit_info.h"

kernel void prepare(global const uchar *hit_info, global uint *buffer, const uint hit_count, const uint work_size)
{
	const int pos = get_global_id(0);
	
	if(pos >= work_size)
	{
		return;
	}
	else
	if(pos >= hit_count)
	{
		vstore2((uint2)(0,0),pos,buffer);
	}
	else
	{
		HitInfo info = hit_info_load(pos,hit_info);
		vstore2(info.pre_size,pos,buffer);
	}
}

kernel void sweep_up(global uint *buffer, global uint *ray_count, const uint dev, const uint work_size)
{
	const int pos = get_global_id(0);
	const int edev = (1<<dev);
	const int epos = 2*(pos + 1)*edev - 1;
	
	if(epos >= work_size)
	{
		return;
	}
	
	uint sum;
	uint2 sum2;
	
	sum2 = vload2(epos-edev,buffer) + vload2(epos,buffer);
	
	vstore2(sum2,epos,buffer);
	
	sum = sum2.x;
	
	if(pos == 0)
	{
		ray_count[0] = sum;
		ray_count[1] = sum2.x;
		ray_count[2] = sum2.y;
	}
}

kernel void sweep_down(global uint *buffer, const uint dev, const uint work_size)
{
	const int pos = get_global_id(0);
	const int edev = (1<<dev);
	const int epos = 2*(pos + 1)*edev - 1;
	
	if(epos >= work_size)
	{
		return;
	}
	
	uint2 elem = (uint2)(0,0);
	if(pos != 0)
	{
		elem = vload2(epos,buffer);
	}
	uint2 sum2 = vload2(epos-edev,buffer) + elem;
	
	vstore2(sum2,epos,buffer);
	vstore2(elem,epos-edev,buffer);
}

kernel void expand(global uchar *hit_info, global const uint *buffer, const uint factor, const uint work_size)
{
	const uint pos = get_global_id(0);
	
	if(pos >= work_size)
	{
		return;
	}
	
	uint2 pre_offset = vload2(pos,buffer);
	
	HitInfo info = hit_info_load(pos,hit_info);
	info.size = info.pre_size.x + info.pre_size.y*factor;
	info.offset = pre_offset.x + pre_offset.y*factor;
	hit_info_store(&info,pos,hit_info);
}
