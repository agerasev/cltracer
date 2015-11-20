#pragma once

#include "opencl.h"

#include "ray.h"
#include "hit.h"
#include "hit_info.h"
#include "random.h"
#include "material.h"
#include "color.h"

#define DELTA 1e-6f

float3 get_sky_color(float3 dir)
{
	//return (float3)(0.6f,0.6f,0.8f)*((float3)(dir.z,dir.z,dir.z)*0.5f + (float3)(0.5f,0.5f,0.5f));
	//return (float3)((int)(10*(dir.x + 1))%2,(int)(10*(dir.y + 1))%2,(int)(10*(dir.z + 1))%2);
	return (float3)(0.0f,0.0f,0.0f);
}

float3 reflect(float3 dir, float3 norm)
{
	return dir - 2.0f*norm*dot(dir,norm);
}

float3 diffuse(float3 norm, uint *seed)
{
	return random_hemisphere_cos(norm, seed);
}

float3 reflect_diffused(float3 dir, float3 norm, float factor, uint *seed)
{
	float3 dif = diffuse(norm,seed);
	float3 ref = reflect(dir,norm);
	float par = dot(dif,norm);
	float3 ort = dif - par*norm;
	return normalize(factor*par*ref + ort);
}

float3 direct(float3 src, float3 norm, global const float *obj, float *f, uint *seed) {
	float3 center = (float3) (0.0f, 0.0f, 0.0f);
	int i;
	for(i = 0; i < 6; ++i) {
		center += vload3(i, obj);
	}
	center /= 6.0f;
	float3 rc = center - src;
	float len2 = dot(rc, rc);
	float len = sqrt(len2);
	
	float rad2 = 0.0f;
	for(i = 0; i < 6; ++i) {
		float3 vd = vload3(i, obj) - center;
		float dist2 = dot(vd, vd);
		if(dist2 > rad2) {
			rad2 = dist2;
		}
	}
	
	float cos_alpha = sqrt(1.0 - rad2/len2);
	float saf = 1.0 - cos_alpha; // solid angle factor ((solid_angle)/(2*PI))
	float3 dir = rc/len;
	dir = random_sphere_cap(dir, cos_alpha, seed);
	*f = saf*dot(dir, norm);
	return dir;
}

float3 direct_diffused(float3 src, float3 dir, float3 norm, float factor, global const float *obj, float *f, uint *seed)
{
	float lf;
	float3 dif = direct(src,norm,obj,&lf,seed);
	float3 ref = reflect(dir,norm);
	float par = dot(dif,ref);
	par = 1.0 - factor*(1.0 - par);
	par = !!(par > 0.0)*par;
	*f = par;
	return dif;
}

__kernel void produce(
	__global const uchar *hit_data, __global uchar *ray_data, __global const uchar *hit_info,
	__global const float *lights,
	__global uint *color_buffer, const uint pitch, const uint work_size,
	__global uint *random
)
{
	const int size = get_global_size(0);
	const int pos = get_global_id(0);
	
	if(pos >= work_size)
	{
		return;
	}
	
	Material mat[4];
	mat[0].type = MAT_DIFF | MAT_REFL;
	mat[1].type = MAT_DIFF;
	mat[2].type = MAT_GLOW;
	mat[0].refl = (float3) (0.2f,0.2f,0.2f);
	mat[0].diff = (float3) (0.2f,0.2f,0.8f); 
	mat[1].diff = (float3) (0.2f,1.0f,0.2f);
	mat[2].glow = (float3) (64.0f,64.0f,48.0f);
	
	Hit hit = hit_load(pos,hit_data);
	
	float3 color = {0.0f,0.0f,0.0f};
	
	Material *m = mat;
	if(hit.object > 0 && hit.object <= 3)
		m = mat + (hit.object - 1);
	
	// glowing
	if(hit.object == 0)
	{
		color += hit.color*get_sky_color(hit.dir);
	}
	else if(hit.type == RAY_TYPE_DIRECT || hit.type == RAY_TYPE_ATTRACTED)
	{
		if(m->type & MAT_GLOW)
			color += hit.color*m->glow;
	}
	
	HitInfo info = hit_info_load(pos,hit_info);
	
	if(info.size > 0)
	{
		uint seed = random[pos];
		
		Ray ray;
		ray.pos = hit.pos + hit.norm*DELTA;
		
		ray.origin = hit.origin;
		ray.source = hit.object;
		ray.target = 0;
		
		uint count = 0;
		
		// reflection
		if(m->type & (MAT_REFL | MAT_ADIF) && info.pre_size.x)
		{
			ray.type = RAY_TYPE_DIRECT;
			if(m->type & MAT_ADIF)
			{
				/* TODO: use proper distribution */
				
				ray.color = hit.color*m->adif;
				ray.dir = reflect_diffused(hit.dir,hit.norm,8.0f,&seed);
				/*
				float f;
				ray.dir = direct_diffused(hit.pos, hit.dir, hit.norm, 8.0f, lights, &f, &seed);
				ray.color = f*hit.color*m->adif*(dot(ray.dir, hit.norm) > 0.0);
				*/
			}
			else
			{
				ray.color = hit.color*m->refl;
				ray.dir = reflect(hit.dir,hit.norm);
			}
			ray_store(&ray, info.offset + count, ray_data);
			++count;
		}
		
		// diffusion
		if(m->type & MAT_DIFF && info.pre_size.y) {
			float3 color = hit.color*m->diff;
			
			// diffuse light attraction
			{
				float f;
				ray.dir = direct(hit.pos, hit.norm, lights, &f, &seed);
				ray.color = f*color;
				ray.type = RAY_TYPE_ATTRACTED;
				if(dot(ray.dir, hit.norm) > 0.0f) {
					ray.target = 3;
					ray_store(&ray, info.offset + count, ray_data);
					++count;
				}
			}
					
			// random direction rays
			ray.color = color/(info.size - count);
			ray.type = RAY_TYPE_DIFFUSE;
			for(; count < info.size; ++count)
			{
				ray.dir = diffuse(hit.norm,&seed);
				ray.target = 0;
				ray_store(&ray,info.offset + count,ray_data);
			}
		}
		
		random[pos] = seed;
	}
	
	// replace with atomic_add for float in later version
	atomic_add(color_buffer + 3*(hit.origin.x + hit.origin.y*pitch) + 0, (uint)(COLOR_PREC*color.x));
	atomic_add(color_buffer + 3*(hit.origin.x + hit.origin.y*pitch) + 1, (uint)(COLOR_PREC*color.y));
	atomic_add(color_buffer + 3*(hit.origin.x + hit.origin.y*pitch) + 2, (uint)(COLOR_PREC*color.z));
	//vstore3(convert_uint4(0x10000*color) + vload3(hit.origin.x + hit.origin.y*pitch,color_buffer),hit.origin.x + hit.origin.y*pitch,color_buffer);
}
