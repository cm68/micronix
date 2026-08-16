/*
 * Characteristics of floating types - there are none.
 *
 * ccc has no float or double type, so this header has nothing to
 * describe.  It also could not describe anything: every limit it used
 * to define (FLT_EPSILON, DBL_MIN, DBL_MAX and the rest) is spelled as
 * a float constant, and float constants no longer lex.
 *
 * See math.h for what a program that needs floating point does
 * instead, and why the choice of format is left to it.
 */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
