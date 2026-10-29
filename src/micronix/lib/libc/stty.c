/*
 * stty - set a terminal's status
 *
 * lib/libc/stty.c
 *
 * The setting half of gtty(3): an ioctl(2) request, TIOCSETA, moving
 * the same six-byte struct sgtty.
 */
#include <types.h>
#include <sys/sgtty.h>

stty(fd, vec)
    int fd;
    struct sgtty *vec;
{
    return (ioctl(fd, TIOCSETA, vec));
}
