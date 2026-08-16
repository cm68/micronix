/*
 * floating point arc cosine
 *
 */
#include	<math.h>

double
acos(x)
double	x;
{
	double	y;

	y = sqrt(1 - x*x);
	return atan(y/x);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
