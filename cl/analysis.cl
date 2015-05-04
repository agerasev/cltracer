/* analysis.cl */

matrix3 get_zmat(float3 z)
{
	matrix3 m;
	float3 x = normalize(cross((fabs(z.z) < 0.7)*(float3)(0,-1,1) + (float3)(0,1,0),z));
	float3 y = cross(z,x);
	m.m[0] = x;
	m.m[1] = y;
	m.m[2] = z;
	return m;
}

float d2(float3 a, float3 b)
{
	return a.x*b.y - b.x*a.y;
}

int solve2(const float3 v[6], float3 pos, float3 dir, float *tp, float3 *cp, float3 *np)
{
	matrix3 m = get_zmat(dir);
	
	float3 
	  _a = v[0] + v[2] - 2*v[5],
	  _b = v[1] + v[2] - 2*v[4],
	  _c = 2*(v[2] + v[3] -  v[4] - v[5]),
	  _d = 2*(v[5] - v[2]),
	  _e = 2*(v[4] - v[2]),
	  _f = v[2];
	
	float3 
	  a = mvmul3(m,_a),
	  b = mvmul3(m,_b),
	  c = mvmul3(m,_c),
	  d = mvmul3(m,_d),
	  e = mvmul3(m,_e),
	  f = mvmul3(m,_f - pos);
	  
	float nc = d2(a,b)*d2(a,b) + d2(a,c)*d2(b,c);
	
	float
		ac = 2*d2(a,e)*d2(a,b) - d2(a,c)*d2(c,e) + d2(a,c)*d2(b,d) + d2(a,d)*d2(b,c),
		bc = d2(a,e)*d2(a,e) + 2*d2(a,b)*d2(a,f) - d2(a,c)*d2(c,f) - d2(a,c)*d2(d,e) - d2(a,d)*d2(c,e) + d2(a,d)*d2(b,d),
		cc = 2*d2(a,e)*d2(a,f) - d2(a,c)*d2(d,f) - d2(a,d)*d2(c,f) - d2(a,d)*d2(d,e),
		dc = d2(a,f)*d2(a,f) - d2(a,d)*d2(d,f);
	
	/*
	if(fabs(nc) < 1e-1)
	{
		return 0;
	}
	*/
	
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
		/*
		if(fabs(d2(a,c)*y + d2(a,d)) < 1e-1)
		{
			return 0;
		}
		*/
		float x = ic.x = -(d2(a,b)*y*y + d2(a,e)*y + d2(a,f))/(d2(a,c)*y + d2(a,d));
		float z = ic.z = 1 - x - y;
		if(x >= 0 && x <= 1 && y >= 0 && y <= 1 && z >= 0 && z <= 1)
		{
			float t = a.z*x*x + b.z*y*y + c.z*x*y + d.z*x + e.z*y + f.z;
			if(t > 0)
			{
				if(found == 0 || t < mt)
				{
					mc = ic;
					mt = t;
					++found;
					n = cross(2*_a*x + _c*y + _d, 2*_b*y + _c*x + _e);
				}
			}
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

int solve1(const float3 vert[3], float3 pos, float3 dir, float *tp, float3 *cp, float3 *np)
{
	matrix3 m = get_zmat(dir);
	const float3 v[3] = {mvmul3(m,vert[0] - pos),mvmul3(m,vert[1] - pos),mvmul3(m,vert[2] - pos)};
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
