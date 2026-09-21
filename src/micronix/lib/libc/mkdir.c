/*
 * mkdir is not a system call in micronix.
 */

int
mkdir(fn, mode)
char *fn;
int mode;
{
	int ret;
	char namebuf[100];
	char parent[100];
	char *p;
	int i;

	mode &= 0777;
	mode |= 040000;

	ret = mknod(fn, mode, 0);
	if (ret != 0)
		return ret;

	sprintf(namebuf, "%s/.", fn);
	ret += link(fn, namebuf);

	sprintf(namebuf, "%s/..", fn);

	/*
	 * The name of the parent has to be cut out of the path, and the
	 * cut is made on a copy: fn is the caller's string.  Shortening
	 * it in place is what broke "mkdir -p": the command walks its
	 * argument a component at a time, and after this call created
	 * /usr/man the rest of its argument was gone - it asked for
	 * /usr, was told EEXIST, and stopped.
	 */
	if (strlen(fn) >= sizeof(parent))
		return ret;
	strcpy(parent, fn);
	p = parent;

	/*
	 * p has a few interesting forms:
	 * /bar/foo		foo/.. -> /bar
	 * /foo			foo/.. -> /
	 * foo			foo/.. -> .
	 * bar/foo		foo/.. -> bar
	 * foo/			foo/.. -> .
	 */

	/*
	 * remove trailing slashes - these are uninteresting
	 */
	for (i = strlen(p) - 1; i > 0 && p[i] == '/'; i--)
		p[i] = '\0';

	/* find the rightmost slash */
	while ((i > 0) && (p[i] != '/'))
		i--;

	if (i <= 0) {
		p = (p[0] == '/') ? "/" : ".";
	} else {
		p[i] = '\0';
	}
	ret += link(p, namebuf);
	return ret;

}

#ifdef TESTING
main(argc, argv)
int argc;
char **argv;
{
	if (argc != 2) {
		printf("mkdir name\n");
		exit (1);
	}
	return mkdir(argv[1], 0777);
}
#endif

