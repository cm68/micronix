/*
 * a volume as the HDCDMA controller sees one
 *
 * lib/hdcdmadisk.c
 *
 * The geometry and the transfers here would suit any hard disk, but what
 * a sector header holds and what read header hands back are this
 * controller's, out of its technical manual - so the file is named for
 * it rather than claiming to be general.  Another controller gets its
 * own beside this one.
 *
 * Changed: <2023-06-16 00:10:23 curt>
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include "../include/lockfile.h"
#include "../include/disklabel.h"


/*
 * This is in lib, which does not have the simulator's trace flags, and
 * the seek used to printf on every access - a line per sector, through
 * every boot and every install.  Off unless somebody turns it on here.
 */
static int hd_debug = 0;
#define NDRIVES 8

struct drive {
    struct disklabel label;
    int fd;
    int lock_fd;            /* advisory lock held on the drive file */
    char *name;
    int nexthdr;            /* rotational position, for read header */
} drive[NDRIVES];

/*
 * What the two fixed bytes of a sector header are.  The second one is
 * the one that carries meaning: the manual has it FE for an id field
 * and F8 for a data field, which is how a driver tells the two apart
 * when read header lands on the wrong thing.
 */
#define HDR_MARK    0xa1        /* the address mark */
#define HDR_ID      0xfe        /* ... and this says id field, not data */

/*
 * Controllers name their drives by unit - hddma-0, hdca-2 - which are
 * looked up in the current directory unless a search directory is set.
 * The blank drives that ship with the tree are in hwsim/resources, and
 * without this the only way to use them was to run the simulator from
 * the directory they live in.  Nothing warns you when you get that
 * wrong, either: drive_open creates what it cannot find, so a run from
 * the wrong place quietly gets a new empty disk instead of an error.
 *
 * A name that already has a / in it is used as given, so a controller
 * or a future per-unit option can still name an exact file.
 */
static char *drive_dir;

void
drive_setdir(char *dir)
{
    drive_dir = strdup(dir);
}

/*
 * Per-unit overrides, which is the "future per-unit option" the comment
 * above anticipated.  A unit named on the command line - hdcdma0:disk,
 * hdca1:other - is recorded here under the name its controller will ask
 * for, and drive_open answers with the file instead of looking in the
 * drive directory.  An override wins over the directory: naming a unit
 * is more specific than saying where units live.
 */
#define NOVERRIDE   16

static struct {
    char *unit;
    char *path;
} override[NOVERRIDE];

static int noverride;

int
drive_setunit(char *unit, char *path)
{
    int i;

    for (i = 0; i < noverride; i++) {
        if (strcmp(override[i].unit, unit) == 0) {
            free(override[i].path);
            override[i].path = strdup(path);
            return 0;
        }
    }
    if (noverride == NOVERRIDE) {
        return -1;
    }
    override[noverride].unit = strdup(unit);
    override[noverride].path = strdup(path);
    noverride++;
    return 0;
}

static char *
drive_override(char *name)
{
    int i;

    for (i = 0; i < noverride; i++) {
        if (strcmp(override[i].unit, name) == 0) {
            return override[i].path;
        }
    }
    return 0;
}

/*
 * when we format the drive, we write the label if it is not present, and whenever we
 * increase they cylinder or head count, we update the label.
 * note that this only will work if we format all the heads on a cylinder, before we
 * step to the next one.   that's the most reasonable method
 */
struct drive *
drive_open(char *name)
{
    int i;
    struct drive *dp;
    char path[PATH_MAX];

    {
        char *over = drive_override(name);

        if (over) {
            snprintf(path, sizeof(path), "%s", over);
        } else if (drive_dir && !strchr(name, '/')) {
            snprintf(path, sizeof(path), "%s/%s", drive_dir, name);
        } else {
            snprintf(path, sizeof(path), "%s", name);
        }
    }

    /*
     * match on the resolved path, not the unit name: two controllers
     * that both call their first unit 0 are different drives, but the
     * same file reached by two names is one drive
     */
    for (i = 0; i < NDRIVES; i++) {
        dp = &drive[i];
        if (!dp->name) break;
        if (strcmp(path, drive[i].name) == 0) {
            return (&drive[i]);
        }
    }
    if (i == NDRIVES) {
        printf("too many drives\n");
        return 0;
    }
    dp->name = strdup(path);
    dp->fd = open(path, O_RDWR|O_CREAT, 0777);
    if (dp->fd < 0) {
        printf("open of %s failed %d\n", path, errno);
        return 0;
    }
    dp->lock_fd = acquire_lock(path);
    if (dp->lock_fd < 0) {
        close(dp->fd);
        return 0;
    }
    // get the label or make a new one
    if (read(dp->fd, &dp->label, sizeof(dp->label)) != sizeof (dp->label)) {
        bzero(&dp->label, sizeof(dp->label));
        dp->label.magic = MAGIC;
        dp->label.secsize = DEF_SECSIZE;
        lseek(dp->fd, 0, SEEK_SET);
        if (write(dp->fd, &dp->label, sizeof(dp->label)) != sizeof (dp->label)) {
            printf("could not write empty label\n");
            close(dp->fd);
            return 0;
        }
    }
    if (dp->label.magic != MAGIC) {
        printf("label had bad magic %x\n", dp->label.magic);
        close(dp->fd);
        return 0;
    }
    return (dp);
}

/*
 * get/set the sector size.
 * we need to be able to reformat the drive, so we must be able to change the sector size
 */
int
drive_sectorsize(struct drive *dp, int secsize)
{
	if (secsize != 0) { 					// pass 0 to get the value
		if (dp->label.secsize != secsize) {
			printf("%s: change sector size from %d to %d\n", dp->name, dp->label.secsize, secsize);
		}
		dp->label.secsize = secsize;
		lseek(dp->fd, 0, SEEK_SET);
		if (write(dp->fd, &dp->label, sizeof(dp->label)) != sizeof (dp->label)) {
			printf("could not update label\n");
			close(dp->fd);
		}
	}
    return dp->label.secsize;
}

/*
 * if we exceed the previous bounds, bump them.  this typically happens when the
 * drive is formatted. if a change happened, update the on-disk label.
 */
static off_t 
diskoff(struct drive *dp, int cylinder, int head, int sector)
{
	int offset;
    int dirty = 0;
    if (dp->label.secsize == 0) {
        printf("can't seek until we set sectorsize\n");
        return DATAOFF;
    }

    /*
     * A blank drive learns its shape from what is written to it.  A
     * formatted one does not: its geometry is the multipliers in the
     * arithmetic below, so growing spt or heads after the fact moves
     * every sector already written and quietly reinterprets the whole
     * volume.  Out of range on a formatted drive is an error, which is
     * also what makes a head probe mean anything - head 5 of a four head
     * drive has to fail rather than make it a five head drive.
     */
    if (dp->label.formatted) {
        if (sector >= dp->label.spt || head >= dp->label.heads ||
            cylinder >= dp->label.cylinders) {
            if (hd_debug)
                printf("drive %s: c %d h %d s %d outside %d/%d/%d\n",
                    dp->name, cylinder, head, sector,
                    dp->label.cylinders, dp->label.heads, dp->label.spt);
            return -1;
        }
    } else {
        if ((sector + 1) > dp->label.spt) {
            dp->label.spt = sector + 1;
            dirty++;
        }
        if ((head + 1) > dp->label.heads) {
            dp->label.heads = head + 1;
            dirty++;
        }
        if ((cylinder + 1) > dp->label.cylinders) {
            dp->label.cylinders = cylinder + 1;
            dirty++;
        }
    }
    if (dirty) {
        lseek(dp->fd, 0, SEEK_SET);
        if (write(dp->fd, &dp->label, sizeof(dp->label)) != sizeof(dp->label)) {
            printf("could not update label\n");
            close(dp->fd);
            return DATAOFF;
        }
    }

    offset =
        DATAOFF + 
        (sector * dp->label.secsize) + 
        (head * dp->label.secsize * dp->label.spt) +
        (cylinder * dp->label.secsize * dp->label.spt * dp->label.heads);

    if (hd_debug)
        printf("c: %d h: %d s: %d = %d\n", cylinder, head, sector, offset);

    return offset;
}

/*
 * Record what a format command said about the sectors it laid down, and
 * hand it back for a read header.  The volume owns this, not the
 * controller: it outlives the run, and a drive file carried to another
 * machine still knows what shape its tracks are.
 */
void
drive_format(struct drive *dp, int firstsec, int seccode, int spt,
    int gap3, int fill, int cyl, int head)
{
    dp->label.firstsec = firstsec;
    dp->label.seccode = seccode;
    dp->label.gap3 = gap3;
    dp->label.fill = fill;
    if (spt)
        dp->label.spt = spt;
    /*
     * The format command is per-track and carries only the sector count,
     * not how many cylinders or heads the whole drive has.  FORMATMW
     * walks every track though, so the geometry is the largest cylinder
     * and head the format reaches.  Grow them, as a blank drive's diskoff
     * does, instead of stamping formatted=1 on the first track and
     * locking in 0/0 - which made every later access out of range.
     */
    if (cyl + 1 > dp->label.cylinders)
        dp->label.cylinders = cyl + 1;
    if (head + 1 > dp->label.heads)
        dp->label.heads = head + 1;
    dp->label.formatted = 1;

    lseek(dp->fd, 0, SEEK_SET);
    if (write(dp->fd, &dp->label, sizeof(dp->label)) != sizeof(dp->label))
        printf("could not update label\n");
}

/*
 * CRC-16-CCITT, the polynomial the id field is written with.  The
 * controller computes this over the mark and the header bytes.  Nothing
 * here checks it, but a header carrying a plausible one costs little and
 * a header carrying zeros invites the question.
 */
static unsigned short
hdrcrc(unsigned char *p, int n)
{
    unsigned short crc = 0xffff;
    int i, b;

    for (i = 0; i < n; i++) {
        crc ^= p[i] << 8;
        for (b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

/*
 * Read Headers hands back the eight bytes that follow the preamble: the
 * mark, an FE saying this is an id field and not data, the four bytes
 * the format laid down - cylinder low, cylinder high, head, sector - and
 * two CRC bytes.  HDCDMA technical manual 2.2.3, with the four from
 * table 2-9, which is also what formatmw puts in the id table.
 *
 * There is no sector argument, and the manual is explicit that the
 * argument field is unused: the command returns whichever header comes
 * under the head next.  That is the whole reason a driver can probe for
 * heads with it - it does not have to know what is there to ask - and it
 * is why retrying "will usually find the next sector header".  The
 * rotational position lives here and advances every call.
 *
 * Returns 0 if the drive has no such place, which is the answer a probe
 * for a head that is not there needs.
 */
int
drive_header(struct drive *dp, int cylinder, int head, char *buf)
{
    unsigned char *p = (unsigned char *) buf;
    unsigned short crc;
    int sector;

    if (!dp->label.formatted)
        return 0;
    if (cylinder >= dp->label.cylinders || head >= dp->label.heads)
        return 0;
    if (dp->label.spt <= 0)
        return 0;

    sector = dp->nexthdr % dp->label.spt;
    dp->nexthdr = sector + 1;

    p[0] = HDR_MARK;
    p[1] = HDR_ID;
    p[2] = cylinder & 0xff;
    p[3] = (cylinder >> 8) & 0xff;
    p[4] = head;
    p[5] = sector + dp->label.firstsec;
    crc = hdrcrc(p, 6);
    p[6] = (crc >> 8) & 0xff;
    p[7] = crc & 0xff;
    return 8;
}

int
drive_geometry(struct drive *dp, int *cyls, int *heads, int *spt)
{
    if (cyls) *cyls = dp->label.cylinders;
    if (heads) *heads = dp->label.heads;
    if (spt) *spt = dp->label.spt;
    return dp->label.formatted;
}

int
drive_write(struct drive *dp, int cylinder, int head, int sector, char *buf)
{
	int i;

    lseek(dp->fd, diskoff(dp, cylinder, head, sector), SEEK_SET);
    i = write(dp->fd, buf, dp->label.secsize);
    if (i < 0) {
        printf("drive %s (c: %d h: %d s: %d) write failed %d\n", dp->name, cylinder, head, sector, i);
    }
    return i;
}

int
drive_read(struct drive *dp, int cylinder, int head, int sector, char *buf)
{
    int i;

    bzero(buf, dp->label.secsize);
    lseek(dp->fd, diskoff(dp, cylinder, head, sector), SEEK_SET);
    i = read(dp->fd, buf, dp->label.secsize);
    if (i < 0) {
        printf("drive %s (c: %d h: %d s: %d) read failed %d\n", dp->name, cylinder, head, sector, i);
    }
    return i;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
