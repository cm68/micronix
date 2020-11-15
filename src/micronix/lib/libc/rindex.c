/*
 * rightmost c in s
 */
char *
rindex(s, c)
char *s;
char c;
{
	char *ret = 0;

	do {
		if (*s == c) ret = s;
	} while (*s++);
	return ret;	
}
