/*
 * trasm main file - mostly arg processing
 *
 * /usr/src/cmd/asz/main.c
 *
 * Changed: <2023-07-05 20:36:59 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

#include <stdio.h>
#include <stdlib.h>

#include "sio.h"
#include "asm.h"

#define VERSION "1.0"

/*
 * flags 
 */
char verbose = 0;
char g_flag = 0;

/*
 * arg zero 
 */
char *progname;

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

	if (verbose)
		printf("TRASM cross assembler v%s\n", VERSION);

    while (argc--) {
        sio_open(*argv++);
        asm_assemble();
        sio_close();
    }
}
