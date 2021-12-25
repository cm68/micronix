/*
 * disas - Z80/Z280 disassembler
 *
 * tools/disas.c
 * Changed: <2021-12-23 15:55:26 curt>
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
 *

This package contains an intelligent Z80/Z280 (compile-time selectable)
disassembler for CP/M .COM files.  It is derived from an excellent dumb
disassembler by Luc Rooijakkers <lwj@cs.kun.nl>.

The disassembler has the following features:
	produces output capable of being assembled.
 	detects code sequences by tracing jump targets
	specific addresses may be forced to be either code or data
	a starting address may be specified
	an ending address may be specified
	disassembly may be started at any byte offset into the file
	a Usage message is issued if no arguments are specified.

Configuration notes:

	several constants are found in the disas.c source file.
	defining any of these in the makefile will result in the
	makefile values overriding the defaults.

	TARGETS:	maximum number of branch targets and data labels
	BUFLEN:		maximum size of disassembly in binary
	
	if qsort is not present in your libc (yuck!), edit the source
	file to #undef BINARY_SEARCH. this will slow down the disassembly
	somewhat.

    23 oct 2020 - added whitesmith's object support
    24 nov 2020 - added support for hitech's object file format
        XXX - this is broken if there are any segments other than text, data, bss..
    the theory for relocatable objects is that we load segments
    into their own discontiguous spaces, and put unresolved symbols
    into their own segment.  that way everybody has an address in the
    listing.  also, no segment goes at 0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../micronix/include/types.h"
#include "../micronix/include/obj.h"
#include "../include/ws.h"
#include "../include/hitech.h"
#include "../include/mnix.h"

/*
 * word should be at least 16 bits unsigned,
 * byte should be at least 8 bits unsigned
 */
typedef unsigned short word;
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
#define	BUFLEN	64*1024
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
#define	ADDR	32
#define	ASCII	38
#define	HEX	50
#define	LIMIT	60

void dump_symbols();
void trace();
void header();
int dis();
void disas();
void main();
void clear();
void outs();
void flush();
void tab();
void outval(word v, int type);
char *lookup(word v, int *flagsp);
int tcmp();
int instr(word addr, int size);
int code_addr(int v);
char *addr_name(word a);
int find_symbol(char *ls);

/*
 * variables
 */

FILE *file;
char *name;
char tracing;

unsigned char line[80];
char *linep;
char col;
char in_code;

#define SEG_ZERO    0
#define SEG_TEXT    1
#define SEG_DATA    2
#define SEG_BSS     3
#define SEG_OTHER   4
#define SEG_UNDEF   5
#define SEG_MAX     SEG_UNDEF

int
nametoseg(char *s)
{
    if (!strcmp(s, "text")) return SEG_TEXT;
    if (!strcmp(s, "data")) return SEG_DATA;
    if (!strcmp(s, "bss")) return SEG_BSS;
    if (s[0] != 0) return SEG_OTHER;
    return SEG_ZERO;
}

struct seg {
    char *name;
    word base;
    word length;
    char absolute;
} seg[SEG_MAX+1] = {
    { "0", 0, 0 },
    { "text", 0, 0 },
    { "data", 0, 0 },
    { "bss", 0, 0 },
    { "other", 0, 0 },
    { "undef", 0, 0 }
};

void
segdump()
{
    int i;

    for (i = SEG_TEXT; i <= SEG_MAX; i++) {
        if (seg[i].length)
            printf(";\t%s\t%04x\t%04x\n", 
                seg[i].name, seg[i].base, seg[i].length);
    }
    printf(";\n");
    dump_symbols();
    printf(";\n");
}

/*
 * return an address offset by segment
 */
int
segoff(int segnum, word offset)
{
    if (seg[segnum].absolute) return offset;
    return seg[segnum].base + offset;
}

int
getseg(word addr)
{
    int i;
    for (i = 0; i <= SEG_MAX; i++) {
        if ((addr >= seg[i].base) && 
            (addr < seg[i].base + seg[i].length)) return i;
    }
    return -1;
}

static char *hirectype[] = { 
    "UNK0", "BLOCK", "UNK2", "RELOC", "SYM", "UNK5", "END"
};

struct hitechobj hihdr;
struct obj obj;
int symlen;

byte codebuf[BUFLEN];
static char tbuf[30];

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

int hitech;
int micronix;

short jraddr;

word mainfunc;
word ncsv;

char *progname;

word instaddr;
word inst_start;

short zcrtpat[]= {
    0x2a, 0x06, 0x00, 0xf9, 0x11, -1, -1, 0xb7,
    0x21, -1, -1, 0xed, 0x52, 0x4d, 0x44, 0x0b,
    0x6b, 0x62, 0x13, 0x36, 0x00, 0xed, 0xb0, 0x21,
    -1, -1, 0xe5, 0x21, 0x80, 0x00, 0x4e, 0x23
};

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

    v = codebuf[instaddr];
    instaddr++;
    return v;
}

byte
byteat(word addr)
{
    return codebuf[addr];
}

word
wordat(word addr)
{
    return (codebuf[addr + 1] << 8) + codebuf[addr];
}

setword(word addr, word new)
{
    codebuf[addr] = new & 0xff;
    codebuf[addr+1] = (new >> 8) & 0xff;
}

/*
 * match pattern p against memory b for len
 */
int
patmatch(short *p, word b, int len)
{
    while (len--) {
        if (*p == -1) {
            p++; b++; continue;
        }
        if (*p++ != byteat(b++)) return 0;
    }
    return 1;
}

struct hireloc {
    word addr;
    unsigned char type;
    char *name;
    char *next;
};

struct hireloc *hirelocs;

void
addhireloc(word addr, unsigned char type, char *name)
{
    struct hireloc *hr;

    hr = malloc(sizeof(*hr));
    if (!hr) {
        fprintf(stderr, "malloc failed");
        exit(1);
    }
    hr->addr = addr;
    hr->type = type;
    hr->name = strdup(name);
    hr->next = hirelocs;
    hirelocs = hr;
    if (debug > 2) 
        printf("reloc at %x type %d name %s\n", hr->addr, hr->type, hr->name);
}

dohirelocs()
{
    struct hireloc *hr, *hn;
    word dw;
    word off;

    for (hr = hirelocs; hr;) {
        if (debug > 1) printf("addr: %04x %d:%s\n", hr->addr, hr->type, hr->name); 

        dw = wordat(hr->addr); 
        if (hr->type == HTREL_SYM) {
            off = find_symbol(hr->name);
        } else if (hr->type == HTREL_SEG) {
            off = seg[nametoseg(hr->name)].base;
        } else {
            fprintf(stderr, "bogus reloc type %x\n", hr->type);
            exit(1);
        }
        dw += off;
        setword(hr->addr, dw);

        hn = hr->next;
        free(hr->name);
        free(hr);
        hr = hn;
    }
}

/*
 * read a null-terminated string
 */
int
higetstr(char *sbuf)
{
    int i = 0;
    char c;

    while (!feof(file)) {
        c = fgetc(file);
        sbuf[i++] = c;
        if (!c) return i;
    }
    fprintf(stderr, "end of file reading string\n");
    exit (1);
}

word lastword;

word
getword()
{
    lastword = getbyte() + (getbyte() << 8);
    return lastword;
}

unsigned char
nextbyte()
{
    static char b;

    if (fread(&b, sizeof(b), 1, file) != 1) {
        printf("premature eof on object file\n");
        return 0;
    }
    return b;
}

/*
 * whitesmith's relocations are a little wierd.
 */
#define WS_END  0
#define WS_BIAS 1
#define WS_TEXT 2
#define WS_DATA 3
#define WS_BSS  4
#define WS_SYM  5

char *wsrname[] = {
    "end", "bias", "text", "data", "bss", "sym"
};

/*
 * the relocation cursor
 */
int wscursor;
int wssym;
int wswhere;

int
getwsreloc()
{
    unsigned char control;
    int i;

    while (1) {
        control = nextbyte();

        if (control == 0) {                                 // done
            return WS_END;
        } else if ((control >= 1) && (control <= 31)) {     // short skip
            wscursor += control;
            continue;
        } else if (control >= 32 && control <= 63) {        // long skip
            wscursor += 32 + ((control - 32) * 256) + nextbyte();
            continue;
        }

        wswhere = wscursor;
        wscursor += (control & 2) ? 4 : 2;
        control -= 64;
        control >>= 2;

        wssym = -1;
        switch (control) {
        case 0:
            return WS_BIAS;
        case 1:
            return WS_TEXT;
        case 2:
            return WS_DATA;
        case 3:
            return WS_BSS;
        default:
            break; 
        }
        control -= 4;
        if (control == 47) {
            control = nextbyte();
            if (control < 128) {
                wssym = control + 128;
            } else {
                wssym = 175 + ((control - 128) * 256) + nextbyte();
            }
        } else {
            wssym = control;
        }
        return WS_SYM;
    }
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

/*
 * brute force symbol table, used by whitesmith's code since we need an
 * index.  so, really, the symbols are ordered by address, and they are
 * in an array indexed by an absolute symbol number
 */
#define MAX_SYMS    10000

struct sym *symtab[MAX_SYMS];
int nsyms;

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
 
    /* gotta be unique */
    if (find_symbol(name) != -1) {
        printf("symbol %s multi add\n", name);
        return;
    }
    if (debug > 2) printf("adding symbol %s : %x\n", name, v);

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
    symtab[nsyms++] = s;
}

char *
lookup_sym(word addr)
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

char anbuf[50];
char *abptr;

char *
addrname(word addr)
{
    char *s;
    unsigned int v = addr;

    if (abptr == anbuf) {
        abptr = &anbuf[25];
    } else {
        abptr = anbuf;
    }
    s = lookup_sym(addr);
    if (s) {
        sprintf(abptr, "%s (0x%04x)", s, v);
    } else {
        sprintf(abptr, "0x%04x", v);
    }
    return abptr;
}

void
dump_symbols()
{
    struct sym *s;
    int v;

    for (s = syms; s; s = s->next) {
        v = s->value;
        if (getseg(v) == SEG_UNDEF) {
            printf("%s\tequ\t%04xh\n", s->name, v);
        }
    }
}

dorelocs(int segnum)
{
    int type;

    wscursor = seg[segnum].base;
    if (debug) printf("ws_relocs %s\n", seg[segnum].name);
    do {
        type = getwsreloc();
        if (debug) printf("%04x %s %d\n", wswhere, wsrname[type], wssym);
        switch (type) {
        case WS_TEXT:
            setword(wswhere, wordat(wswhere) - obj.textoff + seg[SEG_TEXT].base);
            break;
        case WS_DATA:
            setword(wswhere, wordat(wswhere) - obj.dataoff + seg[SEG_DATA].base);
            break;
        case WS_SYM:
            setword(wswhere, symtab[wssym]->value);
            break;
        default:
            break;
        }
    } while (type != WS_END);
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
    if (!sf) {
        symfile = 0;
        return;
    }

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
                    reg_target(instaddr, CODE|REF);
                    instaddr++;
                    reg_target(getword(), CODE|REF);
                }
                continue;
            } else if (strcasecmp(p1, "vector") == 0) {
                // vectors 0x456 10        - vectors table
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
	fprintf(stderr, "%sUsage:\n\t%s%s%s%s%s%s%s%s%s%s%s",
		s, progname,
		"[options] <filename>\n",
		"options:\n",
		"\t[-H\tset hitech\n",
		"\t[-v\tincrease verbosity\n",
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
        "\t\tvector <addr> <length>\n",
        "\t\tstart <addr>\n");
    exit(1);
}

#define ROUNDUP(x)  (((x) + 255) & 0xff00)
void 
main(argc, argv)
int argc;
char **argv;
{
    int v;
	int i;
	int size;
    char o;
    word addr;
    int undefs;

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
		case 'H':	/* debug */
			hitech++;
			break;

		case 'v':	/* debug */
			debug++;
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

    if (!symfile) {
        if (strcasecmp(&name[strlen(name) - 4], ".com") == 0) {
            symfile = strdup(name);
            i = strlen(symfile);
            strcpy(symfile + i - 4, ".sym");
            if (access(symfile, 4) != 0) {
                strcpy(symfile + i - 4, ".SYM");
            }
        } else {
            char sfname[100];
            sprintf(sfname, "%s.sym", name);
            if (access(sfname, 4) != 0) {
                sprintf(sfname, "%s.SYM", name);
            }
            symfile = strdup(sfname);
        }
    } 
    load_symfile();

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

	if ((file = fopen(name, "rb")) == NULL) {
		perror(name);
		exit(1);
	}

    rewind(file);
    if (fread(&obj, sizeof(obj), 1, file) != 1) {
        fprintf(stderr, "whitesmith's header read failed");
        exit(1);
    }

    rewind(file);
    if (fread(&hihdr, sizeof(hihdr), 1, file) != 1) {
        fprintf(stderr, "hitech header read failed");
        exit(1);
    }

    /* whitesmiths */
    if (obj.ident == OBJECT) {
        rewind(file);
        fseek(file, sizeof(obj), SEEK_SET);

        symlen = ((obj.conf & 7) << 1) + 1;
        
        /*
         * assign segment addresses - we do some magic here if it's
         * got relocations, so text won't be at zero
         */
        if (obj.conf == RELOC) {
            seg[SEG_ZERO].length = 0x100;
            seg[SEG_ZERO].base = 0;
        }
        seg[SEG_TEXT].base = obj.textoff + seg[SEG_ZERO].length;
        seg[SEG_TEXT].length = obj.text;

        seg[SEG_DATA].base = obj.dataoff + seg[SEG_ZERO].length;
        seg[SEG_DATA].length = obj.data;

        seg[SEG_BSS].base = ROUNDUP(seg[SEG_DATA].base + seg[SEG_DATA].length);
        seg[SEG_BSS].length = obj.bss;
        seg[SEG_UNDEF].base = ROUNDUP(seg[SEG_BSS].base + seg[SEG_BSS].length);

        /*
         * read in the whitesmith's text segment
         */
        if (fread(&codebuf[seg[SEG_TEXT].base], 1, obj.text, file) 
                != obj.text) {
            fprintf(stderr, "code read failed\n");
            exit(1);
        }

        /*
         * read in the data segment if any
         */
        if (obj.data) {
            if (fread(&codebuf[seg[SEG_DATA].base], 1, obj.data, file) 
                    != obj.data) {
                fprintf(stderr, "data read failed\n");
                exit(1);
            }
        }

        undefs = 0;

        /*
         * now, read in the symbol table if any
         */
        for (i = 0; i < obj.table / (symlen + 3); i++) {
            if (fread(line, symlen + 3, 1, file) != 1) {
                fprintf(stderr, "symbol %d read failed\n", i);
                exit(1);
            }
            o = line[2];
            v = (line[0] + (line[1] << 8)) & 0xffff;
            switch (o) {
            case 0xd:
                add_sym(&line[3], v + seg[SEG_TEXT].base);
                reg_target(v + seg[SEG_TEXT].base, CODE);
                break;
            case 0xe:
                add_sym(&line[3], v + seg[SEG_DATA].base);
				reg_target(v + seg[SEG_DATA].base, REF);
                break;
            case 0x8:
                add_sym(&line[3], seg[SEG_UNDEF].base + undefs++);
                break;
            }
        }
        seg[SEG_UNDEF].length = undefs;
        /*
         * and finally, the relocations if any
         */
        if (obj.conf == RELOC) {
            if (debug) printf("relocs present\n");
            dorelocs(SEG_TEXT);
            dorelocs(SEG_DATA);
        }
        startaddr = obj.textoff;
        endaddr = obj.dataoff + obj.data;
        reg_target(obj.textoff, CODE);
        micronix++;
    } else if (hihdr.magic == HITECH_MAGIC) {
        /*
         * read in the hitech object file
         */
        struct hipre hipre;
        struct hiblkrec hiblkrec;
        struct hisymrec hisymrec;
        struct hirelrec hirelrec;
        struct hiunkrec hiunkrec;
        struct hiunkrec5 hiunkrec5;
        char segbuf[20];
        char symbuf[20];
        int i;
        int j;
        int curseg;
        int loadaddr;
        long hdrpos;
        int where;
 
        hitech++;
        curseg = 0;

        /*
         * we don't like something that can be found in hitech objects:
         * we see a data segment before the text segments are done.
         * so, we do two passes through the object file
         */
        hdrpos = ftell(file);

        /*
         * the first pass only processes HIREC_BLK records
         */
        do {
            if (fread(&hipre, sizeof(hipre), 1, file) != 1) {
                fprintf(stderr, "record prefix unreadable\n");
                exit(1);
            }
            if (debug) 
                printf("%5d: record type %d (%s) len %d\n", where,
                    hipre.code, hirectype[hipre.code], hipre.reclen);

            where = ftell(file);
            if (hipre.code == HIREC_BLK) {
                if (fread(&hiblkrec, sizeof(hiblkrec), 1, file) != 1) {
                    fprintf(stderr, "block record header unreadable\n");
                    exit(1);
                }
                i = hipre.reclen - (sizeof(hiblkrec) + higetstr(segbuf));
                curseg = nametoseg(segbuf);
                seg[curseg].length += i;
                fseek(file, where, SEEK_SET);
            }
            fseek(file, hipre.reclen, SEEK_CUR);
        } while (hipre.code != HIREC_END);

        /*
         * assign segment addresses
         */
        seg[SEG_ZERO].length = 1;
        seg[SEG_ZERO].base = 0;

        for (i = SEG_ZERO + 1; i <= SEG_MAX; i++) {
            seg[i].base = ROUNDUP(seg[i - 1].base + seg[i - 1].length);
        }

        fseek(file, hdrpos, SEEK_SET);

        do {
            if (fread(&hipre, sizeof(hipre), 1, file) != 1) {
                fprintf(stderr, "record prefix unreadable\n");
                exit(1);
            }
            i = hipre.reclen;
            if (debug) 
                printf("%5d: record type %d (%s) len %d\n", where,
                    hipre.code, hirectype[hipre.code], hipre.reclen);

            switch (hipre.code) {

            case HIREC_BLK:
                if (fread(&hiblkrec, sizeof(hiblkrec), 1, file) != 1) {
                    fprintf(stderr, "block record header unreadable\n");
                    exit(1);
                }
                i -= sizeof(hiblkrec);
                i -= higetstr(segbuf);
                curseg = nametoseg(segbuf);
                if (debug)
                    printf("block %s addr: %x i0: %x payload: %d\n",
                        segbuf, hiblkrec.addr, hiblkrec.i0, i);
                if (!i) break;

                loadaddr = segoff(curseg, hiblkrec.addr);
                if (debug) {
                    printf("%s block %d for %x at %x\n", 
                        segbuf, curseg, i, loadaddr);
                }
                if (fread(&codebuf[loadaddr], i, 1, file) != 1) {
                    fprintf(stderr, "block record payload unreadable\n");
                    exit(1);
                }
                break;

            case HIREC_RELOC:
                while (i) {
                    if (fread(&hirelrec, sizeof(hirelrec), 1, file) != 1) {
                        fprintf(stderr, "reloc record unreadable\n");
                        exit(1);
                    }
                    i -= sizeof(hirelrec);
                    i -= higetstr(symbuf);
                    addhireloc(segoff(curseg, hirelrec.addr + hiblkrec.addr), hirelrec.flags, symbuf);
                }
                break;

            case HIREC_SYM:
                if (seg[SEG_DATA].length == 0) {
                    seg[SEG_DATA].base = ROUNDUP(seg[SEG_TEXT].base + seg[SEG_TEXT].length);
                }
                seg[SEG_BSS].base = seg[SEG_DATA].base + seg[SEG_DATA].length;
                seg[SEG_BSS].length = 0;
                seg[SEG_UNDEF].base = 
                    ROUNDUP(seg[SEG_BSS].base + seg[SEG_BSS].length);
                undefs = 0;

                while (i) {
                    if (fread(&hisymrec, sizeof(hisymrec), 1, file) != 1) {
                        fprintf(stderr, "reloc record unreadable\n");
                        exit(1);
                    }
                    i -= sizeof(hisymrec);
                    i -= higetstr(segbuf);
                    i -= higetstr(symbuf);
                    
                    if (hisymrec.flags == HTSYM_DEF) {
                        addr = segoff(nametoseg(segbuf), hisymrec.addr);
                        add_sym(symbuf, addr);
                        if (strcmp(segbuf, "text") == 0) {
                            reg_target(addr, CODE);
                        }
                    } else if (hisymrec.flags == HTSYM_UNDEF) {
                        add_sym(symbuf, seg[SEG_UNDEF].base + undefs++);
                    } else if (hisymrec.flags == HTSYM_EQU) {
                        addr = segoff(nametoseg(segbuf), hisymrec.addr);
                        add_sym(symbuf, addr);
                        reg_target(addr, REF);
                    } else {
                        fprintf(stderr, "unknown symbol %s %s type %x\n",
                            segbuf, symbuf, hisymrec.flags);
                        exit(1);
                    }
                }
                seg[SEG_UNDEF].length = undefs;
                break;

            case HIREC_END:
                break;

            default:
            case HIREC_UNK0:
            case HIREC_UNK2:
                if (fread(&hiunkrec, sizeof(hiunkrec), 1, file) != 1) {
                    fprintf(stderr, "unkrec record unreadable\n");
                    exit(1);
                }
                i -= sizeof(hiunkrec);
                i -= higetstr(segbuf);
                break;

            case HIREC_UNK5:
                if (fread(&hiunkrec5, sizeof(hiunkrec5), 1, file) != 1) {
                    fprintf(stderr, "unkrec5 record unreadable\n");
                    exit(1);
                }
                i -= sizeof(hiunkrec5);
                i -= higetstr(segbuf);
                break;
            }
        } while (hipre.code != HIREC_END);
        dohirelocs();
    } else {
        /*
         * read in the raw binary - this goes into a single text segment
         * based at startaddr
         */
        if (debug) {
            fprintf(stderr, "start address 0x%x\n", startaddr);
        }

        rewind(file);

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
        size = fread(&codebuf[startaddr], 1, size, file);
        seg[SEG_TEXT].base = startaddr;
        seg[SEG_TEXT].length = size;
        reg_target(startaddr, CODE|REF);
    }
	fclose(file);

    if (find_symbol("ncsv") != -1) {
        ncsv = find_symbol("ncsv");
        hitech = 1;
    }

    for (tracing = 5; tracing; tracing--) {
        if (debug) {
            fprintf(stderr, "tracing %d code %d\n", tracing, ntarg);
        }
        for (i = 0; i <= ntarg; i++) {
            if (targets[i].flags & CODE) {
                if (debug > 1) {
                    fprintf(stderr, "trace %d code at %s\n", i,
                        addrname(targets[i].addr));
                }
                trace(targets[i].addr);
            }
        }
    }

	if (debug) {
		fprintf(stderr, "%d targets\n", ntarg);
		for (i = 0; i < ntarg; i++) {
            fprintf(stderr, "%5d %s: %s\n", i, 
                addrname(targets[i].addr), fdump(targets[i].flags));
        }
	}

	tracing = 0;
	header(name);
	disas();

	exit(0);
}

word trace_addr;

void
trace(addr)
int addr;
{
	jumped = 0;
	while (!jumped) {
        trace_addr = addr;

		clear();
        if (getseg(addr) == -1) return;
		addr += dis(addr);
	}
}

void
disas()
{
	int left;
	word addr;
	int len;
    int segnum;
    int i;

    for (segnum = SEG_TEXT; segnum <= SEG_DATA; segnum++) {

        left = seg[segnum].length;
        addr = seg[segnum].base;
        in_code = 0;

        if (left == 0) continue;

        printf("\n\torg\t%04xH\n", addr);

        while (left > 0) {
            len = instr(addr, left);
            addr += len;
            left -= len;
        }
    }
    in_code = 0;
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

	if (in_code) {
		if (jumped && !code_addr(addr)) {
			in_code = 0;
		}
	} else {
		if (code_addr(addr)) {
			in_code = 1;
		}
	}

	if (!in_code) {
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

    tab(ADDR);
    sprintf(tbuf, "; %04x", addr);
    outs(tbuf);

	tab(ASCII);
	outs(" ");

	for (i = 0; i < len; i++) {
		c = codebuf[addr + i];
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
		printf("%02x ", codebuf[addr + i]);
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

void
header(char *name)
{
    int i;

	printf(";\n;\tdisas version %d\n;\t%s\n;\n", VERSION, name);
    if (symfile) {
	    printf(";\tsymbols loaded from %s\n;\n", symfile);
    }
    if (obj.ident == OBJECT) {
	    printf(";\twhitesmiths type %x symlen %d %s\n", 
            obj.conf, symlen, (obj.conf & 0200) ? "norelocs" : "");
	    printf(";\tsymbols: %d text: %x(%x) data: %x(%x) bss: %x\n", 
            obj.table / (symlen + 3), 
            obj.textoff, obj.text,
            obj.dataoff, obj.data,
            obj.bss);
    } else if (hihdr.magic == HITECH_MAGIC) {
	    printf(";\thitech header len %d bytesex %x arch %s\n", 
            hihdr.len, hihdr.bytesex, hihdr.arch);
    }
    if (hitech) {
	    printf(";\thitech C detected\n"); 
    }
    printf(";\n");
    segdump();
}

int
tcmp(xp,yp)
struct target *xp, *yp;
{
	return (xp->addr - yp->addr);
}

/*
 * given an address, return the symbol
 */
char *
lookup(word v, int *flagsp)
{
	int i;
    char *s;

    *flagsp = 0;

    s = lookup_sym(v);

    if (getseg(v) == -1) return s;

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

    if (seg[SEG_UNDEF].base && (v >= seg[SEG_UNDEF].base))
        return;
	for (i = 0; i < ntarg; i++) {
		if (targets[i].addr == v) {
            targets[i].flags |= flags;
			return;
        }
	}

    if (debug > 1) {
        printf("registered target %s as %s from %s\n", 
            addrname(v), fdump(flags), addrname(trace_addr));
    }

	if (ntarg+1 == TARGETS) {
		fprintf(stderr,"target table overflow\n");
		exit(-1);
	}
    targets[ntarg].flags = flags;
	targets[ntarg++].addr = v;
}

int
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
 * modify col with the number of bytes output
 */
void 
outval(value, out)
word value;
int out;
{
	char buf[11];		/* (PC+xxxxH) is the longest */
	char *s = 0;
    int flags;
    int i;
    word swaddr;

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
		value = jraddr;

    case OUT_NNW:
	case OUT_NN:
	case OUT_JP:
	case OUT_CALL:
		s = lookup(value, &flags);
		if (tracing) {
            if (out == OUT_NNW) {
                reg_target(value, WORD|REF);
            } else {
                /* do whitesmith's switch statement */
                if (s && (strcmp(s, "c.switch") == 0)) {
                    if (codebuf[(trace_addr) - 3] == 0x21) {
                        swaddr = wordat(trace_addr - 2);
                        if (debug) fprintf(stderr, "switch table at %x\n", swaddr);
                        while (1) {
                            i = wordat(swaddr);
                            reg_target(swaddr, WORD|REF);
                            reg_target(swaddr+2, WORD|REF);
                            if (i != 0) {
                                reg_target(i, CODE|REF);
                                swaddr += 4;
                            } else {
                                reg_target(wordat(swaddr + 2), CODE|REF);
                                break;
                            }
                        }
                    }
                }
                reg_target(value, (out != OUT_NN) ? CODE|REF : REF);
            }
		} else {
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

    jraddr = inst_start + disp + len;
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

    inst_start = addr;
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
				if (am_code == AM_NULL) {
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
			if (am_code == AM_NULL) {
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
                    if (hitech && lastword == ncsv) {
                        outs(","); outnn(OUT_NN); 
                    }
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
                char sbuf[100];
                if (op == 0xcf && micronix) {
                    outop("SYS");
                    instaddr += mnix_scpr(inst_start, byteat, sbuf);
                    outs(sbuf);
                } else {
                    outop("RST");
                    outval(op & 0x38, OUT_RST);
                }
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

