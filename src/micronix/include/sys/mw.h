/*
 * Controller command structure
 * See HD-DMA manual
 */
struct hddma_cmd
{
    UINT8 drvsel;               /* out<<4 | drv */
#define STEPOUT		0x10		/* step out toward track 0 */
    UINT steps;                 /* number of steps */
    UINT8 headsel;              /* pcmp<<7 | hicur<<6 | (~head&7)<<2 | drv */
#define PRECOMP		0x80		/* Write precompensation */
#define HIGHCUR		0x40		/* Use high write current */
    UINT dma;                   /* dma address */
    UINT8 xdma;                 /* high byte of 24-bit address */

    union {
        UINT word;
        struct {
            UINT8 low;
            UINT8 high;
        } byte;
    } arg0;
    UINT8 byte2;
    UINT8 byte3;

    UINT8 opcode;               /* op code */
    UINT8 status;               /* completion status */
#define	SENSE_TRK0		0x01	/* zero if trk0 */
#define	SENSE_WFAULT	0x02	/* zero if wfault */
#define	SENSE_READY		0x04	/* zero if ready */
#define	SENSE_SEEKDONE	0x08	/* zero if seek done */
#define	SENSE_INDEX		0x10	/* toggles each rotation */
    UINT link;                  /* address of next command */
    UINT8 xlink;                /* high byte of address */
};

#define	byte0	arg0.byte.low
#define	byte1	arg0.byte.low
#define	word0	arg0.word

/*
 * Controller commands
 */
#define OP_READ			0       /* read sector */
#define OP_WRITE        1		/* write sector */
#define OP_HEADER       2		/* read header */
#define OP_FORMAT		3       /* format */
#define OP_LOAD			4       /* load constants */
#define OP_SENSE		5       /* get drive status */
#define OP_NOP			6		/* noop - recalibrate */

/*
 * Controller constants
 */
#define HOMDEL  30              /* step pulse delay during home in 100 us */
#define SETTLE  0               /* controller head-settle time */
#define INT     0x80            /* Interrupt enable bit with step delay */
#define	SEC512	3               /* 512 byte sectors */

/*
 * port addresses 
 */
#define HDC_RESET   0x54        /* Reset to controller */
#define HDC_ATTN    0x55        /* Attention to controller */

#define LCONST		0x30    /* must be set for LOAD constants command */
#define NOTRDY		4       /* bit 2 of sense status */
#define BUSY		0       /* controller is busy */
#define INTOFF		0       /* turn off completion interrupts */

/* status bits */
