#pragma once

#define MAT_FSIZE   (13*sizeof(float))
#define MAT_ISIZE   (1*sizeof(int))
#define MAT_FOFFSET 0
#define MAT_IOFFSET MAT_FSIZE
#define MAT_SIZE    (MAT_FSIZE + MAT_ISIZE)

#define MAT_REFL 0x01
#define MAT_DIFF 0x02
#define MAT_ADIF 0x04
#define MAT_GLOW 0x08

typedef struct Matrial {
	float3 refl; // reflection
	float3 diff; // diffusion
	float3 adif; // anisotropic diffusion
	float3 glow; // light emission
	float fadif; // anisotropy factor
	int type;
} Material;
