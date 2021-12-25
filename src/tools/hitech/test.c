/*
 * another hitech c test file
 *
 * tools/hitech/test.c 
 * Changed: <2021-12-23 16:24:32 curt>
 */
char buffer[4096];

main(argc, argv)
int argc;
char **argv;
{
	int fd;
	int i;

	if (argc < 2) {
		printf("need filename");
		exit(2);
	}
	fd = open(argv[1], 0);
	if (fd < 0) {
		perror(argv[1]);
	}

	i = read(fd, buffer, 4096);
	if (i < 0) {
		printf("read failed");
	}

	write(1, buffer, i);
	exit (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
