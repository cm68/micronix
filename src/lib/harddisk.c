/*
 * this is the hard disk abstraction for a volume
 *
 * lib/harddisk.c
 *
 * Changed: <2023-06-16 00:10:23 curt>
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define	DEF_SECSIZE	2048			// largest possible
#define MAGIC		0xD15CC0DE		// whoo-hoo, aren't we clever

/*
 * every drive has a fixed size label.
 */
struct disklabel {
    int magic;              // a marker
	int secsize;	    	// bytes per sector
	int cylinders;		    // number of cylinders
	int heads;			    // number of heads
	int spt;			    // sectors per track
};

#define DATAOFF 2048         // first 2k is label
#define NDRIVES 8

struct drive {
    struct disklabel label;
    int fd;
    char *name;
} drive[NDRIVES];

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

    for (i = 0; i < NDRIVES; i++) {
        dp = &drive[i];
        if (!dp->name) break;
        if (strcmp(name, drive[i].name) == 0) {
            return (&drive[i]);
        }
    }
    if (i == NDRIVES) {
        printf("too many drives\n");
        return 0;
    }
    dp->name = strdup(name);
    dp->fd = open(name, O_RDWR|O_CREAT, 0777);
    if (dp->fd < 0) {
        printf("open of %s failed %d\n", name, errno);
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

    printf("c: %d h: %d s: %d = %d\n", cylinder, head, sector, offset);

    return offset;
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
