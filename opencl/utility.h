#pragma once

#include <opencl.h>

float __OVERLOADABLE__ _max(float a, float b)
{
	return max(a,b);
}

float __OVERLOADABLE__ _min(float a, float b)
{
	return min(a,b);
}

float2 __OVERLOADABLE__ _max(float2 a, float2 b)
{
	return (float2)(_max(a.x,b.x),_max(a.y,b.y));
}

float2 __OVERLOADABLE__ _min(float2 a, float2 b)
{
	return (float2)(_min(a.x,b.x),_min(a.y,b.y));
}

float3 __OVERLOADABLE__ _max(float3 a, float3 b)
{
	return (float3)(_max(a.x,b.x),_max(a.y,b.y),_max(a.z,b.z));
}

float3 __OVERLOADABLE__ _min(float3 a, float3 b)
{
	return (float3)(_min(a.x,b.x),_min(a.y,b.y),_min(a.z,b.z));
}

float __OVERLOADABLE__ _clamp(float x, float a, float b)
{
	return clamp(x,a,b);
}

float2 __OVERLOADABLE__ _clamp(float2 x, float2 a, float2 b)
{
	return (float2)(clamp(x.x,a.x,b.x),clamp(x.y,a.y,b.y));
}

float3 __OVERLOADABLE__ _clamp(float3 x, float3 a, float3 b)
{
	return (float3)(clamp(x.x,a.x,b.x),clamp(x.y,a.y,b.y),clamp(x.z,a.z,b.z));
}
