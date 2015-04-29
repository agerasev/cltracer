/** random.cl */

uint random_next(uint *seed)
{
	return (*seed = 1103515245**seed + 12345);
}

float random_unif(uint *seed)
{
	return (float)random_next(seed)/(float)0xffffffff;
}

float2 random_disk(uint *seed)
{
	float r = sqrt(random_unif(seed));
	float phi = 2.0*M_PI_F*random_unif(seed);
	return r*(float2)(cos(phi),sin(phi));	
}
