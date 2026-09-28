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
 * character switch.  A character device's requests are the terminal
 * driver's own calls - stty and gtty, sys/cio.c, which is what ioctl(2)
 * was in the sixth edition - and a driver whose bus has no command block
 * to run answers ENOTTY here rather than pretending otherwise.
 *
 * flags says which way the data goes, since the opcode cannot always be
 * asked: INQUIRY reads, READ CAPACITY reads, and READ(10) reads, but
 * their opcodes are 0x12, 0x25 and 0x28, so the low bit is not the
 * answer.  CDB_IN means the drive fills the buffer; without it the
 * buffer's contents are sent to the drive.
 */
struct cdb {
    char cmd[12];               /* the command block, the bus's own format */
    int len;                    /* how many of those bytes are the block */
    int count;                  /* data phase, in bytes; 0 if there is none */
    int flags;                  /* CDB_IN if the data comes back */
    char *buf;                  /* the caller's buffer for that data */
    int hole;                   /* where in cmd[] its address goes; -1 none */
};

#define CDBMAX  12              /* the widest command block there is */
#define CDBDATA 4096            /* the most data one command may move */
#define CDBCMD  0               /* ioctl(fd, ...): run a command block */
#define CDB_IN  1               /* the data phase reads rather than writes */

/*
 * hole is the caller's, not the kernel's, and the kernel only carries it:
 * it says where in the command block the address of the data phase belongs,
 * three bytes little endian, and the driver writes them there.  buf is a
 * caller's virtual address and a bus wants a physical one, so the driver
 * is the only one that can supply it, and a bus whose block has no room
 * for an address - SCSI's, which the adapter holds itself - says so with
 * hole set to -1 and is not asked for one.
 */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
