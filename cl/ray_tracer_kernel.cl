typedef float2 cfloat;

#define ITER 0x40

cfloat cmul(const cfloat a, const cfloat b)
{
	return (cfloat)(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

typedef struct
{
	float3 pos;
	float3 ori[3]; // replace with float3x3 in later version
	float fov;
} Camera;

#define RAY_DATA_SIZE   9
#define RAY_ORIGIN_SIZE 2

typedef struct
{
	float3 pos;
	float3 dir;
	float3 color;
	int2 origin;
} Ray;

Camera camera_load(__constant float *data)
{
	Camera cam;
	cam.pos = vload3(0,data);
	cam.ori[0] = vload3(1,data);
	cam.ori[1] = vload3(2,data);
	cam.ori[2] = vload3(3,data);
	cam.fov = data[12];
	return cam;
}

void ray_load_data(Ray *ray, int offset, __global float *data)
{
	__global float *ray_data = data + offset*RAY_DATA_SIZE;
	ray->pos = vload3(0,ray_data);
	ray->dir = vload3(1,ray_data);
	ray->color = vload3(2,ray_data);
}

int2 ray_load_origin(int offset, __global int *origin)
{
	return vload2(0,origin + offset*RAY_ORIGIN_SIZE);
}

Ray ray_load(int offset, __global float *data, __global int *origin)
{
	Ray ray;
	ray.origin = ray_load_origin(offset,origin);
	ray_load_data(&ray,offset,data);
	return ray;
}

void ray_store(Ray *ray, int offset, __global float *data, __global int *origin)
{
	__global float *ray_data = data + offset*RAY_DATA_SIZE;
	__global int *ray_origin = origin + offset*RAY_ORIGIN_SIZE;
	vstore3(ray->pos,0,ray_data);
	vstore3(ray->dir,1,ray_data);
	vstore3(ray->color,2,ray_data);
	vstore2(ray->origin,0,ray_origin);
}

__kernel void start(__global float *ray_data, __global int *ray_origin, __constant float *cam_data)
{
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const int2 pos = (int2)(get_global_id(0), get_global_id(1));
	const float2 cpos = (2.0f*(float2)(pos.x,-pos.y) - (float2)(size.x,-size.y))/(float)size.y;	

	Camera cam = camera_load(cam_data);
	
	Ray ray;
	ray.origin = pos;
	ray.pos = cam.pos;
	ray.dir = normalize(cam.ori[2] + cam.fov*(cam.ori[0]*cpos.x + cam.ori[1]*cpos.y));
	ray.color = (float3)(1.0f,1.0f,1.0f);
	
	ray_store(&ray, size.x*pos.y + pos.x, ray_data, ray_origin);
}

__kernel void trace(__global float *ray_data, __global int *ray_origin, __write_only image2d_t output)
{
	const int size = get_global_size(0);
	const int pos = get_global_id(0);
	
	Ray ray = ray_load(pos,ray_data,ray_origin);
	
	// Collide with uniform sphere
	const float3 sph_pos[2] = {(float3)(0.0f,4.0f,0.0f),(float3)(3.0f,6.0f,0.0f)};
	const float sph_rad[2] = {1.0f,1.6f};
	int i, c = 0;
	float dist;
	for(i = 0; i < 2; ++i)
	{
		float d = dot(sph_pos[i] - ray.pos,ray.dir);
		if(d > 0.0f && length(ray.pos + d*ray.dir - sph_pos[i]) < sph_rad[i])
		{
			if(c == 0 || d < dist)
			{
				dist = d;
				c = i + 1;
			}
		}
	}
	switch(c)
	{
	case 1:
		ray.color = (float3)(0.0f,0.0f,1.0f);
		break;
	case 2:
		ray.color = (float3)(1.0f,0.0f,0.0f);
		break;
	default:
		ray.color = (float3)(ray.dir.z,ray.dir.z,ray.dir.z)*0.5f + (float3)(0.5f,0.5f,0.5f);
		break;
	}
	
		
	write_imagef(output,ray.origin,(float4)(ray.color,1.0f));
}
