/** intersect.cl */

void get_zmat(float3 m[3], float3 z)
{
	float3 x = normalize(cross((fabs(z.z) < 0.7)*(float3)(0,-1,1) + (float3)(0,1,0),z));
	float3 y = cross(z,x);
	m[0] = x;
	m[1] = y;
	m[2] = z;
}

float3 mvmul(const float3 m[3], float3 v)
{
	return (float3)(dot(m[0],v),dot(m[1],v),dot(m[2],v));
}

float d2(float3 a, float3 b)
{
	return a.x*b.y - b.x*a.y;
}

int solve2(const float3 v[6], float3 pos, float3 dir, float *tp, float3 *np, float3 *cp)
{
	float3 m[3];
	get_zmat(m,dir);
	
	float3 
	  _a = v[0] + v[2] - 2*v[5],
	  _b = v[1] + v[2] - 2*v[4],
	  _c = 2*(v[2] + v[3] -  v[4] - v[5]),
	  _d = 2*(v[5] - v[2]),
	  _e = 2*(v[4] - v[2]),
	  _f = v[2];
	
	float3 
	  a = mvmul(m,_a),
	  b = mvmul(m,_b),
	  c = mvmul(m,_c),
	  d = mvmul(m,_d),
	  e = mvmul(m,_e),
	  f = mvmul(m,_f - pos);
	  
	float nc = d2(a,b)*d2(a,b) + d2(a,c)*d2(b,c);
	
	float
		ac = 2*d2(a,e)*d2(a,b) - d2(a,c)*d2(c,e) + d2(a,c)*d2(b,d) + d2(a,d)*d2(b,c),
		bc = d2(a,e)*d2(a,e) + 2*d2(a,b)*d2(a,f) - d2(a,c)*d2(c,f) - d2(a,c)*d2(d,e) - d2(a,d)*d2(c,e) + d2(a,d)*d2(b,d),
		cc = 2*d2(a,e)*d2(a,f) - d2(a,c)*d2(d,f) - d2(a,d)*d2(c,f) - d2(a,d)*d2(d,e),
		dc = d2(a,f)*d2(a,f) - d2(a,d)*d2(d,f);
	
	float r[4];
	int rc = SolveP4(r,ac/nc,bc/nc,cc/nc,dc/nc);
	if(rc <= 0)
	{
		return 0;
	}
	int i, found = 0;
	float mt;
	float3 mc;
	float3 n = (float3)(0,0,1);
	for(i = 0; i < rc; ++i)
	{
		float3 ic;
		float y = ic.y = r[i];
		float x = ic.x = -(d2(a,b)*y*y + d2(a,e)*y + d2(a,f))/(d2(a,c)*y + d2(a,d));
		float z = ic.z = 1 - x - y;
		if(x < 0 || x > 1 || y < 0 || y > 1 || z < 0 || z > 1)
		{
			continue;
		}
		float t = v[0].z*x*x + v[1].z*y*y + v[2].z*z*z + 2*v[3].z*x*y + 2*v[4].z*y*z + 2*v[5].z*z*x;
		if(t < 0)
		{
			continue;
		}
		if(found == 0 || t < mt)
		{
			mc = ic;
			mt = t;
			++found;
			n = cross(2*_a*x + _c*y + _d, 2*_b*y + _c*x + _e);
		}
	}
	
	if(found)
	{
		*tp = mt;
		*cp = mc;
		n = normalize(n);
		if(dot(n,dir) > 0)
		{
			n = -n;
		}
		*np = n;
	}
	return found;
}

int solve1(const float3 vert[3], float3 pos, float3 dir, float *tp, float3 *np, float3 *cp)
{
	float3 m[3];
	get_zmat(m,dir);
	const float3 v[3] = {mvmul(m,vert[0] - pos),mvmul(m,vert[1] - pos),mvmul(m,vert[2] - pos)};
	float3 c;
	c.y = (v[2].x*(v[0].y - v[2].y) - v[2].y*(v[0].x - v[2].x))/
	  ((v[0].x - v[2].x)*(v[1].y - v[2].y) - (v[1].x - v[2].x)*(v[0].y - v[2].y));
	//c.x = ((v[2].x - v[1].x)*c.y - v[2].x)/(v[0].x - v[2].x);
	c.x = (v[2].x*(v[1].y - v[2].y) - v[2].y*(v[1].x - v[2].x))/
	  ((v[1].x - v[2].x)*(v[0].y - v[2].y) - (v[0].x - v[2].x)*(v[1].y - v[2].y));
	c.z = 1 - c.x - c.y;
	if(c.x < 0 || c.x > 1 || c.y < 0 || c.y > 1 || c.z < 0 || c.z > 1)
	{
		return 0;
	}
	float t = c.x*v[0].z + c.y*v[1].z + c.z*v[2].z;
	if(t < 0)
	{
		return 0;
	}
	float3 n = normalize(cross(vert[0] - vert[2], vert[1] - vert[2]));
	n -= 2*n*(dot(n,dir) > 0);
	*tp = t;
	*cp = c;
	*np = n;
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
	
	// Collide with uniform sphere
	/*
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
	*/
	
	int hit_obj = 0;
	float3 hit_pos, hit_norm;
	
	float3 a[6] = {
	  {0.0,-1.0,0.0},{sqrt(3.0)/2.0,0.5,0.0},{-sqrt(3.0)/2.0,0.5,0.0},
	  {sqrt(3.0)/2.0,-0.5,1.0},{0.0,1.0,1.0},{-sqrt(3.0)/2.0,-0.5,1.0}
	};
	
	hit_obj = 0;
	float t, mt = 1e4;
	float3 n,c;
	if(solve1(a,ray.pos,ray.dir,&mt,&n,&c))
	{
		hit_obj = 1;
		hit_pos = ray.pos + ray.dir*mt;
		hit_norm = n;
	}
	if(solve2(a,ray.pos,ray.dir,&t,&n,&c))
	{
		if(t < mt)
		{
			hit_obj = 2;
			hit_pos = ray.pos + ray.dir*t;
			hit_norm = n;
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
		info.pre_size.x = 1;//sph_mat[hit_obj-1].x;
		info.pre_size.y = 1;//sph_mat[hit_obj-1].y;
	}
	info.pre_offset.x = info.pre_size.x;
	info.pre_offset.y = info.pre_size.y;
	
	hit_info_store(&info,pos,hit_info);
	
	hit_store(&hit,pos,hit_data);
}
