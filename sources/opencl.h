#pragma once

#define global
#define __global
#define local
#define __local
#define constant
#define __constant

typedef unsigned char uchar;
typedef unsigned int  uint;

template <typename T, int N>
struct _CL_TN {
	T data[N];
};

template <typename T>
struct _CL_TN<T,2> {
	T x, y;
};

template <typename T>
struct _CL_TN<T,3> {
	T x, y, z;
};

template <typename T>
struct _CL_TN<T,4> {
	T x, y, z, w;
};

typedef _CL_TN<float,2> float2;
typedef _CL_TN<float,3> float3;
typedef _CL_TN<float,4> float4;
typedef _CL_TN<uint,2> uint2;
typedef _CL_TN<uint,3> uint3;
typedef _CL_TN<uint,4> uint4;
typedef _CL_TN<int,2> int2;
typedef _CL_TN<int,3> int3;
typedef _CL_TN<int,4> int4;

template <typename T>
_CL_TN<T,2> vload2(int offset, const T *data) {
	_CL_TN<T,2> ret;
	const T *ptr = data + offset*2*sizeof(T);
	ret.x = ptr[0];
	ret.y = ptr[1];
	return ret;
}

template <typename T>
_CL_TN<T,3> vload3(int offset, const T *data) {
	_CL_TN<T,3> ret;
	const T *ptr = data + offset*3*sizeof(T);
	ret.x = ptr[0];
	ret.y = ptr[1];
	ret.z = ptr[2];
	return ret;
}

template <typename T>
_CL_TN<T,4> vload4(int offset, const T *data) {
	_CL_TN<T,4> ret;
	const T *ptr = data + offset*4*sizeof(T);
	ret.x = ptr[0];
	ret.y = ptr[1];
	ret.z = ptr[2];
	ret.w = ptr[3];
	return ret;
}

template <typename T>
void vstore2(_CL_TN<T,2> vector, int offset, T *data) {
	T *ptr = data + offset*2*sizeof(T);
	ptr[0] = vector.x;
	ptr[1] = vector.y;
}

template <typename T>
void vstore3(_CL_TN<T,3> vector, int offset, T *data) {
	T *ptr = data + offset*3*sizeof(T);
	ptr[0] = vector.x;
	ptr[1] = vector.y;
	ptr[2] = vector.z;
}

template <typename T>
void vstore4(_CL_TN<T,4> vector, int offset, T *data) {
	T *ptr = data + offset*4*sizeof(T);
	ptr[0] = vector.x;
	ptr[1] = vector.y;
	ptr[2] = vector.z;
	ptr[3] = vector.w;
}
