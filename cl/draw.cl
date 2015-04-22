__kernel void draw(
  __global uint *color_buffer, __global float *accum_buffer, 
  __global float *factor, __write_only image2d_t image
)
{
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const int2 pos = (int2)(get_global_id(0), get_global_id(1));
	
	uint3 icolor = vload3(pos.x + size.x*pos.y,color_buffer);
	float3 color = 
	  *factor*((float3)((float)icolor.x,(float)icolor.y,(float)icolor.z)/(float)0x10000) + 
	  (1.0f - *factor)*vload3(pos.x + size.x*pos.y,accum_buffer);;
	
	vstore3((uint3)(0,0,0),pos.x + size.x*pos.y,color_buffer);
	vstore3(color,pos.x + size.x*pos.y,accum_buffer);
	
	write_imagef(image,pos,(float4)(color,1.0f));
}
