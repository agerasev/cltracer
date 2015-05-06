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

float newton_step_quartic(float x, float a, float b, float c, float d)
{
	float fxs = ((4*x + 3*a)*x + 2*b)*x + c; // f'(x)
	if(fxs == 0)
		return 1e38;
	float fx = (((x + a)*x + b)*x + c)*x + d; // f(x)
	return x - fx/fxs;
}

float newton_step_cubic(float x, float b, float c, float d)
{
	float fxs = (3*x + 2*b)*x + c; // f'(x)
	if(fxs == 0)
		return 1e38;
	float fx = ((x + b)*x + c)*x + d; // f(x)
	return x - fx/fxs;
}

#define CUB_EPS  1e-1
#define HIT_DIST 1e-2
#define NSTEP    4

int intersect_surface(
  global const float *v, const float osf, const int same, 
  const float3 pos, const float3 dir, 
  float *tp, float3 *cp, float3 *np
)
{
	// get transformation
	matrix3 m = get_zmat(dir);
	
	// choose the variable for equation
	int var = 0; // 0(x), 1(y), 2(z)
	int iv;
	float mtp = 0.0;
	for(iv = 0; iv < 3; ++iv)
	{
		float _tp = fabs(dot(dir,cross(
		  vload3((iv+1)%3,v) - vload3((iv+1)%3 + 3,v),
		  vload3((iv+2)%3,v) - vload3((iv+1)%3 + 3,v)
		)));
		if(iv == 0 || _tp >= mtp)
		{
			var = iv;
			mtp = _tp;
		}
	}
	const int idx[6] = 
	{
		(var+2)%3, (var+0)%3, (var+1)%3,
		(var+2)%3 + 3, (var+0)%3 + 3, (var+1)%3 + 3
	};
	
	// compute factors for quartic equation
	const float3 
	  _a = vload3(idx[0],v) + vload3(idx[2],v) - 2*vload3(idx[5],v),
	  _b = vload3(idx[1],v) + vload3(idx[2],v) - 2*vload3(idx[4],v),
	  _c = 2*(vload3(idx[2],v) + vload3(idx[3],v) -  vload3(idx[4],v) - vload3(idx[5],v)),
	  _d = 2*(vload3(idx[5],v) - vload3(idx[2],v)),
	  _e = 2*(vload3(idx[4],v) - vload3(idx[2],v)),
	  _f = vload3(idx[2],v);
	const float3 
	  a = mvmul3(m,_a),
	  b = mvmul3(m,_b),
	  c = mvmul3(m,_c),
	  d = mvmul3(m,_d),
	  e = mvmul3(m,_e),
	  f = mvmul3(m,_f - pos);
	const float k[5] = {
		d2(a,b)*d2(a,b) + d2(a,c)*d2(b,c),
		2*d2(a,e)*d2(a,b) - d2(a,c)*d2(c,e) + d2(a,c)*d2(b,d) + d2(a,d)*d2(b,c),
		d2(a,e)*d2(a,e) + 2*d2(a,b)*d2(a,f) - d2(a,c)*d2(c,f) - d2(a,c)*d2(d,e) - d2(a,d)*d2(c,e) + d2(a,d)*d2(b,d),
		2*d2(a,e)*d2(a,f) - d2(a,c)*d2(d,f) - d2(a,d)*d2(c,f) - d2(a,d)*d2(d,e),
		d2(a,f)*d2(a,f) - d2(a,d)*d2(d,f)
	};
	
	// solve equation
	int rc;
	float r[4];
	const int nsc = NSTEP;
	if(fabs(k[0]) > CUB_EPS*osf)
	{
		rc = gsl_poly_solve_quartic(k[1]/k[0],k[2]/k[0],k[3]/k[0],k[4]/k[0],&r[0],&r[1],&r[2],&r[3]);
		int i,j;
		for(i = 0; i < rc; ++i)
		{
			for(j = 0; j < nsc; ++j)
			{
				r[i] = newton_step_quartic(r[i],k[1]/k[0],k[2]/k[0],k[3]/k[0],k[4]/k[0]);
			}
		}
	}
	else
	{
		rc = gsl_poly_solve_cubic(k[2]/k[1],k[3]/k[1],k[4]/k[1],&r[0],&r[1],&r[2]);
		int i,j;
		for(i = 0; i < rc; ++i)
		{
			for(j = 0; j < nsc; ++j)
			{
				r[i] = newton_step_cubic(r[i],k[2]/k[1],k[3]/k[1],k[4]/k[1]);
			}
		}
	}
	if(rc <= 0)
	{
		return 0;
	}
	
	// select nearest intersection with surface
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
		if(x >= 0 && x <= 1 && y >= 0 && y <= 1 && z >= 0 && z <= 1)
		{
			float t = a.z*x*x + b.z*y*y + c.z*x*y + d.z*x + e.z*y + f.z;
			if(t > HIT_DIST*(same != 0))
			{
				if(found == 0 || t < mt)
				{
					mc = ic;
					mt = t;
					++found;
				}
			}
		}
	}
	
	// return intersection properties
	if(found)
	{
		*tp = mt;
		*cp = mc;
		// get normal
		n = normalize(cross(2*_a*mc.x + _c*mc.y + _d, 2*_b*mc.y + _c*mc.x + _e));
		n *= 1 - 2*(dot(n,dir) > 0);
		*np = n;
	}
	return found;
}

int intersect_plane(global const float3 vert[3], float3 pos, float3 dir, float *tp, float3 *cp, float3 *np)
{
	matrix3 m = get_zmat(dir);
	const float3 v[3] = 
	{
	  mvmul3(m,vert[0] - pos),
	  mvmul3(m,vert[1] - pos),
	  mvmul3(m,vert[2] - pos)
	};
	
	float3 c;
	c.y = (v[2].x*(v[0].y - v[2].y) - v[2].y*(v[0].x - v[2].x))/
	((v[0].x - v[2].x)*(v[1].y - v[2].y) - (v[1].x - v[2].x)*(v[0].y - v[2].y));
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
