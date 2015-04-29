/** intersect.cl */

__kernel void intersect(
	__global const uchar *ray_data, __global uchar *hit_data,
	__global uchar *hit_info, const uint work_size
)
{
	const int size = get_global_size(0);
	const int pos = get_global_id(0);
	
	if(pos >= work_size)
	{
		return;
	}
	
	Ray ray = ray_load(pos,ray_data);
	
	// Collide with uniform sphere
	const float3 sph_pos[4] = {(float3)(0.0f,4.0f,0.0f),(float3)(1.0f,1.0f,0.0f),(float3)(0.0f,2.0f,-4.0f),(float3)(-2.0f,2.0f,1.0f)};
	const float sph_rad[4] = {1.0f,0.8f,3.2f,0.4f};
	const uint2 sph_mat[4] = {{1,1},{1,1},{0,1},{0,0}};
	
	int i, hit_obj = 0;
	float min_dist;
	float3 hit_pos, hit_norm;
	for(i = 0; i < 4; ++i)
	{
		float dist = dot(sph_pos[i] - ray.pos,ray.dir);
		float3 lpos = ray.pos + dist*ray.dir;
		float len = length(lpos - sph_pos[i]);
		float dev = sqrt(sph_rad[i]*sph_rad[i] - len*len);
		float3 hpos = lpos - ray.dir*dev;
		dist -= dev;
		if(dist > 0.0f && len < sph_rad[i])
		{
			if(hit_obj == 0 || dist < min_dist)
			{
				min_dist = dist;
				hit_pos = hpos;
				hit_obj = i + 1;
				hit_norm = (hit_pos - sph_pos[i])/sph_rad[i];
			}
		}
	}
	
	Hit hit;
	hit.pos = hit_pos;
	hit.dir = ray.dir;
	hit.norm = hit_norm;
	hit.color = ray.color;
	hit.origin = ray.origin;
	hit.object = hit_obj;
	
	HitInfo info;
	
	info.size = 0;
	info.offset = info.size;
	
	info.pre_size.x = 0;
	info.pre_size.y = 0;
	if(hit_obj)
	{
		info.pre_size.x = sph_mat[hit_obj-1].x;
		info.pre_size.y = sph_mat[hit_obj-1].y;
	}
	info.pre_offset.x = info.pre_size.x;
	info.pre_offset.y = info.pre_size.y;
	
	hit_info_store(&info,pos,hit_info);
	
	hit_store(&hit,pos,hit_data);
}
