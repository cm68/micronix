/*
 * sio externs
 *
 * /usr/src/cmd/asz/sio.h
 *
 * Changed: <2023-07-05 20:42:37 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
#ifndef SIO_H
#define SIO_H

/* These are the functions needed to interface with the rest of the assembler */
void sio_open();
void sio_close();
char sio_peek();
char sio_next();
void sio_rewind();

void sio_out();

void sio_tmp();
void sio_append();

extern char *infile;
extern int sio_line;

#endif
