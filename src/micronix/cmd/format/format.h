/*
 * format - the one disk formatter
 *
 * cmd/format/format.h
 *
 * The command is one program and several controllers: format.c holds the
 * command line, the device checks and the label, and each backend holds
 * the one controller it knows - mw.c the HD-DMA, hdca.c the HDCA, scsi.c
 * the 5380, ide.c the IDE card (which has no format command and gets a
 * label), dj.c the DJ-DMA floppy.
 *
 * What they share is here: the geometry being laid down, the label read
 * and written through the block device, and the prototype slice table.
 * The one thing that is per-controller and not per-drive is
 * d_firstsec - the HDCA numbers sectors from one - and the caller of
 * putlabel says which it is.
 */

/*
 * types.h has no guard, so it is the including file's first include and
 * not this header's: every backend includes <types.h> before this file.
 */
#include <sys/dlabel.h>

/*
 * What the command line said.  A backend refuses the bits its controller
 * has no use for rather than ignoring them, so a typo is an error and
 * not a format with the wrong shape.
 */
#define	O_MODEL		0x0001
#define	O_CYL		0x0002
#define	O_HEADS		0x0004
#define	O_SPT		0x0008
#define	O_PRECOMP	0x0010
#define	O_LOWCUR	0x0020
#define	O_STEP		0x0040
#define	O_SKEW		0x0080
#define	O_PROBE		0x0100
#define	O_TRACK		0x0200
#define	O_HEAD		0x0400

/*
 * The block device majors, sys/consts.c's biosw: the controller a
 * whole-disk node belongs to is the high byte of its device number.
 */
#define	HDCAMAJ		1
#define	DJMAJ		2
#define	MWMAJ		3
#define	IDEMAJ		4
#define	NCRMAJ		5

extern char *pname;
extern char *devname;
extern int devfd;
extern int drive;		/* the unit, low bits of the minor */
extern int major;
extern int opts;		/* the O_* bits seen */

extern UINT ocyl, oheads, ospt;
extern UINT oprecomp, olowcur, ostep, oskew;
extern UINT otrack, ohead;
extern char *omodel;

/* the geometry being laid down, and the name for the label */
extern UINT cyl, heads, spt;
extern UINT precomp, lowcur, stepdelay, skew;
extern char *mfg, *model;

/* the label's own answer, from getlabel */
extern UINT lcyl, lheads, lspt;

void die(char *);
int num(char *);
int getlabel();
void putlabel(int, int, int);
void protoslices(struct dlabel *);

/* one entry point per controller */
void mw_main();
void hdca_main();
void scsi_main();
void ide_main();
void dj_main();

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
