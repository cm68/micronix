/*
 * reloc entries seem to be a byte offset, followed by an encoded
 ** reloc, which is added to the destination.
 * reloc types:
 */

#include "ws.h"

int	fout;
int	cflg;
int	nflg;
int	uflg;
int	rflg =	1;
int	gflg;
int	pflg;
int verbose;

struct	symtab {
	unsigned short	value;
	unsigned char	flag;
#define	SF_SEG		0x03
#define		SF_UNDEF	0x00
#define		SF_TEXT		0x01
#define		SF_DATA		0x02
#define	SF_DEF		0x04
#define	SF_GLOBAL	0x08
	char	name[9];
} *symtab;
int nsyms;

int object;

struct obj buf;

char *
binout(unsigned char b)
{
	static char buf[9];
	unsigned char i;

	buf[8] = 0;

	for (i = 0; i < 8; i++) {
		buf[i] = (b & (1 << (7 - i))) ? '1' : '0';
	}
	return (buf);
}

relocs(char *seg)
{
	unsigned char control;
	unsigned char next;
	unsigned short offset;

	printf("%s segment relocations:\n", seg);

	offset = 0;

	while (1) {
		read(object, &control, sizeof(control));
		if (control == 0) {
			/* end of reloc */
			break;
		} else if (control < 32) {
			/* one byte skips */
			offset += control;
			continue;
		} else if (control < 64) {
			/* two byte skips */
			offset += 32 + (256 * (control - 32));
			read(object, &control, sizeof(control));
			offset += control;
			continue;
		}
		printf("offset %2x ", offset);
		if (control & 0x03) {
			printf("low control bit set %x\n", control);
		}
		/* a relocation */
		control -= 64;
		control >>= 2;

		if (control == 1) {
			printf("text bias\n");
			offset += 2;
			continue;
		} else if (control == 2) {
			printf("data bias\n");
			offset += 2;
			continue;
		}

		control -= 4;
		if (control == 43) {
			/* one byte extended */
			read(object, &next, sizeof(next));
			if (next < 128) {
				control = next + 43;
			} else {
				/* two byte extended */
				control = (next - 128) * 256;
				read(object, &next, sizeof(next));
				control += 175 + next;	
			}
		}
		printf("symbol %d %s\n",
			control,
			control < nsyms ?
			symtab[control].name :
			"symbol out of range"); 
		offset += 2;
	}
}

main(argc, argv)
char **argv;
{
	int i;
	unsigned short value;
	unsigned char flag;
	struct symtab *sym;

	if (--argc > 0 && *argv[1] == '-') {
		argv++;
		while (*++*argv) switch (**argv) {
		case 'v':
			verbose++;
			continue;

		case 'n':
			nflg++;
			continue;

		case 'c':
			cflg++;
			continue;

		case 'g':
			gflg++;
			continue;

		case 'u':
			uflg++;
			continue;

		case 'r':
			rflg = -1;
			continue;

		case 'p':
			pflg ++;
			continue;

		default:
			continue;
		}
		argc--;
	}
	if (argc==0) {
		object = open("a.out", 0); 
	} else {
		object = open(*++argv, 0);
	}
	if (object < 0) {
		printf("cannot open input\n");
		exit(1);
	}
	read(object, &buf, sizeof(buf));
	if (verbose) {
		printf("magic %x sym:%d text:%d data:%d dataoff:%x\n",
			(buf.ident << 8) + buf.conf, 
			buf.table, buf.text, buf.data, buf.dataoff);
	}
	if(buf.ident != OBJECT) {
		printf("bad format\n");
		exit(1);
	}
	lseek(object, buf.text + buf.data + sizeof(buf), SEEK_SET);

	/* read symbol table */
	nsyms = buf.table / 12;
	if (nsyms == 0) {
		printf("no name list\n");
		exit(1);
	}
	symtab = malloc(nsyms * sizeof(struct symtab));
	read(object, symtab, nsyms * sizeof(struct symtab));

	sym = symtab;
	for(i = 0; i < nsyms; i++) {
		printf("%5d %02x %9s: ", i, i, sym->name);
		value = sym->value;
		flag = sym->flag;

		printf("%4x ", value);
		printf("%2x ", flag);
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

	/* dump out relocs */
	if (buf.conf == RELOC) {
		relocs("code");
		relocs("data");
	}
	printf("\n");
}

printo(v)
{
	int i;

	printf("%c", v<0?'1':'0');
	for(i=0; i<5; i++) {
		printf("%c", ((v>>12)&7)+'0');
		v <<= 3;
	}
}
