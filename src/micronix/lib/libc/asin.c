/*
 * floating point arc sin
 *
 */
#include	<math.h>

double
asin(x)
double	x;
{
	double	y;

	y = sqrt(1 - x*x);
	return atan(x/y);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
