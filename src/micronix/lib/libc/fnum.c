/*
 * _fnum() - the %e %f %g conversion for _doprnt().
 *
 * This is a stub.  There is no floating point: ccc has no float or
 * double type and no float constants, and there is no one Z80 float
 * format worth building in - the machines that shipped with a
 * floating point unit did not agree on one, and neither did the BASIC
 * ROMs.
 *
 * A program that wants these conversions declares its own
 * representation (see math.h), links its own _fnum, and sets FLTSIZE
 * in doprnt.c to the width varargs pushes for it.  Its own object
 * satisfies the reference before the linker reaches this archive
 * member, so nothing here has to be removed for that to work.
 *
 * The argument arrives by address, since a struct is neither passed
 * by value nor returned.
 */

#define	putc(c)	(*pputc)(c)

_fnum(val, prec, width, efmt, pputc)
char *			val;
char			prec;
unsigned char		width;
unsigned char		efmt;
void			(*pputc)();
{
	putc('?');
	return 1;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
