main(argc, argv)
int argc;
char **argv;
{
	int uid = getuid();
	if ((uid != 101) && (uid != 0)) {
		printf("no\n");
		exit(0);
	}
	setuid(0);
	setgid(0);
	argv++;
	execvp(*argv, argv);
}
