/*
 * source input/output adapter
 *
 * /usr/src/cmd/asz/sio.c
 *
 * this file mostly rewritten because it had truly lame file name
 * handling:  output only went to a.out, and all specified files
 * were assembled into it.
 *
 * now, instead, for a file foo.s, we write foo.o as the gods intended
 *
 * Changed: <2023-07-05 18:16:24 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

#include "sio.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * buffer state stuff 
 */
char sio_buf[512];
int sio_bufc;
int sio_bufi;

/*
 * current line number 
 */
int sio_line;

FILE *sio_curr;         /* input */
FILE *sio_fout;         /* output */
FILE *sio_ftmp;         /* temp */

char *infile;
char outfile[32];
char tname[32];

/*
 * given a source file, open it, the output file, and the temp file
 */
void
sio_open(filename)
char *filename;
{
    char *d = outfile;

    infile = filename;

    while (*filename) {
        if ((*d++ = *filename++) == '.') break;
    }
    if (!*filename) {
        *d++ = '.';
    }
    *d++ = 'o';
    *d++ = '\0';

	if (!(sio_curr = fopen(infile, "r"))) {
		printf("cannot open source file %s\n", filename);
		exit(1);
	}
    
	sprintf(tname, "/tmp/atm%d", getpid());
	if (!(sio_ftmp = fopen(tname, "wb"))) {
		printf("cannot open tmp file %s\n", tname);
		exit(1);
	}

	if (!(sio_fout = fopen(outfile, "wb"))) {
		printf("cannot open output %s\n", outfile);
		exit(1);
	}

	sio_rewind();
}

/*
 * closes source files when done
 */
void
sio_close()
{
	if (sio_curr)
		fclose(sio_curr);
	fclose(sio_fout);
	fclose(sio_ftmp);
	sio_curr = NULL;
	sio_fout = NULL;
	sio_ftmp = NULL;

	remove(tname);
}

/*
 * returns what sio_next() would but does not move forward
 */
char
sio_peek()
{
    char c;
    c = fgetc(sio_curr);
    ungetc(c, sio_curr);

    return c;
}

/*
 * returns the next character in the source, or -1 if complete
 */
char
sio_next()
{
    char c;

    c = fgetc(sio_curr);

	if (c == '\n')
		sio_line++;

	return c;
}

/*
 * brings the file pointer back to the beginning of source
 */
void
sio_rewind()
{
    rewind(sio_curr);
}

/*
 * outputs a byte onto output file
 *
 * out = byte to output
 */
void
sio_out(out)
char out;
{
	fwrite(&out, 1, 1, sio_fout);
}

/*
 * writes a byte to the temp file
 *
 * tmp = byte to write to tmp
 */
void
sio_tmp(tmp)
char tmp;
{
	fwrite(&tmp, 1, 1, sio_ftmp);
}

/*
 * appends contents of tmp file to output file
 */
void
sio_append()
{
	char c;

	fclose(sio_ftmp);

	if (!(sio_ftmp = fopen(tname, "rb"))) {
		printf("cannot open tmp file\n");
		exit(1);
	}

	while (0 < fread(&c, 1, 1, sio_ftmp))
		sio_out(c);

	fclose(sio_ftmp);
	if (!(sio_ftmp = fopen(tname, "wb"))) {
		printf("cannot open tmp file\n");
		exit(1);
	}
}
