#define HIT_INFO_SIZE 8

typedef struct
{
	uint size;
	uint offset;
	uint2 pre_size;
	uint2 pre_offset;
	uint2 pre_offset_tmp;
}
HitInfo;

HitInfo hit_info_load(int offset, __global const uint *data)
{
	HitInfo info;
	__global const uint *info_data = data + HIT_INFO_SIZE*offset;
	info.size = info_data[0];
	info.offset = info_data[1];
	info.pre_size = vload2(1,info_data);
	info.pre_offset = vload2(2,info_data);
	info.pre_offset_tmp = vload2(3,info_data);
	return info;
}

void hit_info_store(HitInfo *info, int offset, __global uint *data)
{
	__global uint *info_data = data + HIT_INFO_SIZE*offset;
	info_data[0] = info->size;
	info_data[1] = info->offset;
	vstore2(info->pre_size,1,info_data);
	vstore2(info->pre_offset,2,info_data);
	vstore2(info->pre_offset_tmp,3,info_data);
}
