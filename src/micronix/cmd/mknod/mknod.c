/*
 * the stock micronix mknod is broken, so this is pretty brutal rewrite
 *
 * cmd/mknod/mknod.c
 *
 * Changed: <2022-01-06 16:36:04 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

main(argc, argv)
int argc;
char **argv;
{
	int m, a, b;

	if(argc != 5) {
		printf("arg count\n");
		goto usage;
	}
	if(*argv[2] == 'b')
		m = 0160666; else
	if(*argv[2] == 'c')
		m = 0120666; else
		goto usage;
	a = number(argv[3]);
	if(a < 0)
		goto usage;
	b = number(argv[4]);
	if(b < 0)
		goto usage;
	if(mknod(argv[1], m, (a<<8)|b) < 0)
		perror("mknod");
	exit();

usage:
	printf("usage: mknod name b/c major minor\n");
}

number(s)
char *s;
{
	int n, c;

	n = 0;
	while(c = *s++) {
		if(c<'0' || c>'9')
			return(-1);
		n = n*10 + c-'0';
	}
	return(n);
}

