/*
 * far - floppy archiver
 *      CP/M - Micronix liason
 *
 *      Len Edmondson
 */

#include "far.h"

BOOL cflag = NO, dflag = NO,    /* delete */
    pflag = NO,                 /* print */
    rflag = NO,                 /* replace */
    tflag = NO,                 /* table */
    xflag = NO,                 /* extract */
    verbose = NO,
	alt = NO,
	ddirty = NO,        /* the directory is dirty */
	complete[512] = { 0 }, 
	gmap[MAXGROUP] = { 0 };

TEXT **files = NULL, 
	*device = NULL;

TINY fd = -1;

COUNT errno;                    /* external UNIX error number */

UCOUNT nfiles = 0, userno = 0;

struct fcb thedir[MAXGDIR * GENT] = { 0 };

struct disk *d = NULL;

struct disk disk[] = {
    /*
     * bpv, in., sides, spt, bps, ngrp, bpg, spg, epg, epd, gdir, bpe, epe,
     * npnt 
     */
    {2400, 8, 2, 8, 1024, 600, 2048, 2, 64, 256, 4, K16, 1, 8, 0},
    {1200, 8, 1, 8, 1024, 300, 2048, 2, 64, 128, 2, K16, 1, 8, 0},

    {2250, 8, 2, 15, 512, 562, 2048, 4, 64, 256, 4, K16, 1, 8, 0},
    {1125, 8, 1, 15, 512, 281, 2048, 4, 64, 128, 2, K16, 1, 8, 0},

    {1950, 8, 2, 26, 256, 487, 2048, 8, 64, 256, 4, K16, 1, 8, 0},
    {975, 8, 1, 26, 256, 243, 2048, 8, 64, 128, 2, K32, 2, 16, 0},

    {975, 8, 2, 26, 128, 243, 2048, 16, 64, 128, 2, K32, 2, 16, 0},
    {487, 8, 1, 26, 128, 243, 1024, 8, 32, 64, 2, K16, 1, 16, 0},

    {760, 5, 2, 10, 512, 165, 2048, 4, 64, 64, 1, K32, 2, 16, 0},
    {660, 5, 2, 10, 512, 165, 2048, 4, 64, 64, 1, K32, 2, 16, 0},

    {380, 5, 1, 10, 512, 82, 2048, 4, 64, 64, 1, K32, 2, 16, 0},

    {330, 5, 1, 10, 512, 82, 2048, 4, 64, 64, 1, K32, 2, 16, 0},        /* 2.x */
    {330, 5, 1, 10, 512, 165, 1024, 2, 64, 64, 2, K16, 1, 16, 0},       /* 1.4 */

    {160, 5, 1, 10, 256, 80, 1024, 4, 32, 64, 2, K16, 1, 16, 0},

    /* Morrow Designs micro decision single sided */
    {400, 5, 1, 5, 1024, 93, 2048, 2, 64, 128, 2, K32, 2, 16, 10},

    /* Osborne double density */
    {400, 5, 1, 5, 1024, 185, 1024, 1, 32, 64, 2, K16, 1, 16, 15},
    {800, 5, 2, 5, 1024, 192, 2048, 2, 64, 192, 3, K32, 2, 16, 10},

    /* osb */ 
	{200, 5, 1, 10, 256, 46, 2048, 8, 64, 64, 1, K32, 2, 16, 30},

    /* ibm */ 
	{320, 5, 1, 8, 512, 156, 1024, 2, 32, 64, 2, K16, 1, 16, 8},
    /* xer */ 
	{180, 5, 1, 18, 128, 83, 1024, 8, 32, 32, 1, K16, 1, 16, 54},

    /*
     * bpv, in., sides, spt, bps, ngrp, bpg, spg, epg, epd, gdir, bpe, epe,
     * npnt 
     */

    {0}
};

init(ac, av)
    FAST COUNT ac;
    FAST TEXT **av;
{
    INTERN TEXT buf[32];
    INTERN *arg, sum;

    if (ac == 1)
        inter();                /* interactive questioning */

    if (ac > 1)
        device = av[1];

    if (ac > 2)
        doflag(av[2]);          /* flags */

    if (ac > 3) {
        files = av + 3;         /* file list */
        nfiles = ac - 3;
    }

    sum = dflag + pflag + rflag + tflag + xflag;

    if (sum == 0) {
        tflag = YES;            /* default flag */
        verbose = YES;
    }

    else if (sum > 1) {
        atmost();
    }

    arglist();

    duptest();

    /*
     * if the device name was given as simple file name
     * see if prepending /dev/ will make things better
     */
    if (!fexists(device) && !element('/', device) && lenstr(device) < 16) {
        cpystr(buf, "/dev/", device, NULL);
        device = buf;
    }
}

/*
 * produce an argument list from the file names in the current directory
 */
arglist()
{
    extern char **files;
    extern unsigned nfiles;
    static f;
    static struct stat s;

    struct dir {
        int number;
        char name[16];
    };

    static struct dir d;

    if (rflag == NO || nfiles > 0)
        return;

    f = open(".", READ);

    if (f < 0)
        return;

    fstat(f, &s);               /* measure the directory */

    files = calloc(s.size1 / 16, sizeof *files);
    nfiles = 0;

    for (;;) {
        if (read(f, &d, 16) != 16)
            break;

        if (d.number == 0 || *d.name == '.')
            continue;

        files[nfiles++] = save(d.name);
    }

    close(f);
}

main(ac, av)
    FAST UCOUNT ac;
    FAST TEXT **av;
{
    init(ac, av);
    far();
    finish();
    exit(YES);
}

finish()
{
    INTERN UCOUNT i;

    if (!rflag) {
        for (i = 0; i < nfiles; i++) {
            if (complete[i] == NO) {
                put(files[i]);
                put(": file not on floppy diskette.\n");
            }
        }
    }

    if (verbose)
        space();                /* report space status */
}

doflag(a)
    FAST TEXT *a;
{
    INTERN sum;

    for (; *a; a++) {
        *a = tolower(*a);

        switch (*a) {
        case 'c':
            cflag = YES;
            break;
        case 'd':
            dflag = YES;
            break;
        case 'p':
            pflag = YES;
            break;
        case 'r':
            rflag = YES;
            break;
        case 't':
            tflag = YES;
            break;
        case 'x':
            xflag = YES;
            break;
        case 'v':
            verbose = YES;
            break;

        case 'u':              /* user number */
            {
                char c;

                for (;;) {
                    c = a[1];

                    if (!isdigit(c))
                        break;

                    userno *= 10;
                    userno += c - '0';
                    a++;
                }

                break;
            }

        case '-':
            break;
        default:
            usage();
            break;
        }
    }
}

atmost()
{
    put("At most one of d, r, t, x may be specified.\n");
    exit(NO);
}

put(a)
    FAST TEXT *a;
{
    write(STDOUT, a, lenstr(a));
}

/*
 * read or write 1 group
 */
gio(a, b, c)
    FAST UCOUNT a;
    FAST TEXT *b;
	FAST(*c) ();
{
    INTERN UTINY i, ret;

    ret = YES;

    a *= d->spg;                /* first sector no. of group */

    for (i = 0; i < d->spg; i++) {      /* for each sector in the group */
        if (!sio(a, b, c))
            ret = NO;

        a++;
        b += d->bps;
    }

    return ret;
}

/*
 * sio - sector I/O
 *      read or write 1 sector
 */
sio(a, b, c)
    FAST UCOUNT a;
    FAST TEXT *b;
	FAST(*c) ();
{
    INTERN UCOUNT s, t, n;
    INTERN ULONG o;

    a += d->offset;

    s = a % d->spt;             /* sector no. on the track */
    t = a / d->spt;             /* track no. */

    /*
     * CP/M convolution
     */
    if (d->inches == 5) {       /* 5 1/4 in. diskette */
        /*
         * the tracks are kind of strange too
         */
        if (d->sides == 2 && d->offset == 0) {  /* double sided */
            if (t < 33)         /* Northstar */
                t *= 2;
            else
                t = (65 - t) * 2 + 1;
        }

        if (d->spt == 5) {

            if (d->bpg == 2048) {       /* Morrow */
                if (alt) {
                    n = s * 4;
                } else {
                    n = s * 3;
                }
            } else {
                if (alt) {      /* Osborne double density */
                    n = s * 3;
                } else {
                    n = s;
                }
            }
        }

        else if (d->bpv == 200) {       /* Osborne */
            if (alt)
                n = s;
            else
                n = s * 2 + s / 5;
        }

        else if (d->bpv == 320) {       /* IBM */
            if (alt)
                n = s * 4 + s / 2;
            else
                n = s;
        }

        else if (d->bpv == 180) {       /* Xerox */
            if (alt) {
                static unsigned char tab[] =
                    { 0, 11, 5, 16, 1, 12, 6, 17, 2, 13, 7, 9, 3, 14, 8, 10,
                        4, 15 };

                n = tab[s];
            } else {
                n = s * 5;
            }
        }

        else if (d->bps == 256) {
            if (alt)
                n = s * 5 + s / 2;
            else
                n = s;
        } else {
            if (alt) {
                static unsigned char tab[] = { 0, 7, 5, 3, 1, 8, 6, 4, 2, 9 };

                n = tab[s];
            } else {
                n = (s * 5) + (s / 2);
            }
        }
    }

    else if (d->bps == 128) {
        if (alt) {
            static unsigned char tab[] =
                /*
                 * {0,19,17,11,15,9,13,7,5,24,3,22,1,20,18,12,16,10,14,8,6,25,4,23,2,21};
                 */
            { 0, 3, 6, 9, 12, 2, 5, 8, 11, 1, 4, 7, 10, 13, 16, 19, 22, 25,
                    15, 18, 21, 24, 14, 17, 20, 23 };
            n = tab[s];
        } else {
            n = s * 6 + s / 13;
        }
    }

    else if (d->bps == 256) {
        if (alt) {
            static unsigned char tab[] =
                { 0, 17, 9, 13, 5, 22, 1, 18, 10, 14, 6, 23, 2, 19, 11, 15, 7,
                    24, 3, 20, 12, 16, 8, 25, 4, 21 };

            n = tab[s];
        } else {
            /*
             * n = s * 3; 
             */
            n = ((s % 3) * 9) + (s / 3);
        }
    }

    else if (d->bps == 512) {
        if (alt)
            n = s * 2;
        else
            n = s * 4;
    }

    else if (d->bps == 1024) {
        if (alt) {
            static unsigned char tab[] = { 0, 5, 3, 4, 2, 7, 1, 6 };

            n = tab[s];
        } else {
            n = s * 3;
        }
    } else {
        putstr(STDERR, "Unknown sector size\n", NULL);
        exit(NO);
    }

    n %= d->spt;                /* bring into the range [0, spt] */

    n += t * d->spt;            /* now n is the real sector number */

    o = n;                      /* offset in sectors */

    o *= d->bps;                /* o is the offset in bytes */

    lseek(fd, o);               /* seek to the spot */

    if ((*c) (fd, b, d->bps) == d->bps) {
        return YES;
    } else {                    /* I/O didn't work */
        perror(device);

        if (c == read)
            fill(b, d->bps, NOENT);

        return NO;
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */

