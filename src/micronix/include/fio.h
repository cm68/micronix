/*
 * fio.h - header file for subroutines responsible for 
 *		setting up and tearing down FIO buffers
 *
 *	Len Edmondson
 *	Morrow Designs
 * 	  1981
 *
 *	C library
 *
 * XXX - and where is this used
 */

# define NFIOS 16

FIO	stdin;
FIO	stdout;
FIO	stderr;

FIO *_fios [NFIOS];	/* table of inited FIO's */
