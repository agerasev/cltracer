/** intersect.cl */

int _test2(float2 p0, float2 p1, float2 lp, float2 hp)
{
	float t0,t1,r0,r1;
	
	// y line test
	t0 = (lp.x - p0.x)/(p1.x - p0.x);
	t1 = (hp.x - p0.x)/(p1.x - p0.x);
	if((t0 <= 0 && t1 <= 0) || (t0 >= 1 && t1 >= 1))
		return 0;
	//t0 = fclamp(t0,0,1);
	//t1 = fclamp(t1,0,1);
	r0 = p0.y*(1-t0) + p1.y*t0;
	r1 = p1.y*(1-t1) + p1.y*t1;
	if((r0 <= lp.y && r1 <= lp.y) || (r0 >= hp.y && r1 >= hp.y))
		return 0;
	
	return 1;
}

int _test3v(float3 p, float3 d, float3 v[], int n)
{
	// object bounds
	float3 hp = v[0], lp = v[0];
	int i;
	for(i = 0; i < n; ++i)
	{
		//hp = fmax(v[i],hp);
		//lp = fmin(v[i],lp);
	}
	
	hp -= p;
	lp -= p;
	
	float t0,t1;
	// yz plane test
	t0 = lp.x/d.x;
	t1 = hp.x/d.x;
	if(t0 < 0 && t1 < 0)
		return 0;
	//t0 = fmax(t0,0);
	//t1 = fmax(t1,0);
	if(!_test2(d.yz*t0,d.yz*t1,lp.yz,hp.yz))
		return 0;
	
	return 1;
}

kernel void intersect(
	global const uchar *ray_data, global uchar *hit_data,
	global uchar *hit_info, const uint work_size
)
{
	const int size = get_global_size(0);
	const int pos = get_global_id(0);
	
	if(pos >= work_size)
	{
		return;
	}
	
	Ray ray = ray_load(pos,ray_data);
	
	int hit_obj = 0;
	float3 hit_pos, hit_norm;
	
	const uint2 mat[4] = {{1,1},{1,1},{0,1},{0,0}};
	const float osf[4] = {1,0.1,64,0.1};
	
	// Collide with uniform sphere
	/*
	const float3 sph_pos[4] = {(float3)(0.0f,4.0f,0.0f),(float3)(1.0f,1.0f,0.0f),(float3)(0.0f,2.0f,-4.0f),(float3)(-2.0f,2.0f,1.0f)};
	const float sph_rad[4] = {1.0f,0.8f,3.2f,0.4f};
	
	int i;
	float min_dist;
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
	*/
	
	float3 coord[4*6] = {
	  {0.0,-1.0,-2.0},{sqrt(3.0)/2.0,0.5,-2.0},{-sqrt(3.0)/2.0,0.5,-2.0},
	  {sqrt(3.0)/2.0,-0.5,0.0},{0.0,1.0,0.0},{-sqrt(3.0)/2.0,-0.5,0.0},
	  {0,1,-2},{sqrt(3.0)/2,2.5,-1},{-sqrt(3.0)/2,2.5,-1},
	  {sqrt(3.0)/2,1.5,-1},{0,3,-2},{-sqrt(3.0)/2,1.5,-1},
	  {0,-4,-3},{2*sqrt(3.0),2,-3},{-2*sqrt(3.0),2,-3},
	  {2*sqrt(3.0),-2,-1},{0,4,-1},{-2*sqrt(3.0),-2,-1},
	  {0.0,-0.5,2.0},{sqrt(3.0)/4.0,0.25,2.0},{-sqrt(3.0)/4.0,0.25,2.0},
	  {sqrt(3.0)/4.0,-0.25,1.5},{0.0,0.5,1.5},{-sqrt(3.0)/4.0,-0.25,1.5},
	};
	
	hit_obj = 0;
	int i;
	float mt;
	float3 r[2] = {ray.pos,ray.dir};
	for(i = 0; i < 4; ++i)
	{
		float t;
		float3 n,c;
		if(solve2(coord + 6*i,osf[i],ray.pos,ray.dir,&t,&c,&n))
		{
			if(hit_obj == 0 || t < mt)
			{
				mt = t;
				hit_pos = ray.pos + ray.dir*t;
				hit_obj = i + 1;
				hit_norm = n;
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
		info.pre_size.x = mat[hit_obj-1].x;
		info.pre_size.y = mat[hit_obj-1].y;
	}
	info.pre_offset.x = info.pre_size.x;
	info.pre_offset.y = info.pre_size.y;
	
	hit_info_store(&info,pos,hit_info);
	
	hit_store(&hit,pos,hit_data);
}
