/*
 * binary file input
 * intel hex, motorola srec, and binary dumps
 * rel files
 * elf object files
 * whitesmith's object files
 */
#include "dis.h"
#include "ws.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern int verbose;

long start;

struct seg *seglist;
int high;
int modcount;
char *segname[] = { "absolute", "code", "data", "common" };

int segbase[4];
unsigned short segoffset;

/*
 * random utility function
 */
unsigned char *
binstr(unsigned short val, char count)
{
	static unsigned char binbuf[17];
	unsigned char *s = binbuf;
	int i;

	binbuf[0] = 0;

	for (i = count-1; i >= 0; i--) {
		if ((val >> i) & 0x1) {
			*s++ = '1';
		} else {
			*s++ = '0';
		}
		s[1] = '\0';
	}
	return binbuf;
}

/*
 * the i/o system
 */
typedef struct buffer {
	unsigned char *storage;
	int cursor;
	int base;
	int size;
	int lineno;
	int high;
	char bitoff;
} buffer_t;

void
advance(buffer_t *buf)
{
	if (buf->storage[buf->cursor] == '\n') {
		buf->lineno++;
	}
	buf->cursor++;
}

char *
pos(buffer_t *in)
{
	static char desc[80];
	sprintf(desc, "cursor %d line %d", in->cursor, in->lineno);
	return (desc);
}

unsigned char
look(buffer_t *in, int off)
{
	char c;
	if ((in->cursor + off) > in->size) {
		return 0;
	}
	c = in->storage[in->cursor+off];
	if (verbose & V_BUF) {
		printf("look %d = %c 0x%x\n", off, c, c);
	}
	return (c);
}

unsigned char
get(buffer_t *in)
{
	unsigned char c = in->storage[in->cursor];
	if (verbose & V_BUF) {
		printf("read %c 0x%x\n", c, c);
	}
	return (c);
}

/*
 * the microsoft rel format is built on a bit stream. 
 * we need to be able to grab variable numbers of bits and maintain a bit cursor.
 */
unsigned short
getbits(buffer_t *in, int len)
{
	unsigned short ret = 0;
	char i;
	unsigned char v;

	//printf("\ngetbits0: len %d bitoff %d\n", len, in->bitoff);

	v = in->storage[in->cursor];
//	printf("getbits1: %x\n", v);
	for (i = len - 1; i >= 0; i--) {
		if ((1 << (7 - in->bitoff)) & v) {
//			printf("getbits2: set %x\n", i);
			ret |= 1 << i;
		}
		in->bitoff++;
		if (in->bitoff == 8) {
			advance(in);
			v = in->storage[in->cursor];
			in->bitoff = 0;
		}
	}
	if (verbose & V_BUF) {
		printf("getbits: %d => %x %s\n", len, ret, binstr(ret, len));
	}
	return ret;
}

void
put(buffer_t *out, int where, unsigned char c)
{
	int size;

	if (verbose & V_OUT) {
		printf("put 0x%02x to 0x%04x\n", c, where);
	}
	if (where >= out->size) {
		/* round up to 64k */
		size = (where | 0xffff) + 1;
		out->storage = realloc(out->storage, size);
		out->size = size;
	}
	if (where > out->high) {
		out->high = where;
	}
	out->storage[where++] = c;
	out->cursor = where;
}

int
ishex(char byte)
{
	if ((byte >= '0' && byte <= '9') || 
		(byte >= 'A' && byte <= 'F') || 
		(byte >= 'a' && byte <= 'f')) {
		return 1;
	} else {
		return 0;
	}
}

int
readhex(char c)
{
	if (c >= '0' && c <= '9') {
		c -= '0';
	} else if (c >= 'A' && c <= 'F') {
		c = 0xa + c - 'A';
	} else if (c >= 'a' && c <= 'f') {
		c = 0xa + c - 'a';
	}
	return c;
}

/*
 * get a hex string and decode it into an integer of the appropriate width.
 * adjust the buffer or gripe accordingly, returning a nasty.
 * as a cheesy side effect, optionally calculate the running byte-wide checksum
 */
int
gethex(buffer_t *in, int digits, unsigned char *checksum)
{
	unsigned char c;
	unsigned int v = 0;
	int ask = digits;
	while (digits--) {
		c = get(in);
		if (ishex(c)) {
			c = readhex(c);
		} else {
			printf("malformed hex (0x%x) at %s\n", c, pos(in));
			return -1;
		}
		v = (v << 4) + c;
		advance(in);
		/* every 2 bytes, update the checksum */
		if (checksum && ((digits & 0x1) == 0)) {
			*checksum = *checksum + v;
		}
	}
	if (verbose & V_BUF) {
		printf("gethex for %d returns 0x%x (0x%x)\n", ask, v, checksum ? *checksum : 0);
	}
	return (v);
}

/*
 * binary file handler for intel hex - only really deal with these two types
 * :CCAAAA00DdDd...DdXX   CC bytes of data Dd to be placed starting at AAAA - checksum XX
 * :CCAAAA01XX            eof with start address AAAA
 */
int
do_intel_hex(buffer_t *in, buffer_t *out)
{
	int rectype;
	int bc;
	int addr;
	unsigned char checksum;
	int byte;
	
	if ((byte = get(in)) != ':') {
		printf("missing record start (0x%x) at %s\n", byte, pos(in));
		return 0;
	}
	advance(in);

	checksum = 0;
	bc = gethex(in, 2, &checksum);
	if (bc == -1) {
		printf("missing byte count at %s\n", pos(in));
		return 0;
	}

	addr = gethex(in, 4, &checksum);
	if (addr == -1) {
		printf("missing address at %s\n", pos(in));
		return 0;
	}

	rectype = gethex(in, 2, &checksum);
	if (addr == -1) {
		printf("missing record type at %s\n", pos(in));
		return 0;
	}

	switch (rectype) {

	case 0x0:	/* data record */
		while (bc--) {
			byte = gethex(in, 2, &checksum);
			if (byte == -1) {
				printf("missing data byte at %s\n", pos(in));
				return 0;
			}
			put(out, addr++, byte);
		}
		break;

	case 0x1:	/* eof */
		start = addr;
		break;

	case 0x2:	/* extended segment address record */
		if (addr != 0) {
			printf("nonzero address on type 2 at %s\n", pos(in));
			return 0;
		}
		addr = gethex(in, 4, &checksum);
		if (addr == -1) {
			printf("missing seg offset at %s\n", pos(in));
			return 0;
		}
		out->base = addr * 16;
		break;

	case 0x3:	/* start segment address record */
		if (addr != 0) {
			printf("nonzero address on type 3 at %s\n", pos(in));
			return 0;
		}
		start = gethex(in, 8, &checksum);
		if (start == -1) {
			printf("missing CS:IP at %s\n", pos(in));
			return 0;
		}
		break;

	case 0x4:	/* extended linear address record */
		addr = gethex(in, 4, &checksum);
		if (addr == -1) {
			printf("missing seg offset at %s\n", pos(in));
			return 0;
		}
		out->base = addr * 65536;
		break;

	case 0x5:	/* start linear address record */
		if (addr != 0) {
			printf("nonzero address on type 3 at %s\n", pos(in));
			return 0;
		}
		start = gethex(in, 8, &checksum);
		if (start == -1) {
			printf("missing EIP at %s\n", pos(in));
			return 0;
		}
		break;

	default:
		printf("bogus record type (0x%x) at %s\n", rectype, pos(in));
		return 0;
	}

	if (gethex(in, 2, &checksum) == -1) {
		printf("missing checksum at %s\n", pos(in));
		return 0;
	}
	if (checksum != 0) {
		printf("checksum error (0x%x) at %s\n", checksum, pos(in));
		return 0;
	}

	/* let's swallow line and file ending */
	while ((byte = get(in)) == '\r' || byte == '\n' || byte == 0x26) {
		advance(in);
	}
	if (byte == 0) {
		return 0;
	}
	return (rectype == 0x1) ? 0 : 1;
}

/*
 * binary file handler for motorola S-record  - only really deal with these two types
 * S01CCAAAADdDd...DdXX   CC bytes of data Dd to be placed starting at AAAA - checksum XX
 * S09AAAAXX              eof and start address AAAA
 */
int
do_motorola_srec(buffer_t *in, buffer_t *out)
{
	int rectype;
	int bc;
	int addr;
	unsigned char checksum;
	int byte;
	
	if ((byte = get(in)) != 'S') {
		printf("missing record start (0x%x) at %s\n", byte, pos(in));
		return 0;
	}
	advance(in);

	rectype = gethex(in, 1, &checksum);
	if (rectype == -1) {
		printf("missing record type at %s\n", pos(in));
		return 0;
	}

	/* checksum does not include rec type */
	checksum = 0;

	bc = gethex(in, 2, &checksum);
	if (bc == -1) {
		printf("missing byte count at %s\n", pos(in));
		return 0;
	}
	switch (rectype) {

	case 0x1:	/* 16 bit address data record */
		addr = gethex(in, 4, &checksum);
		if (addr == -1) {
			printf("missing address at %s\n", pos(in));
			return 0;
		}
		bc -= 3;
		while (bc--) {
			byte = gethex(in, 2, &checksum);
			if (byte == -1) {
				printf("missing data byte at %s\n", pos(in));
				return 0;
			}
			put(out, addr++, byte);
		}
		break;

	case 0x9:	/* eof */
		addr = gethex(in, 4, &checksum);
		if (addr == -1) {
			printf("missing address at %s\n", pos(in));
			return 0;
		}
		start = addr;
		break;

	default:
		printf("bogus record type (0x%x) at %s\n", rectype, pos(in));
		return 0;
	}

	if (gethex(in, 2, &checksum) == -1) {
		printf("missing checksum at %s\n", pos(in));
		return 0;
	}
	if (checksum != 0xff) {
		printf("checksum error (0x%x) at %s\n", checksum, pos(in));
		return 0;
	}

	/* let's swallow line and file ending */
	while ((byte = get(in)) == '\r' || byte == '\n' || byte == 0x26) {
		advance(in);
	}
	return (rectype == 0x9) ? 0 : 1;
}

/*
 * binary file handler for simple hex dump:
 * lines consist of [[0x]n[n]*:] [[0x]n[n]*]*
 * the recognizer is that the first significant character must be '0'.
 */
int
do_hexdump(buffer_t *in, buffer_t *out)
{
	int byte;
	unsigned int data = 0;
	int digits = 0;

	/* swallow newlines tabs and spaces */
	while (1) {
		byte = look(in, 0);
		if (byte == '\0') {
			return (0);
		} else if (byte == '\n' || byte == '\t' || byte == ' ') {
			advance(in);
		} else {
			break;
		}
	}

	/* 0x is a benign prefix */
	if ((look(in,0) == '0') && (look(in,1) == 'x')) {
		advance(in);
		advance(in);
	}

	/* let's build a number */
	while (ishex(look(in,0))) {
		data <<= 4;
		data = data + readhex(get(in));
		advance(in);
		digits++;
	} 

	/* ah. we have an address */
	if (look(in, 0) == ':') {
		advance(in);
		if (verbose & V_PARSE) {
			printf("address %x\n", data);
		}

		if (data != out->cursor) {
			if (data < out->cursor) {
				printf("load address ordering inversion\n");
				return (0);
			}
			while (out->cursor < data) {
				put(out, out->cursor, 0);
			}
		}
		return (1);
	}

	digits = (digits + 1) / 2;	
	/* we have data.  let's write the bytes out */
	if (bigendian) {
		int i;
		for (i = (digits - 1) * (bigendian * 2); i >= 0; i -= (bigendian * 2)) {
			byte = (data >> i) & 0xff;
			put(out, out->cursor, byte);
		}
	} else {
		while (digits--) {
			byte = data & 0xff;
			put(out, out->cursor, byte);
			data >>= 8;
		}
	}
	return (1);
}

void
makeseg(char type, int size)
{
	struct seg *sp;

	sp = malloc(sizeof(*sp));
	sp->module = modcount;
	sp->type = type;
	sp->base = high;
	sp->len = size;
	sp->next = seglist;
	high += size;
	segbase[type] = high;
	seglist = sp;
}

struct seg *
lookup_seg(int addr)
{
	struct seg *sp;

	for (sp = seglist; sp; sp = sp->next) {
		if (addr >= sp->base && addr < sp->base + sp->len) {
			return sp;
		}
	}
	return 0;
}

char
is_code(int addr) {
	struct seg *sp;

	sp = lookup_seg(addr);
	if (sp && sp->type == S_CODE) {
		return 1;
	} else {
		return 0;
	}
}

struct symbol *symbols;

/*
 * XXX - there could be more than once symbol with the same value
 * return the first.
 */
char *
getsymname(int offset)
{
	char *s = 0;
	struct symbol *sp;

	for (sp = symbols; sp; sp = sp->next) {
		if (sp->offset == offset) {
			s = sp->name;
			break;
		}
	}
	if (verbose & V_SYM) {
		printf("getsymname: %d %s\n", offset, s ? s : "undefined");
	}
	return s;
}

/*
 * register a symbol
 */
struct symbol *
makesym(char *name, int offset)
{
	struct symbol *sp;

	if (verbose & V_SYM) {
		printf("makesym: %s %d\n", name, offset);
	}
	for (sp = symbols; sp; sp = sp->next) {
		if (strcmp(sp->name, name) == 0) {
			if (sp->offset != offset) {
				printf("makesym: %s already defined old %d new %d\n",
					name, sp->offset, offset);
			}
			return sp;
		}
	}
	sp = malloc(sizeof(*sp));
	sp->next = symbols;
	sp->offset = offset;
	sp->name = strdup(name);
	symbols = sp;
	return (sp);
}

/*
 * invent a symbol abd register it
 */
struct symbol *
makelabel(unsigned int offset)
{
	static char labbuf[20];

	sprintf(labbuf, "L%x", offset);
	return makesym(labbuf, offset);
}

/*
 * references are operand fields that refer to symbols
 */
struct ref *reflist;

char *
refname(int offset)
{
	struct ref *rp;
	char *s = 0;

	for (rp = reflist; rp; rp = rp->next) {
		if (rp->offset == offset) {
			s = rp->sym->name;
			break;
		}
	}
	if (verbose & V_SYM) {
		printf("refname: %d %s\n", offset, s ? s : "undefined");
	}
	return s;
}

/*
 * make a reference to symbol at offset
 */
void
makeref(struct symbol *sym, unsigned int offset)
{
	struct ref *rp;

	if (verbose & V_SYM) {
		printf("makeref: %s at %d\n", sym->name, offset);
	}
	rp = malloc(sizeof(*rp));
	rp->sym = sym;
	rp->offset = offset;
	rp->next = reflist;
	reflist = rp;
}

/*
 * microsoft rel file
 * this is a bit string encoded format with symbols and relocation information
 * one feature of this binary format is that it employs the operand fields of
 * instructions to chain together references to the same address, so if 5 jump
 * instructions all go the same place, they are all chained together.  this
 * is pretty compact.
 */
char *controltype[] = { "special", "code", "data", "common" };

char *specialtype[] = { 
	"entry", "common", "progname", "unused3",
	"unused4", "commonsize", "chainext", "define",
	"unused8", "extoff", "datasize", "setloc",
	"chainaddr", "progsize", "endmodule", "end"
};

/*
 * whenever we get an address chain, we need to chase down the references
 * once we have built the segments.  we need to track this work to do.
 */
struct refchain {
	struct symbol *sym;
	int offset;
	struct refchain *next;
} *rc_head;

void
makechain(struct symbol *sp, unsigned short link)
{
	struct refchain *rc;

	if (verbose & V_SYM) {
		printf("makechain: %s at %d\n", sp->name, link);
	}

	rc = malloc(sizeof(*rc));
	rc->sym = sp;
	rc->offset = link;
	rc->next = rc_head;
}

/*
 * process all the reference chains to create individual refs
 */
void
fixup_xrefs(buffer_t *out)
{
	int addr;
	struct refchain *rc;

	/* each chain is a list of references to the same symbol */
	while ((rc = rc_head)) {
		printf("chain %s\n", rc->sym->name);
		while ((addr = rc->offset) != 0) {
			printf("\tlink %d\n", addr);
			makeref(rc->sym, addr);
			rc->offset = out->storage[addr] + (out->storage[addr+1] << 8);
			out->storage[addr] = out->storage[addr+1] = 0;
		}
		rc_head = rc->next;
		free(rc);
	}
		
}

/*
 * rel file A fields are 2 bits of segment and 16 bits of value
 */
struct value {
	unsigned char seg;
	unsigned short val;
} valdata;

struct value *
readvalue(buffer_t *in)
{
	char seg;
	unsigned short val;

	valdata.seg = getbits(in, 2);
	val = getbits(in, 16);	
	valdata.val = ((val >> 8) & 0xff) | ((val & 0xff) << 8);
	return &valdata;
}

/*
 * B fields are 3 bits of symbol length and that many bytes of name
 */
char *
readsym(buffer_t *in)
{
	static char symbuf[10];
	char symlen;
	char i;

	symbuf[0] = 0;

	symlen = getbits(in, 3);
	for (i = 0; i < symlen; i++) {
		symbuf[i] = getbits(in, 8);
		symbuf[i+1] = 0;
	}
	return symbuf;
}

int
do_rel(buffer_t *in, buffer_t *out)
{
	unsigned short val;
	unsigned char control;
	struct value *valp;
	char *name;
	char done = 0;
	char i;
	struct symbol *sym;

	while (!done) {
		if (in->cursor > in->size) break;
		if (getbits(in, 1)) {
			control = getbits(in, 2);
			if (control == 0) {
				control = getbits(in, 4);
				name = 0; 
				valp = 0;
				switch (control) {
				case 0:		// entry symbol NAME
					name = readsym(in);
					break;
				case 1:		// select common block NAME
					name = readsym(in);
					break;
				case 2:		// program name NAME
					name = readsym(in);
					break;
				case 3:		// unused
				case 4:		// unused
				case 5:		// define common size VALUE
					valp = readvalue(in);
					name = readsym(in);
					break;
				case 6:		// chain external VALUE
					/* entries of this form are the head of a list of references
					 * to the same external symbol */
					valp = readvalue(in);
					name = readsym(in);
					makechain(name, valp->val + segbase[valp->seg]);
					break;
				case 7:		// define entry NAME VALUE
					valp = readvalue(in);
					name = readsym(in);
					makesym(name, segbase[valp->seg] + valp->val);
					break;
				case 8:		// unused
				case 9:		// external plus offset VALUE
					valp = readvalue(in);
					printf("XXX not handled\n");
					break;
				case 0xa:	// define data size VALUE
					valp = readvalue(in);
					makeseg(S_DATA, valp->val);
					break;
				case 0xb:	// set location VALUE
					valp = readvalue(in);
					out->cursor = segbase[valp->seg] + valp->val;
					break;
				case 0xc:	// chain address VALUE
					name = makelabel(out->cursor);
					valp = readvalue(in);
					makechain(name, valp->val + segbase[valp->seg]);
					break;
				case 0xd:	// define program size VALUE
					valp = readvalue(in);
					makeseg(S_CODE, valp->val);
					break;
				case 0xe:	// end module VALUE
					valp = readvalue(in);
					modcount++;
					if (in->bitoff != 0) {
						in->bitoff = 0;
						in->cursor++;
					}
					break;
				case 0xf:	// end file
					done++;
					break;
				break;
				}				
				if (verbose & V_PARSE) {
					printf("special entry %d %s ", control, specialtype[control]);
					if (name) {
						printf("%s ", name);
					}
					if (valp) {
						printf(" = 0x%x (%s)", valp->val, segname[valp->seg]);
					}
					printf("\n");
				}
			} else {
				val = getbits(in, 16);
				val = ((val & 0xff) << 8) | ((val >> 8) & 0xff);
				if (verbose & V_PARSE) {
					printf("%x rel entry %d %s %x %x\n", 
						out->cursor, control, controltype[control], segbase[control], val);
				}
				val += segbase[control];
				
				makeref(makelabel(val), out->cursor);
				put(out, out->cursor, val & 0xff);
				put(out, out->cursor, (val >> 8) & 0xff);
			}
		} else {
			val = getbits(in, 8);
			if (verbose & V_PARSE) {
				printf("%x abs entry %x\n", out->cursor, val);
			}
			put(out, out->cursor, val);
		}
	}
	fixup_xrefs(out);
	return (0);
}

/*
 * the whole object file has been read in, so now just spew the bytes out
 * after all the relocation
 */
int
do_whitesmith(buffer_t *in, buffer_t *out)
{
	struct obj *hp;
	int i;
	struct symtab *symtab;
	int nsyms;
	char *ip;
	struct reloc *rp;
	unsigned short addr;

	hp = (struct obj *)(in->storage);		/* header */
	for (i = 0; i < sizeof(struct obj); i++) {
		advance(in);
	}
	
	start = 0;

	/* snarf the text segment */
	makeseg(S_CODE, hp->text);
	for (i = 0 ; i < hp->text; i++) {
		put(out, out->cursor, get(in));
		advance(in);
	}

	/* read the data segment */
	makeseg(S_DATA, hp->data);
	for (i = 0 ; i < hp->data; i++) {
		put(out, out->cursor, get(in));
		advance(in);
	}

	/* read the symbol table */
	symtab = (struct symbol *)(&in->storage[in->cursor]);
	nsyms = hp->table / sizeof(struct symtab);
	for (i = 0; i < nsyms; i++) {
		out->cursor += sizeof(struct symtab);
		if (symtab[i].flag & SF_DEF) {
			makesym(symtab[i].name, symtab[i].value);
		}
	}
	segoffset = 0;
	while (rp = getreloc(&ip)) {
		addr = out->storage[segoffset] + out->storage[segoffset+1];			
		switch (rp->type) {
                case REL_TEXTOFF:
			makeref(makelabel(addr), addr);
                        break;
                case REL_DATAOFF:
			makeref(makelabel(addr+hp->data), addr+hp->data);
                        break;
                case REL_SYMBOL:
			makeref(&symtab[rp->value], segoffset);
                        break;
		}
	}

	/* process the text relocations */
	/* process the data relocations */
	return (0);
}

/*
 * match various object formats.  the first one to hit is used.
 * note that the rel file is a default and always matches, since it has
 * no magic number to recognize. what a crock.
 */
struct in_format {
	char *name;
	char *match;
	int matchoff;
	int (*handler)(buffer_t *in, buffer_t *out);
} bin_formats[] = {
	{ "intel hex", ":", 0, do_intel_hex },
	{ "motorola S-rec", "S", 0, do_motorola_srec },
	{ "hex dump", "0", 0, do_hexdump },
	{ "whitesmith", "\231", 0, do_whitesmith },
	{ "rel file", 0, 0, do_rel }
};

int
decode(char *src, int size, unsigned char **dest, unsigned long *address)
{
	int i;
	buffer_t *in;
	buffer_t *out;

	for (i = 0; i < sizeof(bin_formats)/sizeof(bin_formats[0]); i++) {
		
		if ((bin_formats[i].match) && 
			strncmp(&src[bin_formats[i].matchoff], 
			bin_formats[i].match, 
			strlen(bin_formats[i].match)) != 0) {
			continue;
		}
	
		if (verbose) {
			printf("# format: %s\n", bin_formats[i].name);
		}
		in = alloca(sizeof(buffer_t));
		in->storage = (unsigned char *)src;
		in->cursor = 0;
		in->base = 0;
		in->size = size;
		in->lineno = 1;
		in->bitoff = 0;

		out = alloca(sizeof(buffer_t));
		out->storage = 0;
		out->cursor = 0;
		out->base = 0;
		out->high = 0;
		out->size = 0;

		while ((*bin_formats[i].handler)(in,out))
			;

		*address = start;
		*dest = out->storage;
		if (verbose) {
			printf("decoded using %s %d bytes\n", bin_formats[i].name, out->high+1);
		}
		return (out->high + 1);
	}
	return (0);
}
