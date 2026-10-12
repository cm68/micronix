/*
 * system call wrappers with real register or carry convention work.
 *
 * The trivial marshal-and-call wrappers (r_dup, r_read, r_open, ...)
 * used to live here too; their marshalling is folded into the workers
 * now, so syssw points at the workers directly.  What is left is the
 * register work that is not a plain word: the trap entry (r_system),
 * fork's parent/child return, the carry and error convention (r_ioctl,
 * r_seek, r_ftruncate), the two-word pipe and time returns, and the two
 * workers that are also called internally (r_close, r_csw).
 *
 * sys/reg.c
 * Changed: <2022-01-04 10:43:12 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>

extern long seconds;

extern int arg[];               /* in-line arguments from system() */

/*
 * The return type has to be said, and this is the only caller that
 * needs it.  An undeclared function returns int, so seek's position
 * would be read out of HL alone and everything above 64K - the whole
 * reason r_seek hands back two words - would arrive as its low half.
 */
extern UINT32 seek(int fd, UINT disp, unsigned from);
extern int ftruncate(int fd, UINT32 length);

/*
 * First, pass registers across the system interface
 */
r_system()
{
    system();
    if (u.error) {
        u.af |= ERRBIT;
        u.hl = u.error;
    } else
        u.af &= ~ERRBIT;
}

r_close()
{
    close(u.hl);
}

r_csw()
{
    u.hl = csw();
}

r_fork()
{
    int id;

    id = fork();
    if (id != 0)                /* no reg changes in child */
        u.pc += 3;              /* old process return */
    u.hl = id;                  /* parent gets pid, child gets 0 */
}

/*
 * ioctl(fd, cmd, arg) - run a raw command block against a block device
 * (include/sys/ioctl.h).  The two arguments are the request code and the
 * caller's structure; bioctl is where the copying, the block and the
 * data movement live, and this is only the register convention.
 *
 * The error path is r_seek's, and for the same reason: the stubs decide
 * success and failure on the carry, not on the value, so a failure has
 * to raise it and put the reason in hl for the stub to store in errno.
 */
r_ioctl()
{
    u.hl = bioctl(u.hl, arg[0], arg[1]);
    if (u.error) {
        u.af |= ERRBIT;
        u.hl = u.error;
    } else {
        u.af &= ~ERRBIT;
    }
}

r_pipe()
{
    int vec[2];

    pipe(vec);
    u.hl = vec[0];
    u.de = vec[1];
}

/*
 * seek returns the new position, so the carry has to mean something:
 * r_system's idiom, and for the same reason - the stubs decide success
 * and failure on it.
 *
 * The position goes back as HL':HL - high half in the shadow bank's HL,
 * low half in HL - which is how the compiler passes and returns a long.
 * DE is deliberately left alone.  It is where a Whitesmiths binary keeps
 * its frame pointer, and unlike time() and wait(), whose long returns
 * the original kernel also put in de, seek's original returned nothing
 * at all: an lseek stub from the distribution reads _fdpos and no
 * register, so it has no save of DE to give back and would lose its
 * frame to this call.  Keeping the whole answer in HL':HL costs the
 * stub nothing and is a convention old binaries are already safe under.
 */
r_seek()
{
    UINT32 off;

    off = seek(u.hl, arg[0], arg[1]);
    if (u.error) {
        u.af |= ERRBIT;
        u.hl = u.error;
    } else {
        u.af &= ~ERRBIT;
        u.hl = (UINT) off;
        u.hl2 = (UINT) (off >> 16);
    }
}

r_ftruncate()
{
    UINT32 length;

    length = ((UINT32) arg[1] << 16) | (UINT32) arg[0];
    u.hl = ftruncate(u.hl, length);
    if (u.error) {
        u.af |= ERRBIT;
        u.hl = u.error;
    } else {
        u.af &= ~ERRBIT;
        u.hl = 0;
    }
}

r_time()
{
    u.de = seconds;
    u.hl = seconds >> 16;
}

/*
 * r_lock () { reclock (u.hl, arg [0]); }
 *
 * r_unlock () { unlock (u.hl); }
 */
/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
