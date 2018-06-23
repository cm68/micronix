/*
 * dump out the symbol table and optionally the relocation entries
 */

#include "ws.h"
#include <fcntl.h>


int verbose;
int rflag;
struct symtab *syms;
int nsyms;

char relocbuf[16384];
char *relocp;

unsigned short segoffset;

void
dumprelocs(char *seg)
{
	struct reloc *rp;

	segoffset = 0;

	while (rp = getreloc(&relocp)) {
		printf("%s:%04x ", seg, rp->offset);
		switch (rp->type) {
		case REL_TEXTOFF:
			printf("text relative\n");
			break;
		case REL_DATAOFF:
			printf("data relative\n");
			break;
		case REL_SYMBOL:
			if (rp->value > nsyms) {
				printf("out of bounds symbol reference %d\n",rp->value);
			} else {
				printf("symbol reference %s\n", syms[rp->value].name);
			}
			break;
		default:
			printf("getreloc lose type %d\n", rp->type);
			break;
		}
	}
}

void
nm(char *oname)
{
	int object;
	struct obj head;
	struct symtab *sym;
	int i;
	unsigned short value;
	unsigned char flag;

	printf("%s:\n", oname);
	object = open(oname, 0);
	if (object < 0) {
		printf("cannot open input\n");
		exit(1);
	}
	read(object, &head, sizeof(head));
	if (verbose) {
		printf("magic %x sym:%d text:%d data:%d dataoff:%x\n",
			(head.ident << 8) + head.conf, 
			head.table, head.text, head.data, head.dataoff);
	}
	if (head.ident != OBJECT) {
		printf("bad object file magic number\n");
		exit(1);
	}
	lseek(object, head.text + head.data + sizeof(head), SEEK_SET);

	/* read symbol table */
	nsyms = head.table / 12;
	if (nsyms == 0) {
		printf("no name list\n");
		exit(1);
	}
	syms = malloc(nsyms * sizeof(struct symtab));
	read(object, syms, nsyms * sizeof(struct symtab));

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

	if (rflag) {
		/* read in the relocs, which are from the symbol table to eof  */
		i = read(object, relocbuf, sizeof(relocbuf));
		if (i < 0) {
			printf("error reading relocs");
			goto out;
		}

		/* dump out relocs */
		if (head.conf == RELOC) {
			relocp = relocbuf;
			dumprelocs("text");
			dumprelocs("data");
		}
	}
out:
	free(syms);
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
