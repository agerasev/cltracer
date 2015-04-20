typedef float2 cfloat;

#define ITER 0x40

cfloat cmul(const cfloat a, const cfloat b)
{
	return (cfloat)(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

__kernel void draw(__write_only image2d_t output)
{
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const int2 pos = (int2)(get_global_id(0), get_global_id(1));
	cfloat c = 2.0f*((cfloat)(pos.x,pos.y) - (cfloat)(size.x,size.y)/2.0f)/(float)size.y;
	cfloat a = (cfloat)(0.0f,0.0f);
	int i;
	for(i = 0; i < ITER; ++i)
	{
		a = cmul(a,a) + c;
		if(a.x*a.x + a.y*a.y > 4.0f)
		{
			break;
		}
	}
	float4 color = (float4)((float)i/(float)(ITER-1),(float)(ITER-i-1)/(float)(ITER-1),0.0,1.0);
	write_imagef (output, (int2)(pos.x, pos.y), color);
}
