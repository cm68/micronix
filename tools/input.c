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

char *segname[] = { "absolute", "code", "data", "common", "undef" };

int segbase[5];
unsigned short segoffset;

/*
 * random utility function
 */
unsigned char binbuf[17];
unsigned char *
binstr(unsigned short val, char count)
{
	unsigned char *s = binbuf;
	int i;

	binbuf[0] = 0;

	for (i = count-1; i >= 0; i--) {
		if ((val >> i) & 0x1) {
			*s++ = '1';
		} else {
			*s++ = '0';
		}
		*s = '\0';
	}
	return binbuf;
}

/*
 * the input source
 */
unsigned char *inbuf;
unsigned char *inptr;
unsigned char *inend;
int bitoff;

/*
 * the output buffer
 */
unsigned char *outbuf;
unsigned short outsize;
unsigned short outhigh;

/*
 * the microsoft rel format is built on a bit stream. 
 * we need to be able to grab variable numbers of bits and maintain a bit cursor.
 */
unsigned short
getbits(int len)
{
	unsigned short ret = 0;
	char i;
	unsigned char v;

	v = *inptr;
	/*
	if (verbose & V_BUF) {
		unsigned short vv = (inptr[0] << 8) + inptr[1];
		printf("\ngetbits0: len %d bitoff %d %04x %s\n", len, bitoff, v,
			binstr(vv, 16));
	}
	*/
	for (i = len - 1; i >= 0; i--) {
		if ((1 << (7 - bitoff)) & v) {
			ret |= 1 << i;
		}
		bitoff++;
		if (bitoff == 8) {
			v = *++inptr;
			bitoff = 0;
		}
	}
	if (verbose & V_BUF) {
		printf("getbits: %d => %x %s\n", len, ret, binstr(ret, len));
	}
	return ret;
}

/*
 * write to our decoded output buffer - allocate memory if need be
 */
void
put(int where, unsigned char c)
{
	int size;

	if (verbose & V_OUT) {
		printf("put 0x%02x to 0x%04x\n", c, where);
	}
	if (where >= outsize) {
		/* round up to 4k */
		size = (where | 0xfff) + 1;
		outbuf = realloc(outbuf, size);
		outsize = size;
	}
	if (where > outhigh) {
		outhigh = where;
	}
	outbuf[where] = c;
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
gethex(int digits, unsigned char *checksum)
{
	unsigned char c;
	unsigned int v = 0;
	int ask = digits;
	while (digits--) {
		c = *inptr;
		if (ishex(c)) {
			c = readhex(c);
		} else {
			printf("malformed hex (0x%x)\n", c);
			return -1;
		}
		v = (v << 4) + c;
		inptr++;
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
do_intel_hex()
{
	int rectype;
	int bc;
	int addr;
	unsigned char checksum;
	int byte;
		
	if ((byte = *inptr) != ':') {
		printf("missing record start (0x%x)\n", byte);
		return 0;
	}
	inptr++;

	checksum = 0;
	bc = gethex(2, &checksum);
	if (bc == -1) {
		printf("missing byte count\n");
		return 0;
	}

	addr = gethex(4, &checksum);
	if (addr == -1) {
		printf("missing address\n");
		return 0;
	}

	rectype = gethex(2, &checksum);
	if (addr == -1) {
		printf("missing record type\n");
		return 0;
	}

	switch (rectype) {

	case 0x0:	/* data record */
		while (bc--) {
			byte = gethex(2, &checksum);
			if (byte == -1) {
				printf("missing data byte\n");
				return 0;
			}
			put(addr++, byte);
		}
		break;

	case 0x1:	/* eof */
		start = addr;
		break;

	case 0x2:	/* extended segment address record */
		if (addr != 0) {
			printf("nonzero address on type 2\n");
			return 0;
		}
		addr = gethex(4, &checksum) << 4;
		if (addr == -1) {
			printf("missing seg offset\n");
			return 0;
		}
		break;

	case 0x3:	/* start segment address record */
		if (addr != 0) {
			printf("nonzero address on type 3\n");
			return 0;
		}
		start = gethex(8, &checksum);
		if (start == -1) {
			printf("missing CS:IP\n");
			return 0;
		}
		break;

	case 0x4:	/* extended linear address record */
		addr = gethex(4, &checksum) << 16;
		if (addr == -1) {
			printf("missing seg offset\n");
			return 0;
		}
		break;

	case 0x5:	/* start linear address record */
		if (addr != 0) {
			printf("nonzero address on type 3\n");
			return 0;
		}
		start = gethex(8, &checksum);
		if (start == -1) {
			printf("missing EIP\n");
			return 0;
		}
		break;

	default:
		printf("bogus record type (0x%x)\n", rectype);
		return 0;
	}

	if (gethex(2, &checksum) == -1) {
		printf("missing checksum\n");
		return 0;
	}
	if (checksum != 0) {
		printf("checksum error (0x%x)\n", checksum);
		return 0;
	}

	/* let's swallow line and file ending */
	while ((byte = *inptr) == '\r' || byte == '\n' || byte == 0x26) {
		inptr++;
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
do_motorola_srec()
{
	int rectype;
	int bc;
	int addr;
	unsigned char checksum;
	int byte;
	
	if ((byte = *inptr) != 'S') {
		printf("missing record start (0x%x)\n", byte);
		return 0;
	}
	inptr++;

	rectype = gethex(1, &checksum);
	if (rectype == -1) {
		printf("missing record type\n");
		return 0;
	}

	/* checksum does not include rec type */
	checksum = 0;

	bc = gethex(2, &checksum);
	if (bc == -1) {
		printf("missing byte count\n");
		return 0;
	}
	switch (rectype) {

	case 0x1:	/* 16 bit address data record */
		addr = gethex(4, &checksum);
		if (addr == -1) {
			printf("missing address\n");
			return 0;
		}
		bc -= 3;
		while (bc--) {
			byte = gethex(2, &checksum);
			if (byte == -1) {
				printf("missing data byte\n");
				return 0;
			}
			put(addr++, byte);
		}
		break;

	case 0x9:	/* eof */
		addr = gethex(4, &checksum);
		if (addr == -1) {
			printf("missing address\n");
			return 0;
		}
		start = addr;
		break;

	default:
		printf("bogus record type (0x%x)\n", rectype);
		return 0;
	}

	if (gethex(2, &checksum) == -1) {
		printf("missing checksum\n");
		return 0;
	}
	if (checksum != 0xff) {
		printf("checksum error (0x%x)\n", checksum);
		return 0;
	}

	/* let's swallow line and file ending */
	while ((byte = *inptr) == '\r' || byte == '\n' || byte == 0x26) {
		inptr++;
	}
	return (rectype == 0x9) ? 0 : 1;
}

/*
 * binary file handler for simple hex dump:
 * lines consist of [[0x]n[n]*:] [[0x]n[n]*]*
 * the recognizer is that the first significant character must be '0'.
 */
int
do_hexdump()
{
	int byte;
	unsigned int data = 0;
	int digits = 0;
	int location = 0;

	/* swallow newlines tabs and spaces */
	while (1) {
		byte = *inptr;
		if (byte == '\0') {
			return (0);
		} else if (byte == '\n' || byte == '\t' || byte == ' ') {
			inptr++;
		} else {
			break;
		}
	}

	/* 0x is a benign prefix */
	if ((inptr[0] == '0') && (inptr[1] == 'x')) {
		inptr += 2;
	}

	/* let's build a number */
	while (ishex(*inptr)) {
		data <<= 4;
		data = data + readhex(*inptr++);
		digits++;
	} 

	/* ah. we have an address */
	if (*inptr == ':') {
		inptr++;
		if (verbose & V_PARSE) {
			printf("address %x\n", data);
		}

		if (data != location) {
			if (data < location) {
				printf("load address ordering inversion\n");
				return (0);
			}
			while (location < data) {
				put(location++, 0);
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
			put(location++, byte);
		}
	} else {
		while (digits--) {
			byte = data & 0xff;
			put(location++, byte);
			data >>= 8;
		}
	}
	return (1);
}

void
makeseg(char type, int addr, int size)
{
	struct seg *sp;

	sp = malloc(sizeof(*sp));
	sp->module = modcount;
	sp->type = type;
	sp->base = addr;
	sp->len = size;
	sp->next = seglist;
	segbase[type] = addr;
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
makesym(char *name, int offset, int seg)
{
	struct symbol *sp;

	if (verbose & V_SYM) {
		printf("makesym: %s %s:%d\n", name, segname[seg], offset);
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
	sp->seg = seg;
	symbols = sp;
	return (sp);
}

/*
 * lookup a symbol
 */
struct symbol *
lookupsym(char *name)
{
	struct symbol *sp = 0;

	if (verbose & V_SYM) {
		printf("lookupsym: %s\n", name);
	}
	for (sp = symbols; sp; sp = sp->next) {
		if (strcmp(sp->name, name) == 0) {
			break;
		}
	}
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
	return makesym(labbuf, offset, S_CODE);
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
		if (verbose & V_SYM) {
			printf("refname: %d %s\n", rp->offset, rp->sym->name);
		}
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
fixup_chains()
{
	int addr;
	struct refchain *rc;

	/* each chain is a list of references to the same symbol */
	while ((rc = rc_head)) {
		printf("chain %s\n", rc->sym->name);
		while ((addr = rc->offset) != 0) {
			printf("\tlink %d\n", addr);
			makeref(rc->sym, addr);
			rc->offset = outbuf[addr] + (outbuf[addr+1] << 8);
			outbuf[addr] = outbuf[addr+1] = 0;
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
readvalue()
{
	char seg;
	unsigned short val;

	valdata.seg = getbits(2);
	val = getbits(16);	
	valdata.val = ((val >> 8) & 0xff) | ((val & 0xff) << 8);
	return &valdata;
}

/*
 * B fields are 3 bits of symbol length and that many bytes of name
 */
char *
readsym()
{
	static char symbuf[10];
	char symlen;
	char i;

	symbuf[0] = 0;

	symlen = getbits(3);
	for (i = 0; i < symlen; i++) {
		symbuf[i] = getbits(8);
		symbuf[i+1] = 0;
	}
	return symbuf;
}

int
do_rel()
{
	unsigned short val;
	unsigned char control;
	struct value *valp;
	char *name;
	char done = 0;
	char i;
	struct symbol *sym;
	unsigned short location;

	while (!done) {
		if (inptr > inend) break;
		if (getbits(1)) {
			control = getbits(2);
			if (control == 0) {
				control = getbits(4);
				name = 0; 
				valp = 0;
				switch (control) {
				case 0:		// entry symbol NAME
					name = readsym();
					break;
				case 1:		// select common block NAME
					name = readsym();
					break;
				case 2:		// program name NAME
					name = readsym();
					break;
				case 3:		// unused
				case 4:		// unused
				case 5:		// define common size VALUE
					valp = readvalue();
					name = readsym();
					break;
				case 6:		// chain external VALUE
					/* entries of this form are the head of a list of references
					 * to the same external symbol */
					valp = readvalue();
					name = readsym();
					sym = makesym(name, 0, S_UNDEF);
					makechain(sym, valp->val + segbase[valp->seg]);
					break;
				case 7:		// define entry NAME VALUE
					valp = readvalue();
					name = readsym();
					makesym(name, segbase[valp->seg] + valp->val, S_CODE);
					break;
				case 8:		// unused
				case 9:		// external plus offset VALUE
					valp = readvalue();
					printf("XXX not handled\n");
					break;
				case 0xa:	// define data size VALUE
					valp = readvalue();
					makeseg(S_DATA, 0, valp->val);
					break;
				case 0xb:	// set location VALUE
					valp = readvalue();
					location = segbase[valp->seg] + valp->val;
					break;
				case 0xc:	// chain address VALUE
					sym = makelabel(location);
					valp = readvalue();
					makechain(sym, valp->val + segbase[valp->seg]);
					break;
				case 0xd:	// define program size VALUE
					valp = readvalue();
					makeseg(S_CODE, 0, valp->val);
					break;
				case 0xe:	// end module VALUE
					valp = readvalue();
					modcount++;
					if (bitoff != 0) {
						bitoff = 0;
						inptr++;
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
				val = getbits(16);
				val = ((val & 0xff) << 8) | ((val >> 8) & 0xff);
				if (verbose & V_PARSE) {
					printf("%x rel entry %d %s %x %x\n", 
						location, control, controltype[control], segbase[control], val);
				}
				val += segbase[control];
				
				makeref(makelabel(val), location);
				put(location++, val & 0xff);
				put(location++, (val >> 8) & 0xff);
			}
		} else {
			val = getbits(8);
			if (verbose & V_PARSE) {
				printf("%x abs entry %x\n", location, val);
			}
			put(location++, val);
		}
	}
	fixup_chains();
	return (0);
}

struct obj *wsobj;
struct ws_symbol *ws_sym;

void
read_reloc(int seg)
{
	unsigned short addr;
	struct ws_reloc *rp;

	segoffset = segbase[seg];

	while ((rp = getreloc(&inptr))) {
		addr = outbuf[segoffset] + outbuf[segoffset+1];			
		switch (rp->type) {
                case REL_TEXTOFF:
			makeref(makelabel(addr), addr);
                        break;
                case REL_DATAOFF:
			makeref(makelabel(addr+wsobj->data), addr+wsobj->data);
                        break;
                case REL_SYMBOL:
			makeref(lookupsym(ws_sym[rp->value].name), rp->offset);
                        break;
		}
	}
}

/*
 * the whole object file has been read in, so now just spew the bytes out
 * after all the relocation
 */
int
do_whitesmith()
{
	int i;
	int nsyms;
	char *ip;
	unsigned short addr;
	unsigned short location;
	struct symbol *sym;
	int seg;

	wsobj = (struct obj *)inptr;		/* header */
	for (i = 0; i < sizeof(struct obj); i++) {
		inptr++;
	}
	
	location = 0;
	start = 0;

	/* snarf the text segment */
	makeseg(S_CODE, 0, wsobj->text);
	for (i = 0 ; i < wsobj->text; i++) {
		put(location++, *inptr++);
	}

	location = wsobj->text;

	/* read the data segment */
	makeseg(S_DATA, wsobj->text, wsobj->data);
	for (i = 0 ; i < wsobj->data; i++) {
		put(location++, *inptr++);
	}

	/* read the symbol table */
	ws_sym = (struct ws_symbol *)inptr;
	nsyms = wsobj->table / sizeof(*ws_sym);
	for (i = 0; i < nsyms; i++) {
		inptr += sizeof(*ws_sym);
		if (ws_sym[i].flag & (SF_TEXT|SF_DEF)) {
			seg = S_CODE;
		} else if (ws_sym[i].flag & (SF_DATA|SF_DEF)) {
			seg = S_DATA;
		} else {
			seg = S_UNDEF;
		}
		sym = makesym(ws_sym[i].name, ws_sym[i].value, seg);
	}

	/* process the relocations */
	read_reloc(S_CODE);
	read_reloc(S_DATA);

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
	int (*handler)();
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

		inbuf = inptr = (unsigned char *)src;
		inend = inptr + size;

		bitoff = 0;

		outsize = 8192;
		outbuf = malloc(outsize);
		outhigh = 0;

		while ((*bin_formats[i].handler)())
			;

		*address = start;
		*dest = outbuf;
		if (verbose) {
			printf("decoded using %s %d bytes\n", bin_formats[i].name, outhigh+1);
		}
		return (outhigh + 1);
	}
	return (0);
}
