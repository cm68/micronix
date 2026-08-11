/*
 * floating point tangent
 *
 */
#include	<math.h>

double
tan(x)
double	x;
{
	return sin(x)/cos(x);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
