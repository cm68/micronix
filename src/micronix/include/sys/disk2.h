/*
 * CompuPro DISK 2 controller interface
 *
 * include/sys/disk2.h
 *
 * The DISK 2 is a hard disk controller for SA4000/Fujitsu 2300 series
 * drives (5 MHz MFM).  Unlike the DISK 3 it is not intelligent: a finite
 * state machine on the board searches for a sector header and moves the
 * data, but the host does the seek itself, one step pulse at a time, and
 * every DMA transfer goes through a second board, the SELECTOR CHANNEL,
 * which the host programs with the transfer's address before each command.
 *
 * The host interface is two ports.  The low one is the control/status
 * register - written to command, read for status.  The high one is the
 * data port - written to load a data register, read to step the head one
 * track.  Four data registers, chosen by the OP1/OP0 bits in the control
 * register, hold the drive and head selection, the cylinder, the head and
 * the sector to search for.
 *
 * The ports are switch selectable; the CompuPro standard is 0xc8/0xc9.
 * The SELECTOR CHANNEL has a port of its own, also switch selectable, and
 * it is what the address of every DMA transfer is written to.
 */
#define D2_CTL   0xc8            /* control (write) / status (read) */
#define D2_DATA  0xc9            /* data (write) / step (read) */
#define D2_SC    0xe0            /* the SELECTOR CHANNEL's port */

/*
 * The control register (written).  Bits 0-1 are the unit select, the same
 * drive number the DRIVE register's high nibble names; bits 3-5 are the
 * opcode when RUN is set and the register select when it is not; FAULT
 * CLR must be zero to run a command; and ATTN* high makes the status
 * register transparent, which is how the board is marked busy.
 */
#define D2_US0    0x01           /* unit select, bit 0 */
#define D2_US1    0x02           /* unit select, bit 1 */
#define D2_FCLR   0x04           /* FAULT CLR: 0 to run a command */
#define D2_OP0    0x08           /* opcode bit 0, also the register select */
#define D2_OP1    0x10           /* opcode bit 1, also the register select */
#define D2_OP2    0x20           /* opcode bit 2, also DIRECTION while stepping */
#define D2_RUN    0x40           /* 1 = automatic operation */
#define D2_ATTN   0x80           /* 1 = make the status transparent (busy) */

/*
 * The four data registers, written through the data port after the
 * control register's OP1/OP0 has named them.  The DRIVE register selects
 * the drive in its high nibble (bit 4 is drive 0) and the head in its low
 * nibble; the other three are single bytes matched against the sector
 * header - so a drive with more than 255 cylinders cannot be addressed,
 * which is a limit of this board, not of the disk.
 */
#define D2_REG_DRIVE 0x00        /* OP1:OP0 = 0 0 */
#define D2_REG_CYL   0x08        /* OP1:OP0 = 0 1 */
#define D2_REG_HEAD  0x10        /* OP1:OP0 = 1 0 */
#define D2_REG_SECT  0x18        /* OP1:OP0 = 1 1 */

/*
 * The status register (read).  ATTN* low means the command finished and
 * the rest of the bits are latched; the other bits are the result.  The
 * four drive signals are active low.
 */
#define D2_TRK0    0x01           /* track 0 (0 = at track 0) */
#define D2_WFLT    0x02           /* write fault (0 = fault) */
#define D2_SEEKC   0x04           /* seek complete (0 = complete) */
#define D2_READY   0x08           /* drive ready (0 = ready) */
#define D2_OVRUN   0x10           /* over run on the bus */
#define D2_CRC     0x20           /* CRC error */
#define D2_TIMEO   0x40           /* timed out */
#define D2_SATTN   0x80           /* ATTN*: 0 = done, 1 = busy */

/*
 * A successful transfer leaves every error bit clear: no time out, no
 * CRC, no overrun, the drive ready and the seek complete.  The write
 * fault bit is active low and is checked separately on a write.
 */
#define D2_ERRMASK (D2_TIMEO | D2_CRC | D2_OVRUN | D2_READY | D2_SEEKC)

/*
 * The commands (opcode in OP2-OP0, with RUN set).  NULL only waits for
 * SEEK COMPLETE and times out after two index pulses, so it is how the
 * driver lets the head settle after a seek; READ and WRITE move one
 * sector between the disk and the address the SELECTOR CHANNEL holds.
 */
#define D2_NULL    0x00           /* 0 0 0 */
#define D2_READ    0x08           /* 0 0 1: read a sector */
#define D2_WRITE   0x10           /* 0 1 0: write a sector */
#define D2_WRHDR   0x18           /* 0 1 1: write a header (format) */
#define D2_RDHDR   0x20           /* 1 0 0: read a header */

/*
 * The SELECTOR CHANNEL's mode byte, the fourth of the four bytes written
 * to its port.  The high bit is W/R*: low writes to memory (a disk read)
 * and high reads it (a disk write).  The low nibble is the complement of
 * the DISK 2's DMA priority, so the channel arbitrates for the one board
 * it belongs to.  The rest stay low - memory, counting up, no wait state.
 */
#define D2_WRMEM   0x00           /* W/R* low: DMA writes memory (disk read) */
#define D2_RDMEM   0x80           /* W/R* high: DMA reads memory (disk write) */
#define D2_DMAPRIO 9              /* the board's DMA priority, jumpered */
#define D2_SCID    (~D2_DMAPRIO & 0x0f)  /* the channel's complement nibble */
