#define RAY_FSIZE 9
#define RAY_ISIZE 2

typedef struct
{
	float3 pos;
	float3 dir;
	float3 color;
	int2 origin;
} 
Ray;

void ray_load_fdata(Ray *ray, int offset, __global const float *data)
{
	__global const float *ray_data = data + offset*RAY_FSIZE;
	ray->pos = vload3(0,ray_data);
	ray->dir = vload3(1,ray_data);
	ray->color = vload3(2,ray_data);
}

void ray_load_idata(Ray *ray, int offset, __global const int *data)
{
	__global const int *ray_data = data + offset*RAY_ISIZE;
	ray->origin = vload2(0,ray_data);
}

Ray ray_load(int offset, __global const float *fdata, __global const int *idata)
{
	Ray ray;
	ray_load_fdata(&ray,offset,fdata);
	ray_load_idata(&ray,offset,idata);
	return ray;
}

void ray_store(Ray *ray, int offset, __global float *fdata, __global int *idata)
{
	__global float *ray_fdata = fdata + offset*RAY_FSIZE;
	__global int *ray_idata = idata + offset*RAY_ISIZE;
	vstore3(ray->pos,0,ray_fdata);
	vstore3(ray->dir,1,ray_fdata);
	vstore3(ray->color,2,ray_fdata);
	vstore2(ray->origin,0,ray_idata);
}
