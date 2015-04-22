#define HIT_FSIZE 12
#define HIT_ISIZE 3

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

void hit_load_fdata(Hit *hit, int offset, __global const float *data)
{
	__global const float *hit_data = data + offset*HIT_FSIZE;
	hit->pos = vload3(0,hit_data);
	hit->dir = vload3(1,hit_data);
	hit->norm = vload3(2,hit_data);
	hit->color = vload3(3,hit_data);
}

void hit_load_idata(Hit *hit, int offset, __global const int *data)
{
	__global const int *hit_data = data + offset*HIT_ISIZE;
	hit->origin = vload2(0,hit_data);
	hit->object = hit_data[2];
}

Hit hit_load(int offset, __global const float *fdata, __global const int *idata)
{
	Hit hit;
	hit_load_fdata(&hit,offset,fdata);
	hit_load_idata(&hit,offset,idata);
	return hit;
}

void hit_store(Hit *hit, int offset, __global float *fdata, __global int *idata)
{
	__global float *hit_fdata = fdata + offset*HIT_FSIZE;
	__global int *hit_idata = idata + offset*HIT_ISIZE;
	vstore3(hit->pos,0,hit_fdata);
	vstore3(hit->dir,1,hit_fdata);
	vstore3(hit->norm,2,hit_fdata);
	vstore3(hit->color,3,hit_fdata);
	vstore2(hit->origin,0,hit_idata);
	hit_idata[2] = hit->object;
}
