/*
 * far - floppy archiver
 *      CP/M - Micronix liason
 *
 *      Len Edmondson
 */

#include "far.h"

int cflag = NO, dflag = NO,    /* delete */
    pflag = NO,                 /* print */
    rflag = NO,                 /* replace */
    tflag = NO,                 /* table */
    xflag = NO,                 /* extract */
    verbose = NO,
	alt = NO,
	ddirty = NO,        /* the directory is dirty */
	complete[512] = { 0 }, 
	gmap[MAXGROUP] = { 0 };

char **files = NULL, 
	*device = NULL;

int fd = -1;

extern int errno;             /* the library's, libu's errno.s */

UINT nfiles, userno, drive;

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
    int ac;
    char **av;
{
    static char buf[32];
    static int *arg, sum;

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
    if (!fexists(device) && !strchr(device, '/') && strlen(device) < 16) {
        strcpy(buf, "/dev/");
        strcat(buf, device);
        device = buf;
    }
}

/*
 * produce an argument list from the file names in the current directory
 */
arglist()
{
    static int f;
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

    files = calloc(s.st_size1 / 16, sizeof *files);
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
    UINT ac;
    char **av;
{
    init(ac, av);
    far();
    finish();
    exit(YES);
}

finish()
{
    static UINT i;

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
    char *a;
{
    static int sum;

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
    char *a;
{
    write(STDOUT, a, strlen(a));
}

/*
 * the same on the error stream, which is where the Whitesmiths
 * putstr(fd, s, NULL, ...) wrote and where the diagnostics below want
 * to go whether or not anything is being extracted
 */
eput(a)
    char *a;
{
    write(STDERR, a, strlen(a));
}

/*
 * read or write 1 group
 *
 * c is READ or WRITE, not a function: every transfer below is one ioctl,
 * so the direction is a flag the controller's command block carries.
 */
gio(a, b, c)
    UINT a;
    char *b;
	int c;
{
    static UINT8 i, ret;

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
 *
 * One ioctl carries the sector, and what it carries is the controller's
 * own program for the transfer: where the data goes, which sector to read
 * or write, and a halt.  Nothing here seeks - a command block names a
 * place on the diskette, not an offset into a device - and the kernel
 * moves d->bps bytes between b and wherever the controller put them.
 *
 * The sector size is far's to state and is not always 512: the diskettes
 * far reads were written by CP/M, whose sectors are 128, 256, 512 or 1024
 * bytes, and the whole reason for the command block is that the block path
 * cannot ask for anything but the one size a Micronix filesystem is made
 * of.
 */
sio(a, b, c)
    UINT a;
    char *b;
	int c;                      /* READ or WRITE */
{
    static struct cdb req;
    static UINT s, t, n, hard, toff;

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
        eput("Unknown sector size\n");
        exit(NO);
    }

    n %= d->spt;                /* bring into the range [0, spt] */

    /*
     * t is a track number in the drive's physical order, and a
     * double-sided diskette's is side 0 first: side 0 of every cylinder,
     * then side 1.  So the low bit of t is the head and the rest is the
     * cylinder - true of the eight inch disks, whose tracks far numbers
     * that way already, and of the five inch ones, whose convolution
     * above has just doubled t for exactly this reason.
     */

    /*
     * The controller's numbering is not the medium's, and the two places
     * they part are what the driver used to apply to a block number on its
     * way out (sys/dj.c's sio).  It is far's to apply now, because far is
     * the one that knows the medium:
     *
     *	toff	the cylinders ahead of the ones CP/M's track 0 lands on.
     *		An eight inch diskette is a CP/M system diskette, and its
     *		first two tracks are the system's; the controller is
     *		told to count cylinders past them.  A five inch diskette
     *		with soft sectors keeps its reserve in the format table
     *		above, in d->offset, so it has none left over here - and
     *		the hard sectored five inch formats, which are the ones
     *		with no offset at all, do reserve two.
     *
     *	the sector origin.  Soft sectored media number their sectors
     *		from one and hard sectored media from zero, which is
     *		where the convolution above starts counting, so it is
     *		the hard sectored formats that need no correction.
     */

    hard = (d->inches == 5 && d->offset == 0);
    toff = (d->inches == 8 || hard) ? 2 : 0;

    if (!hard)
        n++;                    /* 1-origin sectors */

    /*
     * The three bytes of the DMA address are the one thing far cannot
     * write: buf is a virtual address and the controller wants a physical
     * one, so req.hole says which bytes they are and the driver fills them
     * in (include/sys/ioctl.h).
     */

    req.len = 11;               /* SETDMA, the transfer, HALT */
    req.hole = 1;               /* the address is the SETDMA operand */

    req.cmd[0] = SETDMA;
    req.cmd[1] = 0;             /* the driver writes the three address */
    req.cmd[2] = 0;             /* bytes here */
    req.cmd[3] = 0;

    req.cmd[4] = (c == READ) ? SREAD : SWRITE;
    req.cmd[5] = ((d->sides == 2) ? t >> 1 : t) + toff;
    req.cmd[6] = n | ((d->sides == 2 && (t & 1)) ? VERSO : 0);
    req.cmd[7] = drive;         /* which of the board's eight drives */
    req.cmd[8] = 0;             /* the transfer's own answer */

    req.cmd[9] = HALT;
    req.cmd[10] = 0;            /* ... and the halt's */

    req.count = d->bps;         /* the caller's sector size */
    req.flags = (c == READ) ? CDB_IN : 0;
    req.buf = b;

    /*
     * The transfer's own answer is in the block that comes back, at the end
     * of the SREAD or SWRITE command - which is where the controller left
     * it, and the reason the block is handed back at all.  An ioctl that
     * worked only says the program ran.
     */

    if (ioctl(fd, CDBCMD, &req) >= 0 && req.cmd[8] == OKSTAT)
        return YES;

    perror(device);             /* I/O didn't work */

    if (c == READ)
        memset(b, NOENT, d->bps);

    return NO;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */

