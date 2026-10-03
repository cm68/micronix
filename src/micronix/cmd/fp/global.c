/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/global.c
 */

#include "fp.h"

char *device = "/dev/fp",
    *formatprog = "/bin/fdj",
    cflag = NO,
    eflag = NO,
    fflag = NO,
    lflag = NO,
    pflag = NO,
    iflag = NO,
    qflag = NO,
    tflag = NO,
    rflag = NO,
    sflag = NO,        /* silent */
    xflag = NO,
    vflag = YES,       /* verbose */
    debug = NO,
    **files = NULL,
    newvolume = NO,
    dirdirty = NO,
    superdirty = NO,
    haschanged = NO;

int freespace = 0,
    dirsize = 0,       /* number of entries in tape directory */
    nfiles = 0,
    fd = 0,
    reel = 0,
    serial = 0;

long yearago = 0,
    yearfromnow = 0,
    threshhold = 0,
    dumptime = 0;

struct vsuper super = {0};

struct entry *tapedir = NULL;

struct label label = {0};
