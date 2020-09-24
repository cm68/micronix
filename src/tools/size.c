/*
 * dump out the symbol table and optionally the relocation entries
 * of an object file, or if an archive, of each file in the archive
 * just for grins, it can disassemble too.
 */

#include "ws.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int verbose;
int rflag;
int dflag;
struct ws_symbol *syms;
int nsyms;

unsigned char relocbuf[16384];
unsigned char *relocp;

unsigned short location;

struct member {
	char fname[14];
	unsigned char len[2];
} member;

/*
 * the current object header
 */
struct obj head;

void
do_object(int fd, int limit)
{
	struct ws_symbol *sym;
	int i;
	unsigned short value;
	unsigned char flag;

	read(fd, &head, sizeof(head));
	printf("magic %x sym:%d text:%d data:%d bss:%d heap:%d textoff:%x dataoff:%x\n",
		(head.ident << 8) + head.conf, 
		head.table, head.text, head.data, head.bss, head.heap, head.textoff, head.dataoff);
	
	if (head.ident != OBJECT) {
		printf("bad object file magic number %x\n", head.ident);
	}
}

/*
 * process a file from the command line.  if it's an archive, iterate over the members
 */
void
size(char *oname)
{
	unsigned char magic;
	int i;
	int here;
	int fd;

	fd = open(oname, 0);
	if (fd < 0) {
		printf("cannot open %s\n", oname);
		return;
	}
	read(fd, &magic, sizeof(magic));
	if (magic == OBJECT) {
		printf("%s:\n", oname);
		lseek(fd, 0, SEEK_SET);
		do_object(fd, 0);
		close(fd);
		return;
	} else if (magic != 0x75) {	/* archive */
		printf("%s: bad magic: %x\n", oname, magic);
		close(fd);
		return;
	}
	lseek(fd, 2, SEEK_SET);
	while (1) {
		i = read(fd, &member, sizeof(member));
		if (i < sizeof(member)) {
			close(fd);
			return;
		}
		here = lseek(fd, 0, SEEK_CUR);
		i = member.len[0] + (member.len[1] << 8);
		if (i == 0) {
			close(fd);
			return;
		}
		printf("%s %s:\n", oname, member.fname);
		do_object(fd, i);
		lseek(fd, here + i, SEEK_SET);
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
		case 'd':
			dflag++;
			continue;
		case 'r':
			rflag++;
			continue;
		default:
			continue;
		}
		argc--;
	}
	if (argc == 0) {
		size("a.out");
	} else {
		while (argc--) {
			size(*++argv);
		}
	}
}
