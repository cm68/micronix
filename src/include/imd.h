/*
 * interface to the imd library
 *
 * imd.h
 * Changed: <2021-12-23 15:32:43 curt>
 *
 * an imd file is just a header followed by a bunch of tracks.
 *
 * this snarfs an imd file into memory, suitable for a floppy driver to
 * to read it by cyl/head/sector
 * an imd file has the following format:
 * a text header, up to a 0x1a (control-z)
 * repeated for each track:
 * mode     1 byte
 * cylinder 1 byte
 * head     1 byte
 * numsecs  1 byte
 * secsize  1 byte in shifts of 128
 * secmap   numsecs of sector numbers
 * cylmap   numsecs of sector numbers - never seen this
 * headmap  numsecs of sector numbers - never seen this
 * a data code:  0: no data 1: secsize bytes 2: a fill byte
 *
 * writing is an overlay: the image holds the baseline (its framing and any
 * data), and a key store beside it - <imd file name>-delta - holds the
 * sectors written since, keyed by cylinder, head and sector slot.  A load
 * reads the image, then layers the delta's data on top.  The framing a
 * format lays down goes into the image itself, out of band, the way the
 * label does; the delta is data only.
 *
 * building this file with -DSTAND will define a program that does some reporting
 * of imd file contents, and additionally can write a new imd file from the original
 * plus the delta
 */

/* imd data codes */
#define IMD_ABSENT  0
#define IMD_DATA    1
#define IMD_FILL    2
#define IMD_EOC     0x1a

/* layout of the delta overlay */
#define MAXSECSIZE  1024                // the most bytes any sector can be
#define SECTORS     32                  // the most sectors any track holds
#define MAXHEADS    16                  // the head stride of the track array

/*
 * The track array is indexed cylinder-major at a fixed head stride, so a
 * disk whose heads are not known until it is formatted can grow by
 * cylinder without moving the tracks already placed.  The delta is a key
 * store keyed by three bytes - cylinder, head, and a sector slot - holding
 * only sector data.  A track's framing (its layout) lives in the image
 * itself, the way the label does.
 */
#define imd_trknum(ip, c, h)        (((c) * MAXHEADS) + (h))
#define imd_ntracks(ip)             ((ip)->cyls * MAXHEADS)

/*
 * this is our handle for the read/write .imd file support
 */
struct kv;
struct imd {
    int drive;				// assigned by the simulator
    char *comment;                      // what it is
    int heads;                          // number of actual heads
    int cyls;                           // number of cylinders
    int spt;                            // most sectors any track holds
    struct imd_trk **tracks;            // array of tracks - indexed by trknum(c, h)
    struct kv *delta;                   // the write overlay, 0 for none
    int fd;                             // the image, open to append track records
    int lock_fd;                        // advisory lock held on the image
};

/*
 * cp/m disks can have a bizarre format where the first track is single density
 * with one sector size, and the rest of the disk is another sector size and density
 * entirely.  so each track can have it's own format and size.  handle this.
 * this is strongly derived from dave dunfield's file format.  kudos to him for doing
 * the hard work of inventing this abstraction.
 *
 * however, in memory, we just scribble on the same place as the original data.
 * this means that we only read the delta once.
 * 
 * here's splitting a hair:  a track is a hunk of recorded data.  it has an address of (cyl, head)
 * it does not have an externally known 'track number' - that's for internal use
 */
struct imd_trk {
    struct {
        unsigned char mode;
        unsigned short cyl;
        unsigned char head;
        unsigned char nsec;
        unsigned char size;
    } fixed;
    int head;
    int secsize;
    char *secmap;
    char *cylmap;
    char *headmap;
    char **data;
};

extern void imd_close(void *vp);
extern void *imd_load(char *fname, int drive, int create_delta);
extern int imd_firstsec(void *vp, int cyl, int head);
extern void imd_trkinfo(void *vp, int cyl, int head, int *secs, int *secsize);
extern int imd_density(void *vp, int cyl, int head);
extern int imd_write(void *vp, int cyl, int head, int osec, char *buf);
extern int imd_read(void *vp, int cyl, int head, int osec, char *buf);
extern int imd_setfmt(void *vp, int cyl, int head, int firstsec, int secs,
    int secsize, int mode, int fill);
extern void imd_dump_track(struct imd_trk *tp);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

