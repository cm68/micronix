/*
 * The HDCA - the port-mapped, non-DMA hard drive controller of the
 * M10, M20 and M26.  sys/hdca.h
 *
 * Reconstructed from the hd driver in the 1.4 kernel (kernels/micronix.14,
 * symbols _hdopen/_hdclose/_hdstrat/_hdint/_hdsint) and the controller
 * emulation in hwsim/d1/hdca.c.  512-byte sectors are inherent in the
 * controller; the four registers are:
 *
 *	0x50  control (write) / primary status PSR (read)
 *	0x51  command (write)  / secondary status SSR (read)
 *	0x52  function (write) / interrupt clear (read)
 *	0x53  data (write/read)
 *
 * The controller carries 1K of RAM, 512 bytes of sector data and 512 of
 * header, reached through port 0x53 with an auto-incrementing pointer.
 * A sector operation is: write the four header bytes (head, track,
 * sector, key) to the header area, then issue READSEC/WRITESEC; the
 * data area fills at +2 and its pointer wraps at 512, which is why the
 * driver reads two slack bytes first.  See hwsim/d1/hdca.c.
 */

/*
 * Ports.  Writes go to control/cmd/func/data, reads to PSR/SSR/intclr/
 * data; a port is one address whether it is being read or written.
 */
#define	HDCA_PORT	0x50
#define	HDCA_CONTROL	0x50	/* write */
#define	HDCA_CMD	0x51	/* write */
#define	HDCA_FUNC	0x52	/* write */
#define	HDCA_DATA	0x53	/* write/read */
#define	HDCA_PSR	0x50	/* read, primary status */
#define	HDCA_SSR	0x51	/* read, secondary status */
#define	HDCA_INT	0x52	/* read, interrupt clear */

/* control bits (0x50 write) */
#define	CONT_FRENBL	0x01	/* enable drive select and function */
#define	CONT_RUN	0x02	/* controller run enable */
#define	CONT_DSKCLK	0x04	/* use drive clock */
#define	CONT_WPROT	0x08	/* enable disk writes */

/* primary status bits (0x50 read) */
#define	PSR_NTRK0	0x01	/* 0 at track 0 */
#define	PSR_OPDONE	0x02	/* op done */
#define	PSR_COMPLT	0x04	/* all seeks complete */
#define	PSR_TIMOUT	0x08	/* timed out */
#define	PSR_NFAULT	0x10	/* not fault */
#define	PSR_NREADY	0x20	/* not ready */
#define	PSR_ILEVEL	0x40	/* index toggle */
#define	PSR_HALT	0x80	/* not busy */

/* command register (0x51 write) */
#define	CMD_RAMDATA	0	/* reset ram pointer to data area */
#define	CMD_READSEC	1	/* read a sector */
#define	CMD_READHDR	3	/* read a sector header */
#define	CMD_WRITESEC	5	/* write a sector */
#define	CMD_WRITEHDR	7	/* write a sector header */
#define	CMD_RAMHDR	8	/* reset ram pointer to header area */

/* secondary status bits (0x51 read) */
#define	SSR_REV		0xc0	/* board revision */
#define	SSR_DATACRC	0x02	/* data crc error */
#define	SSR_SDONE	0x01	/* seek done */

/* function register (0x52 write) */
#define	FUNC_DRV0	0x01	/* drive select 0 */
#define	FUNC_DRV1	0x02	/* drive select 1 */
#define	FUNC_STEP	0x04	/* step line */
#define	FUNC_DIR	0x08	/* direction: 1 = step down */
#define	FUNC_HEAD	0xf0	/* head select, inverted */
#define	FUNC_DRIVE	0x03

/* the interrupt line the HDCA shares with the HD-DMA */
#define	HDCAINT		0

/*
 * How many seconds a request may be in flight before the drive is
 * declared wedged and the request fails with EIO.  The watchdog tick
 * (hdtick in sys/hdca.c) runs once a second, so the count is seconds.
 */
#define	HDDEADLINE	1

#define	NHD		4	/* drives behind one controller */

/*
 * Per-drive state.  The first eight fields are the geometry, in the
 * order and types sys/dlabel.h's struct dlgeom promises, because
 * dllabel() writes through that view of this struct; the rest is the
 * driver's own.  h_cyl is where the arm is, h_head/h_sector where the
 * request wants it, h_buf/h_cur the request in flight, and h_intr the
 * phase the op-done interrupt resumes.
 */
struct hd {
	UINT	h_tracks;	/* cylinders */
	UINT8	h_heads;
	UINT8	h_sectors;	/* sectors per track */
	UINT	h_maxblk;
	UINT	h_spc;		/* sectors per cylinder */
	UINT	h_roll;
	UINT	h_cylstart;
	UINT8	h_firstsec;

	UINT8	h_flags;	/* busy/write/open/... */
	UINT	h_slice;	/* the slice this drive is open for */
	UINT8	h_retry;	/* retry count */
	UINT	h_cyl;		/* current cylinder */
	int	h_trackdelta;	/* target track - h_cyl */
	UINT	h_head;		/* target head */
	UINT8	h_sector;	/* target sector */
	UINT8	h_drive;	/* the unit, dev & 3 */
	UINT8	h_retry2;	/* retry counter */
	UINT8	h_cmd;		/* the ioctl opcode, 0 for an ordinary request */
	UINT8	h_stat;		/* the ioctl's result, read back in cmd[HDLEN] */
	struct buf *h_buf;	/* the request */
	struct buf *h_cur;	/* the request to iodone */
	int	(*h_intr)();	/* op-done continuation */
	struct hd *h_intarg;	/* continuation argument */
	struct buf *h_next;	/* queued request */
};

/* h_flags */
#define	HDBUSY		0x01
#define	HDWRITE		0x02
#define	HDSEEK		0x04	/* a seek is under way */
#define	HDACTIVE	0x08	/* the drive is held open */
#define	HDSEEKOK	0x10	/* seek completed */
#define	HDOPEN		0x20
#define	HDWAIT		0x40	/* a process sleeps on this drive */
#define	HDERROR		0x80

/*
 * The command block the raw ioctl runs (include/sys/ioctl.h's CDBCMD
 * door).  The HDCA is port-mapped and has no command block of its own,
 * so these four bytes are the driver's invention: an opcode and the
 * head, track and sector the operation seeks to.  The data phase, when
 * the op has one, moves through the caller's buffer - four header bytes
 * for a header read or write, a full sector for a sector read or write -
 * and the completion status comes back in the byte after the block,
 * cmd[HDLEN], the way the HD-DMA's does (include/sys/mw.h).
 */
struct hdca_cmd {
	UINT8	op;		/* what to do */
	UINT8	head;		/* the head to seek to */
	UINT8	track;		/* the track to seek to */
	UINT8	sector;		/* the sector, for a sector read or write */
};

#define HDLEN	4		/* the bytes a caller fills */

/* op */
#define	HDCA_HOME	1	/* step to track 0, no data */
#define	HDCA_READHDR	2	/* read a sector header, 4 bytes back */
#define	HDCA_WRITEHDR	3	/* write a sector header, 4 bytes */
#define	HDCA_READSEC	4	/* read a sector, 512 bytes back */
#define	HDCA_WRITESEC	5	/* write a sector, 512 bytes */
#define	HDCA_FORMAT	6	/* format one track: write all its headers */

extern struct hd hdinfo[NHD];
