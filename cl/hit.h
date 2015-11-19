#pragma once

#include "opencl.h"

#define HIT_FSIZE (12*sizeof(float))
#define HIT_ISIZE (3*sizeof(int))
#define HIT_FOFFSET 0
#define HIT_IOFFSET HIT_FSIZE
#define HIT_SIZE (HIT_FSIZE + HIT_ISIZE)

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

Hit hit_load(int offset, __global const uchar *hit_data)
{
	Hit hit;
	__global const uchar *data = hit_data + offset*HIT_SIZE;
	__global const float *fdata = (__global const float*)(data + HIT_FOFFSET);
	__global const int *idata = (__global const int*)(data + HIT_IOFFSET);
	hit.pos = vload3(0,fdata);
	hit.dir = vload3(1,fdata);
	hit.norm = vload3(2,fdata);
	hit.color = vload3(3,fdata);
	hit.origin = vload2(0,idata);
	hit.object = idata[2];
	return hit;
}

void hit_store(Hit *hit, int offset, __global uchar *hit_data)
{
	__global uchar *data = hit_data + offset*HIT_SIZE;
	__global float *fdata = (__global float*)(data + HIT_FOFFSET);
	__global int *idata = (__global int*)(data + HIT_IOFFSET);
	vstore3(hit->pos,0,fdata);
	vstore3(hit->dir,1,fdata);
	vstore3(hit->norm,2,fdata);
	vstore3(hit->color,3,fdata);
	vstore2(hit->origin,0,idata);
	idata[2] = hit->object;
}
