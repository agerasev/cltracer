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
