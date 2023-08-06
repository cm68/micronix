/*
 * header file for subroutines responsible for 
 *     setting up and tearing down FIO buffers
 *
 * FIO are the predecessor of FILE
 *
 *  Len Edmondson
 *  Morrow Designs
 *    1981
 *
 *  C library
 *
 * include/fio.h
 *
 * Changed: <2023-07-04 10:41:57 curt>
 */

#define NFIOS 16

FIO stdin;
FIO stdout;
FIO stderr;

FIO *_fios[NFIOS];				/* table of inited FIO's */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
