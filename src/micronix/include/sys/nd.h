#include <sys/ints.h>

/*
 * The network disk (ND) — the HDCA's shape, minus the disk.
 *
 * A port-mapped, programmed-I/O block device: three registers, a 4-byte
 * block number and a 512-byte data area, fetched and stored against a
 * backing file on the far side of the card.  sys/nd.c is the driver,
 * hwsim/d1/nic.c the card.
 */

/* ports (hwsim/d1/nicd1.c) */
#define ND_STATUS	0x60	/* read: op-done + error */
#define ND_CMD		0x61	/* write */
#define ND_DATA		0x63	/* write/read */

/* command register (hwsim/d1/nic.c) */
#define ND_BLOCK	0x00	/* reset the pointer to the block number */
#define ND_RAMDATA	0x01	/* reset the pointer to the data */
#define ND_READ		0x02	/* fetch the block, raise op-done */
#define ND_WRITE	0x03	/* store the block, raise op-done */

/* status register */
#define ND_OPDONE	0x01
#define ND_ERROR	0x02

/* the interrupt line the card raises (the DISK3 spare, sys/ints.h) */
#define NDINT		INT_NIC

#define NND		1	/* one network disk */

/*
 * How many seconds a request may be in flight before it is declared
 * wedged and failed with ENXIO.  The watchdog tick (ndtick in sys/nd.c)
 * runs once a second, so the count is seconds.  It is what turns a probe
 * of a drive whose interrupt is not yet claimed into a skip rather than
 * a hang - the card always answers, but the guest only hears it once
 * ndopen() has claimed the line.
 */
#define NDDEADLINE	1

/*
 * Per-drive state.  There is no geometry: a network block device has a
 * flat block number and nothing else, so this is the HDCA's struct hd
 * with every geometry field gone.  n_buf/n_cur are the request in
 * flight, n_next the queue, and n_intr/n_intarg the op-done continuation.
 */
struct nd {
	UINT8	n_flags;
	UINT8	n_cmd;		/* the ioctl opcode, 0 for an ordinary request */
	UINT8	n_stat;		/* the ioctl's result */
	struct buf *n_buf;	/* the request */
	struct buf *n_cur;	/* the request to iodone */
	int	(*n_intr)();	/* op-done continuation */
	struct nd *n_intarg;	/* its argument */
	struct buf *n_next;	/* queued request */
};

/* n_flags */
#define NDWRITE		0x02
#define NDOPEN		0x20
#define NDWAIT		0x40	/* a process sleeps on this drive */
#define NDERROR		0x80

extern struct nd ndinfo[NND];
