/*
 * v7 cc command 
 * swizzled to work with hitech c and whitesmith's
 *
 * cmd/cc/cc.c
 *
 * Changed: <2023-07-10 15:29:17 curt>
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

char *av[50] CINIT;             /* exec arg vector */

char *source[MAXFIL] CINIT;     /* source files */
char nsrc CINIT;

char *object[MAXLIB] CINIT;     /* link files */
int nobj CINIT;

char *gobj[MAXFIL] CINIT;		/* temporary objects */
char ngobj CINIT;

char *cppflags[MAXOPT] CINIT;	/* cpp options */
int ncpp CINIT;

char *outfile CINIT;

char namebuf[100] CINIT;

int Sflag CINIT;
int pflag CINIT;
int sflag CINIT;
int cflag CINIT;
int eflag CINIT;
int oflag CINIT;
int vflag CINIT;
int nflag CINIT;
int hflag = 0;
int iflag = 0;
char *xopt = 0;

int err CINIT;

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
	{ "cpp", 0 },       /* PASS_CPP */
	{ "cp1", 0 },       /* PASS_P1 */
	{ "cp2", 0 },       /* PASS_CGEN */
	{ "optim", 0 },     /* PASS_OPTIM */
	{ "as", 0 },        /* PASS_AS */
	{ "link", 0 }       /* PASS_LINK */
};

char *execpath = "/bin/";

usage()
{
	fprintf(stderr, "usage: %s [options] [sources] [objects] [-l<lib> ...]\n", 
		pname);
    fprintf(stderr, "\t-P<prefix>\tprefix tools with string\n");
	fprintf(stderr, "\t-H\tuse hitech compiler\n");
	fprintf(stderr, "\t-o <output file>\n");
	fprintf(stderr, "\t-D<macro>[=<definition>]\n");
	fprintf(stderr, "\t-I<include directory>\n");
	fprintf(stderr, "\t-i <include directory>\n");
	/* fprintf(stderr, "\t-L<library directory>\n"); */
	fprintf(stderr, "\t-l<library>\n");
	fprintf(stderr, "\t-E\tstop after preprocessing (produce .i)\n");
	fprintf(stderr, "\t-S\tstop before assembly (produce .s)\n");
	fprintf(stderr, "\t-c\tstop before link\n");
	fprintf(stderr, "\t-O\tinvoke optimizer\n");
	fprintf(stderr, "\t-p\tpreserve temporary files\n");
	fprintf(stderr, "\t-n\tonly show commands\n");
	fprintf(stderr, "\t-x#\tpass option to cp2\n");
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
    while (++i < argc) {
        if (*argv[i] == '-') {
            switch (c = argv[i][1]) {

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

			case 'H':			/* hitech */
                cppflags[ncpp++] = "-Dmicronix";
                cppflags[ncpp++] = "-Dz80";
                cppflags[ncpp++] = "-I/include/";
                pass[PASS_P1].name = "p1";
                pass[PASS_CGEN].name = "cgen";
                execpath = "/hitech/";
				hflag++;
				break;

            case 's':			/* strip binary */
                Sflag++;
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

            case 'P':           /* prepend fragment to tool executable */
                execpath = argv[i];
                break;

			case 'h':
				usage();
				break;

			case 'i':			/* compatibility with ws cc */
                iflag++;
                if (++i >= argc) {
                    fprintf(stderr, "need include directory\n");
					exit(3);
				}
				cppflags[ncpp++] = "-i";
				cppflags[ncpp++] = argv[i];
				break;

			case 'x':			/* whitesmith's x option for cp2 */
				xopt = argv[i];
				break;

            case 'I':
                iflag++;
                if (!hflag) {
                    cppflags[ncpp++] = "-i";
                    j = strlen(argv[i]);
                    if (argv[i][j-1] != '/') {
                        j = strlen(argv[i]);
                        s = malloc(j);
                        strcpy(s, argv[i]+2);
                        strcat(s, "/");
                        cppflags[ncpp++] = s;
                    } else {
                        cppflags[ncpp++] = argv[i];
                    }
                } else {
                    cppflags[ncpp++] = argv[i];
                }
                break;
            case 'D':
                if (!hflag) {
                } else {
                    cppflags[ncpp++] = argv[i];
                }
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

	if (!hflag) {
		cppflags[ncpp++] = "-d";
		cppflags[ncpp++] = "unix";
        if (!iflag) {
            cppflags[ncpp++] = "-i";
            cppflags[ncpp++] = "/include/";
        }
		cppflags[ncpp++] = "-x";
		oflag = 0;
	}

	/*
	 * resolve paths to executables
	 */
	for (i = 0; i <= PASS_LINK; i++) {
		sprintf(namebuf, "%s%s", execpath, pass[i].name);
		pass[i].path = strdup(namebuf);
	}

	/*
	 * process all the source files
	 */
    for (i = 0; i < nsrc; i++) {

		char *ofn;
		char *ifn;

		s = source[i];
        if (getsuf(s) == 's') {
			ifn = s;
			goto assemble;
		}

		/* preprocess */
        for (j = 0; j < ncpp; j++) {
            av[j+1] = cppflags[j];
		}
		j++;
		if (hflag) {
			av[j++] = s;
			av[j++] = ofn = setsuf(s, 'i');
		} else {
			av[j++] = "-o";
			av[j++] = ofn = setsuf(s, 'i');
			av[j++] = s;
		}
        av[j] = 0;
        if (run(PASS_CPP, av)) {
			free(ofn);
			cflag++;
            continue;
        }

		if (eflag)
			continue;

		/* parse */
		j = 1;
		ifn = ofn;
		ofn = setsuf(ifn, '1');

		if (hflag) {
			av[j++] = ifn;
			av[j++] = ofn;
		} else {
			av[j++] = "-b0";
			av[j++] = "-m";
			av[j++] = "-u";
			av[j++] = "-o";
			av[j++] = ofn;
			av[j++] = ifn;
			ofn = av[j-2];
		}
		av[j] = 0;
        if (run(PASS_P1, av)) {
			cflag++;
			free(ifn);
			free(ofn);
            continue;
        }
		if (!pflag) cunlink(ifn);
		free(ifn);

		/* generate code */
		j = 1;
		ifn = ofn;
		ofn = setsuf(ifn, 's');

		if (hflag) {
			av[j++] = ofn;
			av[j++] = ifn;
		} else {
			if (xopt) av[j++] = xopt;
			av[j++] = "-o";
			av[j++] = ofn;
			av[j++] = ifn;
		}
		av[j] = 0;
        if (run(PASS_CGEN, av)) {
            cflag++;
			free(ifn);
			free(ofn);
            continue;
        }
		if (!pflag) cunlink(ifn);
		free(ifn);

		/* maybe optimize */
        if (oflag) {
			j = 1;
			ifn = ofn;
            ofn = setsuf(ofn, 'S');

            av[j++] = ifn;
            av[j++] = ofn;
            av[j] = 0;
            if (run(PASS_OPTIM, av)) {
				cflag++;
				free(ifn);
				free(ofn);
				continue;
			}
			if (!pflag) cunlink(ifn);
			free(ifn);
		}

		if (sflag) 
			continue;

assemble:
		j = 1;
		ifn = ofn;
		ofn = setsuf(ifn, 'o');

        av[j++] = "-o";
        av[j++] = ofn;
        av[j++] = ifn;
        av[j] = 0;
        if (run(PASS_AS, av)) {
			free(ifn);
			free(ofn);
            cflag++;
            continue;
        }
		if (ifn != s) {
			if (!pflag) cunlink(ifn);
			free(ifn);
		}
    }

	/* link */
    if (cflag == 0 && nobj != 0) {

		j = 1;
        av[j++] = "-o";
        av[j++] = outfile;
		if (hflag) {
			av[j++] = "/lib/HTCRT0.OBJ";
		} else {
			av[j++] = "-u_main";
			av[j++] = "-eb__memory";
			av[j++] = "-tb0x100";
            if (Sflag) {
                av[j++] = "-tr";
            } else {
                av[j++] = "-r";
            }
			av[j++] = "-dr12";
			av[j++] = "/lib/uhdr.o";
		}
        for (i = 0; i < nobj; i++) {
			if ((!hflag) && object[i][0] == '-' && object[i][1] == 'l') {
				s = malloc(strlen(object[i]) + 2);
				sprintf(s, "%s.a", object[i]);
				object[i] = s;
			}
            av[j++] = object[i];
		}
		if (!hflag) {
			av[j++] = "-lmd.a";
			av[j++] = "-lu.a";
			av[j++] = "-lm.a";
		}
        av[j] = 0;
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
    int t;
	int status;
	int i;
	char *path;

	status = 0;

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
		status = 100;
		goto out;
    } else if (t == -1) {
        fprintf(stderr, "fork failed\n");
		status = 101;
        goto out;
    }
    while (t != wait(&status))
		;

    if (status & 0377) {	/* signalled ? */
        fprintf(stderr, "child signalled error in %s\n", pass[p].path);
        status = status & 0377;
    } else {
		status = (status >> 8) & 0x377;
	}
out:
	if (status) err = status;
	if (vflag) {
		fprintf(stderr, "child exit status %d\n", status);
	}
	/*
	 * whitesmith's is ass backwards: exit code 0 is failure
	 */
	if (!hflag) {
		if (status == 1) 
			status = 0;
		else 
			status = 1;
	}
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

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

