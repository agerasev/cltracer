/** expand.cl */

__kernel void expand(__global uint *hit_info, const uint factor, const uint work_size)
{
	const uint pos = get_global_id(0);
	if(pos >= work_size)
	{
		return;
	}
	HitInfo info = hit_info_load(pos,hit_info);
	info.size = info.pre_size.x + info.pre_size.y*factor;
	info.offset = info.pre_offset.x + info.pre_offset.y*factor - info.size;
	hit_info_store(&info,pos,hit_info);
}
