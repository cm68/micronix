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
	int i;

	mode &= 0777;
	mode |= 040000;

	ret = mknod(fn, mode, 0);
	if (ret != 0)
		return ret;

	sprintf(namebuf, "%s/.", fn);
	ret += link(fn, namebuf);

	sprintf(namebuf, "%s/..", fn);
	i = strlen(fn);

	/*
	 * fn has a few interesting forms:
	 * /bar/foo		foo/.. -> /bar
	 * /foo			foo/.. -> /
	 * foo			foo/.. -> .
	 * bar/foo		foo/.. -> bar
	 */

	/* 
	 * remove trailing slashes - these are uninteresting
	 */
	for (i = strlen(fn); i && fn[i] == '/'; i--)
		;

	/* find the rightmost slash */
	while ((i >= 0) && (fn[i] != '/'))
		i--;

	if (i < 0) {
		fn = ".";
	} else if (i == 0) {
		fn = "/";
	} else {
		fn[i] = '\0';
	}
	ret += link(fn, namebuf);
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

