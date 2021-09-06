/*
 * rdwri.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <errno.h>

/*
 * The pipe size at which the writing process is suspended.
 */
#define PIPSIZE 4096

/*
 * Read system call
 */
read(fd, buf, count)
    int fd, buf, count;
{
    return rdwri(IREAD, fd, buf, count);
}

/*
 * Write system call
 */
write(fd, buf, count)
    int fd, buf, count;
{
    return rdwri(IWRITE, fd, buf, count);
}

/*
 * Common code for read/write system calls.
 * Transfer count bytes between open file fd
 * and memory buf. Errors: file not open for
 * desired access.
 */
rdwri(rw, fd, buf, count)
    int rw, fd, buf, count;
{
    register struct inode *ip;
    register struct file *fp;
    static char mode;
    static int nbytes;

    if (!valid(buf, count))     /* is user memory allocated? */
        return;
    if ((fp = ofile(fd)) == 0)       /* bad descriptor */
        return;
    mode = fp->mode;            /* IREAD | IWRITE | PIPE */
    if ((rw & mode) == 0) {
        u.error = EBADF;
        return;
    }
    if (count == 0)
        return (0);
    ip = fp->inode;
    u.base = buf;
    u.count = count;
    u.offset = fp->rwptr;
    u.segflg = USEG;
    if (mode & PIPE)
        nbytes = (rw == IREAD) ? pread(ip) : pwrite(ip);
    else
        nbytes = (rw == IREAD) ? iread(ip) : iwrite(ip);
    fp->rwptr = u.offset;
    return (nbytes);
}

/*
 * Write a pipe. Use the file size as the write pointer.
 * If there is no reader, signal and return error.
 */
pwrite(ip)
    register struct inode *ip;
{
    register int request, nleft;

    request = nleft = u.count;
  lock:
    ilock(ip);
    if (ip->count < 2) {        /* no reader */
        irelse(ip);
        u.error = EPIPE;
        send(u.p, SIGPIPE);
        return;
    }
    while (nleft != 0) {
        if (ip->size >= PIPSIZE) {
            irelse(ip);         /* wakes reader */
            ip->flags |= IWANT;
            sleep(ip, PRIPIPE); /* wait for a read */
            goto lock;
        }
        u.offset = ip->size;
        u.count = min(PIPSIZE - ip->size, nleft);
        nleft -= iwrite(ip);
        if (u.error)
            break;
    }
    irelse(ip);                 /* wakes reader */
    return (request - nleft);
}

/*
 * Read a pipe. If the pipe is empty, wake the writer and
 * sleep, or if there is no writer, return eof.
 * If the pipe empties during the read, return without
 * waiting for more. This allows interactive pipes.
 */
pread(ip)
    register struct inode *ip;
{
    register int nbytes;

  lock:
    ilock(ip);
    if (u.offset >= ip->size) { /* pipe empty */
        if (ip->count < 2) {    /* no writer */
            nbytes = 0;
            goto done;
        }
        u.offset = ip->size = 0;
        irelse(ip);             /* wakes writer */
        ip->flags |= IWANT;
        sleep(ip, PRIPIPE);     /* wait for a write */
        goto lock;
    }
    nbytes = iread(ip);
  done:
    irelse(ip);                 /* wakes writer */
    return (nbytes);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
