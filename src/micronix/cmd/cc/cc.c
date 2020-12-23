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

/*
 * temporary file names for compilation
 */
char *tmp_cpp;			/* cpp output */
char *tmp_p1;			/* parser output */
char *tmp_cgen;			/* cgen output */
char *tmp_opt;			/* optimizer output */
char *tmp_as;			/* assembler output */

char *outfile;
int mypid;

#define CHSPACE 1000
char ts[CHSPACE + 50];
char *tsa = ts;
char *tsp = ts;
char *av[50];

/* source files */
char *clist[MAXFIL];

char *llist[MAXLIB];
int pflag;				/* preserve temporary files */
int sflag;
int cflag;
int eflag;
int exflag;
int oflag;
int proflag;
int noflflag;
char *chpass;
char *npassname;

char *pass_cpp	= "/bin/CPP";
char *pass_p1	= "/bin/P1";
char *pass_cgen	= "/bin/CGEN";
char *pass_opt	= "/bin/OPT";

char *pref = "/lib/HTCRT0.OBJ";

char *setsuf();
char *strcat();
char *strcpy();
char *pname;

char *
mktmp(char *suffix)
{
	char namebuf[40];

	sprintf(namebuf, "/tmp/ctm%05d-%s", mypid, suffix);
	return strdup(namebuf);
}

usage()
{
	fprintf(stderr, "usage: %s [options] [sources] [objects]\n", pname);
}

main(argc, argv)
    char *argv[];
{
    char *t;
    char *savetsp;
    char *assource;
    char **pv, *ptemp[MAXOPT], **pvt;
    int nc, nl, i, j, c, f20, nxo, na;
    int idexit();

	pname = argv[0];

	mypid = getpid();

    i = nc = nl = f20 = nxo = 0;
    pv = ptemp;

    while (++i < argc) {
        if (*argv[i] == '-') {
            switch (argv[i][1]) {

            case 'S':
                sflag++;
                cflag++;
                break;

            case 'o':
                if (++i < argc) {
                    outfile = argv[i];
                    if ((c = getsuf(outfile)) == 'c' || c == 'o') {
                        error("Would overwrite %s", outfile);
                        exit(8);
                    }
                }
                break;

            case 'O':
                oflag++;
                break;

            case 'E':
                exflag++;

            case 'c':
                cflag++;
                break;

            case 'p':
                pflag++;
                break;

			case 'h':
				usage();
				break;

            case 'D':
            case 'I':
            case 'U':
            case 'C':
                *pv++ = argv[i];
                if (pv >= ptemp + MAXOPT) {
                    error("Too many DIUC options", (char *)NULL);
                    --pv;
                }
                break;

            default:
                error("unknown flag %c\n", argv[i][1]);
				exit(9);
            }
			continue;
        }
        t = argv[i];

		/* if there is a .c or .s file, append it to clist */
        if ((c = getsuf(t)) == 'c' || c == 's' || exflag) {
        	clist[nc++] = t;
            if (nc >= MAXFIL) {
            	error("Too many source files", (char *)NULL);
                exit(1);
            }
            t = setsuf(t, 'o');
        }

		/* append unique object files to llist */
        if (nodup(llist, t)) {
        	llist[nl++] = t;
            if (nl >= MAXLIB) {
            	error("Too many object/library files", (char *)NULL);
                exit(1);
            }
            if (getsuf(t) == 'o')
				nxo++;
        }
    }

    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
        signal(SIGINT, idexit);
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
        signal(SIGTERM, idexit);

	/*
	 * generate temp file names
	 * if no optimizer to be run, put cgen into .s
	 */
	tmp_cpp = mktmp(".e");
	tmp_p1 = mktmp(".1");
	tmp_cgen = mktmp(".2");
	tmp_opt = mktmp(".s");
	tmp_as = mktmp(".obj");
	if (!oflag) {
		tmp_cgen = tmp_opt;
	}

    pvt = pv;
#ifdef notdef

    for (i = 0; i < nc; i++) {
        if (nc > 1)
            printf("%s:\n", clist[i]);
        if (getsuf(clist[i]) == 's') {
            assource = clist[i];
            goto assemble;
        } else
            assource = tmp3;
        savetsp = tsp;
        av[0] = "cpp";
        av[1] = clist[i];
        av[2] = exflag ? "-" : tmp4;
        na = 3;
        for (pv = ptemp; pv < pvt; pv++)
            av[na++] = *pv;
        av[na++] = 0;
        if (callsys(passp, av)) {
            cflag++;
            eflag++;
            continue;
        }
        av[1] = tmp4;
        tsp = savetsp;
        av[0] = "c0";
        av[2] = tmp1;
        av[3] = tmp2;
        if (proflag) {
            av[4] = "-P";
            av[5] = 0;
        } else
            av[4] = 0;
        if (callsys(pass0, av)) {
            cflag++;
            eflag++;
            continue;
        }
        av[0] = "c1";
        av[1] = tmp1;
        av[2] = tmp2;
        if (sflag)
            assource = tmp3 = setsuf(clist[i], 's');
        av[3] = tmp3;
        if (oflag)
            av[3] = tmp5;
        av[4] = 0;
        if (callsys(pass1, av)) {
            cflag++;
            eflag++;
            continue;
        }
        if (oflag) {
            av[0] = "c2";
            av[1] = tmp5;
            av[2] = tmp3;
            av[3] = 0;
            if (callsys(pass2, av)) {
                unlink(tmp3);
                tmp3 = assource = tmp5;
            } else
                unlink(tmp5);
        }
        if (sflag)
            continue;
assemble:
        av[0] = "as";
        av[1] = "-u";
        av[2] = "-o";
        av[3] = setsuf(clist[i], 'o');
        av[4] = assource;
        av[5] = 0;
        cunlink(tmp1);
        cunlink(tmp2);
        cunlink(tmp4);
        if (callsys("/bin/as", av) > 1) {
            cflag++;
            eflag++;
            continue;
        }
    }
nocom:
    if (cflag == 0 && nl != 0) {
        i = 0;
        av[0] = "ld";
        av[1] = "-X";
        av[2] = pref;
        j = 3;
        if (noflflag) {
            j = 4;
            av[3] = "-lfpsim";
        }
        if (outfile) {
            av[j++] = "-o";
            av[j++] = outfile;
        }
        while (i < nl)
            av[j++] = llist[i++];
        if (f20)
            av[j++] = "-l2";
        else {
            av[j++] = "-lc";
        }
        av[j++] = 0;
        eflag |= callsys("/bin/ld", av);
        if (nc == 1 && nxo == 1 && eflag == 0)
            cunlink(setsuf(clist[0], 'o'));
    }
    dexit();
#endif
}

idexit()
{
    eflag = 100;
    dexit();
}

dexit()
{
    if (!pflag) {
        if (!eflag)
			cunlink(tmp_cpp);
        cunlink(tmp_p1);
		if (!oflag)
			cunlink(tmp_cgen);
        if (!sflag)
            cunlink(tmp_opt);
    }
    exit(eflag);
}

error(s, x)
    char *s, *x;
{
    fprintf(exflag ? stderr : stdout, s, x ? x : "" );
    putc('\n', exflag ? stderr : stdout);
    cflag++;
    eflag++;
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

callsys(f, v)
    char f[], *v[];
{
    int t, status;

    if ((t = fork()) == 0) {
        execv(f, v);
        printf("Can't find %s\n", f);
        exit(100);
    } else if (t == -1) {
        printf("Try again\n");
        return (100);
    }
    while (t != wait(&status));
    if (t = status & 0377) {
        if (t != SIGINT) {
            printf("Fatal error in %s\n", f);
            eflag = 8;
        }
        dexit();
    }
    return ((status >> 8) & 0377);
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
    unlink(f);
}
