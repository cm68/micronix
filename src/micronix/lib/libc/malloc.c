char *
malloc(n)
int n;
{
#ifdef LIBDEBUG
	char xx[100];
#endif
	char *ret;
	ret = alloc(n);
#ifdef LIBDEBUG
	sprintf(xx, "malloc: %d ret %x", n, ret);
	logmsg(xx);
#endif
	return ret;
}

