/*
 * select - wait until one of a set of descriptors can be read
 *
 * sys/select.c
 *
 * The kernel had no way to wait on two things at once: a driver's read
 * blocks until its own data arrives, and a modem program that must watch
 * both a terminal and a line (sys/net.c's sockets, cu(1)'s line, an
 * rshd's pty and one socket) had to fork a process per direction.  This
 * is the primitive that removes the fork.
 *
 * The argument is a 16-bit set of open-file descriptor numbers, in HL;
 * the answer is the subset that can be read, in HL, or -1 with EINTR if
 * a signal arrived first.  NOPEN is 16, so the mask is the whole table.
 * There is no timeout and no write set: a caller with a deadline uses
 * alarm(2), which is how a program got one before select existed.
 *
 * The alarm is not a special case in here, and it must not become one:
 * the wait is sleep() at PRIUSER, below SIGPRI, so a caught signal sets
 * EINTR through abort() (sys/sig.c) and the syscall returns to the stub
 * with the carry set.  Nothing in select can mask a signal or restart
 * the sleep, and the loop re-checks the descriptors only after it
 * returns; a program that wants a deadline installs a handler, arms the
 * alarm, and treats -1 as "nothing yet".
 *
 * Readable means a read would not block:
 *
 *	tty	RAW or CBREAK: a byte is in the raw queue.
 *		cooked: a line is in the cooked queue, or a break is
 *		in the raw one; TTYHUP or a closed job gate counts,
 *		because the read then returns end of file.
 *	net	a frame is waiting at the card (NETSTAT's RXRDY).  The
 *		card holds it until the driver reads it, so this looks
 *		and does not consume.
 *
 * Nothing else is selectable.  A regular file or a pipe is not a device
 * with a queue of its own here; a caller that wants one waits for it
 * directly.
 *
 * The wait is a sleep on selectchan.  ttyin() and the net's interrupt
 * wake it; no module has to know which fd is being selected, only that
 * the tty or the card has something.  The net line needs one more step:
 * the card's handler masks the line as it wakes the net waiter, so a
 * select watching the card reopens the line (inton) before it sleeps;
 * otherwise the frame after the one just seen would never interrupt.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/con.h>
#include <sys/tty.h>
#include <sys/pty.h>
#include <sys/net.h>
#include <errno.h>

extern struct file *ofile();
extern int sleep(), wakeup();
extern int in(), inton();
extern struct tty mttys[];
extern struct tty *ptyslave[], *ptymaster[];
extern UINT console_minor;
extern UINT netline;            /* the card's interrupt line, sys/netint.c */

/*
 * The event a selector sleeps on.  Resident: every process has its own
 * copy of the u page, and the drivers that wake it live in modules, so
 * the address has to be one the whole machine shares.
 */
int selectchan;

/*
 * A tty with something to read.  The queue counts are read without
 * interrupts off; a count that changes under the read only costs one
 * more pass through select's loop, and the sleep's wakeup comes after
 * the store that counted.
 */
static
ttyready(t)
    register struct tty *t;
{
    if (t == 0)
        return (0);
    if (t->mstate & TTYHUP)
        return (1);
    if (t->mode & (RAW | CBREAK))
        return (t->rawque.count != 0);
    if (t->cokque.count)
        return (1);
    return (t->rawque.count != 0 && t->nbreak != 0);
}

/*
 * A device number that is one of the ttys, ready or not.  The table is
 * resident (sys/ttyres.c, sys/ptydata.c), so this asks it directly
 * rather than through a driver's page.
 */
static
devready(dev)
    int dev;
{
    register int m;

    if (major(dev) == 1) {
        m = minor(dev) & 15;
        if (m == TTY_CONSOLE)
            m = console_minor;
        switch (m) {
        case TTY_SS1:
            return (ttyready(&mttys[0]));
        case TTY_ACE0:
            return (ttyready(&mttys[1]));
        case TTY_ACE1:
            return (ttyready(&mttys[2]));
        case TTY_ACE2:
            return (ttyready(&mttys[3]));
        }
        return (0);
    }
    if (major(dev) == PTYMAJ) {
        m = minor(dev);
        if (m >= PTYMST && m < PTYMST + NPTY)
            return (ttyready(ptymaster[m - PTYMST]));
        if (m >= PTYSLV && m < PTYSLV + NPTY)
            return (ttyready(ptyslave[m - PTYSLV]));
    }
    return (0);
}

/*
 * select(mask in HL): the readable subset in HL.
 */
select()
{
    register UINT mask, bit, r;
    register int fd;
    register struct file *fp;
    UINT netwanted;
    int dev;

    mask = u.hl;
    for (;;) {
        r = 0;
        bit = 1;
        netwanted = 0;
        for (fd = 0; fd < NOPEN; fd++, bit <<= 1) {
            if (!(mask & bit))
                continue;
            fp = ofile(fd);
            if (fp == 0)
                continue;
            if ((fp->inode->i_mode & IFMT) != IFCHR &&
                (fp->inode->i_mode & IFMT) != IFBLK)
                continue;
            dev = fp->inode->i_addr[0];
            if (major(dev) == NET_MAJOR && minor(dev) == NET_MINOR) {
                netwanted = 1;
                if (in(NETSTAT_PORT) & NETSTAT_RXRDY)
                    r |= bit;
            } else if (devready(dev))
                r |= bit;
        }
        if (r != 0 || mask == 0) {
            u.hl = r;
            return;
        }
        if (netwanted && netline != 0xffff)
            inton(netline);
        sleep(&selectchan, PRIUSER);
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
