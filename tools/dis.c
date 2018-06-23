#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include "getopt.h"
#include "sys/stat.h"
#include "dis.h"

char *pname;
extern char *maketest(long *, int *);

char *vopts[] = { "V_BUF", "V_MATCH", "V_PARSE", "V_OUT", "V_TABLE", 0 };

int bigendian = 0;
int verbose;
int limit;
char *architecture = "z80";

struct option options[] = {
	{"verbose", optional_argument, 0, 'v'},
	{"max", required_argument, 0, 'm'},
	{"arch", required_argument, 0, 'a'},
	{0, 0, 0, 0}
};

void
usage()
{
	char c;
	printf("Usage: %s <option(s)> <file>\n", pname);
	printf("\
-v --verbose[=<verbosity>]\tbe noisy\n\
-m --max=<limit>\tstop after address <limit>\n\
-a --arch=<architecture>\tuse architecture file\n\
");
	for (c = 0; vopts[c]; c++) {
		printf("%x %s\n", 1 << c, vopts[c]);
	}	
	exit(-1);
}

int
main(int argc, char **argv)
{
	char *infile;
	char *srcbuf;
	unsigned char *destbuf;
	unsigned long startaddr;
	opterr = 0;
	int index;
	int option_index = 0;
	char c;
	int ret = 0;

	pname = argv[0];

	while (1) {
		c = getopt_long (argc, argv, "v::m:a:", options, &option_index);

		if (c == -1)
		break;

		switch (c) {

		case 'v':
			if (optarg) {
				if (*optarg == '=') optarg++;
				verbose = strtol(optarg, 0, 0);
			} else {
				verbose = -1;
			}
			if (verbose) {
				setlinebuf(stdout);
				printf("verbose %x ",verbose);
				for (c = 0; vopts[c]; c++) {
					if (verbose & (1 << c)) {
						printf("%s ", vopts[c]);
					}
				}
				printf("\n");
			}
			break;

		case 'm':
			limit = strtol(optarg, 0, 0);
			break;

		case 'a':
			architecture = strdup(optarg);
			break;

		default:
		case '?':
			usage();
			break;
		}
	}

	if ((ret = load_arch(architecture))) {
		exit (ret);
	}

	printf("# %s disassembly\n", architecture);

	for (index = optind; index < argc; index++) {
		int fd;
		struct stat sbuf;
		int size;

		infile = argv[index];

		if (access(infile, R_OK) != 0) {
			perror(infile);
			continue;
		}
		if (stat(infile, &sbuf) != 0) {
			perror(infile);
			continue;
		}	
		srcbuf = malloc(sbuf.st_size+1);
		fd = open(infile, 0);
		if (fd == -1) {
			perror(infile);
			continue;
		}	
		if (read(fd, srcbuf, sbuf.st_size) != sbuf.st_size) {
			printf("short read on %s\n", infile);
			close(fd);
			continue;
		}
		srcbuf[sbuf.st_size] = '\0';
		size = decode(srcbuf, sbuf.st_size, &destbuf, &startaddr);
		if (size == 0) {
			size = sbuf.st_size;
			destbuf = (unsigned char *)srcbuf;
			startaddr = 0;
		}
		disas(destbuf, startaddr, size);
	}

	return (ret);
}

