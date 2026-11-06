/*
 * format - one formatter for every disk on the machine
 *
 * cmd/format/format.c
 *
 *	format [-m model] [-c cyl] [-h heads] [-s spt]
 *	       [-P precomp] [-l lowcur] [-d step] [-k skew]
 *	       [-p] [-t track] [-e head] device
 *
 * The controller is not named on the command line: the whole-disk device
 * node says which it is, in the high byte of its device number, and the
 * geometry says the rest.  Each controller has its own backend and its
 * own set of options; what they share is here - the device checks, the
 * label, and the prototype slice table a fresh drive gets.
 *
 * A device with no whole-disk slice has no formatter: block 0 of a slice
 * is somewhere inside a filesystem, and the label belongs at physical
 * cylinder 0 head 0 sector 0.  The floppy is the exception, because its
 * formatter writes tracks and never a label.
 *
 * See cmd/label/label.c for the labeler, docs/DISKLABEL.md for the
 * label itself, and include/sys/ioctl.h for the raw command interface
 * the backends drive their controllers through.
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/dlabel.h>
#include "format.h"

#define BLOCK	512		/* the sector every one of these drivers moves */

char *pname = "format";
char *devname;
int devfd;
int drive;
int major;
int opts;

UINT ocyl, oheads, ospt;
UINT oprecomp, olowcur, ostep, oskew;
UINT otrack, ohead;
char *omodel;

UINT cyl, heads, spt;
UINT precomp, lowcur, stepdelay, skew;
char *mfg, *model;

UINT lcyl, lheads, lspt;

static char secbuf[BLOCK];
static struct dlabel *lab;
static struct stat sbuf;
static UINT minor;

/*
 * The option letters, so a backend that refuses one can say which.  The
 * order is the order they are listed in the usage.
 */
struct opname {
    int bit;
    int letter;
};

static struct opname optnames[] = {
    { O_MODEL,   'm' },
    { O_CYL,     'c' },
    { O_HEADS,   'h' },
    { O_SPT,     's' },
    { O_PRECOMP, 'P' },
    { O_LOWCUR,  'l' },
    { O_STEP,    'd' },
    { O_SKEW,    'k' },
    { O_PROBE,   'p' },
    { O_TRACK,   't' },
    { O_HEAD,    'e' },
};

#define	NOPTNAME (sizeof optnames / sizeof optnames[0])

void
die(s)
    char *s;
{
    fprintf(stderr, "%s: %s\n", pname, s);
    exit(1);
}

/*
 * A decimal number, or -1 if the argument is not one.  atoi cannot tell
 * "0" from "not a number", and 0 is a legal cylinder, head and track.
 */
int
num(s)
    char *s;
{
    char *p;

    if (*s == 0)
        return (-1);
    for (p = s; *p; p++)
        if (*p < '0' || *p > '9')
            return (-1);
    return (atoi(s));
}

static
usage()
{
    fprintf(stderr, "usage: %s [-m model] [-c cyl] [-h heads] [-s spt]\n", pname);
    fprintf(stderr, "       [-P precomp] [-l lowcur] [-d step] [-k skew]\n");
    fprintf(stderr, "       [-p] [-t track] [-e head] device\n");
    exit(1);
}

/*
 * Refuse the options this controller has no use for.  allowed is the
 * set it does understand; a bit that was seen and is not in it is a
 * mistake about which drive is being formatted, and a formatter that
 * ignored it would lay down a shape nobody asked for.
 */
static
badopts(allowed, what)
    int allowed;
    char *what;
{
    int i;

    for (i = 0; i < NOPTNAME; i++)
        if ((opts & optnames[i].bit) && !(allowed & optnames[i].bit)) {
            fprintf(stderr, "%s: -%c is not a %s option\n",
                pname, optnames[i].letter, what);
            exit(1);
        }
}

/*
 * The whole-disk slice and no other: block 0 of it is physical cylinder
 * 0 head 0 sector 0.  cmd/label/label.c makes the same check for the
 * same reason.
 */
static
whole()
{
    if (((minor >> 5) & 7) != DL_WHOLE)
        die("that node is not the whole-disk slice 'c'");
}

/*
 * The label in block 0, if the disk has one that says its geometry.
 * The answer goes in lcyl, lheads and lspt; the caller decides whether
 * to believe it.
 */
int
getlabel()
{
    int i;

    if (lseek(devfd, 0L, 0) < 0)
        die("cannot seek to block 0");
    if (read(devfd, secbuf, BLOCK) != BLOCK)
        die("cannot read block 0");

    lab = (struct dlabel *) &secbuf[DL_OFFSET];
    for (i = 0; i < 4; i++)
        if (lab->d_magic[i] != DL_MAGIC[i])
            return (0);
    if (lab->d_tracks == 0 || lab->d_heads == 0 || lab->d_spt == 0)
        return (0);

    lcyl = lab->d_tracks;
    lheads = lab->d_heads;
    lspt = lab->d_spt;
    return (1);
}

/*
 * Build the prototype slice table a freshly formatted disk carries.
 *
 * The default layout, from cylinder 0 up:
 *
 *	track 0		the boot and this label, not a filesystem
 *	a		the biggest filesystem that fits, up to the most a
 *			block number can say - s_fsize is a 16-bit UINT, so
 *			65535 blocks is the ceiling (32M, give or take)
 *	b		1M (2048 blocks) of swap, after a
 *	d, e, f, g, h	32M apiece until the drive runs out; the last
 *			one is a runt
 *
 * 'c' is the whole disk (DL_WHOLE), placed by nobody and never an entry
 * here.  Every length is a whole number of cylinders, because a slice's
 * position is in cylinders and never in blocks - a block number is 16
 * bits and a drive's worth of them is not.
 */
void
protoslices(lp)
    struct dlabel *lp;
{
    UINT spc, off, left, maxfs, bsz, sz;
    int i;

    spc = heads * spt;
    off = 1;                    /* cylinder 0: the boot + label */
    left = cyl - 1;

    maxfs = 0xffff / spc;

    bsz = (2048 + spc - 1) / spc;
    if (bsz > left)
        bsz = left;

    if (left > bsz) {
        sz = left - bsz;
        if (maxfs < sz)
            sz = maxfs;
        lp->d_slice[0].d_off = off;
        lp->d_slice[0].d_len = sz;
        lp->d_slice[0].d_type = SL_FS;
        off += sz;
        left -= sz;
    }

    if (bsz) {
        lp->d_slice[1].d_off = off;
        lp->d_slice[1].d_len = bsz;
        lp->d_slice[1].d_type = SL_SWAP;
        off += bsz;
        left -= bsz;
    }

    for (i = 3; i < NSLICE; i++) {
        if (left == 0)
            break;
        if (maxfs < left)
            sz = maxfs;
        else
            sz = left;
        lp->d_slice[i].d_off = off;
        lp->d_slice[i].d_len = sz;
        lp->d_slice[i].d_type = SL_FS;
        off += sz;
        left -= sz;
    }
}

/*
 * Does the label carry a partition table at all?  Zero in every extent
 * is a drive described but not partitioned, and a formatter has to give
 * it the prototype table; a carved drive keeps the table it has when
 * the format did not blank the data fields.
 */
static
carved(lp)
    struct dlabel *lp;
{
    int i;

    for (i = 0; i < NSLICE; i++)
        if (i != DL_WHOLE && lp->d_slice[i].d_len)
            return (1);
    return (0);
}

static
islabel(lp)
    struct dlabel *lp;
{
    int i;

    for (i = 0; i < 4; i++)
        if (lp->d_magic[i] != DL_MAGIC[i])
            return (0);
    return (lp->d_tracks && lp->d_heads && lp->d_spt);
}

/*
 * Write the label: what the drive is, at DL_OFFSET in block 0.
 *
 * firstsec is the controller's numbering, 1 for the HDCA and 0 for
 * everyone else.  timing says whether the drive's step, seek, precomp
 * and write-current numbers belong in the label - the HD-DMA commands
 * them and the other controllers do not.
 *
 * keep says whether the data fields were blanked.  A low-level format
 * that writes data has destroyed the old label and there is nothing to
 * keep; a format that writes only headers, or a card with no format at
 * all, leaves the old table where it was, and a carved drive keeps it.
 * A fresh label gets the prototype table either way.
 *
 * It is written through the block device rather than as a raw command,
 * so that the sector in the cache and the sector on the disk are one
 * object.  mwformat's comment says what happens otherwise.
 */
void
putlabel(firstsec, timing, keep)
    int firstsec, timing, keep;
{
    int i, fresh;

    if (lseek(devfd, 0L, 0) < 0)
        die("cannot seek to block 0");
    if (read(devfd, secbuf, BLOCK) != BLOCK)
        die("cannot read block 0");

    lab = (struct dlabel *) &secbuf[DL_OFFSET];
    fresh = 1;
    if (keep && islabel(lab) && carved(lab))
        fresh = 0;

    if (fresh) {
        memset((char *) lab, 0, sizeof (struct dlabel));
        for (i = 0; i < 4; i++)
            lab->d_magic[i] = DL_MAGIC[i];
        lab->d_version = DL_VERSION;
        protoslices(lab);
    }

    lab->d_tracks = cyl;
    lab->d_heads = heads;
    lab->d_spt = spt;
    lab->d_firstsec = firstsec;

    if (timing) {
        lab->d_step = stepdelay;
        lab->d_seek = 0;
        lab->d_precomp = precomp;
        lab->d_lowcur = lowcur;
    }

    if (mfg)
        strncpy(lab->d_mfg, mfg, sizeof (lab->d_mfg));
    if (model)
        strncpy(lab->d_model, model, sizeof (lab->d_model));

    if (lseek(devfd, 0L, 0) < 0)
        die("cannot seek to block 0");
    if (write(devfd, secbuf, BLOCK) != BLOCK)
        die("cannot write block 0");
}

main(argc, argv)
    int argc;
    char **argv;
{
    char *arg;
    int i, n;

    pname = argv[0];
    devname = 0;
    omodel = 0;
    opts = 0;
    ocyl = oheads = ospt = 0;
    oprecomp = olowcur = ostep = oskew = 0;
    otrack = ohead = 0;

    for (i = 1; i < argc; i++) {
        arg = argv[i];
        if (arg[0] != '-') {
            if (devname) {
                /*
                 * djformat's old shape, "device track": a second bare
                 * number is the one track to format.  -t says the same
                 * thing and is the only form the fixed disks take.
                 */
                if ((n = num(arg)) >= 0 && !(opts & O_TRACK)) {
                    otrack = n;
                    opts |= O_TRACK;
                    continue;
                }
                usage();
            }
            devname = arg;
            continue;
        }
        if (arg[1] == 0 || arg[2] != 0)
            usage();

        if (arg[1] == 'p') {
            opts |= O_PROBE;
            continue;
        }
        if (arg[1] == 'm') {
            if (++i == argc)
                usage();
            omodel = argv[i];
            opts |= O_MODEL;
            continue;
        }

        if (++i == argc)
            usage();
        if ((n = num(argv[i])) < 0)
            usage();
        switch (arg[1]) {
        case 'c':
            ocyl = n;
            opts |= O_CYL;
            break;
        case 'h':
            oheads = n;
            opts |= O_HEADS;
            break;
        case 's':
            ospt = n;
            opts |= O_SPT;
            break;
        case 'P':
            oprecomp = n;
            opts |= O_PRECOMP;
            break;
        case 'l':
            olowcur = n;
            opts |= O_LOWCUR;
            break;
        case 'd':
            ostep = n;
            opts |= O_STEP;
            break;
        case 'k':
            oskew = n;
            opts |= O_SKEW;
            break;
        case 't':
            otrack = n;
            opts |= O_TRACK;
            break;
        case 'e':
            ohead = n;
            opts |= O_HEAD;
            break;
        default:
            usage();
        }
    }
    if (!devname)
        usage();

    if (stat(devname, &sbuf) < 0)
        die("cannot stat the device");
    if ((sbuf.st_mode & IFMT) != IFBLK)
        die("not a block special file");

    major = sbuf.st_addr[0] >> 8;
    minor = sbuf.st_addr[0] & 0xff;

    /*
     * The floppy's formatter writes tracks and needs no label, so it is
     * the one that does not have to be handed the whole disk.  Every
     * other driver's block 0 is only findable on slice 'c'.
     */
    if (major != DJMAJ)
        whole();

    if ((devfd = open(devname, 2)) < 0)
        die("cannot open the device for writing");

    switch (major) {
    case HDCAMAJ:
        drive = minor & 3;
        badopts(O_MODEL | O_CYL | O_HEADS | O_SPT | O_PROBE, "HDCA");
        hdca_main();
        break;
    case MWMAJ:
        drive = minor & 3;
        badopts(O_MODEL | O_CYL | O_HEADS | O_SPT | O_PRECOMP | O_LOWCUR |
            O_STEP | O_SKEW | O_TRACK | O_HEAD, "HD-DMA");
        mw_main();
        break;
    case NCRMAJ:
        drive = minor & 7;
        badopts(O_CYL | O_HEADS | O_SPT, "SCSI");
        scsi_main();
        break;
    case IDEMAJ:
        drive = minor & 1;
        badopts(O_CYL | O_HEADS | O_SPT, "IDE");
        ide_main();
        break;
    case DJMAJ:
        drive = minor & 3;
        badopts(O_TRACK, "DJ-DMA");
        dj_main();
        break;
    default:
        die("not a disk this knows how to format");
    }
    exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
