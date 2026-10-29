/*
 * gtty - get a terminal's status
 *
 * lib/libc/gtty.c
 *
 * A terminal is a character device, so its status is an ioctl(2)
 * request - TIOCGETA - and the six-byte struct sgtty is the block the
 * driver moves.  stty(3) is the other half.  Both used to be system
 * calls; the kernel now has one character-device ioctl and these are
 * two of its requests.
 */
#include <types.h>
#include <sys/sgtty.h>

gtty(fd, vec)
    int fd;
    struct sgtty *vec;
{
    return (ioctl(fd, TIOCGETA, vec));
}
