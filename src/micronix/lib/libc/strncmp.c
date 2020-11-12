char *
strncmp(s1, s2, n)
char *s1;
char *s2;
int n;
{
	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return(n<0 ? 0 : *s1 - *--s2);
}

