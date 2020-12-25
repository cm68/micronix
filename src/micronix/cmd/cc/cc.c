/*
 * v7 cc command swizzled to work with hitech c ported to micronix
 */
#include <stdio.h>
/* #include <ctype.h> */
#include <sys/signal.h>

/* cc command */

#define MAXINC 10
#define MAXFIL 100
#define MAXLIB 100
#define MAXOPT 100

#ifndef HITECH
#define CINIT = 0
#else
#define	CINIT
#endif

char *av[50] CINIT;			/* exec arg vector */

char *source[MAXFIL] CINIT;	/* source files */
char nsrc CINIT;

char *object[MAXLIB] CINIT;	/* link files */
int nobj CINIT;

char *gobj[MAXFIL] CINIT;		/* temporary objects */
char ngobj CINIT;

char *cppflags[MAXOPT] CINIT;	/* cpp options */
int ncpp CINIT;

char *outfile CINIT;

char namebuf[100] CINIT;

int pflag CINIT;
int sflag CINIT;
int cflag CINIT;
int eflag CINIT;
int oflag CINIT;
int vflag CINIT;
int nflag CINIT;

int err CINIT;

char *crt0 = "/lib/HTCRT0.OBJ";

int idexit();
char *setsuf();
char *strcat();
char *strcpy();
char *pname CINIT;

#define	PASS_CPP	0
#define	PASS_P1		1
#define	PASS_CGEN	2
#define	PASS_OPTIM	3
#define	PASS_AS		4
#define	PASS_LINK	5

struct pass {
	char *name;
	char *path;
} pass[] = {
	{ "cpp", 0 },
	{ "p1", 0 },
	{ "cgen", 0 },
	{ "optim", 0 },
	{ "as", 0 },
	{ "link", 0 }
};

char *execpath = "/usr/lib";

usage()
{
	fprintf(stderr, "usage: %s [options] [sources] [objects] [-l<lib> ...]\n", 
		pname);
	fprintf(stderr, "\t-o <output file>\n");
	fprintf(stderr, "\t-P <executable path>]\n");
	fprintf(stderr, "\t-D<macro>[=<definition>]\n");
	fprintf(stderr, "\t-U<macro>[=<definition>]\n");
	fprintf(stderr, "\t-I<include directory>\n");
	fprintf(stderr, "\t-L<library directory>\n");
	fprintf(stderr, "\t-l<library directory>\n");
	fprintf(stderr, "\t-E\tstop after preprocessing (produce .i)\n");
	fprintf(stderr, "\t-S\tstop before assembly (produce .s)\n");
	fprintf(stderr, "\t-c\tstop before link\n");
	fprintf(stderr, "\t-O\tinvoke optimizer\n");
	fprintf(stderr, "\t-p\tpreserve temporary files\n");
	fprintf(stderr, "\tsources:\tfiles with a .c or .s extension\n");
	fprintf(stderr, "\tobjects:\tfiles with a .obj extension\n");
	fprintf(stderr, "\t\talso may be library references with -l<libname>\n");

	exit(9);
}

main(argc, argv)
    char *argv[];
{
	char *s;
	int i;
	int j;
	char c;

	i = 0;

	pname = argv[0];

	/*
	 * predefined symbols
 	 */
	cppflags[ncpp++] = "-Dmicronix";
	cppflags[ncpp++] = "-Dz80";

    while (++i < argc) {
        if (*argv[i] == '-') {
            switch (argv[i][1]) {

            case 'o':
				if (outfile) {
					fprintf(stderr, "only 1 -o allowed\n");
					exit(3);
				}
                if (++i < argc) {
                    outfile = argv[i];
                    if ((c = getsuf(outfile)) == 'c' || c == 's') {
                        fprintf(stderr, "Would overwrite %s\n", outfile);
                        exit(8);
                    }
                }
                break;

			case 'P':			/* executable path */
				if (++i >= argc) {
                	fprintf(stderr, "exec path required\n");
                    exit(3);
				}
				execpath = argv[i];
				break;
            case 'S':			/* don't assemble */
                sflag++;
                break;

            case 'O':			/* optimize */
                oflag++;
                break;

            case 'E':			/* only preprocess */
                eflag++;

            case 'c':			/* don't link */
                cflag++;
                break;

            case 'p':			/* keep all generated files */
                pflag++;
                break;

			case 'v':
				vflag++;		/* list commands being run */
				break;

			case 'l':			/* add library */
				object[nobj++] = strdup(argv[i]);
				if (nobj >= MAXLIB) {
					fprintf(stderr, "Too many object/library files\n");
					exit(1);
				}
				break;

			case 'n':
				nflag++;		/* just list commands being run */
				vflag++;
				break;

			case 'h':
				usage();
				break;

            case 'D':			/* cpp flags */
            case 'I':
            case 'U':
                cppflags[ncpp++] = argv[i];
                if (ncpp >= MAXOPT) {
                    fprintf(stderr, "Too many DIU options\n");
					exit(10);
                }
                break;

            default:
                fprintf(stderr, "unknown flag %c\n", argv[i][1]);
				usage();
            }
			continue;
        }
        s = argv[i];

		/* 
		 * if there is a .c or .s file, append it to sources and objects
		 */
        if ((c = getsuf(s)) == 'c' || c == 's') {
        	source[nsrc++] = s;
            if (nsrc >= MAXFIL) {
            	fprintf(stderr, "Too many source files\n");
                exit(1);
            }
            s = setsuf(s, 'o');
			gobj[ngobj++] = s;
        }

		/* append unique object files to llist */
        if (nodup(object, s)) {
        	object[nobj++] = s;
            if (nobj >= MAXLIB) {
            	fprintf(stderr, "Too many object/library files\n");
                exit(1);
            }
        }
    }

	if (!outfile)
		outfile = "a.out";

    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
        signal(SIGINT, idexit);
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
        signal(SIGTERM, idexit);

	/*
	 * resolve paths to executables
	 */
	for (i = 0; i <= PASS_LINK; i++) {
		sprintf(namebuf, "%s/%s", execpath, pass[i].name);
		pass[i].path = strdup(namebuf);
	}

	/*
	 * process all the source files
	 */
    for (i = 0; i < nsrc; i++) {

		char *ofn;

		s = source[i];
        if (getsuf(s) == 's') {
			ofn = s;
			goto assemble;
		}

        av[1] = s;
		av[2] = ofn = setsuf(s, 'i');
        for (j = 0; j < ncpp; j++) {
            av[j + 3] = cppflags[j];
		}
        av[j + 3] = 0;
        if (run(PASS_CPP, av)) {
			free(ofn);
			cflag++;
            continue;
        }

		if (eflag)
			continue;

        av[1] = ofn;
        av[2] = ofn = setsuf(ofn, '1');
		av[3] = 0;
        if (run(PASS_P1, av)) {
			cflag++;
			free(av[1]);
			free(ofn);
            continue;
        }
		if (!pflag) cunlink(av[1]);
		free(av[1]);

        av[1] = ofn;
        av[2] = ofn = setsuf(ofn, 's');
		av[3] = 0;
        if (run(PASS_CGEN, av)) {
            cflag++;
			free(av[1]);
			free(ofn);
            continue;
        }
		if (!pflag) cunlink(av[1]);
		free(av[1]);

        if (oflag) {
            av[1] = ofn;
            av[2] = ofn = setsuf(ofn, 'S');
            av[3] = 0;
            if (run(PASS_OPTIM, av)) {
				cflag++;
				free(av[1]);
				free(ofn);
				continue;
			}
			if (!pflag) cunlink(av[1]);
			free(av[1]);
		}

		if (sflag) 
			continue;

assemble:
        av[1] = "-o";
        av[2] = setsuf(ofn, 'o');
        av[3] = ofn;
        av[4] = 0;
        if (run(PASS_AS, av)) {
			free(ofn);
			free(av[2]);
            cflag++;
            continue;
        }
		if (ofn != s) {
			if (!pflag) cunlink(ofn);
			free(ofn);
		}
    }

	/*
	 * now maybe link
	 */
    if (cflag == 0 && nobj != 0) {

        av[1] = "-o";
        av[2] = outfile;
		av[3] = crt0;
		j = 4;
        for (i = 0; i < nobj; i++) {
            av[j++] = object[i];
		}
        av[j++] = 0;
        run(PASS_LINK, av);
		for (i = 0; i < ngobj; i++) {
			cunlink(gobj[i]);
		}
    }
    exit(err);
}

idexit()
{
    err = 100;
	exit(err);
}

/*
 * return the suffix, which is the character after a final '.'
 */
getsuf(s)
    char *s;
{
    int l;

	l = strlen(s);
	if (l < 3)
		return 0;
	
    s += l - 2;

	if (*s++ != '.') return 0;

    return (*s);
}

/*
 * set the suffix to ch.  however, also strip any leading path
 */
char *
setsuf(s, ch)
    char *s;
	char ch;
{
    char *s1;

	s1 = rindex(s, '/');
	if (s1) {
		s = ++s1;
	}

    s = strdup(s);
	s[strlen(s) - 1] = ch;
    return (s);
}

run(p, v)
    int p;
	char **v;
{
    int t, status;
	int i;
	char *path;

	v[0] = pass[p].name;
	path = pass[p].path;

	if (vflag) {
		fprintf(stderr, "%s: ", path);
		for (i = 0; v[i]; i++) {
			fprintf(stderr, "%s ", v[i]);
		}
		fputs("\n", stderr);
		if (nflag)
			return 0;
	}

    if ((t = fork()) == 0) {
        execv(path, v);
        fprintf(stderr, "Can't find %s\n", path);
        exit(100);
    } else if (t == -1) {
        fprintf(stderr, "fork failed\n");
        return (100);
    }
    while (t != wait(&status));
    if (err = status & 0377) {
        if (err != SIGINT) {
            fprintf(stderr, "Fatal error in %s\n", pass[p].path);
            err = 8;
        }
        exit(err);
    }
	status = (status >> 8) & 0x377;
	if (status) err = 1;
    return (status);
}

/*
 * if it's a .o, check for uniqueness.  else, just say yes.
 */
nodup(list, object)
    char **list;
	char *object;
{
    if (getsuf(object) != 'o')
        return 1;

    while (*list) { 
		if (strcmp(*list++, object) == 0) 
			return 0;
	}
    return 1;
}

cunlink(f)
    char *f;
{
    if (f == NULL)
        return;
	if (vflag) {
		fprintf(stderr, "remove %s\n", f);
	}
    unlink(f);
}
