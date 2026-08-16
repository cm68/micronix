/*
 * random number generator
 *
 */
static	long	randx = 1;

srand(x)
unsigned x;
{
	randx = x;
}

rand()
{
	return(((randx = randx*1103515245 + 12345)>>16) & 077777);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
