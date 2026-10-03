/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/fp.h
 *
 * This is the recovered original source (from the Morrow disks,
 * extra/ORIGINAL/BU12/lens.m16/usr/src/cmd/fp), ported to the
 * reconstruction.  The original was Whitesmith's C; here the
 * Whitesmith libc helpers (lenstr, cmpstr, cpystr, cpybuf, fill,
 * alloc, element, itob, ltob, scopy, save) are replaced by standard C,
 * the stat fields use the v7 names (st_size0, st_mode, ...), and
 * parameters that defaulted to int now carry an explicit type.
 */

#include <stdio.h>
#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <std.h>

#include "dtab.h"

/*
 * types of seeks or lseeks
 */

#define BYBLOCK   3
#define RELATIVE  1
#define BYBYTE    0

#define SUPER      1
#define PATHSIZE   34
#define DIRBLOCK   2
#define K64        (1L << 16)
#define MAXDEVSIZE K64
#define UDIRSIZE   16
#define UNKNOWN    (-1)
#define K          1024
#define CHUNK      (12 * K)
#define LASTONE    1
#define MAXCHAPTER 256
#define YEAR       (60L * 60L * 24L * 365L)

/*
 * directory entry flags
 */

#define CONTINUED  (1 << 0)
#define LINK       (1 << 1)

#define TAB        "/etc/dtab"
#define MAXREELS   100
#define READONLY   (-1)

/*
 * reasons for changing volume
 */

#define ONEFIRST   0
#define VOLFULL    1
#define STILLMORE  2
#define BADMEDIA   3

/*
 * types of file copies
 */

#define ROUND      0
#define SQUARE     1

/*
 * alternate use of the device member of entry
 */

#define MAXDSIZE   64

/*
 * data found on some freshly formatted disks
 */

#define FORMPATTERN (0xe5e5)
#define MINDSIZE    20

/*
 * location of the volume label
 */

#define LABEL      (SUPER * BUFSIZE + BUFSIZE / 2)

/*
 * The tape-volume superblock, kept at block SUPER.  The original called
 * this struct super, but <sys/fs.h> already owns that name (the kernel's
 * filesystem superblock), so the type is vsuper while the variable is
 * still super.
 */
struct vsuper {
    unsigned short dsize, vsize, reel, flags, free, serial, check;
    unsigned long time;
};

/*
 * A tape directory entry.  main() checks that this is 64 bytes.
 */
struct entry {
    char name[PATHSIZE];
    short mode;
    short user;
    short group;
    long length;
    unsigned long time;
    unsigned char links;
    unsigned device;          /* address on tape */
    unsigned short block;
    unsigned short offset;    /* this chapter's block offset */
    unsigned char chapter;    /* chapter number */
    char flags;
    unsigned inumber;
    unsigned char major, minor;
    char instance;
    short checksum;
};

struct list {
    unsigned char major, minor;
    unsigned int inumber;
    char *name;
    struct list *next;
};

struct label {
    char text[255], xor;
};

extern char *device, *formatprog, cflag, eflag, fflag, lflag, pflag, iflag,
    qflag, tflag, rflag, sflag, xflag, vflag, debug, **files,
    newvolume, dirdirty, superdirty, haschanged;

extern int freespace, dirsize, nfiles, fd, reel, serial;

extern long yearago, yearfromnow, threshhold, dumptime;

extern struct vsuper super;

extern struct entry *tapedir;

extern struct label label;

/*
 * Cross-file functions whose return type is not int.  The original left
 * most of these to Whitesmith's implicit-int declarations; they are named
 * here with their real types so a void or pointer result is not truncated
 * in the caller.
 */
unsigned short sum();
long fsize();
long tabread();
unsigned balloc();
char xor();

char *lookup();
char *parent();
struct entry *tapestat();
struct entry *ealloc();

void fatal();
void usage();
void prs();
void db();
void display();
void ptime();
void plength();
void ppart();
void pvol();
void space();
void missing();
void entdone();
void change();
void closedev();
void writedir();
void writesuper();
void doformat();
void tabwrite();
void readlabel();
void writelabel();
void getlabel();
void setchecksum();
void mkdir();

/* vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
