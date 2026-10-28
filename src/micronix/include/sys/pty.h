/*
 * pseudo-terminal pairing
 *
 * include/sys/pty.h
 *
 * Shared by the driver proper (sys/pty.c) and its leaf half
 * (sys/pty_leaf.c), both of which are linked into pty.mod.  The pairing
 * is two struct tty back to back: even slot slave, odd slot master,
 * PEER() the other end.
 */
#ifndef PTY_H
#define PTY_H

#define NPTY	1		/* pairs; two struct tty each */
#define PTYMAJ	5		/* a char major; block 5 is a different space */
#define PTYMST	0		/* first master minor */
#define PTYSLV	8		/* first slave minor */

struct ptyinfo {
    UINT8 mopen;		/* opens on the master side */
    UINT8 sopen;		/* opens on the slave side */
    UINT8 hung;			/* master closed under an open slave */
    UINT8 rsv;			/* keeps the array stride a power of two */
};

extern struct ptyinfo ptyinfo[NPTY];
extern struct tty ptyttys[2 * NPTY];

/*
 * The two ends by index.  The tables (sys/ptydata.c) cost one pointer
 * per end and turn the decode into a scaled load; the arithmetic form
 * multiplies by sizeof(struct tty), a stride ccc cannot shift, and so
 * calls amul on every use.  These are entry points, not per-character
 * paths, but the calls are not free either.
 */
extern struct tty *ptyslave[], *ptymaster[];

#define SLAVE(i)	(ptyslave[i])
#define MASTER(i)	(ptymaster[i])
#define ISMASTER(dev)	(((dev) & 0377) < PTYSLV)

/*
 * The other end of a pair.  The two are adjacent, slave first, so the
 * side - which the tty's own dev names - says whether the peer is the
 * element above or below: one load and a constant add.
 *
 * It is written this way because the arithmetic form does not survive
 * ccc: ((t) - ptyttys) is a byte count, so indexing by it costs a
 * division by sizeof(struct tty) and the index scaled back by a
 * multiply.  PEER is on the per-character path (pty_put), so that is
 * one adiv and one qmul for every byte a pty moves.
 */
#define PEER(t)	(((t)->dev & 0377) < PTYSLV ? (t) - 1 : (t) + 1)

extern int ptyidx();

#endif
