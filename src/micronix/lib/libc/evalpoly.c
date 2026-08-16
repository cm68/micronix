/*
 * floating point evaluate a polynomial
 *
 */
double
eval_poly(x, d, n)
double	x, d[];
int	n;
{
	int	i;
	double	res;

	res = d[i = n];
	while(i)
		res = x * res + d[--i];
	return res;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
