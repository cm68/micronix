/*
 * PWB/UNIX shell
 * copyright 1973 Bell Telephone Laboratries inc.
 * 2.44 of 5/26/77
 * static char SCCSID[] "@(#)sh.c 2.44";
 *
 * ported to micronix by curt mayer, 17 nov 2020
 * this takes quite a bit of porting
 */

#define	INIT	= 0

#include	<types.h>
#include	<sys/fs.h>
#include	<sys/stat.h>
#include	<errno.h>
#include <setjmp.h>

#define NOPEN	15              /* max open files per process */

#define	INTR	2
#define	QUIT	3
#define	SIGPIPE	13
#define LINSIZ 1000
#define ARGSIZ 50
#define TRESIZ 100

#define QUOTE 0200
#define FAND 1
#define FCAT 2
#define FPIN 4
#define FPOU 8
#define FPAR 16
#define FINT 32
#define FPRS 64

#define TCOM 1
#define TPAR 2
#define TFIL 3
#define TLST 4
#define TOR  5
#define TAND 6

#define DTYP 0
#define DFLG 1
#define DLEF 2
#define DRIT 3
#define DSPR 4
#define DCOM 5
#define N 'n'-'a'
#define P 'p'-'a'
#define R 'r'-'a'
#define S 's'-'a'
#define T 't'-'a'
#define W 'w'-'a'
#define Z 'z'-'a'
#define DOLREPL 1
#define DOLREPQ	2

jmp_buf saved INIT;

/*
 * I am pretty sure this buffer stuff is way overdesigned.
 * we should snarf the whole damn shell script into memory
 * and be done with it.
 */

/*
 * BSIZ = buffer for tty, BSIZFIL = size for file, 1 = size for pipe 
 */
#define BSIZ 64
#define BSIZFIL	512
struct
{
    long start;                 /* absolute file addr of char in buf[0] */
    long linloc;                /* loc of begin of line for while processing */
    int gotten;                 /* number of bytes read in last actual read */
    int nleft;
    char *nextp;
} b INIT;

/*
 * /* primary state var for control of buffering 0 ==> BNOW right; real I/O
 * ptr set at BEND (internal I/O) 1 ==> real ptr has been set from BNOW
 * (pre-exec) 2 ==> post-exec - internals must be reset pre-exec: 0->1;
 * post-exec: 1->2; int cmd: 2->0 pump begin: 1->0 
 */
char bstate INIT;

char b1char INIT;               /* input buffer: 1 at a time */
char *bbuf = &b1char;           /* addr of actual buffer */

#define	BNOW	(b.start + (b.gotten - b.nleft))
#define	BEND	(b.start + b.gotten)
int bnread = BSIZ;              /* must change to 1 if found in pipe */

#define ACNAME	"/etc/sha"

char *dolp INIT;
int idolp INIT;
int oldfil0 INIT;               /* fildes for original file 0 */
char pidp[6] INIT;
char argstr[6] INIT;
int wide = 5;                   /* glitch for 5char pid # */
char devtty[10] INIT;
char **dolv INIT;
int dolc INIT;
char *promp INIT;
char *opromp INIT;
char *optpromp INIT;
char *linep INIT;
char *elinep INIT;
char **argp INIT;
char **eargp INIT;
int *treep INIT;
int *treeend INIT;
char peekc INIT;
char gflg INIT;
char error INIT;

#ifdef notdef
char acctf;                     /* <= 0 ==> no acctg at all, >0 ==> acctg */
char acctfi;                    /* = 0 ==> no acctg for internal cmds */
#endif

char uid INIT;
char setintr INIT;
char *arginp INIT;
char onelflg INIT;
char rflg INIT;
int exitcode INIT;
char exitstr[6] INIT;

/*
 * our 26 variables
 */
char *seta[26] INIT;

char *endcore INIT;
char *endptr INIT;
int oldintr INIT;               /* save INTR state existing at start */
int wasintr INIT;
char gointr[32] INIT;
int catchintr();

/*
 * 1 ==> inside .profile
 * 2 ==> interrupt in .profile 
 */
char proflag INIT;

/*
 * 1 ==> I/O redirection of input; controls pump rebuffering
 */
char redirf INIT;

/*
 * 0 ==> +v, 1 ==> -v; -v ==> print commands
 */
char optfv INIT;

struct stat sb INIT;

/*
 * commands performed internally 
 */
char *comint[] = {
    "chdir",                    /* 0 */
    "shift",                    /* 1 */
    "login",                    /* 2 */
    "newgrp",                   /* 3 */
    "wait",                     /* 4 */
    ":",                        /* 5 */
    "onintr",                   /* 6 */
    "=",                        /* 7 */
    "next",                     /* 8 */
    "if",                       /* 9 */
    "else",                     /* 10 */
    "endif",                    /* 11 */
    "goto",                     /* 12 */
    "test",                     /* 13 */
    "switch",                   /* 14 */
    "break",                    /* 15 */
    "endsw",                    /* 16 */
    "exit",                     /* 17 */
    "while",                    /* 18 */
    "end",                      /* 19 */
    "continue",                 /* 20 */
    "breaksw",                  /* 21 */
    "opt",                      /* 22 */
    "cd",                       /* 23, another name for chdir */
    0
};

/*
 * defines for internal commands 
 */
#define	ZCHDIR	0
#define	ZSHIFT	1
#define	ZLOGIN	2
#define	ZNEWGRP	3
#define	ZWAIT	4
#define	ZLABEL	5
#define	ZONINTR	6
#define	ZEQUALS	7
#define	ZNEXT	8
#define	ZIF	9
#define	ZELSE	10
#define	ZENDIF	11
#define	ZGOTO	12
#define	ZTEST	13
#define	ZSWITCH	14
#define	ZBREAK	15
#define	ZENDSW	16
#define	ZEXIT	17
#define	ZWHILE	18
#define	ZEND	19
#define ZCONTINUE	20
#define ZBREAKSW	21
#define ZOPT	22
#define ZCD	23
#define	ONINTR	comint[ZONINTR]
#define	IF	comint[ZIF]
#define	ENDIF	comint[ZENDIF]
#define	ENDSW	comint[ZENDSW]
#define	END	comint[ZEND]

/*
 * other keywords 
 */
char THEN[] = "then";
char *ARG0 INIT;
char *ARG1 INIT;                /* for diagnostics & internal commands */
int COMTYPE INIT;               /* code for command, -1, or one of Z* */

char *mesg[] = {
    0,
    "Hangup",
    0,
    "Quit",
    "Illegal instruction",
    "Trace/BPT trap",
    "IOT trap",
    "EMT trap",
    "Floating exception",
    "Killed",
    "Bus error",
    "Memory fault",
    "Bad system call",
    "Broken pipe",
    "Alarm clock",
    "Terminated",
    0,
    0,
    0,
    0
};

/*
 * messages 
 */
char *SYNTAX = "syntax error: ";
char *MISS = "missing ";
char *MISSL = "missing label: ";
char *NONUM = "non-numeric arg: ";

char *ARGCNT = "arg count";
char *ARGLNG = "arg list too long";
char *BADARG = "bad arg: ";
char *CANTEX = "cannot execute";
char *CANTOP = "cannot open: ";
char *NOTPER = "not permitted";
char *NOTWHI = "used outside loop";
char *EQERR = "`=' error";

struct stime
{
    long procu, procs, childu, childs, curtim;
} timeb INIT;

struct
{
    char cname[8];
    char lname[6];
    char shtty;
    char tuid;
    long datet;
    long realt;
    long bcput;
    long bsyst;
} tbuf INIT;

#ifdef notdef
long tell();
char *logtty();
#endif

char *nxtarg(), *sname();

trace(s)
    char *s;
{
    char tracebuf[100];

    sprintf(tracebuf, "trace %s\n", s);
    write(1, tracebuf, strlen(tracebuf));
}

/*
 * following items implement while -- end stack of WDEEP levels 
 */
#define	WDEEP	3
#define INWHILE	(wtop < WDEEP)
struct
{
    long sloc;                  /* starting loc = addr of while */
    long eloc;                  /* ending loc = addr of line AFTER end */
} wstk[WDEEP] INIT;
int wtop = WDEEP;               /* top of stack */

main(argc, argv)
    int argc;
    char **argv;
{
    register f;
    register char *p, **v;

    copy(itoa(getpid()), pidp);
    copy("/dev/tty", devtty);
    setxcod(0);

    if (argc >= 2 && (eq(argv[1], "-v") || eq(argv[1], "-x"))) {
        optfv = 1;
        argv[1] = argv[0];
        argv++;
        --argc;
    }
    f = argc - 2;
    f = f < 0 ? 0 : f;
    copy(itoa(f), argstr);
    seta[N] = argstr;

#ifdef notdef
    seta[T] = logtty();
    copyn(logname(), tbuf.lname, sizeof tbuf.lname);
#endif

    tbuf.shtty = devtty[8] = *seta[T];
    v = argv;
    p = sname(*v);
    if (eq(p, "-rsh") || eq(p, "rsh"))
        rflg++;
    if (*p == '-')
        proflag++;
    for (f = 3; f < NOPEN; f++)
        close(f);
    oldfil0 = dup(0);           /* save for pipe into, .profile */
    promp = "% ";
    if (argc > 1) {
        promp = 0;
        tbuf.shtty = 'x';
        signal(INTR, oldintr = signal(INTR, 1));
        if (*v[1] == '-') {
            *p = '-';
            if (v[1][1] == 'c' && argc > 2)
                arginp = v[2];
            else if (v[1][1] == 't')
                onelflg = 2;
        } else {
            close(0);
            f = open(v[1], 0);
            if (f < 0) {
                prs(CANTOP);
                err(v[1]);
                exit(1);
            }
            bnread = BSIZFIL;
        }
    }
    if (onelflg || arginp || (seek(0, 0, 1) == -1 && errno == ESPIPE))
        bnread = 1;             /* no lookahead, no buffering */
    else
        bbuf = sbrk(bnread);
    endptr = endcore = sbrk(0);

#ifdef nodef
    seta[S] = logdir();
#endif

    setwhere();
    if (((uid = getuid()) & 0377) == 0) {
        if (promp)
            promp = ") ";
    }
    pexinit();

#ifdef notdef
    initacct();
#endif

    if (proflag) {
        if (uid != 0 && (f = open(".profile", 0)) >= 0) {
            close(0);
            dup(f);
            close(f);
            promp = 0;
            signal(INTR, catchintr);
        } else
            proflag = 0;
    }
    if (*p == '-' && !proflag) {
        setintr++;
        signal(QUIT, 1);
        signal(INTR, 1);
    }
    dolv = v + 1;
    dolc = argc - 1;

  loop:
    if (promp)
        prs(optpromp ? optpromp : promp);
    main1();
    if (wasintr) {
        if (eq(gointr, "-"))
            goto loop;
        wasintr = 0;
        if (proflag > 1) {
            b.nleft = 0;
            goto loop;          /* will cause faked `next' */
        }
        wtop = WDEEP;           /* clear out while stack */
        ARG0 = ONINTR;
        ARG1 = gointr;
        search(COMTYPE = ZGOTO, 0);
        setxcod(1);
        signal(INTR, 0);
    }
    goto loop;
}

char line[LINSIZ] INIT;

main1()
{
    char *args[ARGSIZ];
    int trebuf[TRESIZ];
    register char c, *cp;
    register *t;

    argp = args;
    eargp = args + ARGSIZ - 5;
    linep = line;
    elinep = line + LINSIZ - 5;
    error = 0;
    gflg = 0;
    b.linloc = BNOW;            /* find where we are, in case while */
    COMTYPE = -1;               /* avoid bad diagnostic from readc */
    do {
        cp = linep;
        word();
        if (optfv) {
            putc(' ');
            prs(cp);
        }
    } while (*cp != '\n');

    treep = trebuf;
    treeend = &trebuf[TRESIZ];
    if (gflg == 0) {
        if (error == 0) {
            setexit();
            if (error)
                return;
            t = syntax(args, argp);
        }
        if (error != 0) {
            err("syntax error");
        } else {
            redirf = 0;         /* no redirect of fildes 0 yet */
            execute(t);
            bsynch(0);          /* adjust internal to real */
        }
    }
}

word()
{
    register char c, c1;
    register dolflag;

    *argp++ = linep;

  loop:
    switch (c = getc(DOLREPL)) {

    case ' ':
    case '\t':
        goto loop;

    case '\'':                 /* '...' : what you see is what you get */
    case '\"':                 /* "..." : \", \$, $ substitution */
        c1 = c;
        dolflag = (c == '"' && !dolp) ? DOLREPQ : !DOLREPL;
        while ((c = getc(dolflag)) != c1) {
            if (c == '\n') {
                error++;
                peekc = c;
                return;
            }
            if (c1 == '"' && c == '\\' &&
                ((peekc = getc(!DOLREPL)) == '$' || peekc == '"')) {
                c = peekc;
                peekc = 0;
            }
            *linep++ = c | QUOTE;
        }
        goto pack;

    case '&':
    case '|':
        *linep++ = c;
        if ((peekc = getc(DOLREPL)) == c)
            peekc = 0;
        else
            linep--;
    case ';':
    case '<':
    case '>':
    case '(':
    case ')':
    case '^':
    case '\n':
        *linep++ = c;
        *linep++ = '\0';
        return;
    case '\\':
        if ((c = getc(!DOLREPL)) == '\n')
            goto loop;
        else {
            c |= QUOTE;
            break;
        }
    }

    peekc = c;

  pack:
    for (;;) {
        if ((c = getc(DOLREPL)) == '\\') {
            if ((c = getc(!DOLREPL)) == '\n')
                c = ' ';
            else
                c |= QUOTE;
        }
        if (any(c, " '\"\t;&<>()|^\n")) {
            peekc = c;
            if (any(c, "\"'"))
                goto loop;
            *linep++ = '\0';
            return;
        }
        *linep++ = c;
    }
}

tree(n)
    int n;
{
    register *t;

    t = treep;
    treep += n;
    if (treep > treeend) {
        prs("Command line overflow\n");
        error++;
        reset();
    }
    return (t);
}

char subchar '$';               /* variable marker, may be changed by pump */

/*
 * flag: !DOLREPL ==> no substitution, DOLREPL ==> substitute, DOLREPQ ==>
 * quoted substitution: "$1" = value of $1 for sure 
 */
getc(flag)
    register flag;
{
    register char c;

    if (peekc) {
        c = peekc;
        peekc = 0;
        return (c);
    }
    if (argp > eargp) {
        argp -= 10;
        while ((c = getc(!DOLREPL)) != '\n');
        argp += 10;
        err("Too many args");
        gflg++;
        return (c);
    }
    if (linep > elinep) {
        linep -= 10;
        while ((c = getc(!DOLREPL)) != '\n');
        linep += 10;
        err("Too many characters");
        gflg++;
        return (c);
    }
  getd:
    if (dolp) {
        if (c = *dolp++) {
            if (flag == DOLREPQ)
                c |= QUOTE;
            return c;
        }
        if (idolp && ++idolp < dolc) {
            dolp = dolv[idolp];
            return (' ');
        }
        dolp = 0;
    }
    c = readc();
    if (c == subchar && flag) {
        c = readc();
        if (c >= '0' && c <= '9') {
            if (c - '0' < dolc)
                dolp = dolv[c - '0'];
            goto getd;
        } else if (c >= 'a' && c <= 'z') {
            dolp = seta[c - 'a'];
            goto getd;
        } else if (c == '$') {
            dolp = pidp;
            goto getd;
        }
        /*
         * $* = $1 $2 .... 
         */
        else if (c == '*') {
            if (dolc > 1) {
                idolp = 1;
                dolp = dolv[1];
            }
            goto getd;
        } else if (c != '\n')
            c = readc();
    }
    return (c & 0177);
}

/*
 * syntax
 *      empty
 *      syn1
 */

syntax(p1, p2)
    register char **p1, **p2;
{

    while (p1 != p2) {
        if (any(**p1, ";&\n"))
            p1++;
        else
            return (syn1(p1, p2));
    }
    return (0);
}

/*
 * syn1
 *      syn1a
 *      syn1a & syntax
 *      syn1a ; syntax
 */

syn1(p1, p2)
    char **p1, **p2;
{
    register char **p;
    register *t, *t1;
    int l;

    l = 0;
    for (p = p1; p != p2; p++)
        switch (**p) {

        case '(':
            l++;
            continue;

        case ')':
            l--;
            if (l < 0)
                error++;
            continue;

        case '&':
            if ((*p)[1] == '&')
                continue;       /* and, not asynch */
        case ';':
        case '\n':
            if (l == 0) {
                l = **p;
                t = tree(4);
                t[DTYP] = TLST;
                t[DLEF] = syn1a(p1, p);
                t[DFLG] = 0;
                if (l == '&') {
                    t1 = t[DLEF];
                    t1[DFLG] |= FAND | FPRS | FINT;
                }
                t[DRIT] = syntax(p + 1, p2);
                return (t);
            }
        }
    if (l == 0)
        return (syn1a(p1, p2));
    error++;
}

/*
 * syn1a
 *      syn1b
 *      syn1b || syn1a
 */

syn1a(p1, p2)
    char **p1, **p2;
{
    register char **p;
    register int l, *t;

    l = 0;
    for (p = p1; p != p2; p++)
        switch (**p) {

        case '(':
            l++;
            continue;

        case ')':
            l--;
            continue;

        case '|':
            if ((*p)[1] == '\0')
                continue;       /* a pipe not an or */
            if (l == 0) {
                t = tree(4);
                t[DTYP] = TOR;
                t[DLEF] = syn1b(p1, p);
                t[DRIT] = syn1a(p + 1, p2);
                t[DFLG] = 0;
                return (t);
            }
        }
    return (syn1b(p1, p2));
}

/*
 * syn1b
 *      syn2
 *      syn2 && syn1b
 */

syn1b(p1, p2)
    char **p1, **p2;
{
    register char **p;
    register int l, *t;

    l = 0;
    for (p = p1; p != p2; p++)
        switch (**p) {

        case '(':
            l++;
            continue;

        case ')':
            l--;
            continue;

        case '&':
            if (l == 0) {
                t = tree(4);
                t[DTYP] = TAND;
                t[DLEF] = syn2(p1, p);
                t[DRIT] = syn1b(p + 1, p2);
                t[DFLG] = 0;
                return (t);
            }
        }
    return (syn2(p1, p2));
}

/*
 * syn2
 *      syn3
 *      syn3 | syn2
 */

syn2(p1, p2)
    char **p1, **p2;
{
    register char **p;
    register int l, *t;

    l = 0;
    for (p = p1; p != p2; p++)
        switch (**p) {

        case '(':
            l++;
            continue;

        case ')':
            l--;
            continue;

        case '|':
        case '^':
            if (l == 0) {
                t = tree(4);
                t[DTYP] = TFIL;
                t[DLEF] = syn3(p1, p);
                t[DRIT] = syn2(p + 1, p2);
                t[DFLG] = 0;
                return (t);
            }
        }
    return (syn3(p1, p2));
}

/*
 * syn3
 *      ( syn1 ) [ < in  ] [ > out ]
 *      word word* [ < in ] [ > out ]
 */

syn3(p1, p2)
    char **p1, **p2;
{
    register char **p;
    char **lp, **rp;
    register *t;
    int n, l, i, o, c, flg;

    flg = 0;
    if (**p2 == ')')
        flg |= FPAR;
    lp = 0;
    rp = 0;
    i = 0;
    o = 0;
    n = 0;
    l = 0;
    for (p = p1; p != p2; p++)
        switch (c = **p) {

        case '(':
            if (l == 0) {
                if (lp != 0)
                    error++;
                lp = p + 1;
            }
            l++;
            continue;

        case ')':
            l--;
            if (l == 0)
                rp = p;
            continue;

        case '>':
            p++;
            if (p1 != p2 && **p == '>') {
                if (l == 0)
                    flg |= FCAT;
            } else
                p--;

        case '<':
            if (l == 0) {
                p++;
                if (p == p2) {
                    error++;
                    p--;
                }
                if (any(**p, "<>("))
                    error++;
                if (c == '<') {
                    if (i != 0)
                        error++;
                    i = *p;
                    continue;
                }
                if (o != 0)
                    error++;
                o = *p;
            }
            continue;

        default:
            if (l == 0)
                p1[n++] = *p;
        }
    if (lp != 0) {
        if (n != 0)
            error++;
        t = tree(5);
        t[DTYP] = TPAR;
        t[DSPR] = syn1(lp, rp);
    } else {
        if (n == 0)
            error++;
        p1[n++] = 0;
        t = tree(n + 5);
        t[DTYP] = TCOM;
        for (l = 0; l < n; l++)
            t[l + DCOM] = p1[l];
    }
    t[DFLG] = flg;
    t[DLEF] = i;
    t[DRIT] = o;
    return (t);
}

scan(at, f)
    int *at;
    int (*f)();
{
    register char *p;
    register *t;

    t = at + DCOM;
    while (p = *t++)
        (*f) (p);
}

tglob(s)
    char *s;
{
    register char *p, c;

    for (p = s; c = *p++;)
        if (any(c, "[?*"))
            gflg = 1;
}

trim(s)
    char *s;
{
    register char *p;

    for (p = s; *p++ &= 0177;);
    return (s);
}

/*
 * this is suspect: av has global and local scope defs
 */
int ap INIT;
int ac INIT;                    /* arg pointer & count for if & related cmds */
char **av INIT;                 /* av[0] = t[DCOM] */
int *savdlef INIT;              /* for cmd piped into, has &t for cmd on */

execute(t, pf1, pf2)
    int *t, *pf1, *pf2;
{
    int i, f, pv[2], wt;
    register *t1;
    register char *cp1, *cp2;

  tryagain:                    /* if expr command may come back here to do * 
                                 * command */
    if (t != 0)
        switch (t[DTYP]) {

        case TCOM:
            ARG0 = cp1 = t[DCOM];
            ARG1 = cp2 = t[DCOM + 1];
            if ((COMTYPE = lookup(cp1)) < 0)
                goto notinternal;

#ifdef notdef
            if (acctfi) {
                times(&timeb);
                time(&timeb.curtim);
            }
#endif

            setxcod(0);         /* assume will be good */
            av = &t[DCOM];
            ap = 1;
            for (ac = 1; av[ac]; ac++);
            if (cp2)
                trim(cp2);
            if (COMTYPE != ZEQUALS)     /* = takes care of self,can't do now */
                bsynch(0);      /* make sure internal ptr right */
            switch (COMTYPE) {
            case ZCHDIR:
            case ZCD:
                if (rflg) {
                    die(NOTPER, 0);
                    break;
                }
                if (cp2 == 0)
                    die(ARGCNT, 0);
                else if (chdir(cp2) < 0)
                    die("bad directory", 0);
                break;

            case ZSHIFT:
                if (dolc < 1) {
                    die(ARGCNT, 0);
                    break;
                }
                if (cp2 == 0) {
                    dolv[1] = dolv[0];
                    dolv++;
                    dolc--;
                    break;
                }
                if ((i = atoi(cp2)) < 0 || i > dolc) {
                    die(BADARG, cp2);
                    break;
                }
                for (; i < dolc; i++)
                    dolv[i] = dolv[i + 1];
                dolc--;
                break;

            case ZLOGIN:
                if (promp != 0) {
                    fclean();
                    execv("/bin/login", t + DCOM);
                }
                die(CANTEX, 0);
                break;

            case ZNEWGRP:
                if (promp != 0) {
                    fclean();
                    execv("/bin/newgrp", t + DCOM);
                }
                die(CANTEX, 0);
                break;

            case ZWAIT:
                pwait(-1, 0);
                break;

            case ZLABEL:
            case ZENDIF:
            case ZENDSW:
                break;

            case ZONINTR:
                /*
                 * suppress onintr if interactive shell, or if noninteractive 
                 * fired up immune to INTR. 
                 */
                if (promp || oldintr)
                    break;
                if (!cp2) {
                    signal(INTR, 0);
                    break;
                }
                copy(cp2, gointr);
                wasintr = 0;
                signal(INTR, catchintr);
                break;

            case ZEQUALS:
                if (t[DFLG] & FPIN)
                    close(pf1[1]);
                i = *cp2 - 'a';
                if (t[DCOM + 3] != 0 && eq(t[DCOM + 2], "")) {
                    t[DCOM + 2] = t[DCOM + 3];
                    setxcod(1);
                }
                /*
                 * 3rd exists & null 2nd ==> use 3rd instead 
                 */
                if (i > 25 || i < 0 || (i == P && rflg))
                    err(EQERR);
                else {
                    f = t[DFLG] & FPIN;
                    if ((seta[i] = rdval((f ? pf1[0] : 0),
                                t[DLEF], t[DCOM + 2])) == 0)
                        err(EQERR);
                    if (f) {    /* piped, must assure synch */
                        pwait(savdlef[DSPR], savdlef);
                        bsynch(2);
                    }
                }
                break;

            case ZNEXT:
                opromp = promp;
                if (!cp2) {
                    f = dup(oldfil0);
                    promp = uid ? "% " : "# ";
                    bnread = BSIZ;
                    proflag = 0;
                } else {
                    if (rflg) {
                        die(NOTPER, 0);
                        break;
                    }
                    promp = 0;
                    f = open(cp2, 0);
                }
                if (f < 0) {
                    promp = opromp;
                    die(CANTOP, cp2);
                } else {
                    close(0);
                    dup(f);
                    close(f);
                    signal(INTR, promp ? 1 : 0);
                    signal(QUIT, promp ? 1 : 0);
                    setintr = promp ? 1 : 0;
                }
                b.nleft = b.gotten = 0;
                b.start = 0;
                for (wtop = 0; INWHILE; wtop++) {
                    wstk[wtop].sloc = 0;
                    wstk[wtop].eloc = 0;
                }
                break;

            case ZEXIT:
                bflush();
                setxcod(cp2 ? atoi(cp2) : 0);
                break;

            case ZGOTO:
                /*
                 * following takes care of (unlikely) case in which goto
                 * exits from loop(s) whose end(s) are as yet unlocated.
                 * although not efficient, it is rigged to act in the
                 * intuitively proper way. 
                 */
                i = wtop;
                while (INWHILE) {
                    if (wstk[wtop].eloc == 0) {
                        search(ZBREAK, 0);      /* find end */
                        wstk[wtop].eloc = BNOW;
                    } else
                        bseek(wstk[wtop].eloc);
                    wtop++;
                }
                wtop = i;

                search(ZGOTO, 0);

                b.linloc = BNOW;        /* effect of goto on while */
                while (INWHILE &&
                    (b.linloc < wstk[wtop].sloc ||
                        b.linloc > wstk[wtop].eloc))
                    wtop++;     /* pop */
                break;

            case ZELSE:
                search(ZELSE, eq(cp2, IF) && eq(av[ac - 1], THEN));
                break;

            case ZBREAK:
                if (INWHILE)
                    toend();
                else
                    die(NOTWHI, 0);
                break;

            case ZBREAKSW:
                ARG1 = 0;       /* ignore possible arg */

            case ZSWITCH:
                search(COMTYPE, 0);     /* ZBREAKSW or ZSWITCH */
                break;

            case ZTEST:
                i = exp();
                if (nxtarg() != 0)
                    die(SYNTAX, av[ap - 1]);
                else
                    setxcod(!i);
                break;

            case ZIF:
                if (exp()) {
                    if (eq(nxtarg(), THEN))
                        break;
                    --ap;       /* ap -> new cmd */
                    /*
                     * shift args to eliminate if 
                     */
                    for (i = DSPR; i >= 0; i--)
                        av[--ap] = t[i];
                    t = &av[ap];

#ifdef notdef
                    enacct(cp1, 1);     /* acct for if part */
#endif

                    goto tryagain;
                } else if (eq(nxtarg(), THEN))
                    search(ZIF, 0);
                break;

            case ZWHILE:
                if (ac > 2) {   /* expr like that of if */
                    i = exp();
                    if (cp1 = nxtarg()) {
                        die(SYNTAX, cp1);
                        break;
                    }
                } else          /* 0 args or 1 null arg = false, else true */
                    i = cp2 != 0 && *cp2 != '\0';
                if (wtop >= WDEEP || wstk[wtop].sloc != b.linloc) {
                    /*
                     * not already in loop 
                     */
                    if (--wtop >= 0) {
                        if (wstk[wtop].sloc != b.linloc) {
                            wstk[wtop].sloc = b.linloc;
                            wstk[wtop].eloc = 0;
                        }
                    } else {
                        die(">3 levels", 0);
                        break;
                    }
                }

                if (i)
                    break;
                toend();        /* condition nomatch */
                break;
            case ZCONTINUE:
                if (INWHILE)
                    bseek(wstk[wtop].sloc);
                else
                    die(NOTWHI, 0);
                break;

            case ZEND:
                if (INWHILE) {  /* while active */
                    wstk[wtop].eloc = BNOW;
                    bseek(wstk[wtop].sloc);
                } else
                    die(NOTWHI, 0);
                break;

            case ZOPT:
                for (i = DCOM + 1; cp2 = t[i]; i++) {
                    if (eq(cp2, "-v") || eq(cp2, "-x"))
                        optfv = 1;
                    else if (eq(cp2, "+v") || eq(cp2, "+x"))
                        optfv = 0;
                    else if (eq(cp2, "-p")) {
                        if (cp2 = t[++i])
                            optpromp = rdval(0, 0, cp2);
                        else
                            optpromp = "";
                    }
                }
                break;
            }

#ifdef notdef
            enacct(cp1, 1);     /* internal commands */
#endif

            return;

          notinternal:
        case TPAR:
            bsynch(1);          /* set real ptr = BNOW */
            f = t[DFLG];
            i = 0;
            if ((f & FPAR) == 0)
                if ((i = dofork()) == -1)
                    return;     /* couldn't fork */
            if (i != 0) {       /* parent */
                if ((f & FPIN) != 0) {
                    close(pf1[0]);
                    close(pf1[1]);
                    t[DLEF] = savdlef;  /* link back for pwait */
                } else
                    t[DLEF] = 0;        /* 1st or only in pipeline */
                t[DSPR] = i;    /* save proc-num for pwait */
                if ((f & FPRS) && promp) {
                    prs(itoa(i));
                    prs("\n");
                }
                if ((f & (FAND | FPOU)) == 0)
                    pwait(i, t);
                bsynch(2);      /* mark possible read by cmd */
                return;
            }
            /*
             * ignore interrupts for asynchronous cmds [code needed
             * for sh proc only; ignore also for onintr -
             */
            if (f & FPRS) {
                signal(INTR, 1);
                signal(QUIT, 1);
            }
            if (eq(gointr, "-"))
                signal(INTR, 1);
            if (t[DLEF] != 0) {
                close(0);
                /*
                 * following for pipe into sh 
                 */
                trim(t[DLEF]);
                if (eq(t[DLEF], "--"))
                    dup(oldfil0);
                else {
                    i = open(t[DLEF], 0);
                    if (i < 0)
                        xdie(CANTOP, t[DLEF]);
                }
                redirf++;
            }
            if (t[DRIT] != 0) {
                if (rflg)
                    xdie(">: ", NOTPER);
                if ((f & FCAT) != 0) {
                    i = open(trim(t[DRIT]), 1);
                    if (i >= 0) {
                        seek(i, 0, 2);
                        goto f1;
                    }
                }
                i = creat(trim(t[DRIT]), 0666);
                if (i < 0)
                    xdie("cannot create: ", t[DRIT]);
              f1:
                close(1);
                dup(i);
                close(i);
            }
            if ((f & FPIN) != 0) {
                close(0);
                dup(pf1[0]);
                close(pf1[0]);
                close(pf1[1]);
                redirf++;
            }
            if ((f & FPOU) != 0) {
                close(1);
                dup(pf2[1]);
                close(pf2[0]);
                close(pf2[1]);
            }
            if ((f & FINT) != 0 && t[DLEF] == 0 && (f & FPIN) == 0) {
                close(0);
                open("/dev/null", 0);
            }
            if ((f & FINT) == 0 && setintr) {
                signal(INTR, 0);
                signal(QUIT, 0);
            }
            if (t[DTYP] == TPAR) {
                if (t1 = t[DSPR])
                    t1[DFLG] |= f & FINT;
                execute(t1);
                exit(exitcode);
            }
            fclean();

            if (rflg && any('/', trim(t[DCOM])))
                xdie(NOTPER, 0);        /* no / in rsh commands */
            gflg = 0;
            scan(t, &tglob);
            if (gflg && t[DCOM + 1]) {
                /*
                 * old glob stuff t[DSPR] = seta[P]; execv("/etc/glob",
                 * t+DSPR); xdie("glob: ",CANTEX); 
                 */
                etcglob(t + DCOM);
            }
            scan(t, &trim);

            if (eq(cp1, "pump"))
                dopump(&t[DCOM + 1], redirf);
            texec(t[DCOM], t);

        case TFIL:
            f = t[DFLG];
            pipe(pv);
            t1 = t[DLEF];
            t1[DFLG] |= FPOU | (f & (FPIN | FINT | FPRS));
            execute(t1, pf1, pv);
            t1 = t[DRIT];
            t1[DFLG] |= FPIN | (f & (FPOU | FINT | FAND | FPRS));
            savdlef = t[DLEF];  /* save so can link pipe together */
            execute(t1, pv, pf2);
            return;

        case TLST:
            f = t[DFLG] & FINT;
            if (t1 = t[DLEF])
                t1[DFLG] |= f;
            execute(t1);
            if (t1 = t[DRIT])
                t1[DFLG] |= f;
            execute(t1);
            return;

        case TOR:
        case TAND:
            f = t[DFLG] & FINT;
            t1 = t[DLEF];
            t1[DFLG] |= f;
            execute(t1);
            i = atoi(seta[R]);
            if ((i == 0) == (t[DTYP] == TAND)) {
                t1 = t[DRIT];
                t1[DFLG] |= f;
                execute(t1);
            }
            return;

        }
}

toend()
{
    if (wstk[wtop].eloc == 0) { /* need to find end */
        ARG1 = 0;
        search(ZBREAK, 0);
    } else
        bseek(wstk[wtop].eloc);
    wtop++;                     /* pop 1 level */
    return;
}

/*
 * look up p in command table. return index or -1
 */
lookup(p)
    char *p;
{
    char *q;
    int i;

    for (i = 0; q = comint[i]; i++) {
        if (eq(p, q))
            return (i);
	}
    return -1;
}

/*
 * loop on trying to fork.  
 * if there are no processes, wait for a bit and try again
 */
dofork()
{
    register wt, i;

    for (wt = 10;; wt += 10) {
        if ((i = fork()) != -1)
            break;
        if (promp == 0 && wt < 60)
            sleep(wt);
        else {
            err("cannot fork;try again");
            break;
        }
    }
    return i;
}

fclean()
{
#ifdef notdef
    if (acctf)
        close(acctf);
#endif

    if (oldfil0)
        close(oldfil0);
    return;
}

texec(f, t)
    register *t;
{
    register char *cp;
    char tline[48];
    char txe2big, txeacces;
    int txtbsy;                 /* kludge cntr for ETXTBSY fix */

    txeacces = txe2big = 0;
    txtbsy = 0;
    cp = seta[P];               /* normal case -- search */
    if (any('/', f)) {
        if (rflg)
            xdie(NOTPER, 0);    /* no / by rsh */
        cp = "";                /* sh: exec only cmd name as given */
    }
    do {
        cp = pcat(cp, f, tline, sizeof tline);
      retry:
        execv(tline, t + DCOM);
        switch (errno) {
        case ENOEXEC:
            t[DCOM] = tline;
            t[DSPR] = seta[Z];
            execv(t[DSPR], t + DSPR);
            xdie(seta[Z], " No shell!");
        case EACESS:
            txeacces++;         /* file there, missing x (probably) */
            break;
        case ENOMEM:
            xdie("too large", 0);
        case E2BIG:
            txe2big++;
            break;
        case ETXTBSY:
            if ((txtbsy += 10) > 60)
                xdie("text busy", 0);
            sleep(txtbsy);
            goto retry;
        }
    } while (cp);
    if (txe2big)
        xdie(ARGLNG, 0);
    if (txeacces)
        xdie("file not executable", 0);
    xdie("not found", 0);
}

char pipebomb INIT;             /* 1 ==> SIGPIPE caught */

catchpipe()
{
    if (promp != 0)
        exit(1);
    pipebomb++;
    return;
}

/*
 * dopump: pump command: pump [+] [-[subchar]] [eofstr] default is to
 * substitute using $ - suppresses substitution -subchar uses subchar instead 
 * of $ for substitution + causes leading tabs in input to be thrown away
 * eofstr defaults to ! eofstr may not begin with `+', which is reserved for
 * future flags.  acts like filter, although actually copy of sh in input,
 * \subchar = subchar, no other escaping performed if interactive, ignores
 * interrupts & quits, but dies if other end of pipe dies; if
 * non-interactive, reads to eofstr or real eof even if other end does die.
 * NOTE: line is used as workarea, both for eofstr and input line eofstr is
 * safe because initial arg can't be at beginning 
 */

dopump(t)
    register char **t;
{
#define	eofstr	line
#define	pumpwk	(line+96)
    register char c, *p;
    char tabeat;

    tabeat = 0;
    if (redirf) {
        b.nleft = b.gotten = 0; /* clear buffer values */
        bnread = 1;             /* be careful with pipes & files */
    } else {
        if (promp == 0)
            bsynch(0);          /* use existing b., if any */
        else {
            signal(INTR, 1);    /* interactive - ignore */
            signal(QUIT, 1);    /* for sake of pump -|ed */
        }
    }
    if (proflag) {
        signal(INTR, 0);
        proflag = 0;
    }
    signal(SIGPIPE, catchpipe);
    setxcod(0);                 /* in case real eof */
    copy("!", eofstr);
    for (; *t; t++)
        switch (**t) {
        case '-':
            subchar = *(*t + 1);        /* \0 turns subst off */
            break;
        case '+':
            tabeat++;
            break;
        default:
            copy(*t, eofstr);
        }

    while (1) {                 /* real eof will cause exit in readc */
        p = pumpwk;
        if (tabeat) {
            while ((c = getc(DOLREPL)) == '\t');
            peekc = c;
        }
        do {
            c = getc(DOLREPL);
            if (c == '\\' && (peekc = getc(!DOLREPL)) == subchar) {
                c = peekc;
                peekc = 0;
            }
            *p++ = c;
        } while (c != '\n');
        *--p = 0;               /* put 0 for \n */
        if (eq(pumpwk, eofstr))
            break;
        *p = '\n';
        if (!pipebomb)
            write(1, pumpwk, p - pumpwk + 1);
    }
    bsynch(1);                  /* in case file */
    exit(pipebomb);
}

atoi(s)
    char *s;
{

    register char *sp;
    register i, neg;

    sp = s;
    i = neg = 0;

    if (*sp == '-') {
        ++neg;
        ++sp;
    }

    while (*sp) {
        if (*sp < '0' || *sp > '9') {
            die(NONUM, s);
            return 0;
        }
        i = i * 10 + (*sp - '0');
        ++sp;
    }
    return neg ? -i : i;
}

xdie(str1, str2)
    char *str1, *str2;
{
    die(str1, str2);
    exit(1);
}

die(str1, str2)
    char *str1, *str2;
{
    prs(ARG0);
    prs(": ");
    prs(str1);
    err(str2);
    return;
}

/*
 * err: emit error message, flush input by seeking to EOF (but only
 *              if reading from 0 in unrestricted way), exit.
 */
err(s)
    char *s;
{

    prs(s);
    prs("\n");

    if (onelflg == 0 && arginp == 0)
        bflush();

    setxcod(1);
}

setexit()
{
    setjmp(&saved);
}

reset()
{
    longjmp(&saved, 0);
}

prs(s)
    char *s;
{
    if (s == 0)
        return;

	write(2, s, strlen(s));
}

putc(c)
{
    write(2, &c, 1);
}

/*
 * true if any 'c' in s
 */
any(c, s)
    char c, *s;
{

    while (*s)
        if (*s++ == c)
            return (1);
    return (0);
}

/*
 * true if strings equal
 */
eq(s1, s2)
    char *s1, *s2;
{

    if (s1 == 0 || s2 == 0)
        return 0;
    while (*s1++ == *s2)
        if (*s2++ == '\0')
            return (1);
    return (0);
}

#ifdef notdef
/*
 * initacct: initialize acctg according to state of ACNAME file: if cannot
 * open ACNAME, no acctg done (acctf == 0) group permissions of ACNAME are
 * used otherwise: 0 cmd level, externals only (acctf > 0, acctfi == 0) 4 cmd 
 * level, add internal cmds (acctf > 0, acctfi > 0) 2 shell proc, externals
 * only (acctf > 0, acctfi == 0) 1 shell proc, add internal cmds (acctf > 0,
 * acctfi > 0) NOTE: THIS IS KLUDGE INTENDED FOR OBLIVION AFTER EITHER NEW MH 
 * ACCTG GETS INSTALLED AND/OR INTERRUPT-HANDLING CHANGE PERMITS BETTER
 * INTERNAL CMD HANDLING. 
 */

initacct()
{
    register f;

    if ((acctf = open(ACNAME, 1)) < 0)
        acctf = 0;              /* no acctg at all */
    else {
        fstat(acctf, &sb);
        f = sb.i_mode;
        if (tbuf.shtty == 'x') {        /* shell proc */
            if (f & 020) {
                if (f & 010)
                    acctfi++;   /* ext + int */
            } else {
                close(acctf);   /* no acct at all */
                acctf = 0;
            }
        } else if (f & 040)
            acctfi++;           /* cmd level, ext + int */
    }
    return;
}
#endif

pwait(i, t)
    int i, *t;
{
    register p, e, *t1;
    int nprocs;
    int s;

    nprocs = (t1 = t) ? 0 : 1;
    while (t1) {                /* count number of procs in current pipeline */
        if (t1[DSPR])
            nprocs++;
        t1 = t1[DLEF];
    }

    if (i != 0)
        do {

#ifdef notdef
            if (acctf) {
                times(&timeb);
                time(&timeb.curtim);
            }
#endif

            p = wait(&s);
            if (wasintr)
                p = wait(&s);
            if (p == -1)
                break;
            e = s & 0177;
            exitcode = (s >> 8) & 0377;
            if (e == INTR && wasintr)   /* process returned intr */
                e = 0;
            if (mesg[e] != 0) {
                if (p != i) {
                    prs(itoa(p));
                    prs(": ");
                }
                prs(mesg[e]);
                if (s & 0200)
                    prs(" -- Core dumped");
            }
            if (p == i)
                setxcod(exitcode);
            if (e != 0)
                err("");
            for (t1 = t; t1; t1 = t1[DLEF]) {   /* hunt proc; elims most
                                                 * **gok */
                if (t1[DSPR] == p) {
                    nprocs--;
                    break;
                }
            }

#ifdef notdef
            acct(t1);
#endif
        } while (nprocs);
}

#ifdef notdef
acct(t)
    int *t;
{
    if (t == 0)
        enacct("**gok", 0);
    else if (*t == TPAR)
        enacct("()", 0);
    else
        enacct(t[DCOM], 0);
}

enacct(as, acctype)
    char *as;
    int acctype;                /* 0 ==> child process, 1 ==> internal cmd */
{
    struct stime timbuf;
    register i;
    register char *np;

    if (uid == 0)
        return;
    if (acctf <= 0)             /* if no accounting active, elim sys calls */
        return;
    if (acctype && acctfi <= 0)
        return;                 /* internal cmd, but no internal acctg */
    times(&timbuf);
    time(&timbuf.curtim);
    tbuf.realt = timbuf.curtim - timeb.curtim;
    if (acctype) {
        tbuf.bcput = timbuf.procu - timeb.procu;
        tbuf.bsyst = timbuf.procs - timeb.procs;
    } else {
        tbuf.bcput = timbuf.childu - timeb.childu;
        tbuf.bsyst = timbuf.childs - timeb.childs;
    }
    np = sname(as);
    for (i = 0; i < 8; i++) {
        tbuf.cname[i] = *np;
        if (*np)
            np++;
    }
    tbuf.datet = timbuf.curtim;
    tbuf.tuid = uid;
    seek(acctf, 0, 2);
    write(acctf, &tbuf, sizeof(tbuf));
}
#endif

rdval(pipef, lef, na)
    char *na;
{
    register char *st, *np;
    char c;

    st = endptr;
    np = na;
    if (!pipef && lef) {
        pipef = eq(lef, "--") ? dup(oldfil0) : open(lef, 0);
        if (pipef < 0)
            return 0;
    }
    for (;;) {
        if (endptr >= endcore - 10)
            endcore = sbrk(64);
        if ((int) endcore == -1)
            return 0;
        if (!na) {
            if (read(pipef, &c, 1) <= 0) {
                setxcod(1);     /* EOF indicator */
                break;
            }
        } else
            c = *np++ & 0177;
        *endptr++ = c;
        if (c == '\n' || c == '\0')
            break;
    }
    if (c == '\n')
        --endptr;
    *endptr++ = '\0';
    if (pipef || lef)
        close(pipef);
    return st;
}

catchintr()
{
    if (proflag)
        proflag++;              /* in .profile, make sure come out ok */
    wasintr++;
    signal(INTR, 1);
}

pcat(so1, so2, si, sz)
    register char *so1, *so2;
    char *si;
    int sz;
{
    register char *s;

    s = si;
    while (*so1 != ':' && *so1 != '\0' && --sz)
        *s++ = *so1++;
    if (si != s && --sz > 0)
        *s++ = '/';
    while (*so2 && --sz > 0)
        *s++ = *so2++;
    if (--sz < 0) {
        *si = '\0';
        die("cmd line overflow", 0);
    } else
        *s = '\0';
    return *so1 ? ++so1 : 0;
}

setxcod(code)
    int code;
{
    copy(itoa(code), exitstr);
    seta[R] = exitstr;
    return;
}

/*
 * high-bit stripping string copy
 */
copy(src, dst)
    char *src, *dst;
{
    while (*dst++ = *src++ & 0177);
}

/*
 * copyn: copy at most n bytes from source to sink.
 */
copyn(src, dst, n)
    char *src, *dst;
    int n;
{
    while (n--) {
        if (!(*dst++ = *src++))
            break;
	}
}

/*
 * integer to decimal ascii in a 5 byte field.
 */
itoa(n)
int n;
{
    char *cp;
    static char *str[12];
    char i;

    for (i = 4; i >= 0; i--) {
        *cp = (n % 10) + '0';
        n /= 10;
        cp--;
    }
}

char *
nxtarg()
{
    register iap;

    if ((iap = ap++) > ac || av[iap] == 0)
        return (0);
    return trim(av[iap]);
}

exp()
{
    int p1;

    p1 = e1();
    if (eq(nxtarg(), "-o"))
        return (p1 | exp());
    ap--;
    return (p1);
}

e1()
{
    int p1;

    p1 = e2();
    if (eq(nxtarg(), "-a"))
        return (p1 & e1());
    ap--;
    return (p1);
}

e2()
{
    if (eq(nxtarg(), "!"))
        return (!e3());
    ap--;
    return (e3());
}

e3()
{
    int ccode;
    int nap;
    int int1, int2;
    register char *a, *p1, *p2;

    ccode = 0;
    a = nxtarg();
    if (eq(a, "(")) {
        ccode = exp();
        if (!eq(nxtarg(), ")"))
            goto erre3;
        return (ccode);
    }

    if (eq(a, "{")) {           /* execute a command for exit code */
        nap = ap;               /* save 1st arg ptr */
        while (ap < ac && !eq(av[ap], "}"))
            trim(av[ap++]);
        if (ap >= ac || nap == ap) {
            die(SYNTAX, av[nap]);
            return 0;
        }
        av[ap++] = 0;           /* change } to 0 for pexec */
        if ((int1 = dofork()) > 0)
            wait(&ccode);       /* parent */
        else if (int1 == 0)     /* child */
            texec(av[nap], &av[nap - DCOM]);
        return (ccode ? 0 : 1);
    }

    p1 = nxtarg();

    /*
     * file predicates 
     */
    if (eq(a, "-r"))
        return (tio(p1, 0));

    if (eq(a, "-w"))
        return (tio(p1, 1));
    if (eq(a, "-f")) {
        if (stat(p1, &sb) == -1)
            return 0;
        return ((sb.st_mode & S_IFMT) == S_IFREG);
    }
    if (eq(a, "-d")) {
        if (stat(p1, &sb) == -1)
            return 0;
        return ((sb.st_mode & S_IFMT) == S_IFDIR);
    }
    if (eq(a, "-s")) {
        if (stat(p1, &sb) == -1)
            return 0;
        return (sb.d.size0 || sb.d.size1);
    }

    /*
     * string predicates 
     */
    if (eq(a, "-n"))
        return (p1 && *p1 != '\0');
    if (eq(a, "-z"))
        return (p1 == 0 || *p1 == '\0');
    if ((p2 = nxtarg()) == 0)
        goto erre3;
    if (eq(p1, "="))
        return (eq(a, p2));

    if (eq(p1, "!="))
        return (!eq(a, p2));

    int1 = atoi(a);
    int2 = atoi(p2);
    if (eq(p1, "-eq"))
        return (int1 == int2);
    if (eq(p1, "-ne"))
        return (int1 != int2);
    if (eq(p1, "-gt"))
        return (int1 > int2);
    if (eq(p1, "-lt"))
        return (int1 < int2);
    if (eq(p1, "-ge"))
        return (int1 >= int2);
    if (eq(p1, "-le"))
        return (int1 <= int2);

  erre3:
    die(SYNTAX, p1);
}

tio(a, f)
    char *a;
    int f;
{
    register int fil;

    if ((fil = open(a, f)) >= 0) {
        close(fil);
        return (1);
    }
    return (0);
}

/*
 * search: forward search for required token, type = ZGOTO: : label ZSWITCH:
 * : label, : default, or unmatched ENDSW ZBREAKSW: unmatched endsw ZIF:
 * unmatched else or endif (or }) (this is failed if-then) ZELSE: unmatched
 * endif (or }) ZBREAK: unmatched end (break, failed while or end) levinit =
 * 0, except when called from else if ... then, when it is 1 
 */
search(type, levinit)
    int type, levinit;
{
    register int level, t;
    register char *aword;
    char fifel, fbr, fswex;
    char wordbuf[128];

    if (type == ZGOTO)
        bseek(0L);
    fifel = type == ZIF || type == ZELSE;
    fbr = type == ZBREAK;
    fswex = type == ZSWITCH || type == ZBREAKSW;
    aword = wordbuf;
    level = levinit;

    do {
        *aword = '\0';
        getword(aword);
        if ((t = lookup(aword)) >= 0)
            switch (t) {
            case ZELSE:
                if (level == 0 && type == ZIF)
                    return;     /* leave rest of else line */
                continue;
            case ZIF:
                while (getword(aword));
                if (fifel && eq(aword, THEN))
                    level++;
                break;
            case ZENDIF:
                if (fifel)
                    level--;
                break;
            case ZWHILE:
                if (fbr)
                    level++;
                break;
            case ZEND:
                if (fbr)
                    level--;
                break;
            case ZSWITCH:
                if (fswex)
                    level++;
                break;
            case ZENDSW:
                if (fswex)
                    level--;
                break;
            case ZLABEL:
                if (getword(aword)) {
                    if (type == ZGOTO) {
                        if (eq(aword, ARG1))
                            level = -1;
                    } else if (type == ZSWITCH &&
                        level == 0 &&
                        (eq(aword, "default") || match(ARG1, aword)))
                        level = -1;
                }
                break;
            }
        getword(0);             /* gobble rest of line, if any */
    } while (level >= 0 && proflag < 2);
    return;
}

/*
 * getword: return next word from input (if any): aword != 0 : get next word
 * (if any, return 1), if none, return 0, do not distturb *aword. aword == 0
 * : consume rest of line thru (nl). return 1 if word, 0 if only newline
 * left. 
 */

getword(aword)
    char *aword;
{
    register int found;         /* 1 ==> found word, 0 ==> not */
    register char c, *wp;

    wp = aword;
    found = 0;
    c = peekc ? peekc : readc();
    peekc = '\0';

    do {
        while (c == ' ' || c == '\t')
            c = readc();
        if (c == '\n') {
            if (wp)
                break;
            else
                return 0;       /* newline only thing left */
        }
        found++;
        do {
            if (wp)
                *wp++ = c;
            c = readc();
            if (c == '\\' && (c = readc()) == '\n')
                c = ' ';
        } while (c != ' ' && c != '\t' && c != '\n');
    } while (!wp);
    peekc = c;
    if (found)
        *wp = '\0';
    return (found);
}

readc()
{
    register c;

    if (arginp) {
        if (arginp == 1)
            goto ONLYEXIT;
        if ((c = *arginp++) == 0) {
            arginp = 1;
            c = '\n';
        }
        return (c);
    }
    if (onelflg == 1)
        goto ONLYEXIT;
    if (b.nleft == 0) {
        b.nextp = bbuf;
        b.start = b.start + b.gotten;   /* increm by last gotten */
        if ((b.nleft = read(0, b.nextp, bnread)) == 0 || proflag > 1) {
            if (proflag) {      /* still inside .profile */
                b.nleft = 5;    /* fake next */
                b.nextp = "next\n";
            } else {
                eoferr();
              ONLYEXIT:
                exit(atoi(seta[R]));
            }
        }
        b.gotten = b.nleft;
    }
    b.nleft--;
    c = *b.nextp++;
    if (c == '\n' && onelflg)
        onelflg--;
    return (c);
}

/*
 * eoferr: issue error message if cmd was in middle of search 
 */
eoferr()
{
    switch (COMTYPE) {
    case ZGOTO:
        die(MISSL, ARG1);
        break;
    case ZIF:
    case ZELSE:
        die(MISS, ENDIF);
        break;
    case ZSWITCH:
    case ZBREAKSW:
        die(MISS, ENDSW);
        break;
    case ZWHILE:
    case ZBREAK:
        die(MISS, END);
        break;
    }
    bsynch(1);
    return;
}

/*
 * bflush: complete input flush 
 */
bflush()
{
    seek(0, 0, 2);
    b.nleft = b.gotten = 0;
    return;
}

/*
 * bsynch: synchronize internal buffering & outside world 
 */
/*
 * btarg gives nominal target value of bstate 
 */
bsynch(btarg)
    register btarg;
{
    register obstate;
    long f;

    if (btarg == bstate || promp || bnread == 1 || redirf)
        return;                 /* no seeking in any of these cases */
    obstate = bstate;
    bstate = btarg;
    if (btarg == 2)
        return;                 /* just remember this; do nothing now */
    switch (obstate) {
    case 0:                    /* 0 --> 1 only possible */
        if (b.nleft > 0)
            lseek(0, BNOW, 0);  /* set real = internal */
        break;
    case 1:                    /* 1 --> 0 (pump begin) */
        lseek(0, BEND, 0);      /* ptr where expected for read */
        break;
    case 2:                    /* 2 --> 0; 2 --> 1, leave as 2, since 1 *
                                 * dominates */
        if (btarg == 0) {
            f = 0;

#ifdef notdef
            f = tell(0);
#endif

            if (f >= 0)
                bseek(f);
        } else
            bstate = 2;
        break;
    }
}

/*
 * bseek: seek, staying within current input buffer b, if possible 
 */
bseek(where)
    long where;
{
    long bend;

    if (promp || bnread == 1 || redirf)
        return;
    bend = BEND;
    if (where >= b.start && where < bend) {
        b.nleft = bend - where; /* 1 <= b.nleft <= b.gotten */
        b.nextp = bbuf + b.gotten - b.nleft;
        lseek(0, bend, 0);      /* really at end ofbuffer */
    } else {
        b.nleft = b.gotten = 0;
        b.start = where;
        lseek(0, where, 0);
    }
    return;
}

/*
 * sname: simple name (last component) of file name 
 */
char *
sname(s)
    register char *s;
{
    register char *p;

    for (p = s; *p;)
        if (*p++ == '/' && *p != '\0')
            s = p;
    return (s);
}

/*
 * setwhere: set up wherev for $w (1st component of pathname) 
 */
char wherev[6] INIT;

setwhere()
{
    register char *s, *w;
    register i;

    s = seta[S];
    seta[W] = w = wherev;
    i = 0;
    do {
        *w++ = *s++;
    } while (++i < 5 && *s != '/');
}

/*
 *      pexinit: fills in pathstr (seta[P]) and shellnam (seta[Z]).
 *      may be invoked before fork to avoid unnecessary .path opening.
 *      returns 0 if OK, -1 if any error.
 */
pexinit()
{
    char pathbuf[128 + 16];
    register n, f;
    char *newpath, *newshell;
    char *p;

    newshell = "/bin/sh";
    pcat(seta[S], ".path", pathbuf, sizeof pathbuf);
    if ((f = open(pathbuf, 0)) < 0)
        newpath = uid ? ":/bin:/usr/bin" : "/bin:/etc:/";
    else {
        n = read(f, pathbuf, sizeof pathbuf);
        close(f);
        if (n <= 0) {
            prs("cannot read .path");
            prs("\n");
            return (-1);
        }
        if (pexline(pathbuf, pathbuf + n, 128, &newpath, &p)
            || pexline(p, pathbuf + n, 16, &newshell, &p))
            return (-1);
    }
    seta[P] = rdval(0, 0, newpath);
    seta[Z] = rdval(0, 0, newshell);
    return (0);
}

/*
 *      pexline: scan for a line (if any) beginning at ptr,
 *      ending at ptrlim -1, for line up to psize bytes long.
 *      convert it to string, return beginning addr in pret
 *      and addr of next line in pnext
 *      return 0 if OK (or not present), -1 if runs off end in middle of line
 *      or if line too long.
 */
pexline(ptr, ptrlim, psize, pret, pnext)
    register char *ptr, *ptrlim;
    int psize;
    char **pret, **pnext;
{
    if (ptr >= ptrlim)
        return (0);
    *pret = ptr;
    if (ptrlim > ptr + psize)
        ptrlim = ptr + psize;
    for (; ptr < ptrlim; ptr++)
        if (*ptr == '\n') {
            *ptr++ = '\0';
            *pnext = ptr;
            return (0);
        }
    prs(".path too long");
    prs("\n");
    return (-1);
}

/*
 *      sh internal version of /etc/glob
 *      (from here thru function cat)
 *
 *      "*" in params matches r.e ".*"
 *      "?" in params matches r.e. "."
 *      "[...]" in params matches character class
 *      "[...a-z...]" in params matches a through z.
 *      if param does not contain "*", "[", or "?", use it as is
 *      if it does, find all files in current directory
 *      which match the param, sort them, and use them
 */

#define	STRSIZ	5300
char **avx INIT;
char *string INIT;
char *ablimit INIT;
int ncoll INIT;

etcglob(argv)
    char *argv[];
{
    char ab[STRSIZ];            /* generated characters */
    char *avxa[500 + DCOM];     /* generated arguments */

    avx = &avxa[DCOM];
    string = ab;
    ablimit = &ab[STRSIZ];
    *avx++ = *argv++;
    while (*argv)
        expand(*argv++);
    if (ncoll == 0)
        gdie("No match");
    texec(avxa[DCOM], avxa);
}

expand(as)
    char *as;
{
    register char *s, *cs;
    register int dirf;
    char **oavx;
    static struct
    {
        int ino;
        char name[16];
    } entry;

    s = cs = as;
    while (*cs != '*' && *cs != '?' && *cs != '[') {
        if (*cs++ == 0) {
            *avx++ = cat(s, "");
            return;
        }
    }
    for (;;) {
        if (cs == s) {
            dirf = open(".", 0);
            s = "";
            break;
        }
        if (*--cs == '/') {
            *cs = 0;
            dirf = open(s == cs ? "/" : s, 0);
            *cs++ = 0200;
            break;
        }
    }
    if (dirf < 0)
        gdie("No directory");
    oavx = avx;
    while (read(dirf, &entry, 16) == 16) {
        if (entry.ino == 0)
            continue;
        if (match(entry.name, cs)) {
            *avx++ = cat(s, entry.name);
            ncoll++;
        }
    }
    close(dirf);
    sort(oavx);
}

sort(oavx)
    char **oavx;
{
    register char **p1, **p2, **c;

    p1 = oavx;
    while (p1 < avx - 1) {
        p2 = p1;
        while (++p2 < avx) {
            if (compar(*p1, *p2) > 0) {
                c = *p1;
                *p1 = *p2;
                *p2 = c;
            }
        }
        p1++;
    }
}

match(s, p)
    char *s, *p;
{
    if (*s == '.' && *p != '.')
        return (0);
    return (amatch(s, p));
}

amatch(as, ap)
    char *as, *ap;
{
    register char *s, *p;
    register scc;
    int c, cc, ok, lc;

    s = as;
    p = ap;
    if (scc = *s++)
        if ((scc &= 0177) == 0)
            scc = 0200;
    switch (c = *p++) {

    case '[':
        ok = 0;
        lc = 077777;
        while (cc = *p++) {
            if (cc == ']') {
                if (ok)
                    return (amatch(s, p));
                else
                    return (0);
            } else if (cc == '-') {
                if (lc <= scc && scc <= (c = *p++))
                    ok++;
            } else if (scc == (lc = cc))
                ok++;
        }
        return (0);

    default:
        if (c != scc)
            return (0);

    case '?':
        if (scc)
            return (amatch(s, p));
        return (0);

    case '*':
        return (umatch(--s, p));

    case '\0':
        return (!scc);
    }
}

umatch(s, p)
    char *s, *p;
{
    if (*p == 0)
        return (1);
    while (*s)
        if (amatch(s++, p))
            return (1);
    return (0);
}

compar(as1, as2)
    char *as1, *as2;
{
    register char *s1, *s2;

    s1 = as1;
    s2 = as2;
    while (*s1++ == *s2)
        if (*s2++ == 0)
            return (0);
    return (*--s1 - *s2);
}

cat(as1, as2)
    char *as1, *as2;
{
    register char *s1, *s2;
    register int c;

    s2 = string;
    s1 = as1;
    while (c = *s1++) {
        if (s2 > ablimit)
            gdie(ARGLNG);
        c &= 0177;
        if (c == 0) {
            *s2++ = '/';
            break;
        }
        *s2++ = c;
    }
    s1 = as2;
    do {
        if (s2 > ablimit)
            gdie(ARGLNG);
        *s2++ = c = *s1++;
    } while (c);
    s1 = string;
    string = s2;
    return (s1);
}

/*
 *      gdie: terminate with error message, but no seek to EOF
 *      purpose is to make internal glob work same as external one.
 *      something should be done about match/nomatch actions.
 */
gdie(str)
    char *str;
{
    prs(ARG0);
    prs(": ");
    prs(str);
    prs("\n");
    exit(1);
}
