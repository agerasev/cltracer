#pragma once

#define HIT_INFO_SIZE (6*sizeof(int))

typedef struct
{
	uint size;
	uint offset;
	uint2 pre_offset;
	uint2 pre_size;
}
HitInfo;
