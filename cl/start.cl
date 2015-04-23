__kernel void start(
  __global float *ray_fdata, __global int *ray_idata, 
  __constant float *cam_fdata, __global uint *random
)
{
	const int2 size = (int2)(get_global_size(0), get_global_size(1));
	const int2 pos = (int2)(get_global_id(0), get_global_id(1));
	const float2 cpos = (2.0f*(float2)(pos.x,pos.y) - (float2)(size.x,size.y))/(float)size.y;	

	Camera cam = camera_load(cam_fdata);
	
	Ray ray;
	ray.origin = pos;
	
	uint seed = random[size.x*pos.y + pos.x];
	
	float t = random_unif(&seed);
	t = 
	  (t < 0.1666f)*(0.333f*sqrt(t/0.1666f)) + 
	  (t > 0.1666f && t < 0.8333f)*(0.5*(t - 0.1666f) + 0.333f) + 
	  (t > 0.8333f)*(0.666f + 0.333f*(1.0f - sqrt((1.0f - t)/0.1666f)));
	
	float3 cam_pos = cam.pos*(1.0f - t) + cam.pre_pos*t;
	float3 cam_ori[3] = {
		cam.ori[0]*(1.0f - t) + cam.pre_ori[0]*t,
		cam.ori[1]*(1.0f - t) + cam.pre_ori[1]*t,
		cam.ori[2]*(1.0f - t) + cam.pre_ori[2]*t
	};
	
	float2 lens = random_disk(&seed);
	ray.pos = cam_pos + cam.rad*(cam_ori[0]*lens.x + cam_ori[1]*lens.y);
	ray.dir = normalize(cam.dof*(cam_ori[2] + cam.fov*(cam_ori[0]*cpos.x + cam_ori[1]*cpos.y)) + cam_pos - ray.pos);
	ray.color = (float3)(1.0f,1.0f,1.0f);
	
	random[size.x*pos.y + pos.x] = seed;
	ray_store(&ray, size.x*pos.y + pos.x, ray_fdata, ray_idata);
}
