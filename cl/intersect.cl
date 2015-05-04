/** intersect.cl */

matrix3 get_zmat3(float3 z)
{
	matrix3 m;
	float3 x = normalize(cross((fabs(z.z) < 0.7)*(float3)(0,-1,1) + (float3)(0,1,0),z));
	float3 y = cross(z,x);
	m.m[0] = x;
	m.m[1] = y;
	m.m[2] = z;
	return m;
}

int collide(const float3 v[10], const float3 r[2], float *tp, float3 *cp, float3 *np)
{
	matrix3 m, im;
	m = get_zmat3(r[1]);
	im = invert3(m);
	
	float3 nv[10];
	{
		int i;
		for(i = 0; i < 10; ++i)
		{
			nv[i] = mvmul3(m,v[i] - r[0]);
		}
	}
		
	float3 nr[2] = {{0,0,0},{0,0,1}};
	
	float t;
	float3 c,n;
	int rv = intersect_bicubic_surface(nv,nr,&t,&c,&n);
	
	// perform neuton method
	if(rv)
	{
		int i;
		float3 p;
		float3 d[2];
		for(i = 0; i < 0x10; ++i)
		{
			p = get_bicubic_point(nv,c);
			get_bicubic_jacobian(nv,c,d);
			matrix2 j;
			j.m[0] = d[0].xy;
			j.m[1] = d[1].xy;
			c.xy = c.xy - mvmul2(invert2(j),p.xy);
			c.z = 1 - c.x - c.y;
		}
		// t = get_bicubic_point(nv,c).z;
		get_bicubic_jacobian(nv,c,d);
		n = normalize(cross(d[0],d[1]));
		if(n.z > 0)
		{
			n = -n;
		}
	}
	
	*tp = t;
	*cp = c;
	*np = mvmul3(im,n);
	return rv;
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
	
	// Collide with uniform sphere
	/*
	const float3 sph_pos[4] = {(float3)(0.0f,4.0f,0.0f),(float3)(1.0f,1.0f,0.0f),(float3)(0.0f,2.0f,-4.0f),(float3)(-2.0f,2.0f,1.0f)};
	const float sph_rad[4] = {1.0f,0.8f,3.2f,0.4f};
	const uint2 sph_mat[4] = {{1,1},{1,1},{0,1},{0,0}};
	
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
	
	float3 a[6] = {
	  {0.0,-1.0,0.0},{sqrt(3.0)/2.0,0.5,0.0},{-sqrt(3.0)/2.0,0.5,0.0},
	  {sqrt(3.0)/2.0,-0.5,2.0},{0.0,1.0,2.0},{-sqrt(3.0)/2.0,-0.5,2.0}
	};
	
	float3 b[10] = {
	  {0.0,-1.0,0.0},{sqrt(3.0)/2.0,0.5,0.0},{-sqrt(3.0)/2.0,0.5,0.0}, // xxx,yyy,zzz
	  {sqrt(3.0)/2.0,-0.5,1.0},{sqrt(3.0)/2.0,-0.5,-1.0}, // xxy,yyx
	  {0.0,1.0,1.0},{0.0,1.0,-1.0}, // yyz, zzy
	  {-sqrt(3.0)/2.0,-0.5,1.0},{-sqrt(3.0)/2.0,-0.5,-1.0}, // zzx,xxz
	  {0.0,0.0,0.0}
	};
	
	hit_obj = 0;
	float t;
	float3 n,c;
	float3 r[2] = {ray.pos,ray.dir};
	if(solve2(a,ray.pos,ray.dir,&t,&c,&n)) //collide(b,r,&t,&c,&n)
	{
			hit_obj = 2;
			hit_pos = ray.pos + ray.dir*t;
			hit_norm = n;
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
		info.pre_size.x = 1;//sph_mat[hit_obj-1].x;
		info.pre_size.y = 1;//sph_mat[hit_obj-1].y;
	}
	info.pre_offset.x = info.pre_size.x;
	info.pre_offset.y = info.pre_size.y;
	
	hit_info_store(&info,pos,hit_info);
	
	hit_store(&hit,pos,hit_data);
}
