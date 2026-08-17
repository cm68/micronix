/*
 * the out-of-band drive label
 *
 * include/disklabel.h
 *
 * A simulator hard disk file begins with a fixed 2048 byte label that the
 * hardware does not have: the geometry of the drive, the sector size, and
 * what the last format command wrote.  FORMATMW scribbles it when it
 * formats the volume, and the simulator's hard disk driver (lib/hdcdmadisk)
 * reads it back to know where the sectors are.
 *
 * This is NOT the in-band disk label - struct dlabel in sys/dlabel.h lives
 * in the second half of physical block 0 and describes the *filesystem*.
 * That one is written by mkfs.  This one is written by a format, and says
 * what the physical disk is.
 */

#define MAGIC		0xD15CC0DE	/* whoo-hoo, aren't we clever */
#define DEF_SECSIZE	2048		/* largest possible */
#define DATAOFF		2048		/* first 2k is label */

struct disklabel {
    int magic;              // a marker
    int secsize;            // bytes per sector
    int cylinders;          // number of cylinders
    int heads;              // number of heads
    int spt;                // sectors per track

    /*
     * What a sector header says, recorded when the volume is formatted.
     * A format command carries this - the sector count, the size code,
     * the gap and the fill - and the controller records it here.  It
     * does not write sector headers anywhere: the real hardware lays
     * them down on the track, but the simulation only stores the
     * parameters, and answers a later read header by constructing one
     * from these.  Keeping them only in the sectors themselves would
     * lose them, because the sectors hold data and not their own ids.
     * One copy for the whole volume: a format here is homogeneous.
     */
    int firstsec;           // number of the first sector on a track
    int seccode;            // the size code the format command used
    int gap3;               // inter sector gap
    int fill;               // what the data was filled with
    int formatted;          // nonzero once a format has been seen
};
