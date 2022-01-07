/*
 * mkdir command - micronix does not have a mkdir system call,
 * but we do have a mkdir library implementation, so the v7 code
 * is pretty much cloned
 *
 * cmd/mkdir/mkdir.c
 * Changed: <2022-01-06 16:34:50 curt>
 */
#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <errno.h>

extern int errno;

int pflag = 0;
int nflag = 0;
int vflag = 0;
char *pname = 0;

usage()
{
	fprintf(stderr, "usage: %s [-p] <dirname> [<dirname>...]\n", pname);
	exit(1);
}

/*
 * given a path, delete any trailing slashes
 */
deletetrailing(s)
char *s;
{
    int e;

    for (e = strlen(s); e >= 0 && (s[e] == '/'); e--) {
        s[e] = '\0';
    }
}

/*
 * given a path, remove redundant slashes
 */
normalize(s)
char *s;
{
    int i;
    int o;

    i = 0;
    o = 0;

    while (s[i]) {
        s[o] = s[i];
        if (s[o] == '/') {
            while (s[i] == '/')
                i++;
        } else {
            i++;
        }
        o++;
    }
    s[o] = '\0';
}

int
mkpred(n)
char *n;
{
    struct stat statb;
    int end, i;
    int e;

    i = 0;

    while (n[i]) {
        if (n[i] == '/') {
            n[i] = '\0';
            if (*n) {
                e = stat(n, &statb);
                if ((e == -1) && (errno != ENOENT)) {
                    fprintf(stderr, "stat of %s failed %d\n", n, errno);
                }
                if ((statb.st_mode & S_IFMT) == S_IFDIR) {
                    return 0;
                }
                if (vflag) {
                    fprintf(stderr, "mkdir %s\n", n);
                }
                e = 0;
                if (!nflag) {
                    e = mkdir(n, 0777);
                }
                if (e) return e;
            }
            n[i] = '/';
        }
        i++;
    }
    return 0;
}

int
main(argc, argv)
int argc;
char **argv;
{
	char *a;
	int e;
	pname = argv[0];
	argv++;

	while (--argc) {
		a = *argv++;
        if (*a == '-') {
            a++;
            while (*a) {
                switch(*a) {
                case 'p':
                    pflag++;
                    break;
                case 'v':
                    vflag++;
                    break;
                case 'n':
                    nflag++;
                    break;
                case 'h':
                default:
                    usage();
                    break;
                }
                a++;
            }
            continue;
		}

        normalize(a);

		if (pflag) {
			e = mkpred(a);
			if (e) {
				exit(e);
			}
		}
        if (vflag) {
            fprintf(stderr, "mkdir %s\n", a);
        }
        if (!nflag) {
            mkdir(a, 0777);
        }
	}
	exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

