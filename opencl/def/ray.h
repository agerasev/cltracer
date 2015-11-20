#pragma once

#define RAY_FSIZE (9*sizeof(float))
#define RAY_ISIZE (5*sizeof(int))
#define RAY_FOFFSET 0
#define RAY_IOFFSET RAY_FSIZE
#define RAY_SIZE (RAY_FSIZE + RAY_ISIZE)

#define RAY_TYPE_DIRECT    0x01
#define RAY_TYPE_DIFFUSE   0x02
#define RAY_TYPE_ATTRACTED 0x03

typedef struct
{
	float3 pos;
	float3 dir;
	float3 color;
	int2 origin;
	int source;
	int target;
	int type;
} 
Ray;
