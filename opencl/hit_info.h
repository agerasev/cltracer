#pragma once

#include "opencl.h"
#include "def/hit_info.h"

HitInfo hit_info_load(int offset, global const uchar *info_data)
{
	HitInfo info;
	global const uint *data = (global const uint*)(info_data + HIT_INFO_SIZE*offset);
	info.size = data[0];
	info.offset = data[1];
	info.pre_size = vload2(1,data);
	info.pre_offset = vload2(2,data);
	return info;
}

void hit_info_store(HitInfo *info, int offset, global uchar *info_data)
{
	global uint *data = (global uint*)(info_data + HIT_INFO_SIZE*offset);
	data[0] = info->size;
	data[1] = info->offset;
	vstore2(info->pre_size,1,data);
	vstore2(info->pre_offset,2,data);
}
