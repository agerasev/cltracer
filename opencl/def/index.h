#pragma once

#include <opencl.h>

#define INDEX_ISIZE (4*sizeof(int))
#define INDEX_FSIZE (4*sizeof(float))
#define INDEX_IOFFSET 0
#define INDEX_FOFFSET INDEX_ISIZE
#define INDEX_SIZE (INDEX_ISIZE + INDEX_FSIZE)

#define OBJECT_TYPE_SPHERE   0x01
#define OBJECT_TYPE_TRIANGLE 0x02
#define OBJECT_TYPE_SURFACE  0x03

#define OBJECT_PROP_ATTRACTIVE 0x01

typedef struct {
	int id;
	int type;
	int prop;
	int ptr;
	float3 pos;
	float rad;
} Index;

Index index_load(int offset, global const uchar *index_data) {
	Index idx;
	global const uchar *data = index_data + offset*INDEX_SIZE;
	global const int *idata = (global const int *) (data + INDEX_IOFFSET);
	global const float *fdata = (global const float *) (data + INDEX_FOFFSET);
	idx.id   = idata[0];
	idx.type = idata[1];
	idx.prop = idata[2];
	idx.ptr  = idata[3];
	idx.pos = vload3(0, fdata);
	idx.rad = fdata[3];
	return idx;
}

void index_store(const Index *idx, int offset, global uchar *index_data) {
	global uchar *data = index_data + offset*INDEX_SIZE;
	global int *idata = (global int *) (data + INDEX_IOFFSET);
	global float *fdata = (global float *) (data + INDEX_FOFFSET);
	idata[0] = idx->id;
	idata[1] = idx->type;
	idata[2] = idx->prop;
	idata[3] = idx->ptr;
	vstore3(idx->pos, 0, fdata);
	fdata[3] = idx->rad;
}
