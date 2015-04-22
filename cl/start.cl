__kernel void start(
  __global float *ray_fdata, __global int *ray_idata, 
  __constant float *cam_fdata, __global uint *random
)
{
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const int2 pos = (int2)(get_global_id(0), get_global_id(1));
	const float2 cpos = (2.0f*(float2)(pos.x,-pos.y) - (float2)(size.x,-size.y))/(float)size.y;	

	Camera cam = camera_load(cam_fdata);
	
	Ray ray;
	ray.origin = pos;
	
	uint seed = random[size.x*pos.y + pos.x];
	float2 lens = random_disk(&seed);
	ray.pos = cam.pos + cam.rad*(cam.ori[0]*lens.x + cam.ori[1]*lens.y);
	
	ray.dir = normalize(cam.dof*(cam.ori[2] + cam.fov*(cam.ori[0]*cpos.x + cam.ori[1]*cpos.y)) + cam.pos - ray.pos);
	
	ray.color = (float3)(1.0f,1.0f,1.0f);
	
	random[size.x*pos.y + pos.x] = seed;
	ray_store(&ray, size.x*pos.y + pos.x, ray_fdata, ray_idata);
}
