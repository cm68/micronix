/*
 * There is no floating point.
 *
 * ccc has no float or double type and no float constants, so none of
 * the functions this header used to declare can be spelled.  float and
 * double are deliberately left as ordinary identifiers rather than
 * reserved words, so a program that needs them can supply its own:
 *
 *	struct myflt { unsigned long bits; };
 *	typedef struct myflt double;
 *
 * and pass them by pointer.  Binary operators will not work on them,
 * and structures are neither assigned nor returned, so any arithmetic
 * has to be written as calls.
 *
 * A program that does that also supplies its own math.h to match its
 * own representation.  There is no single right one here: the Z80
 * machines that shipped with a floating point unit did not agree on a
 * format, and neither did the BASIC ROMs.
 */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
