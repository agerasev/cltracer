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
#define HIT_INFO_SIZE 8

typedef struct
{
	uint size;
	uint offset;
	uint2 pre_size;
	uint2 pre_offset;
	uint2 pre_offset_tmp;
}
HitInfo;

HitInfo hit_info_load(int offset, __global const uint *data)
{
	HitInfo info;
	__global const uint *info_data = data + HIT_INFO_SIZE*offset;
	info.size = info_data[0];
	info.offset = info_data[1];
	info.pre_size = vload2(1,info_data);
	info.pre_offset = vload2(2,info_data);
	info.pre_offset_tmp = vload2(3,info_data);
	return info;
}

void hit_info_store(HitInfo *info, int offset, __global uint *data)
{
	__global uint *info_data = data + HIT_INFO_SIZE*offset;
	info_data[0] = info->size;
	info_data[1] = info->offset;
	vstore2(info->pre_size,1,info_data);
	vstore2(info->pre_offset,2,info_data);
	vstore2(info->pre_offset_tmp,3,info_data);
}
uint random_next(uint *seed)
{
	return (*seed = 1103515245**seed + 12345);
}

float random_unif(uint seed)
{
	return (float)seed/(float)0xffffffff;
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
	__global uint *hit_info, __global const uint *work_size
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
	
	HitInfo info;
	info.size = 2*(hit_obj > 0);
	info.offset = 2*(hit_obj > 0);
	hit_info_store(&info,pos,hit_info);
	
	hit_store(&hit,pos,hit_fdata,hit_idata);
}
#define DELTA 1e-8f

float3 get_sky_color(float3 dir)
{
	return (float3)(dir.z,dir.z,dir.z)*0.5f + (float3)(0.5f,0.5f,0.5f);
}

float3 reflect(float3 dir, float3 norm)
{
	return dir - 2.0f*norm*dot(dir,norm);
}

float3 diffuse(float3 dir, float3 norm, uint *seed)
{
	float3 nx, ny;
	if(dot((float3)(0.0f,0.0f,1.0f),norm) < 0.6 && dot((float3)(0.0f,0.0f,1.0f),norm) > -0.6)
	{
		nx = (float3)(0.0f,0.0f,1.0f);
	}
	else
	{
		nx = (float3)(1.0f,0.0f,0.0f);
	}
	ny = normalize(cross(nx,norm));
	nx = cross(ny,norm);

	float phi = 2.0f*M_PI_F*random_unif(random_next(seed));
	float theta = acos(1.0f - 2.0f*random_unif(random_next(seed)))/2.0f;
	return nx*cos(phi)*sin(theta) + ny*sin(phi)*sin(theta) + norm*cos(theta);
}

__kernel void produce(
	__global const float *hit_fdata, __global const int *hit_idata,
	__global float *ray_fdata, __global int *ray_idata, __global const uint *hit_info,
	__global uint *color_buffer, __global const uint *pitch, __global const uint *work_size,
	__global uint *random
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
		uint seed = random[pos];
		
		Ray ray;
		ray.pos = hit.pos + hit.norm*DELTA;
		
		ray.dir = diffuse(hit.dir,hit.norm,&seed); //reflect(hit.dir,hit.norm);
		
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
		ray.color /= 2.0f;
		
		HitInfo info = hit_info_load(pos,hit_info);
		
		ray.dir = diffuse(hit.dir,hit.norm,&seed); //reflect(hit.dir,hit.norm);
		ray_store(&ray,info.offset - info.size,ray_fdata,ray_idata);
		
		ray.dir = diffuse(hit.dir,hit.norm,&seed); //reflect(hit.dir,hit.norm);
		ray_store(&ray,info.offset - info.size + 1,ray_fdata,ray_idata);
		
		random[pos] = seed;
	}
	
	// replace with atomic_add for float in later version
	atomic_add(color_buffer + 3*(hit.origin.x + hit.origin.y**pitch) + 0, (uint)(0x10000*color.x));
	atomic_add(color_buffer + 3*(hit.origin.x + hit.origin.y**pitch) + 1, (uint)(0x10000*color.y));
	atomic_add(color_buffer + 3*(hit.origin.x + hit.origin.y**pitch) + 2, (uint)(0x10000*color.z));
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
__kernel void compact(__global uint *hit_info, __global uint *ray_count, __global const uint *deviation, __global const uint *work_size)
{
	const int size = *work_size;//get_global_size(0);
	const int pos = get_global_id(0);
	const int dev = *deviation;
	const int edev = (1<<dev);
	
	if(pos >= size)
	{
		return;
	}
	
	uint2 sum;
	if(pos >= edev)
	{
		sum.x = hit_info[HIT_INFO_SIZE*(pos-edev) + (dev%2) + 1] + hit_info[HIT_INFO_SIZE*pos + (dev%2) + 1];
		//sum.y = hit_info[HIT_INFO_SIZE*(pos-edev) + 2*(dev%2) + 5] + hit_info[HIT_INFO_SIZE*pos + 2*(dev%2) + 5];
	}
	else
	{
		sum.x = hit_info[HIT_INFO_SIZE*pos + (dev%2) + 1];
		//sum.y = hit_info[HIT_INFO_SIZE*pos + (dev%2) + 2];
	}
	
	hit_info[HIT_INFO_SIZE*pos + ((dev+1)%2) + 1] = sum.x;
	//hit_info[HIT_INFO_SIZE*pos + 2*((dev+1)%2) + 5] = sum.y;
	
	if(pos == size - 1)
	{
		ray_count[0] = sum.x;
		//ray_count[1] = sum.y;
	}
}
__kernel void expand(__global uint *hit_info, __global const uint *diffuse_factor, __global const uint *work_size)
{
	const uint size = *work_size;//get_global_size(0);
	const uint pos = get_global_id(0);
	const uint factor = *diffuse_factor;
	if(pos >= size)
	{
		return;
	}
	HitInfo info = hit_info_load(pos,hit_info);
	info.size = info.pre_size.x + info.pre_size.y*factor;
	info.offset = info.pre_offset.x + info.pre_offset.y*factor - info.size;
}
