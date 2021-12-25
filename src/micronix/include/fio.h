/*
 * header file for subroutines responsible for 
 *     setting up and tearing down FIO buffers
 * XXX - used by what, exactly?
 *
 *	Len Edmondson
 *	Morrow Designs
 * 	  1981
 *
 *	C library
 *
 * include/fio.h
 * Changed: <2021-12-23 15:08:16 curt>
 */

# define NFIOS 16

FIO	stdin;
FIO	stdout;
FIO	stderr;

FIO *_fios [NFIOS];	/* table of inited FIO's */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
