#pragma once

#include "opencl.h"

__kernel void draw(
  __global const uint *color_buffer, __global float *accum_buffer, 
  const float factor, __write_only image2d_t image
)
{
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const int2 pos = (int2)(get_global_id(0), get_global_id(1));
	
	float3 color = 
	  factor*(convert_float3(vload3(pos.x + size.x*pos.y,color_buffer))/0x10000) + 
	  (1.0f - factor)*vload3(pos.x + size.x*pos.y,accum_buffer);
	
	vstore3(color,pos.x + size.x*pos.y,accum_buffer);
	
	write_imagef(image,pos,(float4)(color,1.0f));
}

kernel void clear(__global uint *color_buffer)
{
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const int2 pos = (int2)(get_global_id(0), get_global_id(1));
	vstore3((uint3)(0,0,0),pos.x + size.x*pos.y,color_buffer);
}
