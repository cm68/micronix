/*
 * the HD-DMA backend: the Morrow disk controller
 *
 * cmd/format/mw.c
 *
 * The drive's controller work is the vendor's formatmw, which ran under
 * CP/M off the boot ROM (stand/formatmw/formatmw.c): load the constants,
 * recalibrate, then a step and a format per track and head.  What is not
 * here is the rest of that program - the verify pass, the data patterns,
 * the bad sector map - all of which are more command blocks on the same
 * interface and none of which a disk needs to be mountable.
 *
 * The geometry comes from a model name, or the disk's own label, or the
 * flags, in that order.  The command block is include/sys/mw.h's
 * sixteen bytes: the first twelve are the command through its opcode,
 * the thirteenth is the status the board answers with, and the last
 * three are the address of the next command, which only the driver can
 * fill.
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mw.h>
#include <sys/dlabel.h>
#include "format.h"

#define	CBKLEN	12		/* the part of the block a caller fills */
#define	CBKDMA	4		/* and where in it the data address belongs */

/*
 * The drive models this machine reaches, dev/devlist's m5, m10, m16, m32
 * and m40 nodes, with the timing that used to be sys/mw.c's specs[]
 * table.  A name is a shorthand for a geometry and nothing more: the
 * driver reads none of this, and -c, -h and the rest override any of it.
 *
 * bpt is the bytes one track holds when it is full, which is what fixes
 * the sector count: formatmw does not store the count, it works it out
 * from the track and the size of a sector, and so does this.
 */
struct model {
    char *name;
    char *mfg;                  /* manufacturer, into the label */
    char *model;                /* model number, into the label */
    UINT cyl;
    UINT8 heads;
    UINT precomp;
    UINT lowcur;
    UINT8 stpdel;
    UINT bpt;
};

static struct model models[] = {
    {"m5",  "Seagate", "ST-506", 153, 4, 0xffff, 128, 30, 10416},
    {"m10", "Seagate", "ST-412", 306, 4, 0xffff, 128,  2, 10416},
    {"m16", "CMI",     "",       306, 6, 128, 128,  2, 10416},
    {"m32", "CMI",     "",       640, 6, 256, 256,  0, 10416},
    {"m40", "Quantum", "Q540",   512, 8, 256, 512, 30, 10416},
    /* Maxtor XT MFM drives */
    {"xt1065", "Maxtor", "XT-1065", 918, 7, 0xffff, 0xffff, 30, 10416},
    {"xt1085", "Maxtor", "XT-1085", 1024, 8, 1025, 0xffff, 30, 10416},
    {"xt1105", "Maxtor", "XT-1105", 918, 11, 0xffff, 0xffff, 30, 10416},
    {"xt1140", "Maxtor", "XT-1140", 918, 15, 0xffff, 0xffff, 30, 10416},
    {"xt2085", "Maxtor", "XT-2085", 1224, 7, 1225, 0xffff, 30, 10416},
    {"xt2140", "Maxtor", "XT-2140", 1224, 11, 1225, 0xffff, 30, 10416},
    {"xt2190", "Maxtor", "XT-2190", 1224, 15, 1225, 0xffff, 30, 10416},
    /* Seagate MFM drives */
    {"st225", "Seagate", "ST-225", 615, 4, 256, 0xffff, 30, 10416},
    {"st251", "Seagate", "ST-251", 820, 6, 0xffff, 0xffff, 30, 10416},
    {"st4038", "Seagate", "ST-4038", 733, 5, 300, 0xffff, 30, 10416},
    {"st4051", "Seagate", "ST-4051", 977, 5, 0xffff, 0xffff, 30, 10416},
    {"st4053", "Seagate", "ST-4053", 1024, 5, 0xffff, 0xffff, 30, 10416},
    {"st4096", "Seagate", "ST-4096", 1024, 9, 0xffff, 0xffff, 30, 10416},
    /* MiniScribe MFM drives */
    {"ms8425", "MiniScribe", "8425", 615, 4, 128, 0xffff, 30, 10416},
    {"ms6053", "MiniScribe", "6053", 1024, 5, 512, 0xffff, 30, 10416},
    {"ms6085", "MiniScribe", "6085", 1024, 8, 512, 0xffff, 30, 10416},
    /* Tandon MFM drives */
    {"tm501", "Tandon", "TM-501", 306, 2, 128, 0xffff, 30, 10416},
    {"tm502", "Tandon", "TM-502", 306, 4, 128, 0xffff, 30, 10416},
    {"tm503", "Tandon", "TM-503", 306, 6, 128, 0xffff, 30, 10416},
    {"tm703", "Tandon", "TM-703", 733, 5, 0xffff, 0xffff, 30, 10416},
    {"tm755", "Tandon", "TM-755", 981, 5, 0xffff, 0xffff, 30, 10416},
    {"tm362", "Tandon", "TM-362", 615, 4, 128, 0xffff, 30, 10416},
    /* Quantum MFM drives */
    {"q520", "Quantum", "Q520", 512, 4, 256, 0xffff, 30, 10416},
    {"q530", "Quantum", "Q530", 512, 6, 256, 0xffff, 30, 10416},
    /* CDC Wren MFM drives */
    {"wren36", "CDC", "9415-36", 697, 5, 0xffff, 0xffff, 30, 10416},
    {"wren48", "CDC", "94155-48", 925, 5, 128, 0xffff, 30, 10416},
    {"wren51", "CDC", "94155-51", 989, 5, 128, 0xffff, 30, 10416},
    {"wren57", "CDC", "94155-57", 925, 6, 128, 0xffff, 30, 10416},
    {"wren67", "CDC", "94155-67", 925, 7, 128, 0xffff, 30, 10416},
    {"wren77", "CDC", "94155-77", 925, 8, 128, 0xffff, 30, 10416},
    {"wren85", "CDC", "94155-85", 1024, 8, 128, 0xffff, 30, 10416},
    {"wren86", "CDC", "94155-86", 925, 9, 128, 0xffff, 30, 10416},
    {"wren96", "CDC", "94155-96", 1024, 9, 128, 0xffff, 30, 10416},
    /* Micropolis MFM drives */
    {"mp1304", "Micropolis", "1304", 830, 6, 0xffff, 0xffff, 30, 10416},
    {"mp1323", "Micropolis", "1323", 1024, 4, 0xffff, 0xffff, 30, 10416},
    {"mp1323a", "Micropolis", "1323A", 1024, 5, 0xffff, 0xffff, 30, 10416},
    {"mp1325", "Micropolis", "1325", 1024, 8, 0xffff, 0xffff, 30, 10416},
    {"mp1335", "Micropolis", "1335", 1024, 8, 0xffff, 0xffff, 30, 10416},
};

#define	NMODEL (sizeof models / sizeof models[0])

/*
 * The shape of a track.  A track is a gap, then one sync field, sector
 * header and gap for each sector, then a last gap for speed variations -
 * and the sector count is whatever fits in the bytes the drive holds, so
 * these add up to the same 10416 bytes on every one of these drives.
 */
#define	DRVSIZE	512
#define	GAP3	43
#define	SYNC	13
#define	IDF	7
#define	GAP2	16
#define	DATAHDR	4
#define	GAP1	16
#define	GAP4	208
#define	FILL	0xe5
#define	DSKEW	3

#define	SELCONST 0x3c
#define	ALLSTEPS 0xffff

/*
 * The sector-header table the format command points the board at: four
 * bytes an entry - cylinder low, cylinder high, head, sector - for the
 * sectors of one track, in the order they are laid down.
 */
static char image[DRVSIZE];

static char *statuses[] = {
    "controller busy",
    "drive not ready",
    "wrong cylinder",
    "wrong head",
    "header not found",
    "data header not found",
    "data overrun",
    "data CRC error",
    "write fault",
    "header CRC error",
};

#define	BADCMD	0xa0

static UINT first, last;
static UINT fhead, lhead;
static UINT curtrk;

/*
 * The name of a status byte, or 0 when there is no name for it.
 */
static char *
stname(st)
    int st;
{
    if (st >= 0 && st < sizeof statuses / sizeof statuses[0])
        return (statuses[st]);
    if (st == BADCMD)
        return ("illegal command");
    return (0);
}

/*
 * How many sectors fit on a track: the bytes the track holds, less the
 * gaps that are not per-sector, divided by what one sector costs.
 */
static UINT
nsectors(bpt)
    UINT bpt;
{
    return ((bpt - (GAP1 + GAP4)) /
        (SYNC + IDF + GAP2 + DATAHDR + DRVSIZE + GAP3));
}

/*
 * Run one command block, and return the status the board answered with,
 * or -1 if the ioctl itself failed.
 */
static int
run(nb, count, buf)
    struct hddma_cmd *nb;
    UINT count;
    char *buf;
{
    struct cdb r;
    int st;
    r.addrbits = 0;

    memset((char *) &r, 0, sizeof r);
    memcpy((char *) &r.cmd[0], (char *) nb, sizeof (struct hddma_cmd));
    r.len = CBKLEN;
    r.count = count;
    r.flags = 0;
    r.buf = (UINT8 *) buf;
    r.hole = count ? CBKDMA : -1;

    if (ioctl(devfd, CDBCMD, &r) < 0)
        return (-1);

    st = r.cmd[CBKLEN] & 0377;
    return (st);
}

static
check(st, what)
    int st;
    char *what;
{
    char *s;

    if (st == OK)
        return;
    if (st < 0)
        die("the driver refused the command");
    if ((s = stname(st)) != 0)
        fprintf(stderr, "%s: %s: %s\n", pname, what, s);
    else
        fprintf(stderr, "%s: %s: status %x\n", pname, what, st);
    exit(1);
}

/*
 * Load the drive's constants: the step rate, the head settle time and
 * the sector size code, with the interrupt bit clear, because every
 * command here is polled.
 */
static
load()
{
    struct hddma_cmd nb;

    memset((char *) &nb, 0, sizeof nb);
    nb.drvsel = drive;
    nb.headsel = drive | SELCONST;
    nb.arg0.byte.high = stepdelay;
    nb.byte2 = SETTLE;
    nb.byte3 = SEC512;
    nb.opcode = OP_LOAD;
    check(run(&nb, 0, 0), "load constants");
}

/*
 * Step out further than the drive is long, which every drive answers by
 * stopping at track zero.
 */
static
recal()
{
    struct hddma_cmd nb;

    memset((char *) &nb, 0, sizeof nb);
    nb.drvsel = drive | STEPOUT;
    nb.headsel = drive | SELCONST;
    nb.steps = ALLSTEPS;
    nb.opcode = OP_NOP;
    check(run(&nb, 0, 0), "recalibrate");
    curtrk = 0;
}

/*
 * Step the head to a track by the difference from where it is.
 */
static
seekto(track)
    UINT track;
{
    struct hddma_cmd nb;

    if (track == curtrk)
        return;
    memset((char *) &nb, 0, sizeof nb);
    nb.drvsel = drive;
    if (track < curtrk)
        nb.drvsel |= STEPOUT;
    nb.headsel = drive;
    nb.steps = (track > curtrk) ? track - curtrk : curtrk - track;
    nb.opcode = OP_NOP;
    check(run(&nb, 0, 0), "seek");
    curtrk = track;
}

/*
 * Format one track of one head.  The three argument bytes are all
 * complemented because the board counts up to overflow rather than down
 * to zero, and the fourth is the byte every data field is filled with.
 *
 * The select byte holds three bits of head; a drive with more than eight
 * heads uses the low-current line as head select line 4, so that bit goes
 * in bit 6.  Heads 0 through 7 leave that line high, which is the same
 * as high write current, so one encoding serves both.
 */
static
fmthead(track, head)
    UINT track, head;
{
    struct hddma_cmd nb;

    memset((char *) &nb, 0, sizeof nb);
    nb.drvsel = drive;
    nb.headsel = drive | ((~head & 7) << 2);
    if (~head & 8)
        nb.headsel |= HIGHCUR;
    if (track >= precomp)
        nb.headsel |= PRECOMP;
    if (heads <= 8 && track >= lowcur)
        nb.headsel &= ~HIGHCUR;
    nb.gap3 = ~(GAP3 - 1);
    nb.sptneg = ~spt;
    nb.fseccode = ~(DRVSIZE / 128 - 1);
    nb.fill = FILL;
    nb.opcode = OP_FORMAT;
    check(run(&nb, spt * 4, image), "format");
}

/*
 * Number the sectors of a track, once: the table says which sector
 * number goes where, and the interleave is the order they are written.
 */
static
builds()
{
    int i, j;

    for (i = 0; i < spt; i++)
        image[i * 4 + 3] = spt + 1;
    for (i = j = 0; j < spt; i = (i + skew) % spt) {
        while (image[i * 4 + 3] <= spt)
            i = (i + 1) % spt;
        image[i * 4 + 3] = j++;
    }
}

static
buildth(track, head)
    UINT track, head;
{
    int i;

    for (i = 0; i < spt; i++) {
        image[i * 4] = track & 0377;
        image[i * 4 + 1] = (track >> 8) & 0377;
        image[i * 4 + 2] = head;
    }
}

/*
 * The model row whose shape this is, or 0 for a drive nobody here has a
 * row for.  A sector count of zero matches any count.
 */
static struct model *
findmodel(c, h, s)
    UINT c, h, s;
{
    static struct model *m;

    for (m = models; m < &models[NMODEL]; m++)
        if (m->cyl == c && m->heads == h &&
            (s == 0 || nsectors(m->bpt) == s))
            return (m);
    return (0);
}

/*
 * Ask the drive whether it is there and up to speed.  The sense command
 * answers active low: a ready drive leaves the ready bit clear.  A format
 * run against a drive that is not ready writes nothing and reports
 * success track after track.
 */
static
sense()
{
    struct hddma_cmd nb;
    int st;

    memset((char *) &nb, 0, sizeof nb);
    nb.drvsel = drive;
    nb.headsel = drive;
    nb.opcode = OP_SENSE;
    st = run(&nb, 0, 0);
    if (st < 0)
        die("the driver refused the sense command");
    if (st & SENSE_READY)
        die("the drive is not ready");
}

void
mw_main()
{
    struct model *mp, *row;
    int have;
    UINT track, head;

    if (sizeof (struct hddma_cmd) > CDBMAX)
        die("the command block is wider than the cdb block");

    mp = 0;
    mfg = model = 0;
    cyl = heads = spt = 0;
    precomp = lowcur = 0;
    stepdelay = 30;
    skew = DSKEW;
    first = last = fhead = lhead = 0;

    if (opts & O_MODEL) {
        for (row = models; row < &models[NMODEL]; row++)
            if (strcmp(omodel, row->name) == 0)
                break;
        if (row == &models[NMODEL])
            die("no such drive model");
        mp = row;
    }

    have = getlabel();
    if (mp) {
        cyl = mp->cyl;
        heads = mp->heads;
        spt = nsectors(mp->bpt);
        precomp = mp->precomp;
        lowcur = mp->lowcur;
        stepdelay = mp->stpdel;
        mfg = mp->mfg;
        model = mp->model;
    } else if (have) {
        cyl = lcyl;
        heads = lheads;
        spt = lspt;
    }
    if (opts & O_CYL)
        cyl = ocyl;
    if (opts & O_HEADS)
        heads = oheads;

    /*
     * How many sectors fit on a track, when nobody has said and no model
     * has either: seventeen of 512 bytes is what every one of these
     * drives holds, because a full track is 10416 bytes on all of them.
     */
    if (opts & O_SPT)
        spt = ospt;
    else if (spt == 0) {
        if ((row = findmodel(cyl, heads, 0)) != 0)
            spt = nsectors(row->bpt);
        else
            spt = nsectors(10416);
    }

    /*
     * A drive recognised by its shape gets its row's timing, since a
     * label that gave the shape has no room for it.
     */
    if (!mp && (row = findmodel(cyl, heads, spt)) != 0) {
        precomp = row->precomp;
        lowcur = row->lowcur;
        stepdelay = row->stpdel;
        mfg = row->mfg;
        model = row->model;
    }
    if (opts & O_PRECOMP)
        precomp = oprecomp;
    if (opts & O_LOWCUR)
        lowcur = olowcur;
    if (opts & O_STEP)
        stepdelay = ostep;
    if (opts & O_SKEW)
        skew = oskew;

    if (cyl == 0 || heads == 0 || spt == 0)
        die("no geometry: name a model with -m, or give -c and -h");
    if (cyl > 0xffff || heads > 0xff || spt > 0xff)
        die("a label cannot hold that geometry");
    if (heads > 16)
        die("sixteen heads is all four select lines can say");
    if (skew >= spt)
        die("the skew is as large as the track");

    if (have && (cyl != lcyl || heads != lheads || spt != lspt))
        fprintf(stderr, "%s: the label said %d/%d/%d\n", pname,
            lcyl, lheads, lspt);

    /*
     * The walk: the whole drive, or as much of it as the command line
     * narrowed.  -t is one cylinder and every head of it, -e one head of
     * every cylinder; neither is saying what the drive is.
     */
    if (opts & O_TRACK)
        first = last = otrack;
    else {
        first = 0;
        last = cyl - 1;
    }
    if (opts & O_HEAD)
        fhead = lhead = ohead;
    else {
        fhead = 0;
        lhead = heads - 1;
    }
    if (last >= cyl || lhead >= heads)
        die("past the end of the drive");

    printf("%s: %d cylinders, %d heads, %d sectors of %d bytes\n",
        devname, cyl, heads, spt, DRVSIZE);

    load();
    sense();
    recal();

    builds();
    for (track = first; track <= last; track++) {
        seekto(track);
        for (head = fhead; head <= lhead; head++) {
            buildth(track, head);
            fmthead(track, head);
        }
        printf("track %d\r", track);
    }
    printf("\n");

    /*
     * The label goes back whenever the walk crossed the sector it lives
     * in - cylinder 0, head 0, sector 0 - whether that was the whole
     * drive or one track of it.  The format has just filled that sector
     * with 0xe5, and a disk with no label is a disk nothing can open.
     */
    if (first == 0 && fhead == 0) {
        putlabel(0, 1, 0);
        printf("label: %d cylinders, %d heads, %d sectors, slices a b d e f g h\n",
            cyl, heads, spt);
    } else {
        printf("formatted; the label is untouched\n");
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
