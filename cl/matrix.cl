/* matrix.cl */

typedef struct
{
	float2 m[2];
}
matrix2;

typedef struct
{
	float3 m[3];
}
matrix3;

float2 mvmul2(const matrix2 m, const float2 v)
{
	return (float2)(dot(m.m[0],v),dot(m.m[1],v));
}

float3 mvmul3(const matrix3 m, const float3 v)
{
	return (float3)(dot(m.m[0],v),dot(m.m[1],v),dot(m.m[2],v));
}

float det2(const matrix2 m)
{
	 return m.m[0].x*m.m[1].y - m.m[0].y*m.m[1].x;
}

matrix2 invert2(const matrix2 m)
{
	matrix2 im;
	float det = det2(m);
	im.m[0].x = +m.m[1].y/det;
	im.m[1].x = -m.m[0].y/det;
	im.m[0].y = -m.m[1].x/det;
	im.m[1].y = +m.m[0].x/det;
	return im;
}

matrix2 transpose2(const matrix2 m)
{
	matrix2 tm;
	tm.m[0].x = m.m[0].x;
	tm.m[1].x = m.m[0].y;
	tm.m[0].y = m.m[1].x;
	tm.m[1].y = m.m[1].y;
	return tm;
}

float det3(const matrix3 m)
{
	return
	  m.m[0].x*m.m[1].y*m.m[2].z - m.m[0].x*m.m[1].z*m.m[2].y +
	  m.m[0].y*m.m[1].z*m.m[2].x - m.m[0].y*m.m[1].x*m.m[2].z +
	  m.m[0].z*m.m[1].x*m.m[2].y - m.m[0].z*m.m[1].y*m.m[2].x;
}

matrix3 invert3(const matrix3 m)
{
	matrix3 im;
	float det = det3(m);
	im.m[0].x = +(m.m[1].y*m.m[2].z - m.m[1].z*m.m[2].y)/det;
	im.m[1].x = -(m.m[1].x*m.m[2].z - m.m[1].z*m.m[2].x)/det;
	im.m[2].x = +(m.m[1].x*m.m[2].y - m.m[1].y*m.m[2].x)/det;
	im.m[0].y = -(m.m[0].y*m.m[2].z - m.m[0].z*m.m[2].y)/det;
	im.m[1].y = +(m.m[0].x*m.m[2].z - m.m[0].z*m.m[2].x)/det;
	im.m[2].y = -(m.m[0].x*m.m[2].y - m.m[0].y*m.m[2].x)/det;
	im.m[0].z = +(m.m[0].y*m.m[1].z - m.m[0].z*m.m[1].y)/det;
	im.m[1].z = -(m.m[0].x*m.m[1].z - m.m[0].z*m.m[1].x)/det;
	im.m[2].z = +(m.m[0].x*m.m[1].y - m.m[0].y*m.m[1].x)/det;
	return im;
}

matrix3 transpose3(const matrix3 m)
{
	matrix3 tm;
	tm.m[0] = (float3)(m.m[0].x,m.m[1].x,m.m[2].x);
	tm.m[1] = (float3)(m.m[0].y,m.m[1].y,m.m[2].y);
	tm.m[2] = (float3)(m.m[0].z,m.m[1].z,m.m[2].z);
	return tm;
}
