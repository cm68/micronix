/*
 * libutil.h - Shared utility functions for ccc
 */
#ifndef LIBUTIL_H
#define LIBUTIL_H

/*
 * Minimal formatter - handles %s, %d, %c, %% (no width/padding)
 * Returns pointer to end of written string
 *
 * There is no %ld here.  A long goes through fmtlong, which is a
 * separate member of the archive so that the long division it needs
 * is linked by the programs that print one and by no others.
 */
char *fmtstr(char *buf, char *fmt, ...);

/* One long, in decimal.  Same contract: terminates, returns the end. */
char *fmtlong(char *buf, long v);

#endif /* LIBUTIL_H */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
