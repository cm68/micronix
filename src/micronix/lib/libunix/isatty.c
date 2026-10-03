/*
 * isatty.c - is this descriptor a terminal
 *
 * gtty is the v6 ioctl and the only question a descriptor can be
 * asked here: the driver answers it for a terminal and fails for
 * anything else, which is exactly what isatty wants to know.
 *
 * The buffer is the six bytes gtty writes - two speeds, erase, kill
 * and a mode word.  It is declared as bytes rather than as struct
 * sgtty so that including <stdio.h> is enough to call this: sgtty.h
 * is built out of the UINT8 and UINT in types.h, and a program that
 * only wants to know whether it is talking to a tty should not have
 * to pull the system's type vocabulary in to ask.
 */

isatty(fd)
int fd;
{
	char buf[6];

	return gtty(fd, buf) == 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
