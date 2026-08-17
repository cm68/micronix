/*
 * advisory lock for disk images
 *
 * lockfile.c
 *
 * A PID lockfile keyed on the image path, so two tools - the hardware
 * simulator and the host image tools - never write the same disk image at
 * once.  The lock is a sidecar "<image>.lock" holding the holder's PID.
 * A lock whose PID is no longer alive is stale and is taken over silently.
 */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

/*
 * is process pid alive?  1 yes, 0 no.
 */
static int
pid_alive(int pid)
{
    if (pid <= 0)
        return 0;
    if (kill(pid, 0) == 0)
        return 1;
    return errno == EPERM;	/* alive, just not ours to signal */
}

/*
 * read a pid from the lock file.  returns the pid, or 0 if the file
 * cannot be read as one.
 */
static int
read_lock_pid(char *path)
{
    FILE *fp;
    int pid = 0;

    fp = fopen(path, "r");
    if (!fp)
        return 0;
    if (fscanf(fp, "%d", &pid) != 1)
        pid = 0;
    fclose(fp);
    return pid;
}

/*
 * acquire an exclusive lock on the disk image at image.  returns an open
 * fd to hold for the lifetime of the image, or -1 if a live process holds
 * the lock.  a stale lock - one whose pid has gone - is taken over without
 * a word.
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

    for (;;) {
        int holder;

        fd = open(lockpath, O_RDWR | O_CREAT | O_EXCL, 0666);
        if (fd >= 0) {
            n = snprintf(pidbuf, sizeof(pidbuf), "%d\n", getpid());
            (void)write(fd, pidbuf, n);
            return fd;
        }
        if (errno != EEXIST) {
            fprintf(stderr, "lock: can't create %s: %s\n",
                lockpath, strerror(errno));
            return -1;
        }
        holder = read_lock_pid(lockpath);
        if (pid_alive(holder)) {
            fprintf(stderr, "%s: in use by pid %d\n", image, holder);
            return -1;
        }
        /* stale: take it over silently */
        if (unlink(lockpath) < 0 && errno != ENOENT) {
            fprintf(stderr, "lock: can't remove %s: %s\n",
                lockpath, strerror(errno));
            return -1;
        }
    }
}
