/*
 * pty state
 *
 * sys/ptydata.c
 *
 * The two struct tty of every pty pair and the open counts, resident
 * because they are one object for the whole machine: the queue sleepers,
 * the function pointers the tty layer calls, and p->tty must all name
 * the same storage.  The code that drives them is a module (sys/pty.c)
 * and the stateless helpers are in the u page (sys/pty_leaf.c); this is
 * the only part that has to be in the kernel.
 */
#include <types.h>
#include <sys/tty.h>
#include <sys/pty.h>

struct ptyinfo ptyinfo[NPTY];

/* even slot is the slave, odd the master of the same pair */
struct tty ptyttys[2 * NPTY];

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
