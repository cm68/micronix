/*
 * pty state
 *
 * sys/ptydata.c
 *
 * The two struct tty of every pty pair and the open counts, resident
 * because they are one object for the whole machine: the queue sleepers,
 * the function pointers the tty layer calls, and p->tty must all name
 * the same storage.  The code that drives them is a module (sys/pty.c)
 * and the stateless helpers are linked into the same page
 * (sys/pty_leaf.c); this is the only part that has to be in the kernel.
 */
#include <types.h>
#include <sys/tty.h>
#include <sys/pty.h>

struct ptyinfo ptyinfo[NPTY];

/* even slot is the slave, odd the master of the same pair */
struct tty ptyttys[2 * NPTY];

/*
 * The ends of each pair, by index.  A load here replaces the multiply
 * by sizeof(struct tty) that the array form costs (sys/pty.h).  NPTY is
 * one, so the initialiser is complete; raising it extends these lists
 * beside the arrays.
 */
struct tty *ptyslave[NPTY] = { &ptyttys[0] };
struct tty *ptymaster[NPTY] = { &ptyttys[1] };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
