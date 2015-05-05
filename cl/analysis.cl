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
#define NSTEP    2

int solve2(const float3 v[6], float osf, float3 pos, float3 dir, float *tp, float3 *cp, float3 *np)
{
	// get transformation
	matrix3 m = get_zmat(dir);
	
	// choose the variable for equation
	int var = 0; // 0(x), 1(y), 2(z)
	int iv;
	float mtp = 0.0;
	for(iv = 0; iv < 3; ++iv)
	{
		float _tp = fabs(dot(dir,cross(v[(iv+1)%3]-v[(iv+1)%3 + 3],v[(iv+2)%3]-v[(iv+1)%3 + 3])));
		if(iv == 0 || _tp >= mtp)
		{
			var = iv;
			mtp = _tp;
		}
	}
	const float3 nv[6] = 
	{
		v[(var+2)%3], v[(var+0)%3], v[(var+1)%3],
		v[(var+2)%3 + 3], v[(var+0)%3 + 3], v[(var+1)%3 + 3]
	};
	
	// compute factors for quartic equation
	float3 
	  _a = nv[0] + nv[2] - 2*nv[5],
	  _b = nv[1] + nv[2] - 2*nv[4],
	  _c = 2*(nv[2] + nv[3] -  nv[4] - nv[5]),
	  _d = 2*(nv[5] - nv[2]),
	  _e = 2*(nv[4] - nv[2]),
	  _f = nv[2];
	float3 
	  a = mvmul3(m,_a),
	  b = mvmul3(m,_b),
	  c = mvmul3(m,_c),
	  d = mvmul3(m,_d),
	  e = mvmul3(m,_e),
	  f = mvmul3(m,_f - pos);
	float k[5] = {
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
			if(t > HIT_DIST)
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
	
	// return intersection properties
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
