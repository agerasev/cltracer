__kernel void expand(__global uint *hit_info, __global const uint *diffuse_factor, __global const uint *work_size)
{
	const uint size = *work_size;//get_global_size(0);
	const uint pos = get_global_id(0);
	const uint factor = *diffuse_factor;
	if(pos >= size)
	{
		return;
	}
	HitInfo info = hit_info_load(pos,hit_info);
	info.size = info.pre_size.x + info.pre_size.y*factor;
	info.offset = info.pre_offset.x + info.pre_offset.y*factor - info.size;
	hit_info_store(&info,pos,hit_info);
}
