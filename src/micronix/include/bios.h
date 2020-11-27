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

struct dpb {
	UINT	spt;	/* sectors per track */
	UINT8	bsh;	/* block shift factor */
	UINT8 blm;
	UINT8	exm;	/* extent mask */

	UINT	dsm;	/* disk storage capacity */
	UINT		drm;	/* directory capacity */

	UINT8	al0;
	UINT8		al1;

	UINT	cks;	/* size of the diectory check vector */
	UINT	off;
};

/*
 * disk parameter header
 */

struct dph {
	UINT *xlt;		/* translation vector */

	UINT	scratch [3];

	char *dirbuf;		/* 128 - byte scratchpad area */

	struct dpb *dpb;	/* disk parameter block */

	UINT8	*csv;
	UINT8  *alv;		/* scratch pad */
};




