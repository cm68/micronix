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

