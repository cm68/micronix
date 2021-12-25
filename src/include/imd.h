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
 * writing is a lot more problematic, since we've already snarfed the data from
 * the imd device file.  so what happens is that we create a drive file that
 * contains tracks that overlay the data from the IMD file, and loading the IMD
 * initially first reads the baseline and then overlays the newly written data
 * that is tagged with the parent. it's named <imd file name>-delta
 *
 * the format of this delta file is brute force and wasteful: all are 5M
 * one byte for every potential sector on the disk
 * 2 * 80 * 32 = 5120 bytes,
 *  32 bytes being the dirty sector indication
 *  2 * 80 * 32 * 1024 bytes of the actual data, where each sector
 *  1024 being every potential sector.
 * so calculating the delta offset is trivial
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

/* layout of the delta file */
#define MAXSECSIZE  1024
#define HEADS       2                   // 0, 1
#define CYLINDERS   80                  // 0 - 79
#define TRACKS      CYLINDERS * HEADS
#define SECTORS     32                  // 0 - 31

#define trknum(c, h) (((c) * HEADS) + (h))

#define DELTA_SIZE      CYLINDERS * HEADS * SECTORS
#define DIRTY_OFF(t, h, s)  (((trknum(t, h) * SECTORS) + (s)))
#define DELTA_OFF(t, h, s)  (DELTA_SIZE + (MAXSECSIZE * DIRTY_OFF(t, h, s)))
#define MAXDELTA        DELTA_OFF(TRACKS, 0, 0)

#define DELTA_NO    0           // use the original
#define DELTA_YES   1           // delta has newer

/*
 * this is our handle for the read/write .imd file support
 */
struct imd {
    int drive;				// assigned by the simulator
    char *comment;                      // what it is
    int heads;                          // number of actual heads
    int cyls;                           // number of cylinders
    struct imd_trk *tracks[TRACKS];     // array of tracks - indexed by trknum(c, h)
    int delta_fd;                       // file descriptor for writes
    char delta_map[DELTA_SIZE];         // the dirty map
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
        unsigned char cyl;
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
extern void imd_trkinfo(void *vp, int cyl, int head, int *secs, int *secsize);
extern int imd_write(void *vp, int cyl, int head, int osec, char *buf);
extern int imd_read(void *vp, int cyl, int head, int osec, char *buf);
extern void imd_dump_track(struct imd_trk *tp);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

