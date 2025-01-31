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
int low;
int high;
int modcount;
char *modname;
int undefs;


char *segname[] = { "absolute", "code", "data", "common", "undef" };

struct seg *curseg[5];
struct seg *myseg;

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
unsigned short location;

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
		*s = '\0';
	}
	return binbuf;
}

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

	location = 0;

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

/*
 * let's create another segment, which is a region into the output buffer
 */
void
makeseg(int module, seg_t seg, int addr, int size)
{
	struct seg *sp;

	checksym();
	if (verbose & V_SYM) {
		printf("makeseg: module: %d(%s) seg: %d(%s) offset: 0x%x size: 0x%x\n",
			module, modname ? modname:"undef", seg, segname[seg], addr, size);
	}

	sp = malloc(sizeof(*sp));
	sp->module = module;
	sp->modname = modname;
	sp->type = seg;
	sp->base = addr;
	sp->len = size;
	sp->next = seglist;

	curseg[seg] = sp;
	seglist = sp;
	myseg = sp;
	checksym();
}

/*
 * for an output buffer offset, what is the segment
 */
struct seg *
lookup_seg(addr_t addr)
{
	struct seg *sp;

	for (sp = seglist; sp; sp = sp->next) {
		if (addr >= sp->base && addr < sp->base + sp->len) {
			return sp;
		}
	}
	return 0;
}

/*
 * if this is code, we want to disassemble, no?
 */
int
is_code(addr_t addr) {
	struct seg *sp;

	sp = lookup_seg(addr);
	if (sp && sp->type == S_CODE) {
		return 1;
	} else {
		return 0;
	}
}

struct symbol *symbols;
char *
symspec(struct symbol *s)
{
	static char symbuf[30];

	sprintf(symbuf, "%s.%s:0x%x", 
		s->name, s->segp->modname, segname[s->segp->type], s->offset);
	return (symbuf);
}

/*
 * look up the symbol by the output buffer offset
 * this is very likely to be different from the segment offset
 */
char *
getsymname(addr_t address)
{
	char *s = 0;
	struct symbol *sp;
	struct seg *segp;
	int offset;
	
	/* first, find out what segment this address is */
	segp = lookup_seg(address);
	if (!segp || (segp->type == S_UNDEF)) {
		printf("getsymname: bogus address 0x%x\n", address);
		return (0);
	}

	/* subtract off the segment base */
	offset = address - segp->base;

	for (sp = symbols; sp; sp = sp->next) {
		if ((sp->segp == segp) && (sp->offset == offset)) {
			s = sp->name;
			break;
		}
	}
	if (verbose & V_SYM) {
		if (verbose & V_EXTRA) {
			printf("getsymname: 0x%x %s.%s:0x%x ", 
				address, segp->modname, segname[segp->type], offset);
			if (s) {
				printf("%s\n",  s);
			} else {
				printf("no label\n");
			}
		}
	}
	return s;
}

/*
 * register a symbol - this is given a bogus offset if it is an external
 */
struct symbol *
makesym(char *name, segoff_t offset, seg_t seg)
{
	struct symbol *sp;

	if (verbose & V_SYM) {
		if (verbose & V_EXTRA) {
			printf("makesym: %s %s.%s:0x%x\n", name, modname, segname[seg], offset);
		}
	}
	for (sp = symbols; sp; sp = sp->next) {
		if (strcmp(sp->name, name) == 0) {
			/* if was undef, and now def, redefine it */
			if ((sp->segp->type == S_UNDEF) && (seg != S_UNDEF)) {
				goto redef;
			}
			if ((seg != S_UNDEF) && ((sp->segp->type != seg) || (sp->offset != offset))) {
				printf("makesym: incompatible redef %s %s.%s:0x%x was %s.%s:0x%x\n", 
					name, modname, segname[seg], offset,
					sp->segp->modname, segname[sp->segp->type], sp->offset);
			}
			return sp;
		}
	}
	sp = malloc(sizeof(*sp));
	sp->next = symbols;
	symbols = sp;
	sp->name = strdup(name);
	if (seg == S_UNDEF) {
		offset = undefs++;
	}
redef:
	sp->offset = offset;
	sp->segp = curseg[seg];
	sp->invented = 0;
	return (sp);
}

/*
 * lookup a symbol by name
 */
struct symbol *
lookupsym(char *name)
{
	struct symbol *sp = 0;

checksym();

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

checksym()
{
	struct symbol *sp;
	for (sp = symbols; sp; sp = sp->next) {
		if (strlen(sp->name) == 0) {
			printf("lose\n");
		}
	}
}

/*
 * invent a symbol abd register it
 */
struct symbol *
makelabel(segoff_t offset, seg_t seg)
{
	struct symbol *s;

	static char labbuf[20];
checksym();

	sprintf(labbuf, "%s%d_%x", (seg == S_CODE) ? "L":"D", modcount, offset);
	s = makesym(labbuf, offset, seg);
	s->invented = 1;
checksym();
	return s;
}

/*
 * dump out symbols
 */
dumpsyms()
{
	struct symbol *s;
	printf("symbols:\n");
	for (s = symbols; s; s = s->next) {
		printf("%s\n", symspec(s));
	}
}

/*
 * references are operand fields that refer to symbols
 */
struct ref *reflist;

struct ref *
getref(addr_t offset)
{
	struct ref *rp;

	for (rp = reflist; rp; rp = rp->next) {
		if (rp->offset == offset) {
			return (rp);
		}
	}
	return 0;
}

/*
 * make a reference to symbol at offset
 */
void
makeref(struct symbol *sym, unsigned int offset)
{
	struct ref *rp;

	rp = malloc(sizeof(*rp));
	rp->sym = sym;
	rp->offset = offset;
	rp->bias = 0;
	rp->next = reflist;
	reflist = rp;
}

/*
 * dump out references
 */
dumprefs()
{
	struct ref *r;
	for (r = reflist ; r ; r = r->next) {
		printf("ref to %s at 0x%x\n", symspec(r->sym), r->offset);
	}
}

/*
 * microsoft rel file
 * this is a bit string encoded format with symbols and relocation information
 * one feature of this binary format is that it employs the operand fields of
 * instructions to chain together references to the same address, so if 5 jump
 * instructions all go the same place, they are all chained together.  this
 * is pretty compact.
 * also, chains have the useful property that they always run backwards from
 * high addresses to low, so we have already scribbled the data into the output
 * buffer.
 *
 */

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

/*
 * walk the chain backwards starting at off in seg, and make a reference to symbol
 * at each of the links
 */
void
walkchain(struct symbol *s, segoff_t off, struct seg *seg)
{
	addr_t addr;

	if (verbose & V_SYM) {
		printf("chain %s.%s base:0x%x %s.%s 0x%x(0x%x)", 
			seg->modname, segname[seg->type], seg->base, 
			s->segp->modname, s->name, 
			off, off + seg->base);
	}
	/* while there are links backwards */
	while (off) {
		/* register absolute adddress of the reference to the symbol */
		addr = off + seg->base;
		makeref(s, addr);

		/* read the next link */
		off = outbuf[addr] + (outbuf[addr+1] << 8);
		if (verbose & V_SYM) {
			printf(" link 0x%x(0x%x)", off, off + seg->base);
		}
		if (off + seg->base > addr) {
			printf(" chain inversion\n");
		}
		outbuf[addr] = outbuf[addr+1] = 0;
	}
	if (verbose & V_SYM) {
		printf("\n");
	}
}

char *controltype[] = { "special", "code", "data", "common" };

/*
 * the bit stream has the following parse:
 *
 * <seg> is 2 bits   	0: absolute 1: code 2: data 3:common
 * <byte> is 8 bits
 * <word> is 16 bits
 * <width> is 3 bits
 * <name> is <width> bytes
 *
 * 0 <byte>		absolute data
 * 1 01 <word>  	program relative
 * 1 10 <word>  	data relative
 * 1 11 <word>  	common relative
 * 1 00 special link
 *      0000 <width> <name>			entry symbol
 *      0001 <width> <name>			select common
 *      0010 <width> <name>			program name
 *      0011 <width> <name>			library search
 *      0100 <width> <name>			RFU	
 *      0101 <seg> <word> <width> <name>	common size
 *      0110 <seg> <word> <width> <name>	chain external
 *      0111 <seg> <word> <width> <name>	define symbol
 *      1000 <seg> <word> <width> <name>	external - offset
 *      1001 <seg> <word>			external + offset
 *      1010 <seg> <word>			data size
 *      1011 <seg> <word>			set location
 *      1100 <seg> <word>			chain location
 *      1101 <seg> <word>			code size
 *      1110 <seg> <word>			end module
 *      1111					end file
 */

/*
 * special link types can have optional items in the bit stream
 */
struct speciallink {
	char *name;
	char flags;
#define	CT_A		0x01	/* has A field - a 16 bit value */
#define	CT_B		0x02	/* has B field - a name */
#define	CT_UNIMPL	0x80	/* print a message */
};

struct speciallink speciallink[] = { 
	"entry", CT_B,				/* 0 */
	"common", CT_B|CT_UNIMPL,		/* 1 */
	"module", CT_B,				/* 2 */
	"libsearch",CT_B|CT_UNIMPL,		/* 3 */
	"rfu", CT_B|CT_UNIMPL,			/* 4 */
	"commonsize", CT_A|CT_B|CT_UNIMPL,	/* 5 */
	"chainext", CT_A|CT_B,			/* 6 */
	"symbol", CT_A|CT_B,			/* 7 */
	"extoffset", CT_A|CT_B|CT_UNIMPL, 	/* 8 */
	"extoff", CT_A,				/* 9 */
	"datasize", CT_A,			/* 10 */
	"setloc", CT_A,				/* 11 */
	"chainaddr", CT_A,		 	/* 12 */
	"progsize", CT_A,		 	/* 13 */
	"endmodule", CT_A,			/* 14 */
	"end", 0,				/* 15 */
};

struct bias {
	unsigned short addition;
	unsigned short address;
	struct bias *next;
} *biases;

void
addbias(unsigned short addr, unsigned short bias)
{
	struct bias *b;

	b = malloc(sizeof(*b));
	b->address = addr;
	b->addition = bias;
	b->next = biases;
	biases = b;
}

void
fixbias()
{
	struct bias *b;
	unsigned short value;
	struct ref *rp;

	for (b = biases ; b; b = b->next) {
		rp = getref(b->address);
		if (!rp) {
			printf("no rel for bias\n");
#ifdef notdef
			value = (outbuf[b->address] << 8) + outbuf[b->address+1] + b->addition;
			outbuf[b->address] = (value >> 8) & 0xff;	
			outbuf[b->address+1] = value & 0xff;	
#endif
			continue;
		}
		rp->bias = b->addition;
	}
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

	undefs = 0;
	makeseg(modcount, S_UNDEF, 65536 + undefs, 65536);
	curseg[S_UNDEF]->modname = "undef";

	location = 0;

	while (!done) {
		if (inptr > inend) break;
		/* absolute byte */
		if (getbits(1) == 0) {
			val = getbits(8);
			if (verbose & V_PARSE) {
				if (verbose & V_EXTRA) {
					printf("(%s.%s:0x%x) 0x%x abs entry %x\n", 
					myseg->modname, segname[myseg->type], location - myseg->base, location, val);
				}
			}
			put(location++, val);
		} else {
			control = getbits(2);
			/* special link item 1 00 */
			if (control > 0) {
				/* relative entry 1 seg: [ 01 10 11 ] */
				val = getbits(16);
				val = ((val & 0xff) << 8) | ((val >> 8) & 0xff);
				if (verbose & V_PARSE) {
					printf("0x%x %s relative 0x%x = 0x%x\n", 
						location, controltype[control], val, curseg[control]->base + val);
				}
				/* this might be part of a chain, and then we want to replace the ref */
				makeref(makelabel(val, control), location);
				put(location++, (val & 0xff));
				put(location++, (val >> 8) & 0xff);
			} else {
				control = getbits(4);
				name = 0; 
				valp = 0;

				if (speciallink[control].flags & CT_A) {
					valp = readvalue();
				}
				if (speciallink[control].flags & CT_B) {
					name = readsym();
				}

				if ((verbose & V_PARSE) || (speciallink[control].flags & CT_UNIMPL)) {
					printf("0x%x special entry %d %s ", 
						location, control, speciallink[control].name);
					if (name) {
						printf("%s ", name);
					}
					if (valp) {
						printf(" = 0x%x (%s)", valp->val, segname[valp->seg]);
					}
					if (speciallink[control].flags & CT_UNIMPL) {
						printf(" not handled");
					}
					printf("\n");
				}

				switch (control) {
				default:
					break;
				case 2:		// program name NAME
					modname = strdup(name);
					break;
				case 6:		// chain external VALUE
					/* entries of this form are the head of a list of references
					 * to the same external symbol */
					sym = makesym(name, 0, S_UNDEF);
					walkchain(sym, valp->val, myseg);
					break;
				case 7:		// define entry NAME VALUE
					makesym(name, valp->val, S_CODE);
					break;
				case 9:		// extoff - add valp to the 16 bits at location
					addbias(location, valp->val);
					break;
				case 0xa:	// define data size VALUE
					makeseg(modcount, S_DATA, location, valp->val);
					break;
				case 0xb:	// set location VALUE
					myseg = curseg[valp->seg];
					location = curseg[valp->seg]->base + valp->val;
					break;
				case 0xc:	// chain address VALUE
					sym = makelabel(location - myseg->base, myseg->type);
					walkchain(sym, valp->val, myseg);
					break;
				case 0xd:	// define program size VALUE
					makeseg(modcount, S_CODE, location, valp->val);
					break;
				case 0xe:	// end module VALUE
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

			}
		}
	}
	if (verbose & V_SYM) dumpsyms();
	if (verbose & V_REF) dumprefs();
	fixbias();
	return (0);
}

struct obj *wsobj;
struct ws_symbol *ws_sym;

void
read_reloc(int seg)
{
	unsigned short addr;
	struct ws_reloc *rp;

	location = curseg[seg]->base;
	myseg = curseg[seg];

	while ((rp = getreloc(&inptr))) {
		addr = outbuf[location] + outbuf[location+1];			
		switch (rp->type) {
                case REL_TEXTOFF:
			makeref(makelabel(addr, S_CODE), addr);
                        break;
                case REL_DATAOFF:
			makeref(makelabel(addr+wsobj->data, S_DATA), addr+wsobj->data);
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
	struct symbol *sym;
	int seg;

	wsobj = (struct obj *)inptr;		/* header */
	for (i = 0; i < sizeof(struct obj); i++) {
		inptr++;
	}
	
	location = 0;
	start = 0;

	/* snarf the text segment */
	makeseg(modcount, S_CODE, 0, wsobj->text);
	for (i = 0 ; i < wsobj->text; i++) {
		put(location++, *inptr++);
	}

	location = wsobj->text;

	/* read the data segment */
	makeseg(modcount, S_DATA, wsobj->text, wsobj->data);
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
 * binary file handler for intel hex - only really deal with these two types
 * all records have a checksum XX appended
 * :CCAAAA00DdDd...DdXX   CC bytes of data Dd starting at AAAA
 * :CCAAAA01XX            eof with start address AAAA
 * :CCAAAA02SSSSXX   	  segment address SSSS
 *
 * because no symbols, we assume that the whole thing is data,
 * learning about code from a start record and/or a jump table
 *
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
		if (addr < low) {
			low = addr;
		}
		while (bc--) {
			byte = gethex(2, &checksum);
			if (byte == -1) {
				printf("missing data byte\n");
				return 0;
			}
			if (addr > high) {
				high = addr;
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
	if ((byte == 0) || (rectype == 0x01)) {
		makeseg(modcount++, S_CODE, low, high - low + 1);
		return (0);
	} 
	return (1);
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
