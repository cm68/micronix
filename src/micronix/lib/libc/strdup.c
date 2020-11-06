char *
strdup(s)
{
#ifdef LIBDEBUG
	char xx[100];
#endif
	char *r;
	int n = strlen(s);

	r = malloc(n + 1);
	if (r) {
		strcpy(r, s);
	}
#ifdef LIBDEBUG
	sprintf(xx, "strdup %s ret %s", s, r);
	logmsg(xx);	
#endif
	return r;	
}

