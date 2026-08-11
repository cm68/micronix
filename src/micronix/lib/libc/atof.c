/*
 * _atof() - the %e %f %g conversion for _doscan().
 *
 * This is a stub, and the counterpart of the one in fnum.c.  There is
 * no floating point: ccc has no float or double type and no float
 * constants, and there is no one Z80 float format worth building in.
 *
 * A program that wants scanf to read these declares its own
 * representation (see math.h) and links its own _atof.  Its own
 * object satisfies the reference before the linker reaches this
 * archive member, so nothing here has to be removed for that to work.
 *
 * Both the text and the destination arrive by address, since a struct
 * is neither passed by value nor returned.
 *
 * atof() itself is gone: it returned a double, so it cannot be
 * declared, and stdlib.h no longer names it.
 */

_atof(s, dest)
char *	s;
char *	dest;
{
	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
