/*
 * mio.h - Colin Kelley  1-18-87
 *   routines to emulate temporary file handling with memory instead
 * part of zmac
 *
 * tools/mio.h
 * Changed: <2021-12-23 16:03:23 curt>
 */

FILE * mfopen(char *filename, char *mode);

int mfclose(FILE *f);

unsigned int mfputc(unsigned int c, FILE *f);

unsigned int mfgetc(FILE *f);

int mfseek(FILE *f,long loc,int origin);

int mfread(char *ptr, unsigned int size, unsigned int nitems,FILE *f);

int mfwrite(char *ptr, int size, int nitems, FILE *f);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
