/*
 * floating point absolute value
 *
 */
double
fabs(d)
double	d;
{
	if(d < 0.0)
		return -d;
	return d;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
