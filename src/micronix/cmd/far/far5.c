#include "far.h"

/*
 * How many 512 byte blocks the drive has.
 *
 * The controller is asked, rather than the device measured.  This used to
 * binary search block seeks, reading a byte at each one and watching for
 * the seek past the end, and then it was the driver that was asked; the
 * drive's geometry is one of the controller's own tables (include/sys/dj.h's
 * DJTAB), and a command block that reaches into controller memory is how to
 * read it.  There is no command that measures a drive - that would be a
 * command the controller does not have - which is why this is a program of
 * two, one for the table and one for the drive's status.
 *
 * The row holds the drive's cylinders and its sectors to a cylinder, and a
 * cylinder is both sides of one, so a double sided diskette's row counts it
 * twice.  Their product is the diskette's volume in sectors, with the
 * cylinders CP/M reserved ahead of its own track 0 already left out - the
 * row is the drive's, and the first and the last cylinder are the drive's
 * too.  The sector size is not in the row; that is the status command's
 * sector length code.  The volume in bytes over 512 is the answer, and the
 * format table above is counted in it.
 */
devsize(a)
    int a;
{
    static struct cdb req;
    static UINT8 row[DJROWSZ];
    static UINT slc, bps;
    static unsigned volume;

    /*
     * The drive's status, for the length of a sector on it.  This one has
     * no data phase: the controller leaves its answer inside the command
     * and the driver hands the block back (include/sys/ioctl.h).
     */

    req.len = 6;                /* STATUS */
    req.hole = -1;              /* no address in it to fill */
    req.count = 0;              /* and no data to move */
    req.flags = 0;

    req.cmd[0] = STATUS;
    req.cmd[1] = drive;
    req.cmd[2] = 0;             /* the drive characteristics byte */
    req.cmd[3] = 0;             /* the sector length code */
    req.cmd[4] = 0;             /* the drive status byte */
    req.cmd[5] = 0;             /* and the command's own answer */

    if (ioctl(a, CDBCMD, &req) < 0) {
        perror(device);
        exit(NO);
    }

    slc = req.cmd[3];

    if (slc > S1024) {
        eput("Unknown sector size\n");
        exit(NO);
    }

    bps = 128 << slc;           /* the code is 128, 256, 512, 1024 */

    /*
     * The drive's own row of the controller's table.  The address the data
     * lands at is the one thing a command block cannot say, so req.hole
     * says where it goes and the driver fills it in.
     */

    req.len = 10;               /* MEMREAD, HALT */
    req.hole = 1;               /* the address is the MEMREAD operand */
    req.count = DJROWSZ;        /* the whole row */
    req.flags = CDB_IN;
    req.buf = (char *) row;

    req.cmd[0] = MEMREAD;
    req.cmd[1] = 0;             /* the driver writes the three address */
    req.cmd[2] = 0;             /* bytes here */
    req.cmd[3] = 0;
    req.cmd[4] = DJROWSZ;       /* how many bytes to bring back */
    req.cmd[5] = 0;
    req.cmd[6] = (DJTAB + drive * DJROWSZ) & 0xff;
    req.cmd[7] = (DJTAB + drive * DJROWSZ) >> 8;
    req.cmd[8] = HALT;
    req.cmd[9] = 0;             /* the halt's own answer */

    if (ioctl(a, CDBCMD, &req) < 0) {
        perror(device);
        exit(NO);
    }

    volume = (row[0] - 1) * row[9];

    /*
     * The volume in bytes over 512, in the order that does not overflow a
     * 16 bit word: a sector is a whole number of 128 byte records, and the
     * volume of them over four is the volume of 512 byte blocks.
     */

    return volume * (bps / 128) / 4;
}

char *
save(a)
    char *a;
{
    char *b;

    b = malloc(strlen(a) + 1);

    strcpy(b, a);

    return b;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */

