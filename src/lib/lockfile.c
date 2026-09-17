/*
 * advisory lock for disk images
 *
 * lockfile.c
 *
 * An exclusive lock on a sidecar "<image>.lock" file, so two tools - the
 * hardware simulator and the host image tools - never write the same disk
 * image at once.
 *
 * The lock is an flock(2) on the sidecar's fd, not a pid left in the
 * file.  The kernel releases the lock the moment the holder exits, for
 * any reason - a crash, a ^C, a kill -9 - so a dead simulator can never
 * strand a stale lock, and there is no pid to be recycled into some other
 * process and mistaken for the holder.  The holder's pid is written into
 * the file only so an "in use" report can say who to look at.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/*
 * acquire an exclusive lock on the disk image at image.  returns an open
 * fd to hold for the lifetime of the image, or -1 if a live process holds
 * the lock.
 */
int
acquire_lock(const char *image)
{
    char real[PATH_MAX];
    char lockpath[PATH_MAX];
    char pidbuf[32];
    int fd;
    int n;

    if (!realpath(image, real))
        snprintf(real, sizeof(real), "%s", image);
    snprintf(lockpath, sizeof(lockpath), "%s.lock", real);

    fd = open(lockpath, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        fprintf(stderr, "lock: can't open %s: %s\n", lockpath, strerror(errno));
        return -1;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        if (errno == EWOULDBLOCK) {
            int holder = 0;

            /* the pid the current holder wrote in, for the message */
            if (pread(fd, pidbuf, sizeof(pidbuf) - 1, 0) > 0) {
                pidbuf[sizeof(pidbuf) - 1] = 0;
                holder = atoi(pidbuf);
            }
            fprintf(stderr, "%s: in use by pid %d\n", image, holder);
        } else {
            fprintf(stderr, "lock: can't lock %s: %s\n",
                lockpath, strerror(errno));
        }
        close(fd);
        return -1;
    }
    /* we hold it: leave our pid for whoever comes asking */
    n = snprintf(pidbuf, sizeof(pidbuf), "%d\n", getpid());
    (void)ftruncate(fd, 0);
    (void)write(fd, pidbuf, n);
    return fd;
}
