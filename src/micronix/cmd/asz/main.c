/*
 * trasm main file - arg processing and file handling
 *
 * /usr/src/cmd/asz/main.c
 *
 * this file has interpolated the original sio.c
 * because it became almost trivial after the buffer stuff was
 * stripped out
 *
 * this file mostly rewritten because it had truly lame file name
 * handling:  output only went to a.out, and all specified files
 * were assembled into it.
 *
 * now, instead, for a file foo.s, we write foo.o as the gods intended
 *
 * Changed: <2023-07-09 14:20:00 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

#include <stdio.h>

#ifdef linux
#include <stdlib.h>
#include <unistd.h>
#define INIT
#else
#define INIT = 0
#define void int
#endif

#include "asm.h"

char verbose INIT;
char g_flag INIT;

char *progname INIT;

int line_num INIT;

FILE *input_file INIT;
FILE *output_file INIT;
FILE *temp_file INIT;

char *infile INIT;
char outfile[32] INIT;
char tname[32] INIT;

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

	if (!(input_file = fopen(infile, "r"))) {
		printf("cannot open source file %s\n", filename);
		exit(1);
	}
    if (verbose) printf("source %s\n", infile);
    
	sprintf(tname, "/tmp/atm%d", getpid());
	if (!(temp_file = fopen(tname, "wb"))) {
		printf("cannot open tmp file %s\n", tname);
		exit(1);
	}

	if (!(output_file = fopen(outfile, "wb"))) {
		printf("cannot open output %s\n", outfile);
		exit(1);
	}

	rewind(input_file);
}

/*
 * closes source files when done
 */
void
sio_close()
{
	if (input_file)
		fclose(input_file);
	fclose(output_file);
	fclose(temp_file);
	input_file = NULL;
	output_file = NULL;
	temp_file = NULL;

	remove(tname);
}

/*
 * outputs a byte onto output file
 *
 * out = byte to output
 */
void
outbyte(out)
char out;
{
	fwrite(&out, 1, 1, output_file);
}

/*
 * writes a byte to the temp file
 *
 * tmp = byte to write to tmp
 */
void
outtmp(tmp)
char tmp;
{
	fwrite(&tmp, 1, 1, temp_file);
}

/*
 * appends contents of tmp file to output file
 */
void
appendtmp()
{
	char c;

	fclose(temp_file);

	if (!(temp_file = fopen(tname, "rb"))) {
		printf("cannot open tmp file\n");
		exit(1);
	}

	while (0 < fread(&c, 1, 1, temp_file))
		fwrite(&c, 1, 1, output_file);

	fclose(temp_file);
	if (!(temp_file = fopen(tname, "wb"))) {
		printf("cannot open tmp file\n");
		exit(1);
	}
}

/*
 * print usage message
 */
void
usage()
{
	printf("usage: %s [-vg] source.s ...\n", progname);
	exit(1);
}

int
main(argc, argv)
int argc;
char **argv;
{
    char *s;

    progname = *argv;
    argv++;
    argc--;
 
	while (argc) {
        s = *argv;

		if (*s++ != '-') {
            break;
        }
        argv++;
        argc--;
 
	    while (*s) {
            switch (*s++) {
			case 'g':
				g_flag++;
				break;

			case 'v':
				verbose++;
				break;

			default:
				usage();
			}
		}
	}

    if (!argc) {
        usage();
    }

    if (verbose) {
        printf("verbose: %d\n", verbose);
    }

    while (argc--) {
        sio_open(*argv++);
        assemble();
        sio_close();
    }
}
