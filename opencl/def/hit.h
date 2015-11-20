#pragma once

#define HIT_FSIZE (12*sizeof(float))
#define HIT_ISIZE (4*sizeof(int))
#define HIT_FOFFSET 0
#define HIT_IOFFSET HIT_FSIZE
#define HIT_SIZE (HIT_FSIZE + HIT_ISIZE)

#define HIT_TYPE_DIRECT  0x01
#define HIT_TYPE_DIFFUSE 0x02

typedef struct
{
	float3 pos;
	float3 dir;
	float3 norm;
	float3 color;
	int2 origin;
	int object;
	int type;
}
Hit;
