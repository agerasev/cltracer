/* geometry.cl */

int intersect_triangle(const float3 v[3], const float3 r[2], float *tp, float3 *cp, float3 *np)
{
	float3 bx = v[1] - v[0], by = v[2] - v[0];
	float3 n = cross(bx,by);
	float t = dot(n, v[0] - r[0])/dot(n, r[1]);
	if(t < 0)
		return 0;
	float3 p = r[0] + r[1]*t;
	float3 rp = p - v[0];
	float lxx = dot(bx,bx), lxy = dot(bx,by), lyy = dot(by,by);
	float3 c;
	c.x = (lyy*dot(bx,rp) - lxy*dot(by,rp))/(lxx*lyy - lxy*lxy);
	c.y = (lxx*dot(by,rp) - lxy*dot(bx,rp))/(lxx*lyy - lxy*lxy);
	c.z = 1 - c.x - c.y;
	if(c.x < 0 || c.x > 1 || c.y < 0 || c.y > 1 || c.z < 0 || c.z > 1)
		return 0;
	if(dot(n,r[1]) > 0)
		n = -n;//return 0;
	n = normalize(n);
	*tp = t;
	*cp = c;
	*np = n;
	return 1;
}

float3 get_biquadratic_point(const float3 v[6], const float3 c)
{
	return v[0]*c.z*c.z + v[1]*c.x*c.x + v[2]*c.y*c.y + 2*v[3]*c.z*c.x + 2*v[4]*c.x*c.y + 2*v[5]*c.y*c.z;
}

float3 get_bicubic_point(const float3 v[10], const float3 c)
{
	return 
	  v[0]*c.x*c.x*c.x + v[1]*c.y*c.y*c.y + v[2]*c.z*c.z*c.z + 
	  3*v[3]*c.x*c.x*c.y + 3*v[4]*c.y*c.y*c.x + 3*v[5]*c.y*c.y*c.z +
	  3*v[6]*c.z*c.z*c.y + 3*v[7]*c.z*c.z*c.x + 3*v[8]*c.x*c.x*c.z +
	  6*v[9]*c.x*c.y*c.z;
}

/*
void get_biquadratic_jacobian(float v[6], float3 c, float3 d[2])
{
	return v[0]*c.z*c.z + v[1]*c.x*c.x + v[2]*c.y*c.y + 2*v[3]*c.z*c.x + 2*v[4]*c.x*c.y + 2*v[5]*c.y*c.z;
}
*/

void get_bicubic_jacobian(const float3 v[10], const float3 c, float3 dp[2])
{
	dp[0] = 
	  3*v[0]*c.x*c.x - 3*v[2]*c.z*c.z + 
	  6*v[3]*c.x*c.y + 3*v[4]*c.y*c.y - 3*v[5]*c.y*c.y -
	  6*v[6]*c.z*c.y + 3*v[7]*(c.z*c.z - 2*c.z*c.x) + 3*v[8]*(2*c.x*c.z - c.x*c.x) +
	  6*v[9]*(c.y*c.z - c.x*c.y);
	dp[1] = 
		3*v[1]*c.y*c.y - 3*v[2]*c.z*c.z + 
	  3*v[3]*c.x*c.x + 6*v[4]*c.y*c.x + 3*v[5]*(2*c.y*c.z - c.y*c.y) +
	  3*v[6]*(c.z*c.z - 2*c.z*c.y) - 6*v[7]*c.z*c.x - 3*v[8]*c.x*c.x +
	  6*v[9]*(c.x*c.z - c.x*c.y);
}

void get_subtriangle_vertices(const float3 v[10], const int2 p, const int l, float3 cp[3], float3 vp[3])
{
	int s = 1<<l;
	cp[0] = 
		(1 - (p.x%2))*(float3)((float)(p.x/2)/s,(float)p.y/s,0.0) + 
		(p.x%2)*(float3)((float)(p.x/2+1)/s,(float)p.y/s,0.0);
	cp[1] = 
		(1 - (p.x%2))*(float3)((float)(p.x/2+1)/s,(float)p.y/s,0.0) + 
		(p.x%2)*(float3)((float)(p.x/2+1)/s,(float)(p.y+1)/s,0.0);
	cp[2] = 
		(1 - (p.x%2))*(float3)((float)(p.x/2)/s,(float)(p.y+1)/s,0.0) + 
		(p.x%2)*(float3)((float)(p.x/2)/s,(float)(p.y+1)/s,0.0);
	cp[0].z = 1 - cp[0].x - cp[0].y;
	cp[1].z = 1 - cp[1].x - cp[1].y;
	cp[2].z = 1 - cp[2].x - cp[2].y;
	vp[0] = get_bicubic_point(v,cp[0]);
	vp[1] = get_bicubic_point(v,cp[1]);
	vp[2] = get_bicubic_point(v,cp[2]);
}

int2 get_subtriangle_index(const int l, const float3 c)
{
	int sx = (2<<l) - 1, s = 1<<l;
	int2 i;
	i.y = (int)(s*c.y);
	i.y -= (i.y >= s)*(i.y - s + 1);
	i.x = (int)(s*c.x);
	i.x -= (i.x >= s)*(i.x - s + 1);
	float2 lc = s*c.xy - (float2)(i.x,i.y);
	i.x = 2*i.x + (lc.x + lc.y > 1);
	return i;
}

int intersect_bicubic_surface(const float3 v[10], const float3 r[2], float *tp, float3 *cp, float3 *np)
{
	int lev = 2;
	int ix, iy;
	
	int found = 0;
	
	float mt;
	float3 c,n;

	int sx = (2<<lev) - 1, sy = 1<<lev;
	for(iy = 0; iy < sy; ++iy)
	{
		for(ix = 0; ix < sx - 2*iy; ++ix)
		{
			float3 vc[3],vv[3];
			get_subtriangle_vertices(v,(int2)(ix,iy),lev,vc,vv);
			float3 lc,ln;
			float t;
			if(intersect_triangle(vv,r,&t,&lc,&ln))
			{
				if(!found || t < mt)
				{
					++found;
					mt = t;
					c = lc;
					n = ln;
				}
			}
		}
	}
	
	*tp = mt;
	*cp = c;
	*np = n;
	return found;
}
