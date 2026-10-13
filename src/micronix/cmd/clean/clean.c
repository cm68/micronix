/*
 * clean - clean CP/M format text files
 *
 * cmd/clean/clean.c
 *
 * A CP/M text file ends each line with CR-LF and the whole file with
 * ^Z.  Clean strips the CR and stops at the ^Z, and clean -u puts them
 * back.  Either way the named file ends up cleaned, so the work is
 * done to a scratch file and renamed over the original.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

char	*mktemp();
char	*strcpy();

char buf[BUFSIZ];

main(argc, argv)
	int argc;
	char **argv;
{
	int i, uflag, ifd, ofd, n, j, stop;
	/* static: a local array's address is a frame offset, which c1
	   cannot pass to a function - see ccc porting gotchas */
	static char tmp[64];
	int err;

	if (argc < 2) {
		fprintf(stderr, "usage: clean [-u] file ...\n");
		exit(1);
	}
	uflag = 0;
	i = 1;
	if (argv[1][0] == '-' && argv[1][1] != '\0') {
		if (argv[1][1] == 'u')
			uflag++;
		else {
			fprintf(stderr, "clean: bad flag %s\n", argv[1]);
			exit(1);
		}
		i = 2;
	}
	err = 0;
	for (; i < argc; i++) {
		ifd = open(argv[i], O_RDONLY);
		if (ifd < 0) {
			fprintf(stderr, "clean: can't open %s\n", argv[i]);
			err = 1;
			continue;
		}
		strcpy(tmp, "clXXXXXX");
		mktemp(tmp);
		ofd = creat(tmp, 0666);
		if (ofd < 0) {
			fprintf(stderr, "clean: can't create %s\n", tmp);
			close(ifd);
			err = 1;
			continue;
		}
		if (uflag) {
			/* LF becomes CR-LF, and ^Z ends the file */
			for (;;) {
				n = read(ifd, buf, BUFSIZ);
				if (n <= 0)
					break;
				for (j = 0; j < n; j++) {
					if (buf[j] == '\n')
						write(ofd, "\r", 1);
					write(ofd, &buf[j], 1);
				}
			}
			write(ofd, "\032", 1);
		} else {
			/* drop CR, stop at ^Z */
			stop = 0;
			for (;;) {
				n = read(ifd, buf, BUFSIZ);
				if (n <= 0)
					break;
				for (j = 0; j < n; j++) {
					if (buf[j] == '\032') {
						stop = 1;
						break;
					}
					if (buf[j] != '\r')
						write(ofd, &buf[j], 1);
				}
				if (stop)
					break;
			}
		}
		close(ifd);
		close(ofd);
		if (rename(tmp, argv[i]) != 0) {
			fprintf(stderr, "clean: can't replace %s\n", argv[i]);
			unlink(tmp);
			err = 1;
		}
	}
	exit(err);
}
