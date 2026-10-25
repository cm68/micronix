/*
 * process an ar file
 *
 * tools/ar.c
 * Changed: <2021-12-23 15:53:30 curt>
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

int verbose;
int xflag;

struct head {
	unsigned short magic;
} head;

struct entry {
	char fname[14];
	unsigned char len0;
	unsigned char len1;
} entry;

void
art(char *oname)
{
	int archive;
	unsigned short value;
	unsigned char flag;
	int i;
	int ofile;
	char *buf;

	printf("%s:\n", oname);
	archive = open(oname, 0);
	if (archive < 0) {
		printf("cannot open input\n");
		exit(1);
	}
	if (read(archive, &head, sizeof(head)) != sizeof(head)) {
		printf("cannot read archive header\n");
		exit(1);
	}
	if (verbose) {
		printf("magic %x \n", head.magic);
	}
	while (1) {
		i = read(archive, &entry, sizeof(entry));
		if (i < sizeof(entry)) {
			break;
		}
		value = entry.len0 + (entry.len1 << 8);
		printf("file: %s %d\n",
			entry.fname,
			value);
		if (xflag) {
			buf = malloc(value);
			if (!buf || read(archive, buf, value) != value) {
				printf("cannot read %s\n", entry.fname);
				free(buf);
				exit(1);
			}
			ofile = open(entry.fname, O_CREAT|O_RDWR, 0777);
			if (ofile < 0) {
				perror(entry.fname);
			} else {
				if (write(ofile, buf, value) != value)
					perror(entry.fname);
				close(ofile);
			}
			free(buf);
		} else {
			lseek(archive, value, SEEK_CUR);
		}
	}
}

int
main(argc, argv)
int argc;
char **argv;
{
	int i;
	unsigned short value;
	unsigned char flag;

	if (--argc > 0 && *argv[1] == '-') {
		argv++;
		while (*++*argv) switch (**argv) {
		case 'v':
			verbose++;
			continue;
		case 'x':
			xflag++;
			continue;
		default:
			continue;
		}
		argc--;
	}
	if (argc == 0) {
		art("a.out");
	} else {
		while (argc--) {
			art(*++argv);
		}
	}
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
