/*
 * version 6 rm ported to micronix
 */
#include <stdio.h>
#include <stat.h>
#include <sgtty.h>

int fflag = 0;
int rflag = 0;
int iflag = 0;
char *pn = 0;

struct sgtty saved = 0;
struct sgtty new = 0;

#ifdef notdef
unlink(s)
char *s;
{
	printf("unlink %s\n", s);
	return 0;	
}
#endif

usage()
{
	fprintf(stderr, "usage:\n%s [-<flags>] path ...\n", pn);
	fprintf(stderr, "\t-f\tforce remove\n");
	fprintf(stderr, "\t-i\tinquire per file\n");
	fprintf(stderr, "\t-r\trecurse into directories\n");
	exit(1);
}

char buf[256] = 0;

main(argc, argv)
    char *argv[];
{
    char *arg;

	pn = argv[0];

    while (--argc > 0) {

        arg = *++argv;
        if (*arg == '-') {
			arg++;
			while (*arg) {
				switch (*arg) {
				case 'f':
					fflag++;
					arg++;
					break;
				case 'r':
					rflag++;
					arg++;
					break;
				case 'i':
					iflag++;
					arg++;
					break;
				case 'h':
					usage();
					break;
				default:
					fprintf(stderr, "unknown flag %c", *arg); 
					usage();
					break;
				}
            }
			continue;
        }
        rm(arg);
    }
	exit(0);
}

prompt(n)
char *n;
{
	int ret = 1;

	if (iflag) {
		ret = 0;

		write(1, n, strlen(n));
		gtty(0, &saved);
		gtty(0, &new);
		new.mode |= RAW;
		stty(0, &new);	
		write(1, ": ", 2);
		if (read(0, buf, 1) == 1) {
			buf[0] |= 0x20;
			write(1, "\n\r", 2);
			/* upper or lower case 'y' suffices */
			if (buf[0] == 'y') {
				ret = 1;
			}
		}
		stty(0, &saved);
	}
	return ret;
}

rm(arg)
char *arg;
{
    char *p;
    struct stat sbuf;
    int pid;
	int status;

    if (stat(arg, &sbuf)) {
        printf("%s: non existent\n", arg);
        return;
    }

	if (!prompt(arg)) {
		return;
	}

	/*
	 * directories are odd - we need to unlink . and ..
	 * and then the actual name, but only if the link count
	 * is 2.
	 */
    if ((sbuf.flags & S_TYPE) == S_ISDIR) {

		/*
		 * if the link count is not 2, we might recurse
		 */
        if (rflag && sbuf.nlinks != 2) {
			printf("recurse into %s\n", arg);

            pid = fork();
            if (pid == -1) {
                printf("%s: fork failed\n", arg);
				exit(2);
            }
            if (pid) {
				/* parent */
                while (wait(&status) != pid)
					;
                return;
            }
            if (chdir(arg)) {
                printf("%s: cannot chdir\n", arg);
                exit(3);
            }
            p = 0;
			sprintf(buf, "%s -r%s *", pn, fflag ? "f" : "");
			fprintf(stderr, "%s\n", buf);
			system(buf);
			exit(0);
        }

		/*
		 * the link count had better be 2 now.
		 */
    	if (stat(arg, &sbuf)) {
			printf("%s: non existent\n", arg);
			return;
		}
		if (sbuf.nlinks != 2) {
			printf("%s: directory not empty\n", arg);
			return;
		}

		sprintf(buf, "%s/..", arg);
		unlink(buf);
		buf[strlen(buf) - 1] = '\0';
		unlink(buf);
		unlink(arg);
        return;
    }

    if (unlink(arg)) {
        if (!fflag) {
			printf("%s: not removed\n", arg);
		}
		printf("unlink perm problem\n");
    }
}
