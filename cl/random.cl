uint random_next(uint *seed)
{
	return (*seed = 1103515245**seed + 12345);
}

float random_unif(uint seed)
{
	return (float)seed/(float)0xffffffff;
}
