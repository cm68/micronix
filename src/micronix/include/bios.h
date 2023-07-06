/*
 * cp/m bios interface
 *
 * /include/bios.h
 *
 * Changed: <2023-07-04 11:19:24 curt>
 */
#define B_BOOT 		0x00
#define	B_WBOOT		0x03
#define	B_CONST		0x06
#define	B_CONIN		0x09
#define	B_CONOUT	0x0c
#define	B_LIST		0x0f
#define	B_PUNCH		0x12
#define	B_READER	0x15
#define	B_HOME		0x18
#define	B_SELDSK	0x1b
#define	B_SETTRK	0x1e
#define	B_SETSEC	0x21
#define	B_SETDMA	0x24
#define	B_READ		0x27
#define	B_WRITE		0x2a
#define	B_LISTST	0x2d
#define	B_SECTRAN	0x30

/*
 * disk parameter block
 */
struct dpb {
	unsigned short spt;			/* sectors per track */
	unsigned char bsh;			/* block shift factor */
	unsigned char blm;
	unsigned char exm;			/* extent mask */

	unsigned short dsm;			/* disk storage capacity */
	unsigned short drm;			/* directory capacity */

	unsigned char al0;
	unsigned char al1;

	unsigned short cks;			/* size of the diectory check vector */
	unsigned short off;
};

/*
 * disk parameter header
 */
struct dph {
	unsigned short *xlt;		/* translation vector */

	unsigned short scratch[3];

	char *dirbuf;				/* 128 - byte scratchpad area */

	struct dpb *dpb;			/* disk parameter block */

	unsigned char *csv;
	unsigned char *alv;			/* scratch pad */
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
