/*
 * more random system calls
 *
 * sys/sys2.c 
 * Changed: <2022-01-04 10:53:07 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/pic8259.h>
extern UINT8 super(void);
extern void istat(struct inode *, INT16 *);
extern void ilock(struct inode *);
extern void irelse(struct inode *);
extern void sleep(INT16, UINT8);
extern UINT8 valid(UINT16, UINT16);

extern INT32 seconds;
extern void itruncat(struct inode *, UINT32);

/*
 * Stat system call
 */

/* prototypes for functions defined below */
INT16 copyout(char *, char *, int);
INT16 procid(struct proc *);
struct inode * iname(UINT8 *);
void x4to3(char *, char *);
void
stat(name, buf)
    UINT8 *name, *buf;
{
    register struct inode *ip;

    if ((ip = iname(name)) != 0)
        istat(ip, buf);
    u.hl = 0;                   /* success: the caller's wrapper reads HL */
}

/*
 * Fstat system call
 */
void
fstat(buf)
    UINT8 *buf;
{
    register struct file *fp;

    if ((fp = ofile(u.hl)) == 0)
        return;
    ilock(fp->inode);
    istat(fp->inode, buf);
}

/*
 * Common code for stat and fstat
 */
void
istat(ip, buf)
    struct inode *ip;
    INT16 *buf;
{
    if (!valid(buf, 36))
        return;
    x4to3(&ip->i_size, &ip->i_size0);       /* update 3-byte size */
    copyout(ip, buf, 36);
    irelse(ip);
}

/*
 * Getpid system call
 */
void
getpid()
{
    u.hl = procid(u.p);
}

/*
 * Stime system call
 */
void
stime()
{
    if (!super())
        return;
    di();
    /*
     * The long's low word is u.de, its high word u.hl.  Packing it with
     * `u.hl << 16` is a qshl loop of sixteen shifts for a word move.
     */
    {
        union { INT32 l; UINT16 w[2]; } t;

        t.w[0] = u.hl;
        t.w[1] = u.de;
        seconds = t.l;
    }
    ei();
    (*rtcwrite)(seconds);
}

/*
 * Seek system call
 * Seek(character-special, 0, 1 or 3) fails.
 *
 * Returns the new position.  That is the kernel's half of the contract
 * that lets libunix drop the _fdpos[] shadow table it keeps: user space
 * cannot know where a descriptor stands, because dup shares one rwptr
 * between two fds and an exec carries a position across with no record
 * of it.  On failure, -1 with u.error set, which r_seek turns into the
 * carry the stubs test.
 */
UINT32
seek(fd, disp, from)
    UINT8 fd;
    register UINT16 disp;         /* block numbers are unsigned short */
    register UINT8 from;
{
    struct file *fp;
    static UINT32 offset;       /* file positions are unsigned (to ~15M) */

    if ((fp = ofile(fd)) == 0)
        goto bad;               /* ofile set u.error = EBADF */
    if (fp->mode & PIPE) {
        u.error = ESPIPE;
        goto bad;
    }
    /*
     * The whences are 0..5 and nothing else.  "from %= 6" mapped
     * 6, 7, ... onto them and quietly accepted a whence the caller
     * never wrote, and it cost a sixteen-bit divide to do it.  A byte
     * range test rejects the lot for two bytes.
     */
    if (from >= 6) {
        u.error = EINVAL;
        goto bad;
    }
    offset = disp;
    if (from >= 3)
        offset <<= 9;
    switch (from) {
    case 0:
    case 3:
        break;
    case 1:
    case 4:
        offset += fp->rwptr;
        break;
    case 2:
    case 5:
        offset += fp->inode->size;
        break;
    }
    /*
     * The "before the start" test is done by lseek() before it reaches
     * here: seekraw() is only ever handed a non-negative block or byte
     * displacement, so offset is already non-negative and the unsigned
     * arithmetic above is exact.  A signed "offset < 0" here would
     * wrongly reject any position with bit 15 set.
     */
    fp->rwptr = offset;
    return (offset);
    /*
     * One long -1 for all three failures.  Each return of it would
     * otherwise be the two 0xffff words and the two exx that put them
     * in HL' and HL - eight bytes - and a jump to the exit; three
     * copies cost more than the three gotos and this block, and every
     * path here has u.error set already.
     */
  bad:
    return (-1);
}

/*
 * Truncate an open file to length bytes, freeing the blocks beyond it.
 * Shrinks only: a length at or past the current size is a no-op.  The
 * file must be open for writing and be a regular file.
 */
INT8
ftruncate(fd, length)
    UINT8 fd;
    UINT32 length;
{
    register struct file *fp;
    register struct inode *ip;

    if ((fp = ofile(fd)) == 0)
        return (-1);            /* ofile set u.error = EBADF */
    if ((fp->mode & IWRITE) == 0) {
        u.error = EBADF;
        return (-1);
    }
    ip = fp->inode;
    if ((ip->i_mode & IFMT) != IFREG) {
        u.error = EINVAL;
        return (-1);
    }
    ilock(ip);
    if (length >= ip->i_size) {
        irelse(ip);
        return (0);
    }
    itruncat(ip, length);
    irelse(ip);
    return (0);
}

/*
 * Sleep system call (not internal sleep)
 */
void
snooze()
{
    register UINT16 secs;
    INT32 waketime;
    extern INT16 revel;

    secs = u.hl;
    di();
    waketime = seconds + secs;
    while (waketime > seconds) {
        secs = waketime - seconds;

        if (revel <= 0 || secs < revel) {
            revel = secs;
        }

        sleep(&revel, PRIUSER);
        di();
    }
    ei();
}

/*
 * Alarm system call.
 * Signal is sent in clock() (time.c).
 */

void
alarm()
{
    static UINT16 old;

    old = u.p->alarm;
    u.p->alarm = u.hl;

    u.hl = old;
}

/*
 * Pause system call
 */
void
pause()
{
    sleep(&pause, PRIUSER);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
