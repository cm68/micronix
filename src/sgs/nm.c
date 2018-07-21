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

unsigned char barray[5];
char blen;

unsigned char
readbyte(unsigned short addr)
{
	unsigned char c;
	lseek(fd, textoff + addr, SEEK_SET);
	read(fd, &c, 1);
	if (blen < sizeof(barray)) {
		barray[blen++] = c;
	}
	return c;
}

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

char *
sym(unsigned short addr)
{
	if (addr < nsyms) {
		return syms[addr].name;
	}
	return 0;
}

char *
label(unsigned short addr)
{
	int i;
	for (i = 0; i < nsyms; i++) {
		if ((syms[i].flag & SF_DEF) && (syms[i].value == addr)) {
			return syms[i].name;
		}
	}
	return 0;
}

unsigned long
reloc(unsigned short addr)
{
	struct reloc *r;
	unsigned long ret = 0;

	r = lookup(addr);

	if (!r) return (ret);

	switch(r->rl.type) {
	case REL_TEXTOFF:
		ret = (1 << 16) + head.textoff;
		break;
	case REL_DATAOFF:
		ret = (2 << 16) + head.dataoff;
		break;
	case REL_SYMBOL:
		ret = (3 << 16) + r->rl.value;
		break;
	default:
		break;
	}
	return ret;
}

disassem()
{
	int bc;
	char outbuf[40];
	char bbuf[40];
	int i;
	char *tag;

	/* dump out hex */
	lseek(fd, textoff, SEEK_SET);
	if (verbose) {
		if (head.text) {
			printf("text:\n");
			dumpmem(&readbyte, head.textoff, head.text);
		}
		if (head.data) {
			printf("data:\n");
			dumpmem(&readbyte, head.dataoff, head.data);
		}
	}
	lseek(fd, textoff, SEEK_SET);
	location = 0;
	while (location < head.text) {
		blen = 0;
		bc = format_instr(location, outbuf, &readbyte, &sym, &reloc);
		if ((tag = label(location))) {
			printf("%s:\n", tag);
		}
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
		dumpmem(&readbyte, head.dataoff, head.data);
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
		return(1);
	}

	/* mark the file positions */
	textoff = lseek(fd, 0, SEEK_CUR);
	dataoff = lseek(fd, head.text, SEEK_CUR);
	lseek(fd, head.data, SEEK_CUR);

	/* read symbol table */
	nsyms = head.table / 12;
	if (nsyms) {
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
		printf("cannot open %s\n", oname);
		return(1);
	}
	read(fd, &magic, sizeof(magic));
	if (magic == OBJECT) {
		printf("%s:\n", oname);
		lseek(fd, 0, SEEK_SET);
		do_object(fd, 0);
		close(fd);
		return;
	} else if (magic != 0x75) {
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

	while (--argc) {
		argv++;
		if (**argv == '-') {
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
		} else {
			break;
		}
	}
	while (argc--) {
		nm(*argv++);
	}
}
