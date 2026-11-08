/*
 * Raw command blocks for block devices
 *
 * include/sys/ioctl.h
 *
 * A controller is told what to do with a command block: the SCSI adapter
 * with a CDB, the IDE card with the seven task-file registers, the HD-DMA
 * board with its own opcode.  The drivers build those themselves for the
 * two commands a filesystem is made of - read this block and write that
 * one - and those are the only two they know.  So nothing above them can
 * ask a drive who it is, what its capacity is, or anything else the
 * filesystem does not need, and that is the gap this closes.
 *
 *	ioctl(fd, CDBCMD, &req)
 *
 * hands the block in req to the driver named by the descriptor's device
 * and lets it run, with the data phase, if the command has one, landing
 * in the caller's buffer.  What the block means is the bus's business and
 * not the kernel's: the kernel copies it, checks its length, moves the
 * data, and never looks inside.
 *
 * The two encodings differ in every way that matters to the driver and in
 * none that matters here.  A SCSI CDB is an array of up to twelve bytes
 * whose first byte is the opcode and whose remaining bytes are the
 * command's own; an IDE command is seven bytes, one per task-file
 * register, which the driver writes out in order.  Both arrive in cmd[]
 * with len saying how many of those bytes are the block, and the driver
 * is the only thing that has an opinion about them.
 *
 * The data phase is one disk block at most when the kernel moves it.  That
 * is not a limit of the buses but of the road: a driver reaches its data
 * through the same one-block window a strategy routine uses (uio.c), so a
 * block is exactly the size the kernel knows how to move.  A command
 * wanting more than that gets no block at all: count names the bytes, the
 * driver is handed the caller's own buf and moves them itself, through a
 * page of its own.  Which of the two it is, the driver can tell from
 * count, and the limit is CDBDATA - one page, since that is the most any
 * staging area here can hold.
 *
 * cmd[] is a way back as well as a way out.  A controller that runs a
 * program out of the command block leaves its per-command status in it -
 * that is where the DJDMA puts the byte each command returns, at a fixed
 * offset inside the command - and the block is copied back out to the
 * caller unchanged when the driver is done, so those bytes arrive.  A
 * caller that sets nothing in cmd[] but its command has nothing to read;
 * one whose bus answers that way has the answer waiting.
 *
 * This is the block half of the old Unix split, and the split is kept: a
 * device number is tagged block or character, and the two are reached
 * through different paths, which is the same reason a raw read of a disk
 * goes through bread while a raw read of a tape goes through the
 * character switch.  A character device's requests are now one ioctl
 * hook of its own: the terminal's status is TIOCGETA/TIOCSETA through
 * stty(3) and gtty(3), and the network card's socket face is the NET*
 * requests of include/sys/net.h.  A driver whose bus has no command
 * block to run answers ENOTTY here rather than pretending otherwise.
 *
 * flags says which way the data goes, since the opcode cannot always be
 * asked: INQUIRY reads, READ CAPACITY reads, and READ(10) reads, but
 * their opcodes are 0x12, 0x25 and 0x28, so the low bit is not the
 * answer.  CDB_IN means the drive fills the buffer; without it the
 * buffer's contents are sent to the drive.
 *
 * The widest command block is the DJ-DMA's: the controller runs a program
 * in its own instructions, and the driver's program space is 26 bytes.
 * A format track is the longest the tree writes - seventeen bytes, SETMP,
 * WRITETRK and the HALT whose status is the block's last byte - which is
 * why the block is wider than the HD-DMA's sixteen.  The HD-DMA's block
 * is twelve bytes of command, the completion status the board writes at
 * cmd[12], and the address it fetches its next command from, which is a
 * physical address and so nobody's to name but the driver's.  A caller
 * says twelve for len and reads the status at cmd[12] afterwards, which
 * is the only way a bus that answers inside the block can be heard when
 * every byte the caller can address is already spoken for.
 */
struct cdb {
    UINT8 cmd[26];               /* the command block, the bus's own format */
    INT16 len;                    /* how many of those bytes are the block */
    INT16 count;                  /* data phase, in bytes; 0 if there is none */
    INT16 flags;                  /* CDB_IN if the data comes back */
    UINT8 *buf;                  /* the caller's buffer for that data */
    INT16 hole;                   /* where in cmd[] its address goes; -1 none */
    /*
     * The addresses inside the block, as a bit vector: bit n set means
     * cmd[n], cmd[n+1], cmd[n+2] are one 24-bit address that the driver
     * has to swizzle - a caller knows its buffer as a virtual address
     * and the controller follows a physical one.  A format's block can
     * name more than the data phase (a sector table, a skipped-sector
     * list), and which words those are differs by command; the caller
     * says so here instead of every driver learning every command.  The
     * value written is the swizzled address of the staged buffer.  hole
     * remains the one-address spelling and is applied the same way.
     */
    UINT32 addrbits;
};

#define CDBMAX  26              /* the widest command block there is */
#define CDBDATA 4096            /* the most data one command may move */
#define CDBCMD  0               /* ioctl(fd, ...): run a command block */
#define CDB_IN  1               /* the data phase reads rather than writes */
#define CDB_ASYNC 2             /* do not wait for the reply (sys/net.c):
                                 * the answer arrives as a pushed frame, so
                                 * a caller that owns the frame stream can
                                 * read it in order with everything else */

/*
 * Addresses in the block: hole and addrbits.
 *
 * A command block can name memory the controller must read or write - the
 * data phase itself, a sector-header table a format lays down, a list of
 * sectors to skip.  The caller knows those as virtual addresses; the bus
 * follows physical ones, and only the kernel can swizzle them: the value
 * is
 *
 *	(segment << 12) | (window address & 0xfff)
 *
 * of the buffer the kernel staged.  Which words of cmd[] are addresses is
 * a property of the command, not of the bus, and the driver cannot know
 * every command - a DJDMA format, a SCSI CDB and an IDE task-file block
 * have nothing in common.  So the caller says so.
 *
 * hole is the single-address spelling: where in cmd[] the data phase's
 * address belongs, three bytes little endian, or -1 if the command has
 * no data phase or the bus keeps the address itself (SCSI's adapter
 * does).  Both drivers have always applied it, and it is the right thing
 * for a command whose only address is its data.
 *
 * addrbits is the general one: bit n set means cmd[n], cmd[n+1] and
 * cmd[n+2] are one 24-bit address to be swizzled.  A word at n with
 * n + 2 >= len is ignored.  The driver writes the swizzled address of
 * the staged data buffer at every flagged word; a caller with several
 * addresses in one block marks each of them, and a caller that wants a
 * zero address in one of them leaves the bit clear (a null sector table
 * is a real thing).  count must be non-zero and no more than CDBDATA for
 * a buffer to be staged and an address to exist.
 *
 * A caller must initialize addrbits.  It is copied in with the rest of
 * the structure and an uninitialized stack word is a request to swizzle
 * whatever bits happen to be set; the command blocks in this tree zero
 * the structure (memset) or the field before filling it in.  Use
 * addrbits for addresses beyond the data phase; hole remains for the
 * old spelling and the two may be used together.  A driver whose bus has
 * no address in its block ignores both, and SCSI callers set hole to -1
 * and leave addrbits zero.
 */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
