/*
 * disas - simple Z80/Z280 disassembler
 * 
 * Copyright 1989 by Luc Rooijakkers <lwj@cs.kun.nl> Permission is hereby
 * granted to use and modify this code for non-commercial use, provided this
 * copyright is retained.
 *
 * disassembler rewritten 26 nov 1991 by curt mayer <curt@toad.com> to:
 *	have a single source file
 *	produce output capable of being assembled.
 *	detect code sequences
 *	disassemble at offset into file, with an address bias.
 *		this is handy when extracting ccp, bdos, and bios.
 *	force a set of addresses to be either code or data
 *	buffer entire file in memory
 */

#include <stdio.h>
#include <string.h>

/*
 * word should be at least 16 bits unsigned,
 * byte should be at least 8 bits unsigned
 */
typedef unsigned int word;
typedef unsigned char byte;

/*
 * output kinds
 */
#define OUT_N		0	/* 8-bit operand */
#define OUT_NN		1	/* 16-bit operand */
#define OUT_D		2	/* 8-bit IX/IY offset */
#define OUT_DD		3	/* 16-bit HL/SP/IX/IY offset */
#define OUT_PC		4	/* 16-bit PC offset */
#define OUT_INP		5	/* 8-bit input port */
#define OUT_OUTP 	6	/* 8-bit output port */
#define OUT_EPU		7	/* 8-bit EPU operand */
#define OUT_JR		8	/* 16-bit JR offset */
#define OUT_JP		9	/* 16-bit JP target */
#define OUT_CALL	10	/* 16-bit CALL target */
#define OUT_RST		11	/* 16-bit RST target */
#define	OUT_A		12	/* byte */
#define OUT_NNW		13	/* 16-bit operand indirect */

#define	VERSION	3

#ifndef BUFLEN
#define	BUFLEN	32*1024
#endif

#ifndef TARGETS
#define	TARGETS	5000
#endif

#ifndef Z80
#ifndef Z280
#define	Z80
#endif
#endif

/*
 * output format:
 *
 * 0	0	1	2	3	4	4	5
 * 0.......8.......6.......4.......2.......0.......8.......6
 * 
 * XXXX:   CALL (IX+XXH),XXH       ; ....	00 00 00 00
 */
#define	OPCODE	8
#define	OPERAND	13
#define	ASCII	32
#define	HEX	44
#define	LIMIT	72

void trace();
int disas();
void main();
void clear();
void outs();
void flush();
void tab();
void outval();
char *lookup();
int tcmp();

/*
 * variables
 */

FILE *file;
char *name;
char tracing;

char line[80];
char *linep;
char col;
char codeseg;

byte codebuf[BUFLEN];
int codelen;

word address;
word skip;
word startaddr = 0;
word endaddr = 0xffff;

#define BYTE    0x01
#define WORD    0x02
#define REF     0x04
#define CODE    0x08

void reg_target(word v, byte flags);

struct target {
	word addr;
	char flags;
} targets[TARGETS];
int ntarg;

char jumped;

int debug;

char *progname;

word instaddr;

static char fbuf[20];
char *
fdump(byte v)
{
    sprintf(fbuf, "%s%s%s%s",
        v & CODE ? "code " : "data ",
        v & BYTE ? "byte " : "",
        v & WORD ? "word " : "",
        v & REF ? "ref" : "");
    return fbuf;
}

byte
getbyte()
{
    byte v;

    if (instaddr < startaddr) {
        return 0;
    }
    v = codebuf[instaddr - startaddr];
    instaddr++;
    return v;
}

word
getword()
{
    return getbyte() + (getbyte() << 8);
}

/*
 * process a control file
 */
char *symfile;

struct sym {
    char *name;
    int value;
    struct sym *next;
} *syms;

int
find_symbol(char *ls)
{
    struct sym *s = syms;

    while (s) {
        if (strcasecmp(s->name, ls) == 0) {
            return (s->value);
        }
        s = s->next;
    }
    return -1;
}

/*
 * add symbols, keeping the list in order
 */
void
add_sym(char *name, int v)
{
    struct sym *s;
    struct sym *p;
 
    if (find_symbol(name) != -1) {
        return;
    }
    // printf("adding symbol %s : %x\n", name, v);

    p = 0;
    // traverse the list, stopping when we find a bigger one
    for (s = syms; s; s = s->next) {
        if (s->value > v)
            break;
        p = s; 
    } 
    s = malloc(sizeof(*s));
    s->name = strdup(name);
    s->value = v;

    if (!p) {
        s->next = syms;
        syms = s;
    } else {
        s->next = p->next;
        p->next = s;
    }
}

char *
lookup_sym(int addr)
{
    struct sym *s = syms;

    while (s) {
        if (s->value == addr) {
            return (s->name);
        }
        s = s->next;
    }
    return 0;
}

dump_symbols()
{
    struct sym *s;
    int v;

    for (s = syms; s; s = s->next) {
        v = s->value;
	    if ((v < startaddr) || (v > endaddr)) {
            printf("%s\tequ\t%04xh\n", s->name, v);
        }
    }
}

/*
 * read a string as some kind of number
 * we have a strong preference for hex.
 */
int
snarfval(char *s)
{
    int b = 10; 
    
    if ((s[strlen(s)-1] | 0x20) == 'h') {
        b = 16;
    }
    if ((s[strlen(s)-1] | 0x20) == 'q') {
        b = 8;
    }

    // get a little wierd here.  if it starts with a 0, 
    // assume hex
    if (s[0] == '0') {
        b = 16;
        if ((s[1] | 0x20) == 'x') {
            s += 2;
        }
    }
    return(strtol(s, 0, b));
}

/*
 * we've got a pretty permissive format for the symbol/control file, 
 *
 * 'define' name value
 * name 'equ' value
 * name value
 * 'words' value length
 * 'bytes' value length
 *
 * name 'code' value
 * 'code' value
 *
 * 'jump' value length
 *
 * 'start' value
 */
void
load_symfile()
{
    FILE *sf;
    char p1[20];
    char p2[20];
    char p3[20];
    char linebuf[100];
    word v;
    word v1;
    char *n;
    int flags;

    if (!symfile) {
        return;
    }
    sf = fopen(symfile, "r");
    if (!sf) return;
    while (1) {
        n = 0;
        flags = 0;

        if (fgets(linebuf, sizeof(linebuf), sf) == 0) {
            break;
        }
        if ((linebuf[0] == '#') || (linebuf[0] == ';')) {
            continue;
        }

        switch (sscanf(linebuf, "%s %s %s", p1, p2, p3)) {
        case 2:
            v = snarfval(p1);
            v1 = snarfval(p2);
            if (v) {
                n = p2; 
                flags = REF;
            } else if (v1) {
                v = v1;
                n = p1;
                if (strcasecmp(n, "code") == 0) {
                    n = 0;
                    flags = CODE;
                } else if (strcasecmp(n, "word") == 0) {
                    n = 0;
                    flags = WORD;
                } else if (strcasecmp(n, "start") == 0) {
                    startaddr = v1;
                    continue;
                }
            }
            break;
        case 3:
            v = snarfval(p3);
            if (strcasecmp(p1, "words") == 0) {
                // array 0x456 10        - word array
                instaddr = snarfval(p2);
                while (v--) {
                    reg_target(instaddr, WORD);
                    instaddr += 2;
                }
                continue;
            } else if (strcasecmp(p1, "bytes") == 0) {
                // array 0x456 10        - byte array
                instaddr = snarfval(p2);
                while (v--) {
                    reg_target(instaddr, BYTE);
                    instaddr++;
                }
                continue;
            } else if (strcasecmp(p1, "jump") == 0) {
                // jump 0x456 10        - jump table
                instaddr = snarfval(p2);
                while (v--) {
                    reg_target(instaddr, WORD);
                    reg_target(getword(), CODE|REF);
                }
                continue;
            } else if ((strcasecmp(p1, "define") == 0) || 
                // define foo 0x677     - symbol
                (strcasecmp(p1, "#define") == 0)) {
                flags = REF;
                n = p2;
            } else if (strcasecmp(p2, "data") == 0) {
                // foo equ 0x766
                flags = REF;
                n = p1;
            } else if (strcasecmp(p2, "equ") == 0) {
                // foo equ 0x766
                flags = REF;
                n = p1;
            } else if (strcasecmp(p2, "code") == 0) {
                // foo code 0x766
                n = p1;
                flags = CODE;
            } else if (strcasecmp(p2, "word") == 0) {
                // foo equ 0x766
                n = p1;
                flags = WORD;
            } else {
                // ignore
                continue;
            }
            break;
        default:        // ignore
            continue;
        }

        if (n) {
            add_sym(n, v);
        }
		reg_target(v, flags);
    }
    fclose(sf);
}

void
usage(char *s)
{
	fprintf(stderr, "%sUsage:\n\t%s%s%s%s%s%s%s%s%s",
		s, progname,
		"[options] <filename>\n",
		"options:\n",
		"\t[-a <startaddr>]\tassume start address\n",
		"\t[-e <endaddr>]\tassume end address\n",
		"\t[-s <skip>]\tbytes to skip in input\n",
		"\t[-c <code addr> ... ]\tplaces that are code\n",
		"\t[-d <data addr> ... ]\tplaces that are data\n",
		"\t[-f <controlfile>\tcan contain:\n");
    fprintf(stderr, "%s%s%s%s%s%s%s%s",
        "\t\tdefine <name> <addr>\n",
        "\t\t<name> equ <addr>\n",
        "\t\twords <addr> <length>\n",
        "\t\tbytes <addr> <length>\n",
        "\t\t<name> code <addr>\n",
        "\t\tcode <addr>\n",
        "\t\tjump <addr> <length>\n",
        "\t\tstart <addr>\n");
    exit(1);
}

void 
main(argc, argv)
int argc;
char **argv;
{
	int i;
	int size;
    char o;

    stderr = stdout;

	progname = *argv;
    argv++;
    argc--;

	while (argc) {
		if (**argv != '-') {
            break;
        }
        argc--;
        o = (*argv)[1];
        argv++;

		switch (o) {
		case 'D':	/* debug */
			debug = 1;
			break;

		case 'a':	/* start address */
            if (!--argc) usage("missing start address\n");
            startaddr = strtol(*argv++, 0, 0);
			break;

		case 'e':	/* endaddr */
            if (!--argc) usage("missing end address\n");
			endaddr = strtol(*argv++, 0, 0);
			break;

		case 's':	/* skip */
            if (!--argc) usage("missing skip count\n");
			skip = strtol(*argv++, 0, 0);
			break;

		case 'd':	/* data address */
			while ((argc > 1) && (**argv != '-')) {
				reg_target(strtol(*argv++, 0, 16), BYTE|REF);
				argc--;
			}
			break;

		case 'c':	/* code address */
			while ((argc > 1) && (**argv != '-')) {
				reg_target(strtol(*argv++, 0, 16), CODE|REF);
				argc--;
			}
			break;

		case 'f':	/* symbol file */
			if (!--argc) usage("missing control file name\n");
            symfile = *argv++;
			break;

		case 'h':
		default:
			usage("");
		}
	}

    if (argc) {
        name = *argv;
    } else {
        name = "/dev/stdin";
    }

    if (strcasecmp(&name[strlen(name) - 4], ".com") == 0) {
        symfile = strdup(name);
        strcpy(&symfile[strlen(symfile) - 4], (name[strlen(name)-1] == 'm') ? ".sym" : ".SYM");
        load_symfile();
    }

    if (strcasecmp(name + strlen(name) - 4, ".com") == 0) {
        if (!startaddr) startaddr = 0x100;
    }

    // if startaddr = 0x100, assume cp/m, define some symbols
    if (startaddr == 0x100) {
        add_sym("boot", 0x0);
        add_sym("dfcb", 0x5b);
        add_sym("bdos", 0x5);
        add_sym("tpa", 0x100);
    }

	reg_target(startaddr, CODE|REF);

	if ((file = fopen(name, "rb")) == NULL) {
		perror(name);
		exit(1);
	}

	if (debug) {
		fprintf(stderr, "start address 0x%x\n", startaddr);
	}

	if (skip) {
		if (debug) {
			fprintf(stderr, "skipping %d bytes\n", skip);
		}
		if (fseek(file, (long) skip, 0) != 0) {
			fprintf(stderr, "%s: Can't skip\n", name);
			exit(1);
		}
	}

	size = BUFLEN;
	if (endaddr - startaddr < size) {
		size = endaddr - startaddr;
	}
	codelen = fread(codebuf, 1, size, file);
    if (startaddr + codelen < endaddr) {
        endaddr = startaddr + codelen;
    }
	fclose(file);

	if (debug) {
		fprintf(stderr, "tracing code\n");
	}
	tracing = 1;
	for (i = 0; i <= ntarg; i++) {
		if (targets[i].flags & CODE) {
			trace(targets[i].addr);
		}
	}

	if (debug) {
		for (i = 0; i < ntarg; i++) {
            fprintf(stderr, "%x: %s\n", targets[i].addr, fdump(targets[i].flags));
        }
	}

	tracing = 0;
	header();
	disas();

	exit(0);
}

void
trace(addr)
int addr;
{
	jumped = 0;
	while (!jumped) {
		clear();
        if ((addr < startaddr) || (addr > endaddr))
            return;
		addr += dis(addr);
	}
}

disas()
{
	int left = codelen;
	word addr = startaddr;
	int len;

	while (left > 0) {
		len = instr(addr, left);
		addr += len;
		left -= len;
	}
}

int 
instr(addr, size)
word addr;
int size;
{
	char linebuf[80];
	int len;
	char *s;
	unsigned i;
	unsigned char c;
	int in_quote = 0;
	int need_sep = 0;
    int flags = 0;
    byte item;

	if (size <= 0)
		return (0);

    len = 0;
	col = 0;
	clear();
	address = addr;

	if (codeseg) {
		if (jumped && !code_addr(addr)) {
			codeseg = 0;
		}
	} else {
		if (code_addr(addr)) {
			codeseg = 1;
		}
	}

	if (!codeseg) {
		s = lookup(addr, &flags);
		if (s && (flags & REF)) {
			outs(s);
			outs(": ");
		}
		tab(OPCODE);
        if (flags & WORD) {
            outs("DW\t");
            item = WORD;
        } else {
            outs("DB\t");
            item = BYTE;
        }

        while (col <= LIMIT) {
            if (len >= size) {
                break;
            }
            instaddr = addr + len;
            if (flags & WORD) {
                i = getword();
                len += 2;
            } else {
                i = getbyte();
                len++;
            }
            if (flags & WORD) {
                if (need_sep) {
                    outs(",");
                }
                outval(i, OUT_CALL);
            } else if ((!(flags & BYTE)) && 
                (i >= ' ') && (i != '\"') && (i < 0x7f)) {
                if (!in_quote) {
                    in_quote = 1;
                    if (need_sep) {
                        outs(",");
                    }
                    outs("\"");
                }
                *linep++ = i;
                col++;
            } else {
                if (in_quote) {
                    in_quote = 0;
                    outs("\"");
                }
                if (need_sep) {
                    outs(",");
                }
                if (flags & BYTE) {
                    outval(i, OUT_N);
                } else {
                    outval(i, OUT_A);
                }
            }
            need_sep = 1;
            if (lookup(addr+len, &flags)) {
                if (flags & REF) {
                    break;
                }
                if ((item & WORD) && (flags & WORD)) {
                    continue;
                }
                if ((item & BYTE) && (flags & BYTE)) {
                    continue;
                }
                break;
            } else if (item & WORD) {
                break;
            }
        }
        if (in_quote) {
            outs("\"");
        }
        outs("\n");
		flush();
		return (len);
	}

	linep = linebuf;
	len = dis(addr + len);

	*linep = '\0';
	col = 0;
	clear();
	s = lookup(addr, &flags);
	if (s) {
		outs(s);
		outs(": ");
	}
	tab(OPCODE);
	outs(linebuf);

	tab(ASCII);
	outs("; ");

	for (i = 0; i < len; i++) {
		c = codebuf[addr + i - startaddr];
		if (c > ' ' && c < 0x7f) {
			*linep++ = c;
		} else {
			*linep++ = '.';
		}
		col++;
	}

	tab(HEX);
	flush();
	for (i = 0; i < len; i++) {
		printf("%02x ", codebuf[addr + i - startaddr]);
	}
	printf("\n");
	if (jumped) {
		printf("\n");
	}

	return (len);
}

void 
clear()
{
	linep = line;
}

void 
outs(s)
char *s;
{
	while (*s) {
		if (*s == '\t')
			col = (col + 8) & ~7;
		else
			col++;
		*linep++ = *s++;
	}
}

void 
flush()
{
	*linep = '\0';
	fputs(line, stdout);
	clear();
	*linep = '\0';
}

void 
tab(pos)
int pos;
{
	while (((col + 8) & ~7) <= pos) {
		*linep++ = '\t';
		col = (col + 8) & ~7;
	}

	while (col < pos) {
		*linep++ = ' ';
		col++;
	}
}

header()
{
	printf(";\tdisas version %d\n", VERSION);
    dump_symbols();
	printf("\n\torg\t0%xh\n\n", startaddr);
}

int
tcmp(xp,yp)
struct target *xp, *yp;
{
	return (xp->addr - yp->addr);
}

char *
lookup(word v, byte *flagsp)
{
	static char tbuf[30];
	int i;
    char *s;

    *flagsp = 0;

    s = lookup_sym(v);

	if ((v < startaddr) || (v > endaddr)) {
		return (s);
	}
	for (i = 0; i < ntarg; i++) {
		if (targets[i].addr == v) {
			sprintf(tbuf, "H%04x", v);
            *flagsp = targets[i].flags;
			return (s ? s : tbuf);
		}
	}
	return (s);
}

void
reg_target(word v, byte flags)
{
	int i;

    // printf("registered target %x as %s\n", v, fdump(flags));

	for (i = 0; i < ntarg; i++) {
		if (targets[i].addr == v) {
            targets[i].flags |= flags;
            // printf("already there\n");
			return;
        }
	}

	if (ntarg+1 == TARGETS) {
		fprintf(stderr,"target table overflow\n");
		exit(-1);
	}
    targets[ntarg].flags = flags;
	targets[ntarg++].addr = v;
}

code_addr(v)
int v;
{
	int i;

	for (i = 0; i < ntarg; i++) {
		if (targets[i].addr == v)
			return(targets[i].flags & CODE);
	}
	return (0);
}

/*
 * hexadecimal output routine
 * 
 */

void 
outval(value, out)
word value;
int out;
{
	char buf[11];		/* (PC+xxxxH) is the longest */
	char *s = 0;
    byte flags;

	switch (out) {

	case OUT_A:
		if ((value >= ' ') && (value != '\"') && (value < 0x7f)) {
			sprintf(buf, "\'%c\'", value);
			break;
		}
	case OUT_N:
	case OUT_INP:
	case OUT_OUTP:
	case OUT_EPU:
		sprintf(buf, "%02XH", value);
		break;

	case OUT_JR:
		value += address;

    case OUT_NNW:
	case OUT_NN:
	case OUT_JP:
	case OUT_CALL:
		if (tracing) {
            if (out == OUT_NNW) {
                reg_target(value, WORD|REF);
            } else {
                reg_target(value, (out != OUT_NN) ? CODE|REF : REF);
            }
		} else {
			s = lookup(value, &flags);
			if (s) {
				sprintf(buf, "%s", s);
			} else {
				sprintf(buf, "%04XH", value);
			}
		}
		break;

	case OUT_D:
		if (value < 0x80)
			sprintf(buf, "+%02XH", value);
		else
			sprintf(buf, "-%02XH", 0x100 - value);
		break;

	case OUT_DD:
		if (value < 0x8000)
			sprintf(buf, "+%04XH", value);
		else
			sprintf(buf, "-%04XH", -value);
		break;

	case OUT_PC:
		if (value < 0x8000)
			sprintf(buf, "(PC+%04XH)", value);
		else
			sprintf(buf, "(PC-%04XH)", -value);
		break;

	case OUT_RST:
		if (value < 0x10)
			sprintf(buf, "%X", value);
		else
			sprintf(buf, "%02XH", value);
		break;

	default:
		sprintf(buf, "?");
		break;
	}

	if (!s && (buf[0] >= 'a' && buf[0] <= 'f'))
		outs("0");

	outs(buf);
}

static void outsp();
static void outjrt();
static void outjpt();
static void expand();
static void dis_CB();
static void dis_ED();

/*
 * disassembler output routines
 * 
 */

static int am_code;		/* addressing mode code */

#define	AM_NULL		0
#define	AM_DD		1
#define	AM_FD		2

#define outop(s)	( outs(s), outs("\t") )

#define	outn(out)	outval(getbyte(),out)

#define	outnn(out)	outval(getword(),out)

static void 
outsp(s, sp)
char *s;
char *sp;
{
	char buf[5];		/* "(RR+" is the longest */
	char *bp;

	for (bp = buf; s < sp;)
		*bp++ = *s++;

	*bp = '\0';

	outs(buf);
}

#define	outepu(s)	( \
	outs(s), \
	outn(OUT_EPU), \
	outs(","), \
	outn(OUT_EPU), \
	outs(","), \
	outn(OUT_EPU), \
	outs(","), \
	outn(OUT_EPU) \
)

static char *tab_xr[] = {

	/* AM_NULL */
	"B", "C", "D", "E",
	"H", "L", "(HL)", "A",

	/* AM_DD */
	"(SP+dd)", "(HL+IX)", "(HL+IY)", "(IX+IY)",
	"IXH", "IXL", "(IX+d)", "(nn)",

	/* AM_FD */
	"(PC+dd)", "(IX+dd)", "(IY+dd)", "(HL+dd)",
	"IYH", "IYL", "(IY+d)", "n"

};

#define outxr(xr)	expand(tab_xr[am_code*8+(xr)])
#define outr(r)		outs(tab_xr[(r)])
#define outir(ir)	( (4<=(ir) && (ir)<=6) ? outxr(ir) : outr(ir) )
#define outim()		outxr(6)

static char *tab_xrp[] = {

	/* AM_NULL */
	"BC", "DE", "HL", "SP",

	/* AM_DD */
	"(HL)", "(nn)", "IX", "(PC+dd)",

	/* AM_FD */
	"(IX+dd)", "(IY+dd)", "IY", "nn"

};

#define outxrp(xrp)	expand(tab_xrp[am_code*4+(xrp)])

#define outprp(prp)	( (am_code==AM_NULL && (prp)==3) ? outs("AF") \
							 : outxrp(prp) \
			)

#define outrp(rp)	outs(tab_xrp[(rp)])

static char *tab_irp[] = {

	/* AM_NULL */
	"BC", "DE", "HL", "SP",

	/* AM_DD */
	"BC", "DE", "IX", "SP",

	/* AM_FD */
	"BC", "DE", "IY", "SP"

};

#define outirp(irp)	outs(tab_irp[am_code*4+(irp)])
#define outihl()	outirp(2)

#ifdef Z280

static char *tab_ea[] = {
	"(SP+dd)", "(HL+IX)", "(HL+IY)", "(IX+IY)",
	"(PC+dd)", "(IX+dd)", "(IY+dd)", "(HL+dd)"
};

#define	outea(ea)	expand(tab_ea[(ea)])

#endif

static char *tab_cc[] = {
#ifdef Z280
	"NZ", "Z", "NC", "C", "NV", "V", "P", "M"
#else
	"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"
#endif
};

#define	outcc(cc)	outs(tab_cc[(cc)])

#ifdef Z280
#define	LDW	"LDW"
#define	INCW	"INCW"
#define	DECW	"DECW"
#else
#define	LDW	"LD"
#define	INCW	"INC"
#define	DECW	"DEC"
#endif

static char *tab_op1[] = {
	"ADD", "ADC", "SUB", "SBC", "AND", "XOR", "OR", "CP"
};

#define	outop1(op)	outop(tab_op1[((op)>>3)&0x07])

#ifdef Z80

static char *tab_a[] = {
	"A,", "A,", "", "A,", "", "", "", ""
};

#define outa(op)	outs(tab_a[((op)>>3)&0x07])

#else				/* Z280 */

#define	outa(op)	outs("A,")

#endif				/* Z80 */

static char *tab_op2[] = {
	"RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF"
};

#define	outop2(op)	outop(tab_op2[((op)>>3)&0x07])

static void 
outjrt(len)
int len;
{
	word disp = getbyte();

	if (disp >= 0x80)
		disp += -0x100;	/* cryptic but portable */

	outval(len + disp, OUT_JR);
}

static void 
outjpt(out)
int out;
{
	switch (am_code) {

	case AM_NULL:
		outnn(out);
		break;

	case AM_DD:
		outs("(HL)");
		break;

	case AM_FD:
		outnn(OUT_PC);
		break;

	}
}

static void 
expand(s)
char *s;
{
	char *sp;

	if (strcmp(s, "(PC+dd)") == 0) {
		outnn(OUT_PC);
	} else if ((sp = strchr(s, 'n')) != NULL) {
		if (sp[1] == 'n') {	/* nn */
			outsp(s, sp);
			outnn(OUT_NN);
			outs(sp + 2);
		} else {	/* n */
			outsp(s, sp);
			outn(OUT_N);
			outs(sp + 1);
		}
	} else if ((sp = strchr(s, '+')) != NULL && sp[1] == 'd') {
		if (sp[2] == 'd') {	/* +dd */
			outsp(s, sp);
			outnn(OUT_DD);
			outs(sp + 3);
		} else {	/* +d */
			outsp(s, sp);
			outn(OUT_D);
			outs(sp + 2);
		}
	} else
		outs(s);
}

/*
 * returns the number of bytes in this stretch of code starting at addr
 */
int 
dis(addr)
int addr;
{
	word op;

	instaddr = addr;    // mark our start

	am_code = AM_NULL;
	jumped = 0;

dis_op:
	op = getbyte();

	switch (op & 0xC0) {

	case 0x00:
		switch (op & 0xC7) {

		case 0x00:
			switch (am_code) {

			case AM_NULL:
				if (op == 0x00)
					outs("NOP");
				else if (op == 0x08) {
					outop("EX");
					outs("AF,AF'");
				} else {
					outop((op == 0x10) ? "DJNZ" : "JR");
					if (op >= 0x20) {
						outcc((op >> 3) & 0x03);
						outs(",");
					}
					outjrt(2);
					if (op == 0x18) {
						jumped = 1;
					}
				}
				break;

			case AM_DD:
#ifdef Z280
				if ((op & 0x37) == 0x20) {
					outop((op == 0x20) ? "JAR" : "JAF");
					outjrt(3);
					break;
				}
#endif
				/* fall-trough */

			case AM_FD:
				outs("?");

			}
			break;

		case 0x01:
			if (op & 0x08) {
				outop("ADD");
				outihl();
				outs(",");
				outirp((op >> 4) & 0x03);
#ifdef Z280
			} else if (am_code != AM_FD || op == 0x21) {
#else
			} else if (am_code == AM_NULL || op == 0x21) {
#endif
				outop(LDW);
				outxrp((op >> 4) & 0x03);
				outs(",");
				outnn(OUT_NN);
			} else
				outs("?");
			break;

		case 0x02:
			if ((op & 0x37) == 0x22) {
				outop(LDW);
				if (op == 0x22) {
					outs("(");
					outnn(OUT_NNW);
					outs("),");
					outihl();
				} else {
					outihl();
					outs(",(");
					outnn(OUT_NNW);
					outs(")");
				}
			} else if (am_code == AM_NULL) {
				static char *aldtab[] = {
					"(BC),A", "A,(BC)",
					"(DE),A", "A,(DE)",
					NULL, NULL,
					"(nn),A", "A,(nn)"
				};

				outop("LD");
				expand(aldtab[(op >> 3) & 0x07]);
			} else
				outs("?");
			break;

		case 0x03:
#ifdef Z280
			if (am_code != AM_FD || op < 0x30) {
#else
			if (am_code == AM_NULL || (op & 0x37) == 0x23) {
#endif
				outop((op & 0x08) ? DECW : INCW);
				outxrp((op >> 4) & 0x03);
			} else
				outs("?");
			break;

		case 0x04:
#ifdef Z280
			if (am_code != AM_FD || op != 0x3C) {
#else
			if (am_code == AM_NULL || op == 0x34) {
#endif
				outop("INC");
				outxr((op >> 3) & 0x07);
			} else
				outs("?");
			break;

		case 0x05:
#ifdef Z280
			if (am_code != AM_FD || op != 0x3D) {
#else
			if (am_code == AM_NULL || op == 0x35) {
#endif
				outop("DEC");
				outxr((op >> 3) & 0x07);
			} else
				outs("?");
			break;

		case 0x06:
#ifdef Z280
			if (am_code != AM_FD || op != 0x3E) {
#else
			if (am_code == AM_NULL || op == 0x36) {
#endif
				outop("LD");
				outxr((op >> 3) & 0x07);
				outs(",");
				outn(OUT_N);
			} else
				outs("?");
			break;

		case 0x07:
			if (am_code == AM_NULL) {
				outop2(op);
#ifdef Z280
				if (op == 0x2F)
					outs("A");
#endif
			} else
				outs("?");
			break;
		}
		break;

	case 0x40:
		if (op == 0x76)
			if (am_code == AM_NULL)
				outs("HALT");
			else
				outs("?");
#ifdef Z280
		else if (am_code == AM_NULL || op >= 0x60 && op != 0x7F ||
			 0x04 <= (op & 0x07) && (op & 0x07) <= 0x06)
#else
		else if (am_code == AM_NULL || (op & 0xF8) == 0x70 ||
			 (op & 0x07) == 0x06)
#endif
		{
			outop("LD");
			if (op == 0x66 || op == 0x6E)
				outr((op >> 3) & 0x07);
			else
				outir((op >> 3) & 0x07);
			outs(",");
			if (op == 0x74 || op == 0x75)
				outr(op & 0x07);
			else if (op >= 0x78)
				outxr(op & 0x07);
			else
				outir(op & 0x07);
		} else
			outs("?");
		break;

	case 0x80:
#ifdef Z280
		if (am_code != AM_FD || (op & 0x07) != 0x07) {
#else
		if (am_code == AM_NULL || (op & 0x07) == 0x06) {
#endif
			outop1(op);
			outa(op);
			outxr(op & 0x07);
		} else
			outs("?");
		break;

	case 0xC0:
		switch (op & 0xC7) {

		case 0xC0:
			if (am_code == AM_NULL) {
				outop("RET");
				outcc((op >> 3) & 0x07);
			} else
				outs("?");
			break;

		case 0xC1:
			if ((op & 0xEF) != 0xC9 || am_code == AM_NULL) {
				if (op & 0x08) {
					if (op == 0xC9) {
						outs("RET");
						jumped = 1;
					} else if (op == 0xD9)
						outop("EXX");
					else if (op == 0xE9) {
						outop("JP");
						outs("(");
						outihl();
						outs(")");
						jumped = 1;
					} else {
						outop(LDW);
						outs("SP,");
						outihl();
					}
#ifdef Z280
				} else if (am_code != AM_FD || op == 0xE1) {
#else
				} else if (am_code == AM_NULL || op == 0xE1) {
#endif
					outop("POP");
					outprp((op >> 4) & 0x03);
				} else
					outs("?");
			} else
				outs("?");
			break;

		case 0xC2:
#ifdef Z80
			if (am_code == AM_NULL) {
#endif
				outop("JP");
				outcc((op >> 3) & 0x07);
				outs(",");
				outjpt(OUT_JP);
#ifdef Z80
			} else
				outs("?");
#endif
			break;

		case 0xC3:
			switch (op) {

			case 0xC3:
#ifdef Z280
				if (am_code != AM_DD) {
#else
				if (am_code == NULL) {
#endif
					outop("JP");
					outjpt(OUT_JP);
					jumped = 1;
				} else
					outs("?");
				break;

			case 0xCB:
				dis_CB();
				break;

			case 0xD3:
				if (am_code == AM_NULL) {
					outop("OUT");
					outs("(");
					outn(OUT_OUTP);
					outs("),A");
				} else
					outs("?");
				break;

			case 0xDB:
				if (am_code == AM_NULL) {
					outop("IN");
					outs("A,(");
					outn(OUT_INP);
					outs(")");
				} else
					outs("?");
				break;


			case 0xE3:
				outop("EX");
				outs("(SP),");
				outihl();
				break;

			case 0xEB:
#ifdef Z280
				outop("EX");
				if (am_code == AM_NULL)
					outs("DE");
				else
					outihl();
				outs(",HL");
#else				/* Z80 */
				if (am_code == AM_NULL) {
					outop("EX");
					outs("DE,HL");
				} else
					outs("?");
#endif				/* Z280 */
				break;

			case 0xF3:
				if (am_code == AM_NULL)
					outs("DI");
				else
					outs("?");
				break;

			case 0xFB:
				if (am_code == AM_NULL)
					outs("EI");
				else
					outs("?");
				break;

			}
			break;

		case 0xC4:
#ifdef Z80
			if (am_code == NULL) {
#endif
				outop("CALL");
				outcc((op >> 3) & 0x07);
				outs(",");
				outjpt(OUT_CALL);
#ifdef Z80
			} else
				outs("?");
#endif
			break;

		case 0xC5:
#ifdef Z280
			if (op & 0x08) {
#else
			if ((op & 0x08) && am_code == AM_NULL) {
#endif
				if (op == 0xCD) {
					outop("CALL");
					outjpt(OUT_CALL);
				} else if (op == 0xED) {
					dis_ED();
				} else {
					if (am_code == AM_NULL) {
						if (op == 0xDD)
							am_code = AM_DD;
						else
							am_code = AM_FD;
						goto dis_op;
					} else
						outs("?");
				}
#ifdef Z280
			} else if (am_code != AM_FD || op >= 0xE0) {
#else
			} else if (am_code == AM_NULL || op == 0xE5) {
#endif
				outop("PUSH");
				outprp((op >> 4) & 0x03);
			} else
				outs("?");
			break;

		case 0xC6:
			if (am_code == AM_NULL) {
				outop1(op);
				outa(op);
				outn(OUT_N);
			} else
				outs("?");
			break;

		case 0xC7:
			if (am_code == AM_NULL) {
				outop("RST");
				outval(op & 0x38, OUT_RST);
			} else
				outs("?");
			break;

		}
		break;

	}
	return (instaddr - addr);
}

static char *tab_CB1[] = {
	"RLC", "RRC", "RL", "RR", "SLA", "SRA", "TSET", "SRL"
};

static char *tab_CB2[] = {
	NULL, "BIT", "RES", "SET"
};

static void 
dis_CB()
{
	word op, off;
	char bit[2];

	if (am_code == AM_NULL)
		op = getbyte();
	else {
		off = getbyte();
		op = getbyte();
		if ((op & 0x07) != 6) {
			outs("?");
			return;
		}
	}

#ifdef Z80
	if ((op & 0xF8) == 0x30) {
		outs("?");
		return;
	}
#endif

	if ((op & 0xC0) == 0x00)
		outop(tab_CB1[op >> 3]);
	else {
		outop(tab_CB2[op >> 6]);
		bit[0] = '0' + ((op >> 3) & 0x07);
		bit[1] = '\0';
		outs(bit);
		outs(",");
	}

	if (am_code == AM_NULL)
		outr(op & 0x07);
	else {
		outs("(");
		outihl();
		outval(off, OUT_D);
		outs(")");
	}
}

static void 
dis_ED()
{
	word op;

	op = getbyte();

	switch (op & 0xC7) {

#ifdef Z280

	case 0x02:
		outop("LDA");
		outihl();
		outs(",");
		outea((op >> 3) & 0x07);
		break;

	case 0x03:
		if (am_code == AM_NULL) {
			outop("LD");
			outea((op >> 3) & 0x07);
			outs(",A");
		} else
			outs("?");
		break;

	case 0x04:
		outop(LDW);
		outihl();
		outs(",");
		outea((op >> 3) & 0x07);
		break;

	case 0x05:
		outop(LDW);
		outea((op >> 3) & 0x07);
		outs(",");
		outihl();
		break;

	case 0x06:
		if (op & 0x08) {
			outop(LDW);
			outrp((op >> 4) & 0x03);
			outs(",");
			outim();
		} else {
			outop(LDW);
			outim();
			outs(",");
			outrp((op >> 4) & 0x03);
		}
		break;

	case 0x07:
		if (am_code != AM_FD || op != 0x3F) {
			outop("EX");
			outs("A,");
			outxr((op >> 3) & 0x07);
		} else
			outs("?");
		break;

#endif				/* Z280 */

	case 0x40:
		if (op == 0x70) {
			if (am_code == AM_NULL) {
#ifdef Z280
				outop("TSTI");
				outs("(C)");
#else				/* Z80 */
				outop("IN");
				outs("F,(C)");
#endif				/* Z280 */
			} else
				outs("?");
		} else if (am_code != AM_FD || op != 0x78) {
			outop("IN");
			outxr((op >> 3) & 0x07);
			outs(",(C)");
		} else
			outs("?");
		break;

	case 0x41:
#ifdef Z280
		if (op == 0x71) {
			if (am_code == AM_NULL) {
				outop("SC");
				outnn(OUT_NN);
			} else
				outs("?");
		} else if (am_code != AM_FD || op != 0x79) {
#else				/* Z80 */
		if (am_code == AM_NULL && op != 0x71) {
#endif				/* Z280 */
			outop("OUT");
			outs("(C),");
			outxr((op >> 3) & 0x07);
		} else
			outs("?");
		break;

	case 0x42:
		outop((op & 0x08) ? "ADC" : "SBC");
		outihl();
		outs(",");
		outirp((op >> 4) & 0x03);
		break;

	case 0x43:
		if (am_code == AM_NULL && (op & 0xF7) != 0x63) {
			if (op & 0x08) {
				outop(LDW);
				outrp((op >> 4) & 0x03);
				outs(",(");
				outnn(OUT_NN);
				outs(")");
			} else {
				outop(LDW);
				outs("(");
				outnn(OUT_NN);
				outs("),");
				outrp((op >> 4) & 0x03);
			}
		} else
			outs("?");
		break;

	case 0x44:
#ifdef Z280
		if (am_code == AM_NULL) {
			switch (op) {

			case 0x44:
				outop("NEG");
				outs("A");
				break;

			case 0x4C:
				outop("NEG");
				outs("HL");
				break;

			case 0x64:
				outop("EXTS");
				outs("A");
				break;

			case 0x6C:
				outop("EXTS");
				outs("HL");
				break;

			default:
				outs("?");
				break;

			}
#else				/* Z80 */
		if (am_code == AM_NULL && op == 0x44) {
			outs("NEG");
#endif				/* Z280 */
		} else
			outs("?");
		break;

	case 0x45:
#ifdef Z280
		if (op == 0x6D) {
			outop("ADD");
			outihl();
			outs(",A");
		} else {
			if (am_code == AM_NULL) {
				static char *tab[] = {
					"RETN", "RETI",
					"RETIL", "?",
					"PCACHE", NULL,
					"?", "?"
				};

				outop(tab[(op >> 3) & 0x07]);
			} else
				outs("?");
		}
#else				/* Z80 */
		if (am_code == AM_NULL && op == 0x45)
			outs("RETN");
		else if (am_code == AM_NULL && op == 0x4D)
			outs("RETI");
		else
			outs("?");
#endif				/* Z280 */
		break;

	case 0x46:
#ifdef Z280
		if (am_code == AM_NULL && op < 0x60) {
#else
		if (am_code == AM_NULL && op < 0x60 && op != 0x4E) {
#endif
			static char *imtab[] = {
				"0", "3", "1", "2"
			};

			outop("IM");
			outs(imtab[(op >> 3) & 0x07]);
#ifdef Z280
		} else if (op == 0x66) {
			outop("LDCTL");
			outihl();
			outs(",(C)");
		} else if (op == 0x6E) {
			outop("LDCTL");
			outs("(C),");
			outihl();
#endif				/* Z280 */
		} else
			outs("?");
		break;

	case 0x47:
#ifdef Z280
		if (am_code == AM_NULL) {
#else
		if (am_code == AM_NULL && op < 0x70) {
#endif
			static char *tab1[] = {
				"LD", "LD", "LD", "LD",
				"RRD", "RLD", "DI", "EI"
			};
			static char *tab2[] = {
				"I,A", "R,A", "A,I", "A,R",
				"", "", "n", "n"
			};

			outop(tab1[(op >> 3) & 0x07]);
			expand(tab2[(op >> 3) & 0x07]);
		} else
			outs("?");
		break;

	case 0x80:
		if (am_code == AM_NULL) {
			static char *ldtab[] = {
				"?", "?", "?", "?",
				"LDI", "LDD", "LDIR", "LDDR"
			};

			outop(ldtab[(op >> 3) & 0x07]);
		} else
			outs("?");
		break;

	case 0x81:
		if (am_code == AM_NULL) {
			static char *cptab[] = {
				"?", "?", "?", "?",
				"CPI", "CPD", "CPIR", "CPDR"
			};

			outop(cptab[(op >> 3) & 0x07]);
		} else
			outs("?");
		break;

	case 0x82:
		if (am_code == AM_NULL) {
			static char *intab[] = {
#ifdef Z280
				"INIW", "INDW",
				"INIRW", "INDRW",
#else
				"?", "?",
				"?", "?",
#endif
				"INI", "IND",
				"INIR", "INDR"
			};

			outop(intab[(op >> 3) & 0x07]);
		} else
			outs("?");
		break;

	case 0x83:
		if (am_code == AM_NULL) {
			static char *outab[] = {
#ifdef Z280
				"OUTIW", "OUTDW",
				"OTIRW", "OTDRW",
#else
				"?", "?",
				"?", "?",
#endif
				"OUTI", "OUTD",
				"OTIR", "OTDR"
			};

			outop(outab[(op >> 3) & 0x07]);
		} else
			outs("?");
		break;

#ifdef Z280

	case 0x84:
	case 0x85:
		if (am_code == AM_NULL) {
			outop(((op & 0xC7) == 0x84) ? "EPUM" : "MEPU");
			outea((op >> 3) & 0x07);
			outepu(",");
		} else
			outs("?");
		break;

	case 0x86:
		if (op < 0xA0) {
			outop((op < 0x90) ? "LDUD" : "LDUP");
			if (op & 0x08) {
				outim();
				outs(",A");
			} else {
				outs("A,");
				outim();
			}
		} else if (am_code == AM_NULL && (op & 0xF7) == 0xA6) {
			outop((op & 0x08) ? "MEPU" : "EPUM");
			outs("(HL)");
			outepu(",");
		} else
			outs("?");
		break;

	case 0x87:
		switch (op) {

		case 0x87:
			outop("LDCTL");
			outihl();
			outs(",USP");
			break;

		case 0x8F:
			outop("LDCTL");
			outs("USP,");
			outihl();
			break;

		case 0x97:
		case 0x9F:
			if (am_code == NULL) {
				outop((op == 0x97) ? "EPUF" : "EPUI");
				outepu("");
			} else
				outs("?");
			break;

		case 0xA7:
		case 0xAF:
			if (am_code == NULL) {
				outop((op == 0xA7) ? "EPUM" : "MEPU");
				outs("(");
				outnn(OUT_NN);
				outs(")");
				outepu(",");
			} else
				outs("?");
			break;

		case 0xB7:
			if (am_code == NULL) {
				outop("INW");
				outs("HL,(C)");
			} else
				outs("?");
			break;

		case 0xBF:
			if (am_code == NULL) {
				outop("OUTW");
				outs("(C),HL");
			} else
				outs("?");
			break;

		}
		break;

	case 0xC0:
		outop("MULT");
		outs("A,");
		outxr((op >> 3) & 0x07);
		break;

	case 0xC1:
		outop("MULTU");
		outs("A,");
		outxr((op >> 3) & 0x07);
		break;

	case 0xC2:
		if (op & 0x08) {
			outop("DIVW");
			outs("DEHL,");
		} else {
			outop("MULTW");
			outs("HL,");
		}
		outxrp((op >> 4) & 0x03);
		break;

	case 0xC3:
		if (op & 0x08) {
			outop("DIVUW");
			outs("DEHL,");
		} else {
			outop("MULTUW");
			outs("HL,");
		}
		outxrp((op >> 4) & 0x03);
		break;

	case 0xC4:
		outop("DIV");
		outs("HL,");
		outxr((op >> 3) & 0x07);
		break;

	case 0xC5:
		outop("DIVU");
		outs("HL,");
		outxr((op >> 3) & 0x07);
		break;

	case 0xC6:
		outop((op & 0x08) ? "SUBW" : "ADDW");
		outs("HL,");
		outxrp((op >> 4) & 0x03);
		break;

	case 0xC7:
		if (am_code == AM_NULL && op == 0xEF) {
			outop("EX");
			outs("H,L");
		} else if (op & 0x08)
			outs("?");
		else {
			outop("CPW");
			outs("HL,");
			outxrp((op >> 4) & 0x03);
		}
		break;

#endif				/* Z280 */

	default:
		outs("?");
		break;

	}
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

