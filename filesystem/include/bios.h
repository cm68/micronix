# define 	B_BOOT 		0x00
# define	B_WBOOT		0x03
# define	B_CONST		0x06
# define	B_CONIN		0x09
# define	B_CONOUT	0x0c
# define	B_LIST		0x0f
# define	B_PUNCH		0x12
# define	B_READER	0x15
# define	B_HOME		0x18
# define	B_SELDSK	0x1b
# define	B_SETTRK	0x1e
# define	B_SETSEC	0x21
# define	B_SETDMA	0x24
# define	B_READ		0x27
# define	B_WRITE		0x2a
# define	B_LISTST	0x2d
# define	B_SECTRAN	0x30


/*
 * disk parameter block
 */

struct dpb
	{
	UCOUNT	spt;	/* sectors per track */

	UTINY	bsh,	/* block shift factor */
		blm,
		exm;	/* extent mask */

	UCOUNT	dsm,	/* disk storage capacity */
		drm;	/* directory capacity */

	UTINY	al0,
		al1;

	UCOUNT	cks,	/* size of the diectory check vector */
		off;
	};








/*
 * disk parameter header
 */


struct dph
	{
	UCOUNT *xlt;		/* translation vector */

	UCOUNT	scratch [3];

	TEXT *dirbuf;		/* 128 - byte scratchpad area */

	struct dpb *dpb;	/* disk parameter block */

	TEXT	*csv,
		*alv;		/* scratch pad */
	};




