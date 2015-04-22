__kernel void compact(__global uint *hit_info, __global uint *ray_count, __global const uint *deviation, __global const uint *work_size)
{
	const int size = *work_size;//get_global_size(0);
	const int pos = get_global_id(0);
	const int dev = *deviation;
	const int edev = (1<<dev);
	
	if(pos >= size)
	{
		return;
	}
	
	uint2 sum;
	if(pos >= edev)
	{
		sum.x = hit_info[HIT_INFO_SIZE*(pos-edev) + (dev%2) + 1] + hit_info[HIT_INFO_SIZE*pos + (dev%2) + 1];
		//sum.y = hit_info[HIT_INFO_SIZE*(pos-edev) + 2*(dev%2) + 5] + hit_info[HIT_INFO_SIZE*pos + 2*(dev%2) + 5];
	}
	else
	{
		sum.x = hit_info[HIT_INFO_SIZE*pos + (dev%2) + 1];
		//sum.y = hit_info[HIT_INFO_SIZE*pos + (dev%2) + 2];
	}
	
	hit_info[HIT_INFO_SIZE*pos + ((dev+1)%2) + 1] = sum.x;
	//hit_info[HIT_INFO_SIZE*pos + 2*((dev+1)%2) + 5] = sum.y;
	
	if(pos == size - 1)
	{
		ray_count[0] = sum.x;
		//ray_count[1] = sum.y;
	}
}
