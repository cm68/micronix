/*
 * grunge through a hitech object file and output the object
 * bits in a form that the patching tool can eat it
 *
 * this is a collossal hack simply to facilitate the writing of
 * the hitech patch file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../micronix/include/types.h"
#include "../micronix/include/obj.h"
#include "../include/ws.h"
#include "../include/hitech.h"

#define BUFLEN 65536

/*
 * word should be at least 16 bits unsigned,
 * byte should be at least 8 bits unsigned
 */
typedef unsigned short word;
typedef unsigned char byte;

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

/*
 * variables
 */

FILE *file;
char *name;
char tracing;

char line[80];
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

struct hitechobj hihdr;
struct obj obj;
int symlen;

int undefs;
int addr;

byte relocb[BUFLEN];
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

struct symref {
  char *name;
  int at;
  struct symref *next;
};

struct symref *symrefs;

void addsymref(char *name, int addr)
{
  struct symref *s;
  s = malloc(sizeof(*s));
  s->name = strdup(name);
  s->at = addr;
  s->next = symrefs;
  symrefs = s;
}

struct symref *
getsymref(int addr)
{
  struct symref *s;
  for (s = symrefs; s; s = s->next) {
    if (s->at == addr) {
      break;
    }
  }
  return s;
}

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
    int k = 0;
    int a;
    struct symref *s;
    
    for (i = 0; i < seg[SEG_TEXT].length; i++) {
      a = seg[SEG_TEXT].base + i;
      if (relocb[a]) {
	s = getsymref(a);
	if (s) {
	  printf("%s ", s->name);
	  i++;
	  k = 99;
	} else {
	  printf("ANY ");
	}
      } else {
	printf("%02x ", codebuf[a]);
      }
      if (++k > 8) {
	k = 0;
	printf("\n");
      }
    }
    printf("\n");
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
	relocb[hr->addr] = 1;
	relocb[hr->addr+1] = 1;
        if (hr->type == HTREL_SYM) {
            off = find_symbol(hr->name);
	    addsymref(hr->name, hr->addr);
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
    if (debug > 1)
      printf("adding symbol %s : %x\n", name, v);

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

void
usage(char *s)
{
	fprintf(stderr, "%sUsage:\n\t%s%s%s%s",
		s, progname,
		"[options] <filename>\n",
		"options:\n",
		"\t[-v\tincrease verbosity\n");
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

		case 'v':	/* debug */
			debug++;
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

    doit(name);
}



void
doit(char *name)
{

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

    if ((file = fopen(name, "rb")) == NULL) {
	perror(name);
	exit(1);
    }

    if (fread(&hihdr, sizeof(hihdr), 1, file) != 1) {
        fprintf(stderr, "hitech header read failed");
        exit(1);
    }

    if (hihdr.magic != HITECH_MAGIC) {
        fprintf(stderr, "not a hitech object file");
        exit(1);
    }

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
		      //                        reg_target(addr, CODE);
                    }
                } else if (hisymrec.flags == HTSYM_UNDEF) {
                    add_sym(symbuf, seg[SEG_UNDEF].base + undefs++);
                } else if (hisymrec.flags == HTSYM_EQU) {
                    addr = segoff(nametoseg(segbuf), hisymrec.addr);
                    add_sym(symbuf, addr);
                    		      //reg_target(addr, REF);
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
    segdump();
}
