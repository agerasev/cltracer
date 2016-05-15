#pragma once

#include <opencl.h>

#define OBJECT_SIZE 0

typedef struct Object {
	
} Object;

#define SPHERE_SIZE (OBJECT_SIZE + 4*sizeof(float))

typedef struct Sphere {
	Object obj;
	// float3 p;
	// float r;
} Sphere;

#define TRIANGLE_SIZE (OBJECT_SIZE + 3*3*sizeof(float))

typedef struct Triangle {
	Object obj;
	float3 v[3];
} Triangle;

#define SURFACE_SIZE (OBJECT_SIZE + 3*6*sizeof(float))

typedef struct Surface {
	Object obj;
	float3 v[6];
} Surface;

Object object_load(global const uchar *data) {
	Object obj;
	return obj;
}

void object_store(Object *obj, global uchar *data) {
	
}

Sphere sphere_load(global const uchar *data) {
	Sphere sph;
	sph.obj = object_load(data);
	// global const float *fdata = (global const float *) (data + OBJECT_SIZE);
	// sph.p = vload3(0, fdata);
	// sph.r = fdata[3];
	return sph;
}

void sphere_store(Sphere *sph, global uchar *data) {
	object_store(sph->obj, data);
	// global float *fdata = (global float *) (data + OBJECT_SIZE);
	// vstore3(sph->p, 0, fdata);
	// fdata[3] = sph.r;
}

Triangle triangle_load(global const uchar *data) {
	Triangle tri;
	tri.obj = object_load(data);
	global const float *fdata = (global const float *) (data + OBJECT_SIZE);
	int i;
	for(i = 0; i < 3; ++i)
		tri.v[i] = vload3(i, fdata);
	return tri;
}

void triangle_store(Triangle *tri, global uchar *data) {
	object_store(tri->obj, data);
	global float *fdata = (global float *) (data + OBJECT_SIZE);
	int i;
	for(i = 0; i < 3; ++i)
		vstore3(tri->v[i], i, fdata);
}

Surface surface_load(global const uchar *data) {
	Surface sur;
	sur.obj = object_load(data);
	global const float *fdata = (global const float *) (data + OBJECT_SIZE);
	int i;
	for(i = 0; i < 6; ++i)
		sur.v[i] = vload3(i, fdata);
	return sur;
}

void surface_store(Surface *sur, global uchar *data) {
	object_store(sur->obj, data);
	global float *fdata = (global float *) (data + OBJECT_SIZE);
	int i;
	for(i = 0; i < 6; ++i)
		vstore3(sur->v[i], i, fdata);
}
