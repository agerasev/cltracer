/** ray.cl */

#define RAY_FSIZE (9*sizeof(float))
#define RAY_ISIZE (2*sizeof(int))
#define RAY_FOFFSET 0
#define RAY_IOFFSET RAY_FSIZE
#define RAY_SIZE (RAY_FSIZE + RAY_ISIZE)

typedef struct
{
	float3 pos;
	float3 dir;
	float3 color;
	int2 origin;
} 
Ray;

Ray ray_load(int offset, __global const uchar *ray_data)
{
	Ray ray;
	__global const uchar *data = ray_data + offset*RAY_SIZE;
	__global const float *fdata = (__global const float*)(data + RAY_FOFFSET);
	__global const int *idata = (__global const int*)(data + RAY_IOFFSET);
	ray.pos = vload3(0,fdata);
	ray.dir = vload3(1,fdata);
	ray.color = vload3(2,fdata);
	ray.origin = vload2(0,idata);
	return ray;
}

void ray_store(Ray *ray, int offset, __global uchar *ray_data)
{
	__global uchar *data = ray_data + offset*RAY_SIZE;
	__global float *fdata = (__global float*)(data + RAY_FOFFSET);
	__global int *idata = (__global int*)(data + RAY_IOFFSET);
	vstore3(ray->pos,0,fdata);
	vstore3(ray->dir,1,fdata);
	vstore3(ray->color,2,fdata);
	vstore2(ray->origin,0,idata);
}
