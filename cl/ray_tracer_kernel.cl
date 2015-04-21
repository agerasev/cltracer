#define DELTA 1e-8

typedef struct
{
	float3 pos;
	float3 ori[3]; // replace with float3x3 in later version
	float fov;
} Camera;

Camera camera_load(__constant const float *data)
{
	Camera cam;
	cam.pos = vload3(0,data);
	cam.ori[0] = vload3(1,data);
	cam.ori[1] = vload3(2,data);
	cam.ori[2] = vload3(3,data);
	cam.fov = data[12];
	return cam;
}

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

#define HIT_FSIZE 12
#define HIT_ISIZE 3

#define HIT_INFO_SIZE 2

typedef struct
{
	float3 pos;
	float3 dir;
	float3 norm;
	float3 color;
	int2 origin;
	int object;
}
Hit;

void hit_load_fdata(Hit *hit, int offset, __global const float *data)
{
	__global const float *hit_data = data + offset*HIT_FSIZE;
	hit->pos = vload3(0,hit_data);
	hit->dir = vload3(1,hit_data);
	hit->norm = vload3(2,hit_data);
	hit->color = vload3(3,hit_data);
}

void hit_load_idata(Hit *hit, int offset, __global const int *data)
{
	__global const int *hit_data = data + offset*HIT_ISIZE;
	hit->origin = vload2(0,hit_data);
	hit->object = hit_data[2];
}

Hit hit_load(int offset, __global const float *fdata, __global const int *idata)
{
	Hit hit;
	hit_load_fdata(&hit,offset,fdata);
	hit_load_idata(&hit,offset,idata);
	return hit;
}

void hit_store(Hit *hit, int offset, __global float *fdata, __global int *idata)
{
	__global float *hit_fdata = fdata + offset*HIT_FSIZE;
	__global int *hit_idata = idata + offset*HIT_ISIZE;
	vstore3(hit->pos,0,hit_fdata);
	vstore3(hit->dir,1,hit_fdata);
	vstore3(hit->norm,2,hit_fdata);
	vstore3(hit->color,3,hit_fdata);
	vstore2(hit->origin,0,hit_idata);
	hit_idata[2] = hit->object;
}

float3 get_sky_color(float3 dir)
{
	return (float3)(dir.z,dir.z,dir.z)*0.5f + (float3)(0.5f,0.5f,0.5f);
}

__kernel void start(__global float *ray_fdata, __global int *ray_idata, __constant float *cam_fdata)
{
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const int2 pos = (int2)(get_global_id(0), get_global_id(1));
	const float2 cpos = (2.0f*(float2)(pos.x,-pos.y) - (float2)(size.x,-size.y))/(float)size.y;	

	Camera cam = camera_load(cam_fdata);
	
	Ray ray;
	ray.origin = pos;
	ray.pos = cam.pos;
	ray.dir = normalize(cam.ori[2] + cam.fov*(cam.ori[0]*cpos.x + cam.ori[1]*cpos.y));
	ray.color = (float3)(1.0f,1.0f,1.0f);
	
	ray_store(&ray, size.x*pos.y + pos.x, ray_fdata, ray_idata);
}

__kernel void intersect(
	__global const float *ray_fdata, __global const int *ray_idata,
	__global float *hit_fdata, __global int *hit_idata, 
	__global int *hit_info, __global const uint *work_size
)
{
	const int size = get_global_size(0);
	const int pos = get_global_id(0);
	
	if(pos >= *work_size)
	{
		return;
	}
	
	Ray ray = ray_load(pos,ray_fdata,ray_idata);
	
	// Collide with uniform sphere
	const float3 sph_pos[3] = {(float3)(0.0f,4.0f,0.0f),(float3)(3.0f,6.0f,0.0f),(float3)(0.0f,4.0f,-4.0f)};
	const float sph_rad[3] = {1.0f,1.6f,2.4f};
	int i, hit_obj = 0;
	float min_dist;
	float3 hit_pos, hit_norm;
	for(i = 0; i < 3; ++i)
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
	
	Hit hit;
	hit.pos = hit_pos;
	hit.dir = ray.dir;
	hit.norm = hit_norm;
	hit.color = ray.color;
	hit.origin = ray.origin;
	hit.object = hit_obj;
	
	hit_info[HIT_INFO_SIZE*pos] = (hit_obj > 0);
	
	hit_store(&hit,pos,hit_fdata,hit_idata);
}

__kernel void produce(
	__global const float *hit_fdata, __global const int *hit_idata,
	__global float *ray_fdata, __global int *ray_idata, __global const uint *hit_info,
	__global uint *color_buffer, __global const uint *pitch, __global const uint *work_size
)
{
	const int size = get_global_size(0);
	const int pos = get_global_id(0);
	
	if(pos >= *work_size)
	{
		return;
	}
	
	Hit hit = hit_load(pos,hit_fdata,hit_idata);
	float3 color = hit.color;
	
	if(hit.object == 0)
	{
		color *= get_sky_color(hit.dir);
	}
	else
	{
		color *= (float3)(0.0f,0.0f,0.0f);
	}
	
	if(hit_info[HIT_INFO_SIZE*pos + 0] > 0)
	{
		Ray ray;
		ray.pos = hit.pos + hit.norm*DELTA;
		ray.dir = hit.dir - 2.0f*hit.norm*dot(hit.dir,hit.norm);
		ray.color = hit.color;
		ray.origin = hit.origin;
		switch(hit.object)
		{
		case 1:
			ray.color *= (float3)(0.4f,0.4f,1.0f);
			break;
		case 2:
			ray.color *= (float3)(1.0f,0.4f,0.4f);
			break;
		case 3:
			ray.color *= (float3)(0.4f,1.0f,0.4f);
			break;
		}
		ray_store(&ray,hit_info[HIT_INFO_SIZE*pos + 1],ray_fdata,ray_idata);
	}
	
	// replace with atomic_add for float in later version
	atomic_add(color_buffer + 3*(hit.origin.x + hit.origin.y*800) + 0, (uint)(0x10000*color.x));
	atomic_add(color_buffer + 3*(hit.origin.x + hit.origin.y*800) + 1, (uint)(0x10000*color.y));
	atomic_add(color_buffer + 3*(hit.origin.x + hit.origin.y*800) + 2, (uint)(0x10000*color.z));
}

__kernel void draw(__global uint *color_buffer, __write_only image2d_t image)
{
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const int2 pos = (int2)(get_global_id(0), get_global_id(1));
	
	uint3 icolor = vload3(pos.x + size.x*pos.y,color_buffer);
	float3 color = (float3)((float)icolor.x,(float)icolor.y,(float)icolor.z)/(float)0x10000;
	
	vstore3((uint3)(0,0,0),pos.x + size.x*pos.y,color_buffer);
	
	write_imagef(image,pos,(float4)(color,1.0f));
}

__kernel void compact(__global int *hit_info, __global uint *ray_count)
{
	const uint size = *ray_count;
	uint sum = 0;
	uint i;
	for(i = 0; i < size; ++i)
	{
		hit_info[HIT_INFO_SIZE*i + 1] = sum;
		sum += hit_info[HIT_INFO_SIZE*i + 0];
	}
	*ray_count = sum;
}
