/*
 * mwformat - format a Morrow HD-DMA hard disk
 *
 * cmd/mwformat/mwformat.c
 *
 *	mwformat [-c cyl] [-h heads] [-s spt] [-p precomp] [-l lowcur]
 *		 [-d step] [-k skew] [-m model] [-t track] [-e head] device
 *
 * What the drive is comes from the command line, or from a model name, or
 * from the disk's own label - in that order - and never from the device
 * node, which names a drive and a slice and nothing else.  The geometry
 * is written back to the disk when the format is done, because a format
 * blanks the label along with everything else on the cylinder it sits on,
 * and a disk with no label cannot be opened at all.
 *
 * The device must be the drive's whole-disk slice, 'c' - /dev/hd0c for
 * drive 0 - which is the same requirement cmd/label/label.c has and for
 * the same reason: block 0 of that slice is physical cylinder 0 head 0
 * sector 0 whatever the geometry turns out to be, which is where the
 * label goes and the only address that can be found before the label has
 * been read.  It is also the only way to open a disk that has no label,
 * and so the only way to give one a label.
 *
 * The drive's controller work is the vendor's formatmw, which ran under
 * CP/M off the boot ROM (stand/formatmw/formatmw.c): load the constants,
 * recalibrate, then a step and a format per track and head.  What is not
 * here is the rest of that program - the verify pass, the data patterns,
 * the bad sector map - all of which are more command blocks on the same
 * interface and none of which a disk needs to be mountable.
 *
 * See docs/DISKLABEL.md for the label, and include/sys/ioctl.h for the
 * command-block interface the driver runs these through.
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mw.h>
#include <sys/dlabel.h>

/*
 * The command block is the board's own, sixteen bytes (include/sys/mw.h):
 * the first twelve are the command through its opcode and are this
 * program's to fill, the thirteenth is the status the board answers with,
 * and the last three are the address of the next command, which is the
 * driver's because it is an address and only the driver knows one.
 */
#define CBKLEN	12		/* the part of the block a caller fills */
#define CBKDMA	4		/* and where in it the data address belongs */

/*
 * The drive models this machine reaches, dev/devlist's m5, m10, m16, m32
 * and m40 nodes, with the timing that used to be sys/mw.c's specs[] table.
 * A name is a shorthand for a geometry and nothing more: the driver reads
 * none of this, and -c, -h and the rest override any of it.
 *
 * bpt is the bytes one track holds when it is full, which is what fixes
 * the sector count: formatmw does not store the count, it works it out
 * from the track and the size of a sector, and so does this - see nsectors
 * below.
 */
struct model
{
    char *name;
    UINT cyl;                   /* cylinders, tracks per surface */
    UINT8 heads;
    UINT precomp;               /* cylinder where write precomp starts */
    UINT lowcur;                /* cylinder where low write current starts */
    UINT8 stpdel;               /* step pulse delay, 100 us */
    UINT bpt;                   /* bytes on a track */
};

static struct model models[] = {
    {"m5",  153, 4, 128, 128, 30, 10416},   /* Seagate 5 meg */
    {"m10", 306, 4, 128, 128,  2, 10416},   /* generic 10 meg */
    {"m16", 306, 6, 128, 128,  2, 10416},   /* CMI 16 meg */
    {"m32", 640, 6, 256, 256,  0, 10416},   /* CMI 32 meg */
    {"m40", 733, 5, 300, 733, 30, 10416},   /* Seagate 40 meg */
};

#define NMODEL (sizeof models / sizeof models[0])

/*
 * The shape of a track.  A track is a gap, then one sync field, sector
 * header and gap for each sector, then a last gap for speed variations -
 * and the sector count is whatever fits in the bytes the drive holds, so
 * these add up to the same 10416 bytes on every one of these drives.
 * formatmw's own constants, and the numbers its arithmetic uses.
 */
#define DRVSIZE	512		/* bytes in a sector; Micronix reads no other */
#define GAP3	43		/* the gap after a sector, for 512-byte sectors */
#define	SYNC	13		/* the sync field before a header */
#define	IDF	7		/* the sector header */
#define	GAP2	16		/* header to data field */
#define	DATAHDR	4		/* the size of a header field less its data */
#define	GAP1	16		/* the first gap, after the index hole */
#define	GAP4	208		/* the last gap: 2% for speed variations */
#define	FILL	0xe5		/* the byte every data field is filled with */
#define DSKEW	3		/* the interleave, formatmw's default */

/*
 * The select byte a LOAD and a HOME carry: LCONST, which the controller
 * requires for a load-constants command, and the head select bits the
 * vendor's own formatter set while doing it.
 */
#define SELCONST 0x3c

#define ALLSTEPS 0xffff		/* more step pulses than any drive has tracks */

static char *pname = "mwformat";

/*
 * The geometry being laid down, and the disk's own answer.  lcyl, lheads
 * and lspt are what the label said, if it said anything, and are kept
 * only so that a disagreement can be reported before the label is
 * overwritten.
 */
static UINT cyl, heads, spt, precomp, lowcur, stepdelay, skew;
static UINT lcyl, lheads, lspt;
static int drive;		/* which of the controller's four drives */
static UINT first, last;	/* the cylinders to walk */
static UINT fhead, lhead;	/* and the heads */
static UINT curtrk;		/* where the head is, as far as this knows */

/*
 * The sector-header table the format command points the board at: four
 * bytes an entry - cylinder low, cylinder high, head, sector - for the
 * sectors of one track, in the order they are laid down.  That order is
 * the interleave, and it is what a read header will report later.
 */
static char image[DRVSIZE];

/*
 * The board's status byte, in words.  A format that fails says which way,
 * and the driver does not read statuses: it hands the byte back and the
 * program that asked for the work is the one that knows what it means.
 */
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

#define BADCMD 0xa0		/* the board's answer to an opcode it has not */

void
die(msg)
    char *msg;
{
    fprintf(stderr, "%s: %s\n", pname, msg);
    exit(1);
}

/*
 * The name of a status byte, or 0 when there is no name for it.
 */
char *
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
 * A number from the command line, or -1 if the argument is not one.  atoi
 * alone cannot tell "0" from "not a number", and 0 is a legal cylinder,
 * head and step delay.
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

/*
 * How many sectors fit on a track: the bytes the track holds, less the
 * gaps that are not per-sector, divided by what one sector costs - its
 * header, its gap, its sync and its data.  formatmw works the count out
 * this way rather than storing it, which is why it cannot disagree with
 * the size of a sector.
 */
UINT
nsectors(bpt)
    UINT bpt;
{
    return ((bpt - (GAP1 + GAP4)) /
        (SYNC + IDF + GAP2 + DATAHDR + DRVSIZE + GAP3));
}

/*
 * Run one command block, and return the status the board answered with, or
 * -1 if the ioctl itself failed.
 *
 * count and buf are the command's data phase, which a format has - the
 * sector-header table above - and a seek does not.  The block's own
 * address for that data is not something this program can write: a
 * program's buffer is a virtual address and the board follows a physical
 * one, so the address is left to the driver, which holds the buffer and
 * knows where it really is.  All this says is where in the block the
 * address belongs, which is the block's own layout and not a choice.
 */
int
run(fd, nb, count, buf)
    int fd;
    struct hddma_cmd *nb;
    UINT count;
    char *buf;
{
    struct cdb r;
    int st;

    memset((char *) &r, 0, sizeof r);
    memcpy((char *) &r.cmd[0], (char *) nb, sizeof (struct hddma_cmd));
    r.len = CBKLEN;
    r.count = count;
    r.flags = 0;
    r.buf = buf;
    r.hole = count ? CBKDMA : -1;

    if (ioctl(fd, CDBCMD, &r) < 0)
        return (-1);

    /*
     * The status comes back in the byte after the command: the board
     * writes its completion into the block in memory, and that byte is
     * the one part of the block a caller reads and does not write.
     */
    st = r.cmd[CBKLEN] & 0377;
    return (st);
}

/*
 * Report a command that did not answer OK, and give up.  Every one of
 * these means the drive is not doing what it was told, and a format that
 * carried on through one would be formatting a disk nobody had checked.
 */
void
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
 * Load the drive's constants: the step rate, the head settle time and the
 * sector size code.  What the controller does afterwards - how fast it
 * steps, how big a sector it reads and writes - is what this set, so it
 * goes first, and with the interrupt bit clear: every command this program
 * sends is polled to completion, and an interrupt on top of that would be
 * answered by a driver that is not waiting for one.
 */
void
load(d)
    int d;
{
    struct hddma_cmd nb;

    memset((char *) &nb, 0, sizeof nb);
    nb.drvsel = drive;
    nb.headsel = drive | SELCONST;
    nb.arg0.byte.high = stepdelay;
    nb.byte2 = SETTLE;
    nb.byte3 = SEC512;
    nb.opcode = OP_LOAD;
    check(run(d, &nb, 0, 0), "load constants");
}

/*
 * Recalibrate: step out further than the drive is long, which every drive
 * answers by stopping at track zero.  That is the one way to know where
 * the head is, and nothing else here can start from an unknown track.
 */
void
recal(d)
    int d;
{
    struct hddma_cmd nb;

    memset((char *) &nb, 0, sizeof nb);
    nb.drvsel = drive | STEPOUT;
    nb.headsel = drive | SELCONST;
    nb.steps = ALLSTEPS;
    nb.opcode = OP_NOP;
    check(run(d, &nb, 0, 0), "recalibrate");
    curtrk = 0;
}

/*
 * Step the head to a track, by the difference from where it is.  The
 * controller steps by a count and a direction and has no idea which track
 * that lands on, so this is the program's arithmetic and its bookkeeping.
 */
void
seekto(d, track)
    int d;
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
    check(run(d, &nb, 0, 0), "seek");
    curtrk = track;
}

/*
 * Format one track of one head.  The three argument bytes the controller
 * wants are all complemented - the gap after each sector, the number of
 * sectors, and the code for the size of one - because the board counts up
 * to overflow rather than down to zero, and the fourth is the byte every
 * data field is filled with.
 *
 * The select byte is the head again, and one thing about it is not
 * obvious: it holds three bits of head, and a drive with more than eight
 * heads is one that uses the low-current line as head select line 4.  So
 * the fourth head bit goes in the select byte's bit 6, where that line is,
 * and not beside the other three - bit 5 of the byte is spare and has no
 * wire on the cable.  Heads 0 through 7 then leave that line high, which is
 * the same thing as high write current, so one encoding serves both a drive
 * that is told about its current and a drive whose ninth head is on that
 * line.
 *
 * Which means a drive with more than eight heads cannot be told to change
 * its write current - that line is a head line now - and the drives large
 * enough to want the extra heads are the ones that stopped needing to be
 * told (sys/TODO has the cable).  The write current is otherwise chosen
 * here rather than by the drive: high until the cylinder where the drive
 * wants the low setting, and precompensation from the cylinder where it
 * wants that.  Both come from the geometry being laid down and both are the
 * program's to decide - the driver has no table to look them up in.
 */
void
fmthead(d, track, head)
    int d;
    UINT track, head;
{
    struct hddma_cmd nb;

    memset((char *) &nb, 0, sizeof nb);
    nb.drvsel = drive;
    nb.headsel = drive | ((~head & 7) << 2);
    if (~head & 8)              /* heads 0-7: bit 6, high current, and with
                                 * it head select line 4 held low */
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
    check(run(d, &nb, spt * 4, image), "format");
}

/*
 * Number the sectors of a track, once: the table says which sector number
 * goes where on the track, and the interleave is the order they are
 * written in.  Taken from formatmw, which took it from the hardware's own
 * formatter - the sectors come round in that order because that is what
 * makes the next one arrive under the head in time to be read.
 */
void
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

/*
 * And put this track and head into every entry of it.
 */
void
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
 * Write the label: what the drive is, at DL_OFFSET in the boot sector of
 * the drive's 'c', which the format has just filled with 0xe5 along with
 * every other sector it touched.
 *
 * This is the same thing cmd/label/label.c's -w does to a disk that has
 * no label, and it is written the same way - through the block device
 * rather than as a raw command block, so that the sector in the cache and
 * the sector on the disk are one object.  A raw write would go under the
 * cache, the next open would read the sector the cache still had, and the
 * disk would look as if it had no label: the very state this write exists
 * to end.
 *
 * The boot fields are zero, because there is no boot yet - mkfs -i puts
 * one there and rewrites this whole label with its numbers when it does.
 * The slice table is empty, which every reader takes as one slice over
 * the whole drive, rolled: the layout d_roll describes and the one every
 * disk made before the table existed has.
 */
void
putlabel(d)
    int d;
{
    static char buf[DRVSIZE];
    static struct dlabel *lp;
    int i;

    if (lseek(d, 0L, 0) < 0)
        die("cannot seek to block 0");
    if (read(d, buf, DRVSIZE) != DRVSIZE)
        die("cannot read block 0");

    lp = (struct dlabel *) &buf[DL_OFFSET];
    memset((char *) lp, 0, sizeof (struct dlabel));
    for (i = 0; i < 4; i++)
        lp->d_magic[i] = DL_MAGIC[i];
    lp->d_version = DL_VERSION;
    lp->d_tracks = cyl;
    lp->d_heads = heads;
    lp->d_spt = spt;
    lp->d_roll = cyl >> 1;

    if (lseek(d, 0L, 0) < 0)
        die("cannot seek to block 0");
    if (write(d, buf, DRVSIZE) != DRVSIZE)
        die("cannot write block 0");
}

/*
 * What the disk says it is, if it says anything: the geometry out of the
 * label in block 0, which is the sector the format is about to fill with
 * 0xe5.  A disk with no label, or one whose label carries no geometry,
 * answers no.  The label has no field for the step delay, the
 * precompensation cylinder or the low-current cylinder, so it cannot
 * answer for those at all - findmodel below is what makes up the
 * difference.
 */
int
getlabel(d)
    int d;
{
    static char buf[DRVSIZE];
    static struct dlabel *lp;
    int i;

    if (lseek(d, 0L, 0) < 0)
        die("cannot seek to block 0");
    if (read(d, buf, DRVSIZE) != DRVSIZE)
        die("cannot read block 0");

    lp = (struct dlabel *) &buf[DL_OFFSET];
    for (i = 0; i < 4; i++)
        if (lp->d_magic[i] != DL_MAGIC[i])
            return (0);
    if (lp->d_tracks == 0 || lp->d_heads == 0 || lp->d_spt == 0)
        return (0);

    lcyl = lp->d_tracks;
    lheads = lp->d_heads;
    lspt = lp->d_spt;
    return (1);
}

/*
 * The model row whose shape this is, or 0 for a drive nobody here has a
 * row for.  It is how a disk that already has a label gets its timing
 * back: the label records the shape and has no field for the step delay or
 * the two cylinders, so a drive recognised by its shape is a drive whose
 * row can still say how to step it and where its write current changes.
 * A sector count of zero matches any count, for the caller that wants the
 * row by shape alone.
 */
struct model *
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
 * answers with the drive's status bits, not with a completion code, and
 * they are active low: a bit that is set is a condition that is *not*
 * true, which is why a ready drive answers with the ready bit clear.  A
 * format run against a drive that is not ready writes nothing and reports
 * success track after track, so this is worth the one command.
 */
void
sense(d)
    int d;
{
    struct hddma_cmd nb;
    int st;

    memset((char *) &nb, 0, sizeof nb);
    nb.drvsel = drive;
    nb.headsel = drive;
    nb.opcode = OP_SENSE;
    st = run(d, &nb, 0, 0);
    if (st < 0)
        die("the driver refused the sense command");
    if (st & SENSE_READY)
        die("the drive is not ready");
}

usage()
{
    fprintf(stderr, "usage: %s [-c cyl] [-h heads] [-s spt]\n", pname);
    fprintf(stderr, "       [-p precomp] [-l lowcur] [-d step] [-k skew]\n");
    fprintf(stderr, "       [-m model] [-t track] [-e head] device\n");
    fprintf(stderr, "       models: m5 m10 m16 m32 m40\n");
    exit(1);
}

main(argc, argv)
    int argc;
    char **argv;
{
    static char *device;
    static char *arg;
    static struct model *mp, *row;
    static struct stat sbuf;
    static int fd, i, n, have;
    static UINT track, head;
    static int cylset, headsset, sptset, tset, eset;
    static UINT wcyl, wheads, wspt;
    static UINT wprecomp, wlowcur, wstep, wskew;
    static int wprecompset, wlowcurset, wstepset, wskewset;

    pname = argv[0];

    /*
     * One fact the compiler does not check, and the whole interface rests
     * on: the block this program builds has to be the block the driver and
     * the board use.  Sixteen bytes with no padding between them - a pad
     * byte would move every field after it, and the format would lay down
     * whatever those bytes happened to mean.  The status byte the board
     * answers with is the thirteenth, which is why the two sizes have to be
     * equal and not merely close.
     */
    if (sizeof (struct hddma_cmd) != CDBMAX)
        die("the command block is not the cdb block");

    device = 0;
    mp = 0;
    cyl = heads = spt = 0;
    precomp = lowcur = 0;
    stepdelay = 30;             /* the boot loader's, and the driver's */
    skew = DSKEW;
    cylset = headsset = sptset = tset = eset = 0;
    wprecompset = wlowcurset = wstepset = wskewset = 0;
    first = last = fhead = lhead = 0;

    for (i = 1; i < argc; i++) {
        arg = argv[i];
        if (arg[0] != '-') {
            if (device)
                usage();
            device = arg;
            continue;
        }
        if (arg[1] == 0 || arg[2] != 0)
            usage();

        if (arg[1] == 'm') {
            if (++i == argc)
                usage();
            for (row = models; row < &models[NMODEL]; row++)
                if (strcmp(argv[i], row->name) == 0)
                    break;
            if (row == &models[NMODEL])
                die("no such drive model");
            mp = row;
            continue;
        }

        if (++i == argc)
            usage();
        if ((n = num(argv[i])) < 0)
            usage();
        switch (arg[1]) {
        case 'c':
            wcyl = n;
            cylset = 1;
            break;
        case 'h':
            wheads = n;
            headsset = 1;
            break;
        case 's':
            wspt = n;
            sptset = 1;
            break;
        case 'p':
            wprecomp = n;
            wprecompset = 1;
            break;
        case 'l':
            wlowcur = n;
            wlowcurset = 1;
            break;
        case 'd':
            wstep = n;
            wstepset = 1;
            break;
        case 'k':
            wskew = n;
            wskewset = 1;
            break;
        case 't':
            first = last = n;
            tset = 1;
            break;
        case 'e':
            fhead = lhead = n;
            eset = 1;
            break;
        default:
            usage();
        }
    }
    if (!device)
        usage();

    if (stat(device, &sbuf) < 0)
        die("cannot stat the device");
    if ((sbuf.st_mode & IFMT) != IFBLK)
        die("not a block special file");
    /*
     * The whole-disk slice and no other: block 0 of it is physical cylinder
     * 0 head 0 sector 0, and block 0 of any other slice is somewhere inside
     * a filesystem - a label written there would be written over somebody's
     * data.  cmd/label/label.c makes the same check for the same reason.
     */
    if (((sbuf.st_addr[0] >> 5) & 7) != DL_WHOLE)
        die("that node is not the whole-disk slice 'c'");
    drive = sbuf.st_addr[0] & 3;

    if ((fd = open(device, 2)) < 0)
        die("cannot open the device for writing");

    /*
     * What the drive is, in the order that matters: the model named on the
     * command line, then the disk's own label, then the geometry the flags
     * give.  The device node is not consulted for any of it - it names a
     * drive and a slice, and no geometry at all.
     */
    have = getlabel(fd);
    if (mp) {
        cyl = mp->cyl;
        heads = mp->heads;
        spt = nsectors(mp->bpt);
        precomp = mp->precomp;
        lowcur = mp->lowcur;
        stepdelay = mp->stpdel;
    } else if (have) {
        cyl = lcyl;
        heads = lheads;
        spt = lspt;
    }
    if (cylset)
        cyl = wcyl;
    if (headsset)
        heads = wheads;

    /*
     * How many sectors fit on a track, when nobody has said and no model
     * has either: seventeen of 512 bytes is what every one of these drives
     * holds, because a full track is 10416 bytes on all of them.
     */
    if (sptset)
        spt = wspt;
    else if (spt == 0) {
        if ((row = findmodel(cyl, heads, 0)) != 0)
            spt = nsectors(row->bpt);
        else
            spt = nsectors(10416);
    }

    /*
     * The timing, when no model was named: a drive recognised by its shape
     * gets its row's timing, since a label that gave the shape has no room
     * for any of it.  A shape no row has keeps the conservative values set
     * above - precompensation from cylinder 0, and the low write current
     * everywhere.
     */
    if (!mp && (row = findmodel(cyl, heads, spt)) != 0) {
        precomp = row->precomp;
        lowcur = row->lowcur;
        stepdelay = row->stpdel;
    }
    if (wprecompset)
        precomp = wprecomp;
    if (wlowcurset)
        lowcur = wlowcur;
    if (wstepset)
        stepdelay = wstep;
    if (wskewset)
        skew = wskew;

    if (cyl == 0 || heads == 0 || spt == 0)
        die("no geometry: name a model with -m, or give -c and -h");
    if (cyl > 0xffff || heads > 0xff || spt > 0xff)
        die("a label cannot hold that geometry");
    /*
     * The head is four bits of the select byte: three of them where the
     * byte has room for them, and the fourth on the low-current line,
     * which is what a drive with more than eight heads uses as head select
     * line 4 (fmthead below).  Sixteen heads is the whole of what four
     * lines can say, so that is the end of the geometry this can lay down.
     *
     * Note that the simulator does not model that line: it decodes three
     * bits, so heads 8 and up read back as head 0 on the one machine this
     * can be tested on, and a disk with them is real hardware only.
     */
    if (heads > 16)
        die("sixteen heads is all four select lines can say");
    if (skew >= spt)
        die("the skew is as large as the track");

    /*
     * Say so when what is being laid down is not what the disk says it is.
     * The disk is about to stop saying it either way, so this is the last
     * moment the disagreement can be seen - the warning sys/mw.c used to
     * print, in the only place that still has both numbers.
     */
    if (have && (cyl != lcyl || heads != lheads || spt != lspt))
        fprintf(stderr, "%s: the label said %d/%d/%d\n", pname,
            lcyl, lheads, lspt);

    /*
     * The walk: the whole drive, or as much of it as the command line
     * narrowed.  -t is one cylinder and every head of it, -e one head of
     * every cylinder, and neither is saying what the drive is - a partial
     * format is a repair, one track or one head that will not read.
     */
    if (!tset) {
        first = 0;
        last = cyl - 1;
    }
    if (!eset) {
        fhead = 0;
        lhead = heads - 1;
    }
    if (last >= cyl || lhead >= heads)
        die("past the end of the drive");

    printf("%s: %d cylinders, %d heads, %d sectors of %d bytes\n",
        device, cyl, heads, spt, DRVSIZE);

    load(fd);
    sense(fd);
    recal(fd);

    builds();
    for (track = first; track <= last; track++) {
        seekto(fd, track);
        for (head = fhead; head <= lhead; head++) {
            buildth(track, head);
            fmthead(fd, track, head);
        }
        printf("track %d\n", track);
    }

    /*
     * The label goes back whenever the walk crossed the sector it lives in
     * - cylinder 0, head 0, sector 0 - whether that was the whole drive or
     * one track of it.  The format has just filled that sector with 0xe5,
     * and a disk with no label is a disk nothing can open: the label is
     * what says which geometry to map with.  A walk that missed it leaves
     * the label where it was.
     */
    if (first == 0 && fhead == 0) {
        putlabel(fd);
        printf("label: %d cylinders, %d heads, %d sectors, roll %d\n",
            cyl, heads, spt, cyl >> 1);
    } else {
        printf("formatted; the label is untouched\n");
    }
    exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
