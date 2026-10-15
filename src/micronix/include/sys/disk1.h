/*
 * CompuPro DISK 1 and DISK 1A floppy controller interface
 *
 * include/sys/disk1.h
 *
 * The DISK 1 is an Intel 8272 (NEC uPD765) floppy disk controller - the
 * chip that owns the head, seeks it, finds a sector's address mark and
 * moves its data - behind a small pile of CompuPro glue: a 24 bit DMA
 * address register, a drive status register, and a motor register that
 * also turns the board's boot EPROM off.  The 8272 itself answers to two
 * of the four ports: its main status register, which is how the host
 * asks whether the chip wants a byte, and its data register, which every
 * command byte and every result byte passes through.
 *
 * The four ports are one block, set by DIP switch S3 and fixed at 0xC0
 * for every software package CompuPro shipped (DISK 1A manual, "CompuPro
 * Drive Interface Address"):
 *
 *	0xC0	read  FDC main status register (MSR)
 *	0xC0	write drive select register (DSR)
 *	0xC1	read/write FDC data register
 *	0xC2	read  drive status register
 *	0xC2	write DMA address register, three bytes, most significant first
 *	0xC3	read  not used
 *	0xC3	write motor control register
 *
 * The DISK 1A carries both drive banks at this one block: four 8 inch
 * drives and four 5 1/4 inch ones, the bank chosen by the DSR's data rate
 * bit, which also halves the 8272's clock (8 MHz to 4 MHz) and so changes
 * the timing the SPECIFY command is given.  The driver therefore presents
 * eight drives, the low three bits of the minor naming them exactly as
 * the DJ-DMA's do: 0-3 the 8 inch bank, 4-7 the 5 1/4.
 *
 * No interrupt line is named here.  Under the probe/attach model
 * (sys/INTR.md) a driver supplies a probe that tickles its card and an
 * interrupt handler, and the resident half discovers which of the S-100
 * vectored lines the card is on at boot.  So this header carries no
 * INT_* and includes no sys/ints.h.
 */

/*
 * The port block.
 */
#define	D1_MSR		0xc0	/* read: FDC main status register */
#define	D1_DSEL		0xc0	/* write: drive select register */
#define	D1_DATA		0xc1	/* read/write: FDC data register */
#define	D1_DSTAT	0xc2	/* read: drive status register */
#define	D1_DMA		0xc2	/* write: DMA address register */
#define	D1_MOTOR	0xc3	/* write: motor control register */

/*
 * The FDC main status register.  RQM is the one that matters: it says
 * the data register holds a byte for the host or wants one from it, and
 * DIO says which of the two.  A byte is written only when RQM is set and
 * DIO clear, and read only when both are set - which is what the boot
 * ROM's poll (IN 0xC0 / OR A / JNS) tests.
 */
#define	D1_RQM		0x80	/* data register ready for a byte */
#define	D1_DIO		0x40	/* ... and the direction is chip to host */
#define	D1_NDMA		0x20	/* non-DMA mode (this board always DMA) */
#define	D1_BUSY		0x10	/* a command is in execution phase */

/*
 * The drive select register (write 0xC0).  It is the board's
 * configuration, not a per-command unit select: the unit comes from the
 * 8272 command byte's US0/US1 like any other 8272, and these bits decide
 * how the board's glue drives the cable.  NORMAL is zero for the first
 * three; the two-sided bit is forced only for a minifloppy whose cable
 * has no side select line, and the data rate bit is the bank.
 */
#define	D1_US0		0x01	/* alternate unit select "0" */
#define	D1_US1		0x02	/* alternate unit select "1" */
#define	D1_ALTSEL	0x04	/* alternate drive select (normal = 0) */
#define	D1_TWOSIDE	0x08	/* force the two sided line (normal = 0) */
#define	D1_ALTLOAD	0x10	/* alternate head load (load = 1) */
#define	D1_FIVE		0x20	/* 5 1/4 inch data rate (8 inch = 0) */

/*
 * The drive status register (read 0xC2).  Bit 7 is the board's copy of
 * the 8272's interrupt pin: it is set while a command's result is
 * waiting to be read, and cleared by reading that result - or, for a
 * command with no result phase (RECALIBRATE, SEEK), by the SENSE
 * INTERRUPT STATUS command.
 */
#define	D1_RDY		0x01	/* drive ready */
#define	D1_INDEX	0x02	/* index pulse, a pulse not a level */
#define	D1_SENSE	0x04	/* board sense switch S3-1, on = 0 */
#define	D1_INT		0x80	/* FDC interrupt pending */

/*
 * The motor control register (write 0xC3).  Writing a zero to bit 0
 * disables the board's boot EPROM, and only a system reset brings it
 * back - which is why the driver writes 1 there and never 0.  Bits 4-6
 * run the four 8 inch motors one for one; bit 7 runs the fourth 8 inch
 * motor and every 5 1/4 inch motor.  The board times the motors out
 * after about fifteen seconds on its own.
 */
#define	D1_BOOTOFF	0x01	/* 0 disables the boot EPROM */
#define	D1_M0		0x10	/* 8 inch drive 0 motor */
#define	D1_M1		0x20	/* 8 inch drive 1 motor */
#define	D1_M2		0x40	/* 8 inch drive 2 motor */
#define	D1_M3		0x80	/* 8 inch drive 3, and every 5 1/4 motor */
#define	D1_MOTORS	0xf0	/* every motor line at once */

/*
 * The 8272's commands.  The low five bits are the opcode; the three
 * above it are modifiers the chip masks off before it decodes, so a
 * dispatcher sees the opcode alone.  MT is multi-track - keep going
 * after the end of the track - MFM is the recording method (clear is
 * FM, single density, which this driver never asks for), and SK skips
 * sectors carrying a deleted data address mark, which a filesystem
 * never wants.
 */
#define	FDC_MT		0x80	/* multi-track */
#define	FDC_MFM		0x40	/* double density */
#define	FDC_SK		0x20	/* skip deleted sectors */

#define	FDC_READTRK	0x02	/* read a whole track into memory */
#define	FDC_SPECIFY	0x03	/* step rate, head load and unload times */
#define	FDC_SENSEDRV	0x04	/* one result byte: the drive's status */
#define	FDC_WRITE	0x05	/* write one sector */
#define	FDC_READ	0x06	/* read one sector */
#define	FDC_RECAL	0x07	/* step out to track 0, no result phase */
#define	FDC_SENSEI	0x08	/* the two byte answer to a recal or seek */
#define	FDC_WRDEL	0x09	/* write a deleted-data sector */
#define	FDC_READID	0x0a	/* the next sector header, in the result */
#define	FDC_READDEL	0x0c	/* read a deleted-data sector */
#define	FDC_FORMAT	0x0d	/* format a whole track from a table */
#define	FDC_SEEK	0x0f	/* seek to a cylinder, no result phase */

/*
 * The read/write command is nine bytes: the opcode, then the drive and
 * head, the cylinder, the head, the sector, the sector size code, the
 * last sector on the track, the gap length, and the data length.  Only
 * the first six say where the sector is; the last three say how the
 * track is laid out around it.  The sector size code is a power of two
 * - 0 is 128 bytes, 1 is 256, 2 is 512 - and the data length is read
 * only when the code is zero, so a driver that asks for 512 byte sectors
 * fills it with 0xff and the chip ignores it.
 */
#define	D1_N		2	/* sector size code: 512 bytes */
#define	D1_NBYTES	512	/* ... which is one filesystem block */
#define	D1_GPL		0x1b	/* MFM gap length for a 512 byte sector */

/*
 * The result bytes every command but SPECIFY, SEEK and RECALIBRATE ends
 * with: status 0, status 1, status 2, then the cylinder, head, sector
 * and size code the chip actually landed on.
 */
#define	ST0_IC		0xc0	/* interrupt code: 0 is a normal finish */
#define	ST0_SE		0x20	/* seek ended */
#define	ST0_EC		0x10	/* equipment check */
#define	ST0_NR		0x08	/* the drive was not ready */
#define	ST0_HD		0x04	/* the head the command used */
#define	ST0_US		0x03	/* the unit the command used */

#define	ST1_EN		0x80	/* end of cylinder */
#define	ST1_DE		0x20	/* data error: a CRC in the data field */
#define	ST1_OR		0x10	/* overrun */
#define	ST1_ND		0x04	/* no data: the sector was not found */
#define	ST1_NW		0x02	/* not writable: the disk is protected */
#define	ST1_MA		0x01	/* missing address mark */
#define	ST1_ERR		0xb7	/* every bit above, ORed up */

#define	ST2_CM		0x40	/* control mark: a deleted data mark */
#define	ST2_DD		0x20	/* a CRC in the sector's data field */
#define	ST2_WC		0x10	/* wrong cylinder */
#define	ST2_BC		0x02	/* bad cylinder */
#define	ST2_MD		0x01	/* missing data mark */
#define	ST2_ERR		0x63	/* ... except the control mark, which a
				   read of ordinary data never sets */

/*
 * Any of these makes a read or a write a failure: the chip did not
 * finish normally, or the drive answered that it could not.
 */
#define	D1_ERRMASK	(ST0_IC | ST0_EC | ST0_NR)

/*
 * vim: set tabstop=4 shiftwidth=4 expandtab:
 */
