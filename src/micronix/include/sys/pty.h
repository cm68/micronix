/*
 * pseudo-terminal pairing
 *
 * include/sys/pty.h
 *
 * Shared by the driver proper (sys/pty.c) and the part of it that lives
 * in the u page (sys/pty_leaf.c).  The pairing is two struct tty back to
 * back: even slot slave, odd slot master, PEER() the other end.
 */
#ifndef PTY_H
#define PTY_H

#define NPTY	1		/* pairs; two struct tty each */
#define PTYMAJ	12		/* a char major clear of the block table */
#define PTYMST	0		/* first master minor */
#define PTYSLV	8		/* first slave minor */

struct ptyinfo {
    UINT8 mopen;		/* opens on the master side */
    UINT8 sopen;		/* opens on the slave side */
    UINT8 hung;			/* master closed under an open slave */
};

extern struct ptyinfo ptyinfo[NPTY];
extern struct tty ptyttys[2 * NPTY];

#define SLAVE(i)	(&ptyttys[2 * (i)])
#define MASTER(i)	(&ptyttys[2 * (i) + 1])
#define PEER(t)		(&ptyttys[((t) - ptyttys) ^ 1])
#define ISMASTER(dev)	(((dev) & 0377) < PTYSLV)

extern int ptyidx();

#endif
