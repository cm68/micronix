/*
 * dump out the symbol table and optionally the relocation entries
 * of an object file, or if an archive, of each file in the archive
 * just for grins, it can disassemble too.
 */

#include "ws.h"
#include <fcntl.h>


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

struct reloc {
	unsigned short seg;
	struct ws_reloc rl;
	struct reloc *next;
} *rlhead;
#define	SEG_TEXT	0
#define	SEG_DATA	1

struct reloc *
lookup(int i)
{
	struct reloc *r;
	for (r = rlhead; r; r = r->next) {
		if (r->rl.offset == i) {
			break;
		}
	}
	return r;
}

void 
render(char *buf, struct reloc *r, unsigned short value)
{
	switch(r->rl.type) {
	case REL_TEXTOFF:
		sprintf(buf, "T+%x", value);
		break;
	case REL_DATAOFF:
		sprintf(buf, "D+%x", value);
		break;
	case REL_SYMBOL:
		sprintf(buf, syms[r->rl.value].name);
		break;
	default:
		sprintf(buf, "BZZZT");
	}
}

/*
 * the current object header
 */
struct obj head;

/*
 * file offsets to deal with disassembly inside an archive, since the disassembly is two-pass
 */
int textoff;
int dataoff;
int endoff;
int fd;

dumpseg(int bc, int base)
{
	unsigned int i;
	unsigned char b;
	int c = 0;

	for (i = 0; i < bc; i++) {
		read(fd, &b, 1);
		if (c == 0) {
			printf("%04x: ", i + base);
		}
		printf("%02x ", b);	
		c++;
		if (c == 16) {
			c = 0;
			printf("\n");
		}
	}
	if (c) {
		printf("\n");
	}
}

unsigned char barray[5];
char blen;

unsigned char
readbyte(unsigned short addr)
{
	unsigned char c;
	lseek(fd, textoff + addr, SEEK_SET);
	read(fd, &c, 1);
	barray[blen++] = c;

	return c;
}

char *
symname(unsigned short addr)
{
	return 0;
}

disassem()
{
	int bc;
	char outbuf[40];
	char bbuf[40];
	int i;

	/* dump out hex */
	lseek(fd, textoff, SEEK_SET);
	if (verbose) {
		if (head.text) {
			printf("text:\n");
			dumpseg(head.text, head.textoff);
		}
		if (head.data) {
			printf("data:\n");
			dumpseg(head.data, head.dataoff);
		}
	}
	/* process instructions to build xrefs and generate labels */
	lseek(fd, textoff, SEEK_SET);
	location = 0;
	while (location < head.text) {
		blen = 0;
		bc = format_instr(location, outbuf, &readbyte, &symname);
		printf("%04x: %-30s", location, outbuf, bbuf);
		printf(" ; ");
		for (i = 0; i < sizeof(barray); i++) {
			if (i < blen) {
				printf("%02x ", barray[i]);
			} else {
				printf("   ");
			}
		}
		for (i = 0; i < sizeof(barray); i++) {
			if (i < blen) {
				if ((barray[i] <= 0x20) || (barray[i] >= 0x7f)) {
					printf(".");
				} else {
					printf("%c", barray[i]);
				}
			} else {
				printf(" ");
			}
		}
		printf("\n");
		location += bc;
	}
	if (head.data) {
		printf("data:\n");
		dumpseg(head.data, head.dataoff);
	}
	/* list */
}

void
makerelocs(unsigned char seg)
{
	struct ws_reloc *rp;
	struct reloc *r;

	location = 0;

	while ((rp = getreloc(&relocp)) != 0) {
		r = malloc(sizeof(struct reloc));		
		r->seg = seg;
		r->rl.offset = rp->offset;
		r->rl.value = rp->value;
		r->rl.type = rp->type;
		r->next = rlhead;
		rlhead = r;
	}
}

void
dumprelocs()
{
	struct reloc *r;

	for (r = rlhead; r; r = r->next) {
		printf("%s:%04x ", r->seg ? "DATA" : "TEXT", r->rl.offset);
		switch (r->rl.type) {
		case REL_TEXTOFF:
			printf("text relative\n");
			break;
		case REL_DATAOFF:
			printf("data relative\n");
			break;
		case REL_SYMBOL:
			if (r->rl.value > nsyms) {
				printf("out of bounds symbol reference %d\n",r->rl.value);
			} else {
				printf("symbol reference %s\n", syms[r->rl.value].name);
			}
			break;
		default:
			printf("getreloc lose type %d\n", r->rl.type);
			break;
		}
	}
}

void
freerelocs()
{
	struct reloc *r;

	while (rlhead) {
		r = rlhead;
		rlhead = r->next;
		free(r);
	}
}

void
do_object(int fd, int limit)
{
	struct ws_symbol *sym;
	int i;
	unsigned short value;
	unsigned char flag;

	read(fd, &head, sizeof(head));
	if (verbose) {
		printf("magic %x sym:%d text:%d data:%d textoff:%x dataoff:%x\n",
			(head.ident << 8) + head.conf, 
			head.table, head.text, head.data, head.textoff, head.dataoff);
	}
	if (head.ident != OBJECT) {
		printf("bad object file magic number %x\n", head.ident);
		exit(1);
	}

	/* mark the file positions */
	textoff = lseek(fd, 0, SEEK_CUR);
	dataoff = lseek(fd, head.text, SEEK_CUR);
	lseek(fd, head.data, SEEK_CUR);

	/* read symbol table */
	nsyms = head.table / 12;
	if (nsyms == 0) {
		printf("no name list\n");
		exit(1);
	}
	syms = malloc(nsyms * sizeof(*syms));
	read(fd, syms, nsyms * sizeof(*syms));

	sym = syms;
	for(i = 0; i < nsyms; i++) {
		printf("%5d %9s: ", i, sym->name);
		value = sym->value;
		flag = sym->flag;

		printf("%04x ", value);
		if (flag & SF_GLOBAL)
			printf("global ");
		if (flag & SF_DEF)
			printf("defined ");
		switch (flag & SF_SEG) {
		case SF_TEXT:
			printf("code ");
			break;
		case SF_DATA:
			printf("data ");
			break;
		case 0:
			break;
		default:
			printf("unknown segment");
			break;
		}
		printf("\n");
		sym++;
	}

	/* read in the relocs, which are from the symbol table to logical eof  */
	if (limit == 0) {
		limit = sizeof(relocbuf);
	} else {
		limit -= sizeof(head) + head.text + head.data + head.table;
	}

	i = read(fd, relocbuf, limit);
	if (i < 0) {
		printf("error reading relocs");
		goto out;
	}
	if (head.conf == RELOC) {
		relocp = relocbuf;
		makerelocs(SEG_TEXT);
		makerelocs(SEG_DATA);
	}
	endoff = lseek(fd, 0, SEEK_CUR);

	if (rflag) {
		dumprelocs();
	}
	if (dflag) {
		disassem();
	}
out:
	lseek(fd, endoff, SEEK_SET);
	freerelocs();
	free(syms);
}

/*
 * process a file from the command line.  if it's an archive, iterate over the members
 */
void
nm(char *oname)
{
	unsigned char magic;
	int i;

	fd = open(oname, 0);
	if (fd < 0) {
		printf("cannot open input\n");
		exit(1);
	}
	read(fd, &magic, sizeof(magic));
	if (magic == OBJECT) {
		printf("%s:\n", oname);
		lseek(fd, 0, SEEK_SET);
		do_object(fd, 0);
		close(fd);
		return;
	} else if (magic != 0x75) {
		printf("bad magic: %x\n", magic);
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
		i = member.len[0] + (member.len[1] << 8);
		if (i == 0) {
			close(fd);
			return;
		}
		printf("%s %s:\n", oname, member.fname);
		do_object(fd, i);
	}
}

int
main(argc, argv)
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
		nm("a.out");
	} else {
		while (argc--) {
			nm(*++argv);
		}
	}
}
