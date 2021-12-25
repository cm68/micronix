/*
 * hitech c test file
 *
 * tools/hitech/hello.c
 * Changed: <2021-12-23 16:23:09 curt>
 */
main(argc, argv)
int argc;
char **argv;
{
	int i;
	for (i = 1; i < argc; i++) {
		write(1, argv[i], strlen(argv[i]));
		write(1, " ", 1);
	}
	write(1, "\n", 1);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
