/*
 * fio.h - header file for subroutines responsible for 
 *		setting up and tearing down FIO buffers
 *
 *	Len Edmondson
 *	Morrow Designs
 * 	  1981
 *
 *	C library
 */

# define NFIOS 16

FIO	stdin,
	stdout;

FIO *_fios [NFIOS];	/* table of inited FIO's */
