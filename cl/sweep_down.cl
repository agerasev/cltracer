/** sweep_down.cl */

__kernel void sweep_down(__global uint *hit_info, __global uint *ray_count, __global const uint *deviation, __global const uint *work_size)
{
	const int size = *work_size;//get_global_size(0);
	const int pos = get_global_id(0);
	const int dev = *deviation;
	const int edev = (1<<dev);
	const int epos = 2*(pos + 1)*edev - 1;
	
	if(epos >= size)
	{
		return;
	}
	
	uint sum;
	uint2 sum2;
	uint bs = dev%2, nbs = (dev+1)%2;
	
	sum2.x = hit_info[HIT_INFO_SIZE*(epos-edev) + 2*bs + 4] + hit_info[HIT_INFO_SIZE*epos + 2*bs + 4];
	sum2.y = hit_info[HIT_INFO_SIZE*(epos-edev) + 2*bs + 5] + hit_info[HIT_INFO_SIZE*epos + 2*bs + 5];
	
	hit_info[HIT_INFO_SIZE*(epos-edev) + 2*nbs + 4] = hit_info[HIT_INFO_SIZE*epos + 2*bs + 4];
	hit_info[HIT_INFO_SIZE*(epos-edev) + 2*nbs + 5] = hit_info[HIT_INFO_SIZE*epos + 2*bs + 5];
	
	hit_info[HIT_INFO_SIZE*epos + 2*nbs + 4] = sum2.x;
	hit_info[HIT_INFO_SIZE*epos + 2*nbs + 5] = sum2.y;
	
	sum = sum2.x;
	
	if(pos + 1 == ray_count[0] && dev <= 0)
	{
		ray_count[0] = sum;
		ray_count[1] = sum2.x;
		ray_count[2] = sum2.y;
	}
}
