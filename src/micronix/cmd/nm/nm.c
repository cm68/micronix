/*
 * nm - Whitesmith's object file dump utility
 *
 * Displays symbol table, relocations, and hex dump of segments
 * disassembles, too.
 */
#if defined(linux) || defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stdio.h>
#endif

#include <obj.h>
#ifdef DO_HITECH
#include "hiobj.h"
#endif

FILE *fp;
unsigned char *filebuf;         /* only ht_report still indexes this */
unsigned char get_byte();
int dbyte();
long filesize;
int bflag;      /* -b: hex dump segments */
int dflag;      /* -d: disassemble */
int gflag;      /* -g: generate .s files */
int rflag;      /* -r: show relocations */
int vflag;      /* -v: show header */
FILE *gfile;    /* output file for -g */
char gname[256]; /* output filename for -g */

extern char *wsSegNames[];	/* segment names, in wsobj.c */

/*
 * Decode Whitesmith symbol type to text
 * type byte: bits 0-2 = segment (4=abs,5=text,6=data,7=bss), bit 3 = global
 */
void
ws_decode_sym(type, seg, scope, stype)
unsigned char type;
char *seg;
char *scope;
char *stype;
{
    int segval = type & 0x07;  /* bits 0-2 for segment encoding */

    /* global flag is bit 3 */
    if (type & 0x08)
        strcpy(scope, "global");
    else
        strcpy(scope, "local");

    /* segment from bits 0-2 */
    switch (segval) {
    case 4: strcpy(seg, "abs"); strcpy(stype, "def"); break;
    case 5: strcpy(seg, "text"); strcpy(stype, "def"); break;
    case 6: strcpy(seg, "data"); strcpy(stype, "def"); break;
    case 7: strcpy(seg, "bss"); strcpy(stype, "def"); break;
    default: strcpy(seg, "-"); strcpy(stype, "undef"); break;  /* extern/undef */
    }
}

#ifdef DO_HITECH
/*
 * Decode HiTech symbol flags to text
 * flags: bit 4 = global, bits 0-3 = type (0=def, 2=common, 6=undef)
 */
void
ht_decode_sym(flags, psect, scope, stype, seg)
unsigned short flags;
char *psect;
char *scope;
char *stype;
char *seg;
{
    /* global flag is bit 4 (0x10) */
    if (flags & 0x10)
        strcpy(scope, "global");
    else
        strcpy(scope, "local");

    /* type from low nibble */
    switch (flags & 0x0f) {
    case 0: strcpy(stype, "def"); break;    /* defined */
    case 2: strcpy(stype, "common"); break; /* common block */
    case 6: strcpy(stype, "undef"); break;  /* undefined/external */
    default: sprintf(stype, "?%d", flags & 0x0f); break;
    }

    /* segment from psect name or flags */
    if (psect[0]) {
        strcpy(seg, psect);
    } else if (flags & 0x80) {
        strcpy(seg, "abs");
    } else {
        strcpy(seg, "-");
    }
}
#endif

/*
 * Micronix syscall table - argbytes is bytes after rst 08h (including syscall number)
 * Total syscall block size is 1 + argbytes
 */
struct syscall {
    unsigned char argbytes;
    char *name;
} syscalls[] = {
    {3, "indir"},   /* 0 - indirect syscall */
    {1, "exit"},    /* 1 */
    {1, "fork"},    /* 2 */
    {5, "read"},    /* 3 */
    {5, "write"},   /* 4 */
    {5, "open"},    /* 5 */
    {1, "close"},   /* 6 */
    {1, "wait"},    /* 7 */
    {5, "creat"},   /* 8 */
    {5, "link"},    /* 9 */
    {3, "unlink"},  /* 10 */
    {5, "exec"},    /* 11 */
    {3, "chdir"},   /* 12 */
    {1, "time"},    /* 13 */
    {7, "mknod"},   /* 14 */
    {5, "chmod"},   /* 15 */
    {5, "chown"},   /* 16 */
    {3, "sbrk"},    /* 17 */
    {5, "stat"},    /* 18 */
    {5, "seek"},    /* 19 */
    {1, "getpid"},  /* 20 */
    {7, "mount"},   /* 21 */
    {3, "umount"},  /* 22 */
    {1, "setuid"},  /* 23 */
    {1, "getuid"},  /* 24 */
    {1, "stime"},   /* 25 */
    {7, "ptrace"},  /* 26 */
    {1, "alarm"},   /* 27 */
    {3, "fstat"},   /* 28 */
    {1, "pause"},   /* 29 */
    {1, "bad"},     /* 30 */
    {3, "stty"},    /* 31 */
    {3, "gtty"},    /* 32 */
    {5, "access"},  /* 33 */
    {1, "nice"},    /* 34 */
    {1, "sleep"},   /* 35 */
    {1, "sync"},    /* 36 */
    {3, "kill"},    /* 37 */
    {1, "csw"},     /* 38 */
    {1, "ssw"},     /* 39 */
    {1, "bad"},     /* 40 */
    {1, "dup"},     /* 41 */
    {1, "pipe"},    /* 42 */
    {3, "times"},   /* 43 */
    {9, "profil"},  /* 44 */
    {1, "bad"},     /* 45 */
    {1, "bad"},     /* 46 */
    {1, "bad"},     /* 47 */
    {5, "signal"},  /* 48 */
    {3, "lock"},    /* 49 */
    {1, "unlock"},  /* 50 */
    {0, 0}
};
#define NSYS (sizeof(syscalls)/sizeof(syscalls[0]) - 1)

/* Z80 register names */
char *r8[] = { "b", "c", "d", "e", "h", "l", "(hl)", "a" };
char *r16[] = { "bc", "de", "hl", "sp" };
char *r16a[] = { "bc", "de", "hl", "af" };  /* for push/pop */
char *cc[] = { "nz", "z", "nc", "c", "po", "pe", "p", "m" };
char *alu[] = { "add a,", "adc a,", "sub ", "sbc a,", "and ", "xor ", "or ", "cp " };

/*
 * Names live in one block per object, not inside the structures.
 *
 * They were char[16] in struct sym and char[64] in struct usym, and
 * the kernel has 411 symbols nine characters wide: 8,220 bytes in one
 * table and 29,592 in the other, both live at once, on a machine with
 * about 26k to spare.  That is what stopped it disassembling.
 *
 * A pointer costs two bytes and the name costs its own length, so the
 * two tables now come to a tenth of that.  The block is sized exactly
 * when the symbol count and the symbol width are both known, and it
 * is never grown - a realloc would move it and every pointer into it
 * would be left behind.
 */
char *strpool;
long poolsize;
long poolused;

void
poolinit(bytes)
long bytes;
{
    if (strpool)
        free(strpool);
    strpool = (char *)malloc((unsigned)bytes);
    poolsize = strpool ? bytes : 0;
    poolused = 0;
}

char *
poolstr(str)
char *str;
{
    int n;
    char *p;

    n = strlen(str) + 1;
    if (!strpool || poolused + n > poolsize)
        return "";              /* no room: better blank than adrift */
    p = strpool + poolused;
    strcpy(p, str);
    poolused += n;
    return p;
}

/* symbol table for disassembly */
struct sym {
    unsigned short value;
    unsigned char type;
    char *name;
} *symtab;
int nsyms;
int symlen_g;

/* unified relocation table for disassembler */
struct ureloc {
    unsigned long offset;
    unsigned char size;     /* 1=byte, 2=word */
    unsigned char hilo;     /* 0=word, 1=lo, 2=hi (for byte relocs) */
    unsigned char segment;  /* USEG_TEXT or USEG_DATA */
    char target[80];        /* resolved target name with offset */
} *ureltab;
int nurels;
int disasm_pc;      /* current PC during disassembly, for reloc lookup */

/* legacy Whitesmith relocation table */
struct reloc {
    unsigned short offset;
    int symidx;     /* -4=abs, -1=text, -2=data, -3=bss, >=0=symbol index */
    unsigned char hilo;  /* 0=word, 1=lo, 2=hi */
} *reltab;
int nrels;
unsigned short text_off_g, data_off_g, bss_off_g;  /* segment offsets for reloc fixup */

#ifdef DO_HITECH
/* HiTech relocation entry for collection */
struct ht_reloc {
    unsigned long offset;
    unsigned char type;
    char target[64];
};

/* HiTech symbol entry for collection */
struct ht_sym {
    unsigned long value;
    unsigned short flags;
    char psect[32];
    char name[64];
};
#endif

/*
 * Unified object data structures
 * Both Whitesmiths and HiTech readers populate these for common output
 */

/* segment types */
#define USEG_UNDEF  0
#define USEG_ABS    1
#define USEG_TEXT   2
#define USEG_DATA   3
#define USEG_BSS    4

/* symbol scope */
#define USCOPE_LOCAL  0
#define USCOPE_GLOBAL 1

/* unified symbol entry */
struct usym {
    char *name;                 /* into strpool - see poolstr */
    unsigned long value;
    int segment;      /* USEG_* */
    int scope;        /* USCOPE_* */
};

/* unified object data */
struct uobj {
    unsigned char *text;
    unsigned long textsize;
    unsigned long textbase;     /* original base address for .org */
    long textoff;               /* where the text is in the file */
    long dataoff;               /* and the data - see ws_load_uobj */
    unsigned char *data;
    unsigned long datasize;
    unsigned long database;     /* original base address for .org */
    unsigned long bsssize;
    struct usym *syms;
    int nsyms;
    struct ureloc *relocs;
    int nrelocs;
};

/* global unified object for current file */
struct uobj uobj;

/* synthetic local labels - offsets within segments that are referenced */
unsigned short *datarefs = NULL;
int ndatarefs = 0;
unsigned short *textrefs = NULL;
int ntextrefs = 0;
unsigned short *bssrefs = NULL;
int nbssrefs = 0;
unsigned short *relrefs = NULL;  /* relative jump targets */
int nrelrefs = 0;

/* forward declarations */
struct reloc *find_reloc();
char *reloc_name();
char *relocNmByte();

/*
 * find data reference index, returns -1 if not found
 */
int
find_data_ref(offset)
int offset;
{
    int i;
    for (i = 0; i < ndatarefs; i++) {
        if (datarefs[i] == offset)
            return i;
    }
    return -1;
}

/*
 * add a data reference offset, returns the index
 */
int
add_data_ref(offset)
int offset;
{
    int i;
    /* check if already in list */
    for (i = 0; i < ndatarefs; i++) {
        if (datarefs[i] == offset)
            return i;
    }
    /* add to list */
    datarefs = (unsigned short *)realloc(datarefs,
               (ndatarefs + 1) * sizeof(unsigned short));
    datarefs[ndatarefs] = offset;
    return ndatarefs++;
}

/*
 * find text reference index, returns -1 if not found
 */
int
find_text_ref(offset)
int offset;
{
    int i;
    for (i = 0; i < ntextrefs; i++) {
        if (textrefs[i] == offset)
            return i;
    }
    return -1;
}

/*
 * add a text reference offset, returns the index
 */
int
add_text_ref(offset)
int offset;
{
    int i;
    /* check if already in list */
    for (i = 0; i < ntextrefs; i++) {
        if (textrefs[i] == offset)
            return i;
    }
    /* add to list */
    textrefs = (unsigned short *)realloc(textrefs,
               (ntextrefs + 1) * sizeof(unsigned short));
    textrefs[ntextrefs] = offset;
    return ntextrefs++;
}

/*
 * find bss reference index, returns -1 if not found
 */
int
find_bss_ref(offset)
int offset;
{
    int i;
    for (i = 0; i < nbssrefs; i++) {
        if (bssrefs[i] == offset)
            return i;
    }
    return -1;
}

/*
 * add a bss reference offset, returns the index
 */
int
add_bss_ref(offset)
int offset;
{
    int i;
    /* check if already in list */
    for (i = 0; i < nbssrefs; i++) {
        if (bssrefs[i] == offset)
            return i;
    }
    /* add to list */
    bssrefs = (unsigned short *)realloc(bssrefs,
               (nbssrefs + 1) * sizeof(unsigned short));
    bssrefs[nbssrefs] = offset;
    return nbssrefs++;
}

/*
 * find relative jump reference index, returns -1 if not found
 */
int
find_rel_ref(offset)
int offset;
{
    int i;
    for (i = 0; i < nrelrefs; i++) {
        if (relrefs[i] == offset)
            return i;
    }
    return -1;
}

/*
 * add a relative jump reference offset, returns the index
 */
int
add_rel_ref(offset)
int offset;
{
    int i;
    /* check if already in list */
    for (i = 0; i < nrelrefs; i++) {
        if (relrefs[i] == offset)
            return i;
    }
    /* add to list */
    relrefs = (unsigned short *)realloc(relrefs,
               (nrelrefs + 1) * sizeof(unsigned short));
    relrefs[nrelrefs] = offset;
    return nrelrefs++;
}

/*
 * free synthetic references
 */
void
freeSynthRef()
{
    if (datarefs) {
        free(datarefs);
        datarefs = NULL;
    }
    ndatarefs = 0;
    if (textrefs) {
        free(textrefs);
        textrefs = NULL;
    }
    ntextrefs = 0;
    if (bssrefs) {
        free(bssrefs);
        bssrefs = NULL;
    }
    nbssrefs = 0;
    if (relrefs) {
        free(relrefs);
        relrefs = NULL;
    }
    nrelrefs = 0;
}

/*
 * initialize unified object
 */
void
uobj_init()
{
    memset(&uobj, 0, sizeof(uobj));
}

/*
 * free unified object
 */
void
uobj_free()
{
    if (uobj.text) free(uobj.text);
    if (uobj.data) free(uobj.data);
    if (uobj.syms) free(uobj.syms);
    if (uobj.relocs) free(uobj.relocs);
    memset(&uobj, 0, sizeof(uobj));
}

/*
 * lookup symbol by value and segment in unified object
 */
char *
usym_lookup(val, seg)
unsigned long val;
int seg;
{
    register struct usym *sp = uobj.syms;
    int n = uobj.nsyms;

    while (n--) {
        if (sp->segment == seg && sp->value == val)
            return sp->name;
        sp++;
    }
    return NULL;
}

/*
 * find relocation in unified table, NULL if absent.  The entry is
 * eighty-odd bytes, so handing back an index made every caller pay
 * the multiply again for each field it read.
 */
struct ureloc *
find_ureloc(offset)
unsigned long offset;
{
    register struct ureloc *rp = ureltab;
    int n = nurels;

    while (n--) {
        if (rp->offset == offset)
            return rp;
        rp++;
    }
    return NULL;
}

/*
 * find relocation in uobj relocation table for given segment
 */
struct ureloc *
findObjRloc(offset, segment)
unsigned long offset;
int segment;
{
    register struct ureloc *rp = uobj.relocs;
    int n = uobj.nrelocs;

    while (n--) {
        if (rp->offset == offset && rp->segment == segment)
            return rp;
        rp++;
    }
    return NULL;
}

/*
 * format address with symbol if available
 * uses relocation table when available for accurate symbol resolution
 * falls back to symbol table lookup for non-relocatable files
 */
void
fmt_addr(buf, val)
char *buf;
unsigned short val;
{
    struct ureloc *up;
    struct reloc *rp;

    /* check unified relocation table first */
    if (disasm_pc >= 0 && ureltab) {
        up = find_ureloc(disasm_pc);
        if (up && up->size == 2) {
            strcpy(buf, up->target);
            return;
        }
    }

    /* check legacy Whitesmith relocation table */
    if (disasm_pc >= 0 && reltab) {
        rp = find_reloc(disasm_pc);
        if (rp) {
            reloc_name(rp->symidx, val, buf);
            return;
        }
    }

    /* no relocation - use literal value */
    /* hex numbers starting with a-f need leading 0 for assembler */
    if ((val >> 12) >= 10)
        sprintf(buf, "0%04xh", val);
    else
        sprintf(buf, "%04xh", val);
}

/*
 * format a byte operand, checking for hi/lo relocations
 */
void
fmt_byte(buf, val)
char *buf;
unsigned char val;
{
    struct ureloc *up;
    struct reloc *rp;
    char symbuf[64];

    /* check unified relocation table first */
    if (disasm_pc >= 0 && ureltab) {
        up = find_ureloc(disasm_pc);
        if (up && up->size == 1) {
            if (up->hilo == 1)
                sprintf(buf, "low(%s)", up->target);
            else if (up->hilo == 2)
                sprintf(buf, "high(%s)", up->target);
            else
                strcpy(buf, up->target);
            return;
        }
    }

    /* check legacy Whitesmith relocation table */
    if (disasm_pc >= 0 && reltab) {
        rp = find_reloc(disasm_pc);
        if (rp && rp->hilo != 0) {
            relocNmByte(rp->symidx, val, rp->hilo, symbuf);
            sprintf(buf, "%s(%s)", rp->hilo == 1 ? "lo" : "hi", symbuf);
            return;
        }
    }

    /* no byte relocation - use literal value */
    if (val >= 0xa0)
        sprintf(buf, "0%02xh", val);
    else
        sprintf(buf, "%02xh", val);
}

/*
 * format relative jump target address
 * when gflag is set, creates synthetic R0, R1... labels
 */
void
fmt_rel_addr(buf, target)
char *buf;
unsigned short target;
{
    if (gflag) {
        sprintf(buf, "R%d", add_rel_ref(target));
    } else {
        if ((target >> 12) >= 10)
            sprintf(buf, "0%04xh", target);
        else
            sprintf(buf, "%04xh", target);
    }
}

/*
 * Where disasm gets its bytes.
 *
 * It never looks further than three past the one it is on, so it does
 * not need the code in memory - it needs a byte at a time, in order.
 * Point it at a buffer when there is one and at the file otherwise,
 * and a text section of any size costs only the reading.
 */
unsigned char *dis_buf;         /* a resident image, if there is one */
long dis_base;                  /* else the file offset of offset 0 */
long dis_limit;                 /* and how far the section runs */

int
dbyte(a)
long a;
{
    /*
     * Past the end of the section is nothing.
     *
     * disasm reads up to three bytes beyond the instruction it is on,
     * so at the end of a section it reads past it.  That was harmless
     * while the section was its own allocation - it read whatever was
     * after it, which was reliably zero - and streaming makes it read
     * the next member of the archive instead, which is how it turned
     * up.  The answer is the same either way for the section proper;
     * this only decides what a truncated last instruction shows.
     */
    if (dis_limit && a >= dis_limit)
        return 0;
    if (dis_buf)
        return dis_buf[a];
    return get_byte(dis_base + a);
}

/*
 * "iy+4", "ix-3" - an index register and its displacement.
 *
 * Written "%+d" once, which asks printf for an explicit plus on a
 * positive number.  The guest's printf had no such flag: it took the
 * "+" for the conversion character, consumed no argument, and left
 * the "d" to print as a letter, so every operand came out "(iy+d)"
 * and every argument after it in the call was off by one.  The flag
 * is worth having and is in lib/libc now, but a disassembler need not
 * depend on it - and one conversion at each of ten call sites reads
 * better than three.
 */
char *
fmt_idx(reg, d)
char *reg;
int d;
{
    static char b[16];
    int v;

    v = (char)d;
    sprintf(b, "%s%c%d", reg, v < 0 ? '-' : '+', v < 0 ? -v : v);
    return b;
}

/*
 * disassemble one instruction, return length
 * for -g mode, operand_pc is the PC for relocation lookup
 */
int
disasm(addr, pc, buf)
long addr;
int pc;
char *buf;
{
    unsigned char op, op2, d, n;
    unsigned short nn;
    int len = 1;
    char abuf[32];
    char rel;
    int r, b;
    static char *rot[] = { "rlc", "rrc", "rl", "rr", "sla", "sra", "sll", "srl" };

    op = dbyte(addr);

    /* CB prefix - bit operations */
    if (op == 0xcb) {
        op2 = dbyte(addr + 1);
        len = 2;
        r = op2 & 7;
        b = (op2 >> 3) & 7;
        if (op2 < 0x40) {
            sprintf(buf, "%s %s", rot[b], r8[r]);
        } else if (op2 < 0x80) {
            sprintf(buf, "bit %d,%s", b, r8[r]);
        } else if (op2 < 0xc0) {
            sprintf(buf, "res %d,%s", b, r8[r]);
        } else {
            sprintf(buf, "set %d,%s", b, r8[r]);
        }
        return len;
    }

    /* ED prefix - extended */
    if (op == 0xed) {
        op2 = dbyte(addr + 1);
        len = 2;
        switch (op2) {
        case 0x40: sprintf(buf, "in b,(c)"); break;
        case 0x41: sprintf(buf, "out (c),b"); break;
        case 0x42: sprintf(buf, "sbc hl,bc"); break;
        case 0x43: nn = dbyte(addr+2) | (dbyte(addr+3)<<8); len=4;
                   disasm_pc = pc + 2;
                   fmt_addr(abuf, nn); sprintf(buf, "ld (%s),bc", abuf); break;
        case 0x44: sprintf(buf, "neg"); break;
        case 0x45: sprintf(buf, "retn"); break;
        case 0x46: sprintf(buf, "im 0"); break;
        case 0x47: sprintf(buf, "ld i,a"); break;
        case 0x48: sprintf(buf, "in c,(c)"); break;
        case 0x49: sprintf(buf, "out (c),c"); break;
        case 0x4a: sprintf(buf, "adc hl,bc"); break;
        case 0x4b: nn = dbyte(addr+2) | (dbyte(addr+3)<<8); len=4;
                   disasm_pc = pc + 2;
                   fmt_addr(abuf, nn); sprintf(buf, "ld bc,(%s)", abuf); break;
        case 0x4d: sprintf(buf, "reti"); break;
        case 0x4f: sprintf(buf, "ld r,a"); break;
        case 0x50: sprintf(buf, "in d,(c)"); break;
        case 0x51: sprintf(buf, "out (c),d"); break;
        case 0x52: sprintf(buf, "sbc hl,de"); break;
        case 0x53: nn = dbyte(addr+2) | (dbyte(addr+3)<<8); len=4;
                   disasm_pc = pc + 2;
                   fmt_addr(abuf, nn); sprintf(buf, "ld (%s),de", abuf); break;
        case 0x56: sprintf(buf, "im 1"); break;
        case 0x57: sprintf(buf, "ld a,i"); break;
        case 0x58: sprintf(buf, "in e,(c)"); break;
        case 0x59: sprintf(buf, "out (c),e"); break;
        case 0x5a: sprintf(buf, "adc hl,de"); break;
        case 0x5b: nn = dbyte(addr+2) | (dbyte(addr+3)<<8); len=4;
                   disasm_pc = pc + 2;
                   fmt_addr(abuf, nn); sprintf(buf, "ld de,(%s)", abuf); break;
        case 0x5e: sprintf(buf, "im 2"); break;
        case 0x5f: sprintf(buf, "ld a,r"); break;
        case 0x60: sprintf(buf, "in h,(c)"); break;
        case 0x61: sprintf(buf, "out (c),h"); break;
        case 0x62: sprintf(buf, "sbc hl,hl"); break;
        case 0x67: sprintf(buf, "rrd"); break;
        case 0x68: sprintf(buf, "in l,(c)"); break;
        case 0x69: sprintf(buf, "out (c),l"); break;
        case 0x6a: sprintf(buf, "adc hl,hl"); break;
        case 0x6f: sprintf(buf, "rld"); break;
        case 0x72: sprintf(buf, "sbc hl,sp"); break;
        case 0x73: nn = dbyte(addr+2) | (dbyte(addr+3)<<8); len=4;
                   disasm_pc = pc + 2;
                   fmt_addr(abuf, nn); sprintf(buf, "ld (%s),sp", abuf); break;
        case 0x78: sprintf(buf, "in a,(c)"); break;
        case 0x79: sprintf(buf, "out (c),a"); break;
        case 0x7a: sprintf(buf, "adc hl,sp"); break;
        case 0x7b: nn = dbyte(addr+2) | (dbyte(addr+3)<<8); len=4;
                   disasm_pc = pc + 2;
                   fmt_addr(abuf, nn); sprintf(buf, "ld sp,(%s)", abuf); break;
        case 0xa0: sprintf(buf, "ldi"); break;
        case 0xa1: sprintf(buf, "cpi"); break;
        case 0xa2: sprintf(buf, "ini"); break;
        case 0xa3: sprintf(buf, "outi"); break;
        case 0xa8: sprintf(buf, "ldd"); break;
        case 0xa9: sprintf(buf, "cpd"); break;
        case 0xaa: sprintf(buf, "ind"); break;
        case 0xab: sprintf(buf, "outd"); break;
        case 0xb0: sprintf(buf, "ldir"); break;
        case 0xb1: sprintf(buf, "cpir"); break;
        case 0xb2: sprintf(buf, "inir"); break;
        case 0xb3: sprintf(buf, "otir"); break;
        case 0xb8: sprintf(buf, "lddr"); break;
        case 0xb9: sprintf(buf, "cpdr"); break;
        case 0xba: sprintf(buf, "indr"); break;
        case 0xbb: sprintf(buf, "otdr"); break;
        default: sprintf(buf, "db 0edh,0%02xh", op2); break;
        }
        return len;
    }

    /* DD/FD prefix - IX/IY */
    if (op == 0xdd || op == 0xfd) {
        char *ir = (op == 0xdd) ? "ix" : "iy";
        op2 = dbyte(addr + 1);
        len = 2;

        if (op2 == 0x21) {
            nn = dbyte(addr+2) | (dbyte(addr+3)<<8); len=4;
            disasm_pc = pc + 2;
            fmt_addr(abuf, nn); sprintf(buf, "ld %s,%s", ir, abuf);
        } else if (op2 == 0x22) {
            nn = dbyte(addr+2) | (dbyte(addr+3)<<8); len=4;
            disasm_pc = pc + 2;
            fmt_addr(abuf, nn); sprintf(buf, "ld (%s),%s", abuf, ir);
        } else if (op2 == 0x23) {
            sprintf(buf, "inc %s", ir);
        } else if (op2 == 0x2a) {
            nn = dbyte(addr+2) | (dbyte(addr+3)<<8); len=4;
            disasm_pc = pc + 2;
            fmt_addr(abuf, nn); sprintf(buf, "ld %s,(%s)", ir, abuf);
        } else if (op2 == 0x2b) {
            sprintf(buf, "dec %s", ir);
        } else if (op2 == 0x34) {
            d = dbyte(addr+2); len=3;
            sprintf(buf, "inc (%s)", fmt_idx(ir, d));
        } else if (op2 == 0x35) {
            d = dbyte(addr+2); len=3;
            sprintf(buf, "dec (%s)", fmt_idx(ir, d));
        } else if (op2 == 0x36) {
            d = dbyte(addr+2); n = dbyte(addr+3); len=4;
            sprintf(buf, "ld (%s),0%02xh", fmt_idx(ir, d), n);
        } else if (op2 == 0xe1) {
            sprintf(buf, "pop %s", ir);
        } else if (op2 == 0xe3) {
            sprintf(buf, "ex (sp),%s", ir);
        } else if (op2 == 0xe5) {
            sprintf(buf, "push %s", ir);
        } else if (op2 == 0xe9) {
            sprintf(buf, "jp (%s)", ir);
        } else if (op2 == 0xf9) {
            sprintf(buf, "ld sp,%s", ir);
        } else if ((op2 & 0xc0) == 0x40 && (op2 & 7) == 6) {
            /* ld r,(ix+d) */
            d = dbyte(addr+2); len=3;
            sprintf(buf, "ld %s,(%s)", r8[(op2>>3)&7], fmt_idx(ir, d));
        } else if ((op2 & 0xc0) == 0x40 && ((op2>>3) & 7) == 6) {
            /* ld (ix+d),r */
            d = dbyte(addr+2); len=3;
            sprintf(buf, "ld (%s),%s", fmt_idx(ir, d), r8[op2&7]);
        } else if ((op2 & 0xc0) == 0x80 && (op2 & 7) == 6) {
            /* alu (ix+d) */
            d = dbyte(addr+2); len=3;
            sprintf(buf, "%s(%s)", alu[(op2>>3)&7], fmt_idx(ir, d));
        } else if (op2 == 0xcb) {
            /* DD/FD CB d op */
            int b;
            static char *rot[] = { "rlc", "rrc", "rl", "rr", "sla", "sra", "sll", "srl" };
            d = dbyte(addr+2);
            op2 = dbyte(addr+3);
            len = 4;
            b = (op2 >> 3) & 7;
            if (op2 < 0x40) {
                sprintf(buf, "%s (%s)", rot[b], fmt_idx(ir, d));
            } else if (op2 < 0x80) {
                sprintf(buf, "bit %d,(%s)", b, fmt_idx(ir, d));
            } else if (op2 < 0xc0) {
                sprintf(buf, "res %d,(%s)", b, fmt_idx(ir, d));
            } else {
                sprintf(buf, "set %d,(%s)", b, fmt_idx(ir, d));
            }
        } else {
            sprintf(buf, "db 0%02xh,0%02xh", op, op2);
        }
        return len;
    }

    /* main opcodes */
    switch (op) {
    case 0x00: sprintf(buf, "nop"); break;
    case 0x01: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "ld bc,%s", abuf); break;
    case 0x02: sprintf(buf, "ld (bc),a"); break;
    case 0x03: sprintf(buf, "inc bc"); break;
    case 0x04: sprintf(buf, "inc b"); break;
    case 0x05: sprintf(buf, "dec b"); break;
    case 0x06: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "ld b,%s", abuf); len=2; break;
    case 0x07: sprintf(buf, "rlca"); break;
    case 0x08: sprintf(buf, "ex af,af'"); break;
    case 0x09: sprintf(buf, "add hl,bc"); break;
    case 0x0a: sprintf(buf, "ld a,(bc)"); break;
    case 0x0b: sprintf(buf, "dec bc"); break;
    case 0x0c: sprintf(buf, "inc c"); break;
    case 0x0d: sprintf(buf, "dec c"); break;
    case 0x0e: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "ld c,%s", abuf); len=2; break;
    case 0x0f: sprintf(buf, "rrca"); break;

    case 0x10: rel = dbyte(addr+1); len=2;
               nn = pc + 2 + rel;
               fmt_rel_addr(abuf, nn); sprintf(buf, "djnz %s", abuf); break;
    case 0x11: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "ld de,%s", abuf); break;
    case 0x12: sprintf(buf, "ld (de),a"); break;
    case 0x13: sprintf(buf, "inc de"); break;
    case 0x14: sprintf(buf, "inc d"); break;
    case 0x15: sprintf(buf, "dec d"); break;
    case 0x16: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "ld d,%s", abuf); len=2; break;
    case 0x17: sprintf(buf, "rla"); break;
    case 0x18: rel = dbyte(addr+1); len=2;
               nn = pc + 2 + rel;
               fmt_rel_addr(abuf, nn); sprintf(buf, "jr %s", abuf); break;
    case 0x19: sprintf(buf, "add hl,de"); break;
    case 0x1a: sprintf(buf, "ld a,(de)"); break;
    case 0x1b: sprintf(buf, "dec de"); break;
    case 0x1c: sprintf(buf, "inc e"); break;
    case 0x1d: sprintf(buf, "dec e"); break;
    case 0x1e: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "ld e,%s", abuf); len=2; break;
    case 0x1f: sprintf(buf, "rra"); break;

    case 0x20: rel = dbyte(addr+1); len=2;
               nn = pc + 2 + rel;
               fmt_rel_addr(abuf, nn); sprintf(buf, "jr nz,%s", abuf); break;
    case 0x21: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "ld hl,%s", abuf); break;
    case 0x22: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "ld (%s),hl", abuf); break;
    case 0x23: sprintf(buf, "inc hl"); break;
    case 0x24: sprintf(buf, "inc h"); break;
    case 0x25: sprintf(buf, "dec h"); break;
    case 0x26: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "ld h,%s", abuf); len=2; break;
    case 0x27: sprintf(buf, "daa"); break;
    case 0x28: rel = dbyte(addr+1); len=2;
               nn = pc + 2 + rel;
               fmt_rel_addr(abuf, nn); sprintf(buf, "jr z,%s", abuf); break;
    case 0x29: sprintf(buf, "add hl,hl"); break;
    case 0x2a: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "ld hl,(%s)", abuf); break;
    case 0x2b: sprintf(buf, "dec hl"); break;
    case 0x2c: sprintf(buf, "inc l"); break;
    case 0x2d: sprintf(buf, "dec l"); break;
    case 0x2e: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "ld l,%s", abuf); len=2; break;
    case 0x2f: sprintf(buf, "cpl"); break;

    case 0x30: rel = dbyte(addr+1); len=2;
               nn = pc + 2 + rel;
               fmt_rel_addr(abuf, nn); sprintf(buf, "jr nc,%s", abuf); break;
    case 0x31: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "ld sp,%s", abuf); break;
    case 0x32: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "ld (%s),a", abuf); break;
    case 0x33: sprintf(buf, "inc sp"); break;
    case 0x34: sprintf(buf, "inc (hl)"); break;
    case 0x35: sprintf(buf, "dec (hl)"); break;
    case 0x36: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "ld (hl),%s", abuf); len=2; break;
    case 0x37: sprintf(buf, "scf"); break;
    case 0x38: rel = dbyte(addr+1); len=2;
               nn = pc + 2 + rel;
               fmt_rel_addr(abuf, nn); sprintf(buf, "jr c,%s", abuf); break;
    case 0x39: sprintf(buf, "add hl,sp"); break;
    case 0x3a: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "ld a,(%s)", abuf); break;
    case 0x3b: sprintf(buf, "dec sp"); break;
    case 0x3c: sprintf(buf, "inc a"); break;
    case 0x3d: sprintf(buf, "dec a"); break;
    case 0x3e: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "ld a,%s", abuf); len=2; break;
    case 0x3f: sprintf(buf, "ccf"); break;

    case 0x76: sprintf(buf, "halt"); break;

    case 0xc0: sprintf(buf, "ret nz"); break;
    case 0xc1: sprintf(buf, "pop bc"); break;
    case 0xc2: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "jp nz,%s", abuf); break;
    case 0xc3: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "jp %s", abuf); break;
    case 0xc4: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "call nz,%s", abuf); break;
    case 0xc5: sprintf(buf, "push bc"); break;
    case 0xc6: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "add a,%s", abuf); len=2; break;
    case 0xc7: sprintf(buf, "rst 00h"); break;
    case 0xc8: sprintf(buf, "ret z"); break;
    case 0xc9: sprintf(buf, "ret"); break;
    case 0xca: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "jp z,%s", abuf); break;
    case 0xcc: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "call z,%s", abuf); break;
    case 0xcd: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "call %s", abuf); break;
    case 0xce: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "adc a,%s", abuf); len=2; break;
    case 0xcf: sprintf(buf, "rst 08h"); break;

    case 0xd0: sprintf(buf, "ret nc"); break;
    case 0xd1: sprintf(buf, "pop de"); break;
    case 0xd2: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "jp nc,%s", abuf); break;
    case 0xd3: sprintf(buf, "out (0%02xh),a", dbyte(addr+1)); len=2; break;
    case 0xd4: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "call nc,%s", abuf); break;
    case 0xd5: sprintf(buf, "push de"); break;
    case 0xd6: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "sub %s", abuf); len=2; break;
    case 0xd7: sprintf(buf, "rst 10h"); break;
    case 0xd8: sprintf(buf, "ret c"); break;
    case 0xd9: sprintf(buf, "exx"); break;
    case 0xda: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "jp c,%s", abuf); break;
    case 0xdb: sprintf(buf, "in a,(0%02xh)", dbyte(addr+1)); len=2; break;
    case 0xdc: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "call c,%s", abuf); break;
    case 0xde: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "sbc a,%s", abuf); len=2; break;
    case 0xdf: sprintf(buf, "rst 18h"); break;

    case 0xe0: sprintf(buf, "ret po"); break;
    case 0xe1: sprintf(buf, "pop hl"); break;
    case 0xe2: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "jp po,%s", abuf); break;
    case 0xe3: sprintf(buf, "ex (sp),hl"); break;
    case 0xe4: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "call po,%s", abuf); break;
    case 0xe5: sprintf(buf, "push hl"); break;
    case 0xe6: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "and %s", abuf); len=2; break;
    case 0xe7: sprintf(buf, "rst 20h"); break;
    case 0xe8: sprintf(buf, "ret pe"); break;
    case 0xe9: sprintf(buf, "jp (hl)"); break;
    case 0xea: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "jp pe,%s", abuf); break;
    case 0xeb: sprintf(buf, "ex de,hl"); break;
    case 0xec: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "call pe,%s", abuf); break;
    case 0xee: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "xor %s", abuf); len=2; break;
    case 0xef: sprintf(buf, "rst 28h"); break;

    case 0xf0: sprintf(buf, "ret p"); break;
    case 0xf1: sprintf(buf, "pop af"); break;
    case 0xf2: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "jp p,%s", abuf); break;
    case 0xf3: sprintf(buf, "di"); break;
    case 0xf4: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "call p,%s", abuf); break;
    case 0xf5: sprintf(buf, "push af"); break;
    case 0xf6: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "or %s", abuf); len=2; break;
    case 0xf7: sprintf(buf, "rst 30h"); break;
    case 0xf8: sprintf(buf, "ret m"); break;
    case 0xf9: sprintf(buf, "ld sp,hl"); break;
    case 0xfa: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "jp m,%s", abuf); break;
    case 0xfb: sprintf(buf, "ei"); break;
    case 0xfc: nn = dbyte(addr+1) | (dbyte(addr+2)<<8); len=3;
               disasm_pc = pc + 1;
               fmt_addr(abuf, nn); sprintf(buf, "call m,%s", abuf); break;
    case 0xfe: disasm_pc = pc + 1;
               fmt_byte(abuf, dbyte(addr+1)); sprintf(buf, "cp %s", abuf); len=2; break;
    case 0xff: sprintf(buf, "rst 38h"); break;

    default:
        /* ld r,r' and alu r patterns */
        if (op >= 0x40 && op < 0x80) {
            sprintf(buf, "ld %s,%s", r8[(op>>3)&7], r8[op&7]);
        } else if (op >= 0x80 && op < 0xc0) {
            sprintf(buf, "%s%s", alu[(op>>3)&7], r8[op&7]);
        } else {
            sprintf(buf, "db 0%02xh", op);
        }
        break;
    }

    return len;
}

void
error(msg)
char *msg;
{
    fprintf(stderr, "nm: %s\n", msg);
    exit(1);
}

void
error2(msg, arg)
char *msg;
char *arg;
{
    fprintf(stderr, "nm: %s: %s\n", msg, arg);
    exit(1);
}

unsigned short
get_word(off)
long off;
{
    return get_byte(off) | (get_byte(off + 1) << 8);
}

/*
 * The file is READ THROUGH, not held.
 *
 * It used to be slurped whole and indexed, which caps this program at
 * whatever is left of the 64k once it has loaded itself - about 26k.
 * libc.a is 39k and the linked kernel 61k, so both said "out of
 * memory", and neither is an unusual thing to ask about.
 *
 * What has to be resident is the symbol table and the relocations,
 * because a disassembled operand is named from them and they are
 * consulted out of order.  They already are their own allocations,
 * and they are the worst case: the kernel's 411 symbols are about
 * eight kilobytes, and a relocation costs more than a symbol does.
 *
 * The code is not consulted out of order, so it comes through this
 * buffer, which refills when asked for something it does not hold.  A
 * backwards seek refills it too - correct, just slower.
 */
#define IOBUFSZ 512

unsigned char iobuf[IOBUFSZ];
long iobase = -1;
int iolen;

unsigned char
get_byte(off)
long off;
{
    if (iobase < 0 || off < iobase || off >= iobase + iolen) {
        if (fseek(fp, off, 0) != 0)
            return 0;
        iobase = off;
        iolen = fread(iobuf, 1, IOBUFSZ, fp);
        if (iolen <= 0) {
            iolen = 0;
            iobase = -1;
            return 0;
        }
    }
    return iobuf[off - iobase];
}

/*
 * decode segment from type byte
 */
int
decode_seg(type)
unsigned char type;
{
    switch (type & 0x07) {
    case 4: return SEG_ABS;
    case 5: return SEG_TEXT;
    case 6: return SEG_DATA;
    case 7: return SEG_BSS;
    default: return SEG_EXT;  /* extern/undef */
    }
}

/*
 * hex dump a region
 * addr_base is the displayed address offset
 */
void
hexdump(name, start, size, addr_base)
char *name;
long start;
int size;
int addr_base;
{
    int i, j;
    unsigned char c;

    if (size == 0) {
        printf("\n%s: (empty)\n", name);
        return;
    }

    printf("\n%s: %d bytes\n", name, size);

    for (i = 0; i < size; i += 16) {
        printf("  %04x: ", addr_base + i);

        /* hex bytes */
        for (j = 0; j < 16; j++) {
            if (i + j < size)
                printf("%02x ", get_byte(start + i + j));
            else
                printf("   ");
            if (j == 7)
                printf(" ");
        }

        printf(" |");

        /* ascii */
        for (j = 0; j < 16 && i + j < size; j++) {
            c = get_byte(start + i + j);
            if (c >= 0x20 && c < 0x7f)
                printf("%c", c);
            else
                printf(".");
        }

        printf("|\n");
    }
}

/*
 * dump symbol table
 */
void
dump_symbols(symtab_off, symtab_size, symlen)
long symtab_off;
int symtab_size;
int symlen;
{
    int num_syms, i, k;
    unsigned short val;
    unsigned char type;
    char name[16];
    char scope[16], stype[16];
    long off;

    num_syms = symtab_size / (symlen + 3);

    printf("\nSymbol table: %d symbols\n", num_syms);
    printf("  Value   Segment  Scope   Type   Name\n");
    printf("  ------  -------  ------  -----  ----\n");

    off = symtab_off;
    for (i = 0; i < num_syms; i++) {
        char seg[16];

        val = get_word(off);
        type = get_byte(off + 2);
        for (k = 0; k < symlen; k++)
            name[k] = get_byte(off + 3 + k);
        name[symlen] = '\0';

        ws_decode_sym(type, seg, scope, stype);

        printf("  0x%04x  %-7s  %-6s  %-5s  %s\n",
               val, seg, scope, stype, name);

        off += symlen + 3;
    }
}

/*
 * read next relocation record from stream
 * off: file offset pointer (updated)
 * pos: position pointer (updated)
 * symidx: output symbol index (-4=abs, -1=text, -2=data, -3=bss, >=0=symbol)
 * hilo: output hilo (0=word, 1=lo, 2=hi)
 * returns: 1 if relocation found, 0 if end of stream
 */
int
read_reloc(off, pos, symidx, hilo)
long *off;
int *pos;
int *symidx;
int *hilo;
{
    unsigned char b;
    int bump, idx;

    while (1) {
        b = get_byte((*off)++);
        if (b == 0) return 0;  /* end of relocs */

        if (b < REL_BUMP_EXT) {
            *pos += b;
        } else if (b < REL_ABS) {
            bump = ((b - REL_BUMP_EXT) << 8) + get_byte((*off)++) + REL_BUMP_EXT;
            *pos += bump;
        } else {
            /* control byte - decode relocation */
            *hilo = b & 3;

            if ((b & ~3) == REL_ABS) {
                *symidx = -4;
            } else if ((b & ~3) == REL_TEXT) {
                *symidx = -1;
            } else if ((b & ~3) == REL_DATA) {
                *symidx = -2;
            } else if ((b & ~3) == REL_BSS) {
                *symidx = -3;
            } else if (b >= REL_SYM_BASE && b < REL_SYM_EXT) {
                *symidx = (b - REL_SYM_BASE) >> 2;
            } else if ((b & ~3) == REL_SYM_EXT) {
                b = get_byte((*off)++);
                if (b < REL_EXT_LONG) {
                    idx = b + REL_EXT_THR1 - REL_SYM_OFS;
                } else {
                    idx = ((b - REL_EXT_LONG) << 8) + get_byte((*off)++) + REL_EXT_THR2 - REL_SYM_OFS;
                }
                *symidx = idx;
            } else {
                *symidx = -100;  /* unknown */
            }
            return 1;
        }
    }
}

/*
 * dump relocation table
 */
void
dump_relocs(name, reloc_off, symtab_off, symlen, num_syms)
char *name;
long reloc_off;
long symtab_off;
int symlen;
int num_syms;
{
    long off = reloc_off;
    int pos = 0;
    int symidx, hilo;
    int count = 0;
    static char *segtab[] = { "text", "data", "bss", "abs" };

    printf("\n%s relocations:\n", name);
    printf("  Offset  Size  Segment  Target\n");
    printf("  ------  ----  -------  ------\n");

    while (read_reloc(&off, &pos, &symidx, &hilo)) {
        char *seg;
        char *size;
        char target[32];
        char symname[20];

        size = hilo == 0 ? "word" : (hilo == 1 ? "lo" : "hi");

        if (symidx >= 0) {
            /* look up symbol name from symbol table */
            if (symidx < num_syms) {
                long soff = symtab_off + symidx * (symlen + 3);
                int i;
                for (i = 0; i < symlen && i < 19; i++) {
                    symname[i] = get_byte(soff + 3 + i);
                    if (symname[i] == '\0') break;
                }
                symname[i] = '\0';
                strcpy(target, symname);
            } else {
                sprintf(target, "sym[%d]", symidx);
            }
            seg = "-";
        } else if (symidx >= -4) {
            seg = segtab[-symidx - 1];
            target[0] = '\0';
        } else {
            seg = "?";
            sprintf(target, "(%d)", symidx);
        }

        if (target[0])
            printf("  0x%04x  %-4s  %-7s  %s\n", pos, size, seg, target);
        else
            printf("  0x%04x  %-4s  %s\n", pos, size, seg);
        pos += hilo ? 1 : 2;
        count++;
    }

    if (count == 0) {
        printf("  (none)\n");
    }
}

/*
 * parse relocation stream into reltab array
 * returns offset after relocations
 */
long
parse_relocs(reloc_off, limit)
long reloc_off;
long limit;
{
    long off = reloc_off;
    int pos = 0;
    int symidx, hilo;

    nrels = 0;
    reltab = 0;

    /* first pass: count relocations */
    while (read_reloc(&off, &pos, &symidx, &hilo))
        nrels++;

    if (nrels == 0)
        return off;

    /* allocate */
    reltab = (struct reloc *)malloc(nrels * sizeof(struct reloc));

    /* second pass: fill in relocations */
    off = reloc_off;
    pos = 0;
    nrels = 0;

    while (read_reloc(&off, &pos, &symidx, &hilo)) {
        reltab[nrels].offset = pos;
        reltab[nrels].symidx = symidx;
        reltab[nrels].hilo = hilo;
        pos += hilo ? 1 : 2;
        nrels++;
    }

    return off;
}

/*
 * check if there's a relocation at given offset
 * returns relocation index or -1
 */
struct reloc *
find_reloc(offset)
int offset;
{
    register struct reloc *rp = reltab;
    int n = nrels;

    while (n--) {
        if (rp->offset == offset)
            return rp;
        rp++;
    }
    return NULL;
}

/*
 * get symbol name for relocation target
 * for segment-relative relocs, look up symbol at target offset
 * addend is the raw value from the instruction (absolute offset)
 */
char *
reloc_name(symidx, addend, buf)
int symidx;
int addend;
char *buf;
{
    int i, seg;
    int rel_off;  /* offset relative to segment start */

    /* for segment-relative, find symbol at or before target offset */
    if (symidx >= -3 && symidx <= -1) {
        /* -1=text(5), -2=data(6), -3=bss(7) */
        seg = 5 - symidx - 1;  /* convert to type seg value */

        /* addend is already segment-relative for segment relocations */
        rel_off = addend;

        /* check for exact symbol match at this offset */
        for (i = 0; i < nsyms; i++) {
            int sym_off;
            if ((symtab[i].type & 0x07) != seg)
                continue;
            /* normalize symbol value to segment-relative offset */
            if (seg == 5)
                sym_off = symtab[i].value - text_off_g;
            else if (seg == 6)
                sym_off = symtab[i].value - data_off_g;
            else
                sym_off = symtab[i].value - bss_off_g;
            if (sym_off == rel_off) {
                sprintf(buf, "%s", symtab[i].name);
                return buf;
            }
        }

        /* no exact match - use synthetic local label */
        if (symidx == -1) {
            sprintf(buf, "T%d", add_text_ref(rel_off));
        } else if (symidx == -2) {
            sprintf(buf, "D%d", add_data_ref(rel_off));
        } else {
            sprintf(buf, "B%d", add_bss_ref(rel_off));
        }
    } else if (symidx >= 0 && symidx < nsyms) {
        /* for undefined (external) symbols, ignore addend - it's just placeholder */
        if ((symtab[symidx].type & 0x07) == 0 && addend) {
            /* external symbol - addend is placeholder, ignore it */
            sprintf(buf, "%s", symtab[symidx].name);
        } else if (addend) {
            sprintf(buf, "%s+%d", symtab[symidx].name, addend);
        } else {
            sprintf(buf, "%s", symtab[symidx].name);
        }
    } else {
        sprintf(buf, "?sym%d", symidx);
    }
    return buf;
}

/*
 * get symbol name for byte relocation target
 * symidx: symbol index or segment (-1=text, -2=data, -3=bss)
 * val: the byte value at the relocation site
 * hilo: 1=lo, 2=hi
 */
char *
relocNmByte(symidx, val, hilo, buf)
int symidx;
unsigned char val;
int hilo;
char *buf;
{
    /* symbol reference - just use symbol name */
    if (symidx >= 0 && symidx < nsyms) {
        sprintf(buf, "%s", symtab[symidx].name);
        return buf;
    }

    /* segment-relative: use segment+offset form (can't recover local symbol name) */
    if (symidx >= -3 && symidx <= -1) {
        if (symidx == -1)
            sprintf(buf, "_.text+0%02xh", val);
        else if (symidx == -2)
            sprintf(buf, "_.data+0%02xh", val);
        else
            sprintf(buf, "_.bss+0%02xh", val);
        return buf;
    }

    sprintf(buf, "?sym%d", symidx);
    return buf;
}

/*
 * Generate unified .s file from uobj
 * This is the common output function for both Whitesmiths and HiTech formats
 * When dflag is set, output goes to stdout with hex dump per line
 * When gflag is set, output goes to a .s file
 */
void
genUobjSfl(name)
char *name;
{
    /*
     * pc is an UNSIGNED SHORT: sixteen bits, which is exactly this
     * machine's address space, so a section offset cannot be anything
     * else.
     *
     * It was "int", and "pc < (int)uobj.textsize" truncates - the
     * kernel's text is 45,313 bytes, which as a SIGNED sixteen bit
     * int is negative, so the loop did not run once and .text came
     * out empty while the 11,253 byte .data was fine.  Widening to
     * long fixes that and is the wrong answer: it is four bytes and
     * every comparison in the disassembler's main loop becomes a
     * library call on a z80.  Unsigned is the fix; wider is not.
     */
    unsigned short pc;
    int i, len, ref_idx;
    struct ureloc *up;
    char nbuf[128];
    char *p, *sym;

    if (dflag) {
        /* disassembly mode: output to stdout */
        gfile = stdout;
    } else {
        /* generate mode: output to file */
        strncpy(gname, name, sizeof(gname) - 3);
        gname[sizeof(gname) - 3] = '\0';
        p = strrchr(gname, '.');
        if (p && (strcmp(p, ".obj") == 0 || strcmp(p, ".o") == 0)) {
            strcpy(p, ".s");
        } else {
            strcat(gname, ".s");
        }

        gfile = fopen(gname, "w");
        if (!gfile) {
            fprintf(stderr, "nm: cannot create %s\n", gname);
            return;
        }

        printf("  -> %s\n", gname);
    }

    /* copy uobj.relocs to ureltab for disassembler fmt_addr to use */
    ureltab = uobj.relocs;
    nurels = uobj.nrelocs;

    /* header */
    fprintf(gfile, "; Generated from %s by nm -g\n", name);

    /* emit externs */
    for (i = 0; i < uobj.nsyms; i++) {
        if (uobj.syms[i].segment == USEG_UNDEF) {
            fprintf(gfile, "\t.extern %s\n", uobj.syms[i].name);
        }
    }

    /* emit globals */
    for (i = 0; i < uobj.nsyms; i++) {
        if (uobj.syms[i].scope == USCOPE_GLOBAL &&
            uobj.syms[i].segment != USEG_UNDEF) {
            fprintf(gfile, "\t.global %s\n", uobj.syms[i].name);
        }
    }

    /* text section */
    if (uobj.textsize > 0) {
        fprintf(gfile, "\n\t.text\n");

        dis_buf = uobj.text;    /* 0 for whitesmiths: read the file */
        dis_base = uobj.textoff;
        dis_limit = uobj.textsize;

        /* pre-pass: disassemble to find all relative jump targets */
        pc = 0;
        while (pc < (unsigned short)uobj.textsize) {
            disasm_pc = -1;
            len = disasm(pc, pc, nbuf);
            pc += len;
        }

        /* main pass: output with labels */
        pc = 0;
        while (pc < (unsigned short)uobj.textsize) {
            /* check for symbol at this address */
            sym = usym_lookup(pc, USEG_TEXT);
            if (sym) {
                fprintf(gfile, "%s:\n", sym);
            }
            /* check for synthetic text label */
            ref_idx = find_text_ref(pc);
            if (ref_idx >= 0) {
                fprintf(gfile, "T%d:\n", ref_idx);
            }
            /* check for relative jump target label */
            ref_idx = find_rel_ref(pc);
            if (ref_idx >= 0) {
                fprintf(gfile, "R%d:\n", ref_idx);
            }

            /* check for relocation at this address */
            up = findObjRloc(pc, USEG_TEXT);
            if (up) {
                if (up->size == 2) {
                    if (dflag) {
                        fprintf(gfile, "  %04x  %02x %02x        ", (int)(pc + uobj.textbase),
                                dbyte(pc), dbyte(pc+1));
                    } else {
                        fprintf(gfile, "\t");
                    }
                    fprintf(gfile, ".dw %s\n", up->target);
                    pc += 2;
                } else {
                    if (dflag) {
                        fprintf(gfile, "  %04x  %02x           ", (int)(pc + uobj.textbase), dbyte(pc));
                    } else {
                        fprintf(gfile, "\t");
                    }
                    if (up->hilo == 1)
                        fprintf(gfile, ".db low(%s)\n", up->target);
                    else if (up->hilo == 2)
                        fprintf(gfile, ".db high(%s)\n", up->target);
                    else
                        fprintf(gfile, ".db %s\n", up->target);
                    pc += 1;
                }
            } else {
                /* disassemble instruction */
                disasm_pc = -1;
                len = disasm(pc, pc, nbuf);
                if (dflag) {
                    fprintf(gfile, "  %04x  ", (int)(pc + uobj.textbase));
                    for (i = 0; i < 4; i++) {
                        if (i < len)
                            fprintf(gfile, "%02x ", dbyte(pc + i));
                        else
                            fprintf(gfile, "   ");
                    }
                    fprintf(gfile, " %s\n", nbuf);
                } else {
                    fprintf(gfile, "\t%s\n", nbuf);
                }
                pc += len;
            }
        }

        dis_buf = 0;
        dis_limit = 0;
    }

    /* data section */
    if (uobj.datasize > 0) {
        int linelen, in_string, line_start;
        unsigned char c;

        dis_buf = uobj.data;    /* 0 for whitesmiths: read the file */
        dis_base = uobj.dataoff;
        dis_limit = uobj.datasize;

        fprintf(gfile, "\n\t.data\n");

        pc = 0;
        while (pc < (unsigned short)uobj.datasize) {
            /* check for symbol at this address */
            sym = usym_lookup(pc, USEG_DATA);
            if (sym) {
                fprintf(gfile, "%s:\n", sym);
            }
            /* check for synthetic data label */
            ref_idx = find_data_ref(pc);
            if (ref_idx >= 0) {
                fprintf(gfile, "D%d:\n", ref_idx);
            }

            /* check for relocation at this address */
            up = findObjRloc(pc, USEG_DATA);
            if (up) {
                if (up->size == 2) {
                    if (dflag) {
                        fprintf(gfile, "  %04x  %02x %02x        ", (int)(pc + uobj.database),
                                dbyte(pc), dbyte(pc+1));
                    } else {
                        fprintf(gfile, "\t");
                    }
                    fprintf(gfile, ".dw %s\n", up->target);
                    pc += 2;
                } else {
                    if (dflag) {
                        fprintf(gfile, "  %04x  %02x           ", (int)(pc + uobj.database), dbyte(pc));
                    } else {
                        fprintf(gfile, "\t");
                    }
                    if (up->hilo == 1)
                        fprintf(gfile, ".db low(%s)\n", up->target);
                    else if (up->hilo == 2)
                        fprintf(gfile, ".db high(%s)\n", up->target);
                    else
                        fprintf(gfile, ".db %s\n", up->target);
                    pc += 1;
                }
                continue;
            }

            if (dflag) {
                /* dflag mode: emit one byte per line with hex dump */
                c = dbyte(pc);
                fprintf(gfile, "  %04x  %02x           .db 0%02xh\n", (int)(pc + uobj.database), c, c);
                pc++;
            } else {
                /* no relocation - emit raw bytes with string detection */
                fprintf(gfile, "\t.db\t");
                linelen = 8;
                in_string = 0;
                line_start = pc;

                while (pc < (unsigned short)uobj.datasize && linelen < 60) {
                    /* check for symbol, data ref, or relocation - must break line */
                    if (pc > line_start && (usym_lookup(pc, USEG_DATA) ||
                                   find_data_ref(pc) >= 0 ||
                                   findObjRloc(pc, USEG_DATA) != NULL)) {
                        break;
                    }

                    c = dbyte(pc);

                    /* printable ASCII (excluding quotes and backslash) */
                    if (c >= 0x20 && c <= 0x7e && c != '"' && c != '\\') {
                        if (!in_string) {
                            if (linelen > 8) {
                                fprintf(gfile, ",");
                                linelen++;
                            }
                            fprintf(gfile, "\"");
                            linelen++;
                            in_string = 1;
                        }
                        fprintf(gfile, "%c", c);
                        linelen++;
                    } else {
                        if (in_string) {
                            fprintf(gfile, "\"");
                            linelen++;
                            in_string = 0;
                        }
                        if (linelen > 8) {
                            fprintf(gfile, ",");
                            linelen++;
                        }
                        fprintf(gfile, "0%02xh", c);
                        linelen += 4;
                    }
                    pc++;
                }

                if (in_string) {
                    fprintf(gfile, "\"");
                }
                fprintf(gfile, "\n");
            }
        }
    }

    /* bss section */
    if (uobj.bsssize > 0) {
        fprintf(gfile, "\n\t.bss\n");
        fprintf(gfile, "_.bss:\n");

        pc = 0;
        while (pc < (int)uobj.bsssize) {
            /* check for symbol at this address */
            sym = usym_lookup(pc, USEG_BSS);
            if (sym) {
                fprintf(gfile, "%s:\n", sym);
            }
            /* check for synthetic bss label */
            ref_idx = find_bss_ref(pc);
            if (ref_idx >= 0) {
                fprintf(gfile, "B%d:\n", ref_idx);
            }

            /* find next symbol/label or end */
            for (i = pc + 1; i <= (int)uobj.bsssize; i++) {
                if (i == (int)uobj.bsssize ||
                    usym_lookup(i, USEG_BSS) ||
                    find_bss_ref(i) >= 0) {
                    break;
                }
            }

            fprintf(gfile, "\t.ds %d\n", i - (int)pc);
            pc = i;
        }
    }

    /* only close if writing to a file, not stdout */
    if (!dflag) {
        fclose(gfile);
    }

    /* clear ureltab reference */
    ureltab = NULL;
    nurels = 0;
}

/*
 * Load Whitesmiths object data into unified object structure
 */
void
ws_load_uobj(base, objsize)
long base;
long objsize;
{
    int k;
    unsigned char config;
    int symlen;
    unsigned short symtab_size, text_size, data_size, bss_size;
    long symtab_off, textRelocOff, dataRelocOff;
    int num_syms, i, seg;
    long limit = base + objsize;
    int addend;
    int text_relocs, data_relocs, total_relocs;
    char nbuf[80];
    struct reloc *text_reltab;
    int text_nrels;
    int idx;

    uobj_init();

    config = get_byte(base + 1);
    symlen = (config & CONF_SYMASK) * 2 + 1;
    symtab_size = get_word(base + 2);
    text_size = get_word(base + 4);
    data_size = get_word(base + 6);
    bss_size = get_word(base + 8);
    text_off_g = get_word(base + 12);
    data_off_g = get_word(base + 14);
    bss_off_g = data_off_g + data_size;
    num_syms = symtab_size / (symlen + 3);

    /* copy text segment */
    if (text_size > 0) {
        /*
         * NOT copied in.  A linked kernel's text is 45k and its data
         * 11k, which is the whole budget on a 64k machine; what is
         * kept is where to find them.
         */
        uobj.text = 0;
        uobj.textoff = base + 16;
        uobj.textsize = text_size;
    }

    /* copy data segment */
    if (data_size > 0) {
        uobj.data = 0;
        uobj.dataoff = base + 16 + text_size;
        uobj.datasize = data_size;
    }

    uobj.bsssize = bss_size;
    uobj.textbase = text_off_g;
    uobj.database = data_off_g;

    /* load symbol table into legacy symtab for reloc_name to use */
    symtab_off = base + 16 + text_size + data_size;
    nsyms = num_syms;
    symlen_g = symlen;
    if (nsyms > 0) {
        long soff = symtab_off;
        char nb[64];

        symtab = (struct sym *)malloc(nsyms * sizeof(struct sym));
        poolinit((long)nsyms * (symlen + 1));
        for (i = 0; i < nsyms; i++) {
            symtab[i].value = get_word(soff);
            symtab[i].type = get_byte(soff + 2);
            for (k = 0; k < symlen; k++)
                nb[k] = get_byte(soff + 3 + k);
            nb[symlen] = '\0';
            symtab[i].name = poolstr(nb);
            soff += symlen + 3;
        }
    }

    /* convert symbols to unified format */
    if (nsyms > 0) {
        uobj.syms = (struct usym *)malloc(nsyms * sizeof(struct usym));
        for (i = 0; i < nsyms; i++) {
            uobj.syms[i].name = symtab[i].name;  /* the same pooled string */

            /* convert segment: WS uses 4=abs, 5=text, 6=data, 7=bss, <4=undef */
            seg = symtab[i].type & 0x07;
            if (seg < 4) {
                uobj.syms[i].segment = USEG_UNDEF;
                uobj.syms[i].value = symtab[i].value;
            } else if (seg == 4) {
                uobj.syms[i].segment = USEG_ABS;
                uobj.syms[i].value = symtab[i].value;
            } else if (seg == 5) {
                uobj.syms[i].segment = USEG_TEXT;
                /* normalize to segment-relative offset */
                uobj.syms[i].value = symtab[i].value - text_off_g;
            } else if (seg == 6) {
                uobj.syms[i].segment = USEG_DATA;
                /* normalize to segment-relative offset */
                uobj.syms[i].value = symtab[i].value - data_off_g;
            } else {
                uobj.syms[i].segment = USEG_BSS;
                /* normalize to segment-relative offset */
                uobj.syms[i].value = symtab[i].value - bss_off_g;
            }

            /* global flag is bit 3 */
            uobj.syms[i].scope = (symtab[i].type & 0x08) ? USCOPE_GLOBAL : USCOPE_LOCAL;
        }
        uobj.nsyms = nsyms;
    }

    /* count and load relocations */
    if (!(config & CONF_NORELO)) {
        /* parse text relocations */
        textRelocOff = symtab_off + symtab_size;
        dataRelocOff = parse_relocs(textRelocOff, limit);
        text_relocs = nrels;

        /* save text relocs temporarily */
        text_reltab = reltab;
        text_nrels = nrels;
        reltab = NULL;
        nrels = 0;

        /* parse data relocations */
        parse_relocs(dataRelocOff, limit);
        data_relocs = nrels;

        /*
         * The addend for a text relocation is read out of the text,
         * which is not held any more - point the fetch at it.
         */
        dis_buf = uobj.text;
        dis_base = uobj.textoff;
        dis_limit = uobj.textsize;

        /* allocate unified relocs */
        total_relocs = text_relocs + data_relocs;
        if (total_relocs > 0) {
            uobj.relocs = (struct ureloc *)malloc(total_relocs * sizeof(struct ureloc));

            /* convert text relocations */
            for (i = 0; i < text_nrels; i++) {
                uobj.relocs[i].offset = text_reltab[i].offset;
                uobj.relocs[i].hilo = text_reltab[i].hilo;
                uobj.relocs[i].size = text_reltab[i].hilo ? 1 : 2;
                uobj.relocs[i].segment = USEG_TEXT;

                /* resolve target name */
                if (text_reltab[i].hilo) {
                    relocNmByte(text_reltab[i].symidx,
                                   dbyte(text_reltab[i].offset),
                                   text_reltab[i].hilo, nbuf);
                } else {
                    addend = dbyte(text_reltab[i].offset) |
                            (dbyte(text_reltab[i].offset + 1) << 8);
                    reloc_name(text_reltab[i].symidx, addend, nbuf);
                }
                strncpy(uobj.relocs[i].target, nbuf, sizeof(uobj.relocs[i].target) - 1);
                uobj.relocs[i].target[sizeof(uobj.relocs[i].target) - 1] = '\0';
            }

            /* convert data relocations */
            for (i = 0; i < data_relocs; i++) {
                idx = text_nrels + i;
                uobj.relocs[idx].offset = reltab[i].offset;
                uobj.relocs[idx].hilo = reltab[i].hilo;
                uobj.relocs[idx].size = reltab[i].hilo ? 1 : 2;
                uobj.relocs[idx].segment = USEG_DATA;

                /* resolve target name, out of the data section */
                dis_buf = uobj.data;
                dis_base = uobj.dataoff;
                dis_limit = uobj.datasize;
                if (reltab[i].hilo) {
                    relocNmByte(reltab[i].symidx,
                                   dbyte(reltab[i].offset),
                                   reltab[i].hilo, nbuf);
                } else {
                    addend = dbyte(reltab[i].offset) |
                            (dbyte(reltab[i].offset + 1) << 8);
                    reloc_name(reltab[i].symidx, addend, nbuf);
                }
                strncpy(uobj.relocs[idx].target, nbuf, sizeof(uobj.relocs[idx].target) - 1);
                uobj.relocs[idx].target[sizeof(uobj.relocs[idx].target) - 1] = '\0';
            }

            uobj.nrelocs = total_relocs;
        }

        /* free temporary reloc tables */
        if (text_reltab) free(text_reltab);
        if (reltab) free(reltab);
        reltab = NULL;
        nrels = 0;
    }

    /* free legacy symtab - we've converted to uobj.syms */
    if (symtab) {
        free(symtab);
        symtab = NULL;
    }
    nsyms = 0;
}

/*
 * generate .s file from Whitesmiths object - uses unified path
 */
void
gen_sfile(name, base, objsize)
char *name;
long base;
long objsize;
{
    /* load into unified structure */
    ws_load_uobj(base, objsize);

    /* generate output using unified function */
    genUobjSfl(name);

    /* cleanup */
    uobj_free();
    freeSynthRef();
}

/*
 * process object at given offset in filebuf
 * objsize is the size limit for this object (for bounds checking)
 */
void
processObj(name, base, objsize)
char *name;
long base;
long objsize;
{
    int k;
    unsigned char magic, config;
    int symlen;
    unsigned short symtab_size, text_size, data_size, bss_size;
    unsigned short heap_size, text_off, data_off;
    long symtab_off, textRelocOff, dataRelocOff;
    int num_syms;
    long off;
    long limit;
    int i;

    limit = base + objsize;

    /* parse header */
    magic = get_byte(base);
    if (magic != MAGIC) {
        fprintf(stderr, "nm: %s: bad magic 0x%02x\n", name, magic);
        return;
    }

    /* -g or -d mode: generate .s file (or stdout with hex dump) and return */
    if (gflag || dflag) {
        gen_sfile(name, base, objsize);
        return;
    }

    config = get_byte(base + 1);
    symlen = (config & CONF_SYMASK) * 2 + 1;
    symtab_size = get_word(base + 2);
    text_size = get_word(base + 4);
    data_size = get_word(base + 6);
    bss_size = get_word(base + 8);
    heap_size = get_word(base + 10);
    text_off = get_word(base + 12);
    data_off = get_word(base + 14);

    num_syms = symtab_size / (symlen + 3);

    printf("=== %s ===\n", name);
    if (vflag) {
        printf("\nHeader:\n");
        printf("  Magic:       0x%02x\n", magic);
        printf("  Config:      0x%02x", config);
        if (config & CONF_LITTLE) printf(" little-endian");
        if (config & CONF_INT32) printf(" 32-bit-int");
        if (config & CONF_NORELO) printf(" no-reloc");
        printf("\n");
        printf("  Symlen:      %d chars\n", symlen);
        printf("  Symtab:      %d bytes (%d symbols)\n", symtab_size, num_syms);
        printf("  Text:        %d bytes\n", text_size);
        printf("  Data:        %d bytes\n", data_size);
        printf("  BSS:         %d bytes\n", bss_size);
        printf("  Heap:        %d bytes\n", heap_size);
        printf("  Text offset: 0x%04x\n", text_off);
        printf("  Data offset: 0x%04x\n", data_off);
    }

    /* load symbol table for disassembly */
    symtab_off = base + 16 + text_size + data_size;
    nsyms = num_syms;
    symlen_g = symlen;
    if (nsyms > 0) {
        long soff = symtab_off;
        char nb[64];

        symtab = (struct sym *)malloc(nsyms * sizeof(struct sym));
        poolinit((long)nsyms * (symlen + 1));
        for (i = 0; i < nsyms; i++) {
            symtab[i].value = get_word(soff);
            symtab[i].type = get_byte(soff + 2);
            for (k = 0; k < symlen; k++)
                nb[k] = get_byte(soff + 3 + k);
            nb[symlen] = '\0';
            symtab[i].name = poolstr(nb);
            soff += symlen + 3;
        }
    }

    textRelocOff = symtab_off + symtab_size;

    /* hex dump segments if -b */
    if (bflag) {
        hexdump("Text segment", base + 16, text_size, text_off);
        if (data_size > 0)
            hexdump("Data segment", base + 16 + text_size, data_size, data_off);
    }

    /* dump symbol table */
    if (symtab_size > 0) {
        dump_symbols(symtab_off, symtab_size, symlen);
    } else {
        printf("\nSymbol table: (none)\n");
    }

    /* dump relocations */
    if (rflag) {
        if (!(config & CONF_NORELO)) {
            dump_relocs("Text", textRelocOff, symtab_off, symlen, num_syms);

            /* find data reloc offset by scanning past text relocs */
            off = textRelocOff;
            while (off < limit) {
                unsigned char b = get_byte(off++);
                if (b == 0) break;
                if (b >= REL_BUMP_EXT && b < REL_ABS) off++;
                else if (b == REL_SYM_EXT) {
                    b = get_byte(off++);
                    if (b >= REL_EXT_LONG) off++;
                }
            }
            dataRelocOff = off;

            dump_relocs("Data", dataRelocOff, symtab_off, symlen, num_syms);
        } else {
            printf("\nRelocations: (stripped)\n");
        }
    }

    printf("\n");
    if (symtab)
        free(symtab);
    symtab = 0;
    nsyms = 0;
}

#ifdef DO_HITECH
/*
 * generate relocation target name for HiTech
 */
void
ht_reloc_name(relocs, ri, syms, nsyms, addend, textbase, database, buf)
struct ht_reloc *relocs;
int ri;
struct ht_sym *syms;
int nsyms;
int addend;
unsigned long textbase;
unsigned long database;
char *buf;
{
    int i;
    char *target = relocs[ri].target;
    int norm_addend;

    if ((relocs[ri].type & 0xf0) == HT_RPSECT) {
        /* psect-relative: check for exact symbol match */
        for (i = 0; i < nsyms; i++) {
            if (strcmp(syms[i].psect, target) == 0 &&
                (int)syms[i].value == addend) {
                sprintf(buf, "%s", syms[i].name);
                return;
            }
        }
        /* no exact match - normalize addend and use synthetic local label */
        if (strcmp(target, "data") == 0) {
            norm_addend = addend - database;
            sprintf(buf, "D%d", add_data_ref(norm_addend));
        } else if (strcmp(target, "text") == 0) {
            norm_addend = addend - textbase;
            sprintf(buf, "T%d", add_text_ref(norm_addend));
        } else if (strcmp(target, "bss") == 0) {
            sprintf(buf, "B%d", add_bss_ref(addend));
        } else {
            sprintf(buf, "_.%s+%d", target, addend);
        }
    } else {
        /* external symbol - addend is just placeholder value, ignore it */
        sprintf(buf, "%s", target);
    }
}
#endif

#ifdef DO_HITECH
/*
 * Load HiTech object data into unified object structure
 */
void
ht_load_uobj(textbuf, textbase, textsize, databuf, database, datasize, bsssize, relocs, nrelocs, syms, nsyms)
unsigned char *textbuf;
unsigned long textbase;
unsigned long textsize;
unsigned char *databuf;
unsigned long database;
unsigned long datasize;
unsigned long bsssize;
struct ht_reloc *relocs;
int nrelocs;
struct ht_sym *syms;
int nsyms;
{
    int i, addend;
    unsigned long tlen, dlen;

    uobj_init();

    /* copy text segment - normalize to start from 0 */
    tlen = textsize > textbase ? textsize - textbase : 0;
    if (tlen > 0) {
        uobj.text = (unsigned char *)malloc(tlen);
        memcpy(uobj.text, textbuf + textbase, tlen);
        uobj.textsize = tlen;
        uobj.textbase = textbase;
    }

    /* copy data segment - normalize to start from 0 */
    dlen = datasize > database ? datasize - database : 0;
    if (dlen > 0) {
        uobj.data = (unsigned char *)malloc(dlen);
        memcpy(uobj.data, databuf + database, dlen);
        uobj.datasize = dlen;
        uobj.database = database;
    }

    uobj.bsssize = bsssize;

    /* convert symbols - adjust for base offset */
    if (nsyms > 0) {
        uobj.syms = (struct usym *)malloc(nsyms * sizeof(struct usym));
        for (i = 0; i < nsyms; i++) {
            uobj.syms[i].name = poolstr(syms[i].name);

            /* convert segment */
            if ((syms[i].flags & 0x0f) == 6) {
                uobj.syms[i].segment = USEG_UNDEF;
                uobj.syms[i].value = syms[i].value;
            } else if (strcmp(syms[i].psect, "text") == 0) {
                uobj.syms[i].segment = USEG_TEXT;
                uobj.syms[i].value = syms[i].value - textbase;
            } else if (strcmp(syms[i].psect, "data") == 0) {
                uobj.syms[i].segment = USEG_DATA;
                uobj.syms[i].value = syms[i].value - database;
            } else if (strcmp(syms[i].psect, "bss") == 0) {
                uobj.syms[i].segment = USEG_BSS;
                uobj.syms[i].value = syms[i].value;
            } else {
                uobj.syms[i].segment = USEG_ABS;
                uobj.syms[i].value = syms[i].value;
            }

            /* convert scope */
            uobj.syms[i].scope = (syms[i].flags & 0x10) ? USCOPE_GLOBAL : USCOPE_LOCAL;
        }
        uobj.nsyms = nsyms;
    }

    /* convert relocations - adjust for base offset */
    if (nrelocs > 0) {
        uobj.relocs = (struct ureloc *)malloc(nrelocs * sizeof(struct ureloc));
        for (i = 0; i < nrelocs; i++) {
            uobj.relocs[i].offset = relocs[i].offset - textbase;
            uobj.relocs[i].size = relocs[i].type & HT_RSIZE_MASK;
            uobj.relocs[i].hilo = 0;  /* HiTech doesn't have hi/lo byte relocs */
            uobj.relocs[i].segment = USEG_TEXT;  /* HiTech relocs are in text */

            /* get addend from data */
            addend = textbuf[relocs[i].offset];
            if (uobj.relocs[i].size == 2 && relocs[i].offset + 1 < textsize)
                addend |= textbuf[relocs[i].offset + 1] << 8;

            /* resolve target name */
            ht_reloc_name(relocs, i, syms, nsyms, addend, textbase, database, uobj.relocs[i].target);
        }
        uobj.nrelocs = nrelocs;
    }
}
#endif

#ifdef DO_HITECH
/*
 * Guarded to match its caller, which is already inside
 * DO_HITECH.  Without this the definition survives when the
 * macro is off and calls ht_load_uobj, which does not - so
 * nm would not link for a target that has no Hi-Tech
 * support.  Micronix is the first such target.
 */
/*
 * generate .s file from HiTech object - uses unified path
 */
void
gen_ht_sfile(name, textbuf, textbase, textsize, databuf, database, datasize, bsssize, relocs, nrelocs, syms, nsyms)
char *name;
unsigned char *textbuf;
unsigned long textbase;
unsigned long textsize;
unsigned char *databuf;
unsigned long database;
unsigned long datasize;
unsigned long bsssize;
struct ht_reloc *relocs;
int nrelocs;
struct ht_sym *syms;
int nsyms;
{
    /* load into unified structure */
    ht_load_uobj(textbuf, textbase, textsize, databuf, database, datasize, bsssize, relocs, nrelocs, syms, nsyms);

    /* generate output using unified function */
    genUobjSfl(name);

    /* cleanup */
    uobj_free();
    freeSynthRef();
}

#endif /* DO_HITECH */

#ifdef DO_HITECH
/*
 * The tail of processHitech: the relocation and symbol-table
 * listings.  Split out because the whole of processHitech was one
 * four-hundred-line function whose frame - two 64-byte name
 * buffers and three 16-byte decode buffers in the innermost block
 * among them - was more than c0 could hold on the 64K machine.  It
 * reads what the record walk collected and nothing else.
 */
static void
ht_report(relocs, nrelocs, symbol_off, symbol_len)
struct ht_reloc *relocs;
int nrelocs;
long symbol_off;
int symbol_len;
{
    int i;

    /* print collected relocations */
    if (rflag && nrelocs > 0) {
        printf("\nRelocations:\n");
        printf("  Offset  Size  Segment  Target\n");
        printf("  ------  ----  -------  ------\n");

        for (i = 0; i < nrelocs; i++) {
            char *seg;
            char *size;

            size = (relocs[i].type & HT_RSIZE_MASK) == 1 ? "byte" : "word";

            if ((relocs[i].type & 0xf0) == HT_RPSECT) {
                seg = relocs[i].target;
                printf("  0x%04lx  %-4s  %s\n", relocs[i].offset, size, seg);
            } else {
                seg = "-";
                printf("  0x%04lx  %-4s  %-7s  %s\n", relocs[i].offset, size, seg, relocs[i].target);
            }
        }
    }

    /* print symbol table */
    if (symbol_len > 0) {
        long soff = symbol_off;
        long send = symbol_off + symbol_len;
        int nsym = 0;

        /* count symbols */
        while (soff < send) {
            int nlen;
            if (soff + 7 > send) break;
            soff += 6;
            while (soff < send && filebuf[soff]) soff++;
            soff++;
            for (nlen = 0; soff + nlen < send && filebuf[soff + nlen]; nlen++);
            soff += nlen + 1;
            nsym++;
        }

        printf("\nSymbol table: %d symbols\n", nsym);
        printf("  Value     Segment  Scope   Type    Name\n");
        printf("  --------  -------  ------  ------  ----\n");

        soff = symbol_off;
        while (soff < send) {
            unsigned long val;
            unsigned short flags;
            char psect[64], symname[64];
            char scope[16], stype[16], seg[16];
            int plen, nlen;

            if (soff + 7 > send) break;

            val = filebuf[soff] | (filebuf[soff+1] << 8) |
                  ((unsigned long)filebuf[soff+2] << 16) |
                  ((unsigned long)filebuf[soff+3] << 24);
            flags = filebuf[soff+4] | (filebuf[soff+5] << 8);
            soff += 6;

            for (plen = 0; plen < 63 && soff + plen < send; plen++) {
                psect[plen] = filebuf[soff + plen];
                if (psect[plen] == '\0') break;
            }
            psect[plen] = '\0';
            soff += plen + 1;

            for (nlen = 0; nlen < 63 && soff + nlen < send; nlen++) {
                symname[nlen] = filebuf[soff + nlen];
                if (symname[nlen] == '\0') break;
            }
            symname[nlen] = '\0';
            soff += nlen + 1;

            ht_decode_sym(flags, psect, scope, stype, seg);

            printf("  0x%06lx  %-7s  %-6s  %-6s  %s\n",
                   val, seg, scope, stype, symname);
        }
    }

}
#endif

#ifdef DO_HITECH
/*
 * process HiTech object file
 */
void
processHitech(name)
char *name;
{
    long off = 0;
    int reclen, rectype;
    char machine[8];
    int i;

    /* collected data */
    struct ht_reloc *relocs = NULL;
    int nrelocs = 0;
    int reloc_alloc = 0;
    struct ht_sym *htsyms = NULL;
    int nhtsyms = 0;
    int htsym_alloc = 0;
    long symbol_off = 0;
    int symbol_len = 0;
    unsigned long cur_text_off = 0;  /* offset of current TEXT chunk */
    int in_text_psect = 0;           /* current TEXT record is "text" psect */
    unsigned char *textbuf = NULL;   /* combined text data */
    unsigned long textsize = 0;      /* total text size */
    unsigned long textbase = 0xffffffff; /* base address of text */
    unsigned long textalloc = 0;     /* allocated size */
    unsigned char *databuf = NULL;   /* combined data segment */
    unsigned long datasize = 0;      /* total data size */
    unsigned long database = 0xffffffff; /* base address of data */
    unsigned long dataalloc = 0;     /* allocated size */
    unsigned long bsssize = 0;       /* total bss size */

    if (!gflag)
        printf("=== %s (HiTech) ===\n", name);

    /* first pass: collect relocations and find symbol record */
    off = 0;
    while (off < filesize - 3) {
        reclen = filebuf[off] | (filebuf[off + 1] << 8);
        rectype = filebuf[off + 2];
        off += 3;

        if (off + reclen > filesize)
            break;

        if (rectype == HT_TEXT && reclen >= 5) {
            /* collect TEXT data - only from "text" psect */
            char psect[64];
            int plen, dlen;
            unsigned long endoff;

            cur_text_off = filebuf[off] | (filebuf[off+1] << 8) |
                          ((unsigned long)filebuf[off+2] << 16) |
                          ((unsigned long)filebuf[off+3] << 24);

            /* get psect name */
            for (plen = 0; plen < 63 && off + 4 + plen < filesize; plen++) {
                psect[plen] = filebuf[off + 4 + plen];
                if (psect[plen] == '\0') break;
            }
            psect[plen] = '\0';

            dlen = reclen - 4 - plen - 1;
            if (dlen < 0) dlen = 0;

            /* collect text and data psects */
            in_text_psect = (strcmp(psect, "text") == 0);
            if (in_text_psect) {
                endoff = cur_text_off + dlen;
                if (endoff > textsize) textsize = endoff;
                if (cur_text_off < textbase) textbase = cur_text_off;

                /* grow buffer if needed, zero-init new bytes */
                if (endoff > textalloc) {
                    unsigned long oldalloc = textalloc;
                    textalloc = endoff + 1024;
                    textbuf = (unsigned char *)realloc(textbuf, textalloc);
                    if (!textbuf) { fprintf(stderr, "out of memory\n"); return; }
                    memset(textbuf + oldalloc, 0, textalloc - oldalloc);
                }

                /* copy text data */
                if (dlen > 0) {
                    memcpy(textbuf + cur_text_off, filebuf + off + 4 + plen + 1, dlen);
                }
            } else if (strcmp(psect, "data") == 0) {
                endoff = cur_text_off + dlen;
                if (endoff > datasize) datasize = endoff;
                if (cur_text_off < database) database = cur_text_off;

                /* grow buffer if needed, zero-init new bytes */
                if (endoff > dataalloc) {
                    unsigned long oldalloc = dataalloc;
                    dataalloc = endoff + 1024;
                    databuf = (unsigned char *)realloc(databuf, dataalloc);
                    if (!databuf) { fprintf(stderr, "out of memory\n"); return; }
                    memset(databuf + oldalloc, 0, dataalloc - oldalloc);
                }

                /* copy data */
                if (dlen > 0) {
                    memcpy(databuf + cur_text_off, filebuf + off + 4 + plen + 1, dlen);
                }
            } else if (strcmp(psect, "bss") == 0) {
                /* BSS has no data, just track size */
                endoff = cur_text_off + dlen;
                if (endoff > bsssize) bsssize = endoff;
            }
        } else if (rectype == HT_RELOC && (rflag || gflag || dflag) && in_text_psect) {
            /*
             * Offsets into filebuf, which is one malloc: on a machine
             * with 16-bit pointers the buffer cannot be bigger than
             * the address space, so these fit a short by construction.
             * Declared long, every subscript below became a 32-bit sum
             * truncated to 16 bits to make an address - and the add of
             * a short to a long wants both halves in HL, which is the
             * one shape pass2 has no answer for.
             */
            unsigned short roff = off;
            unsigned short rend = off + reclen;

            while (roff < rend) {
                unsigned short reloff;
                unsigned char reltype;
                char target[64];
                int tlen;

                if (roff + 3 > rend) break;

                reloff = filebuf[roff] | (filebuf[roff+1] << 8);
                reltype = filebuf[roff + 2];
                roff += 3;

                for (tlen = 0; tlen < 63 && roff + tlen < rend; tlen++) {
                    target[tlen] = filebuf[roff + tlen];
                    if (target[tlen] == '\0') break;
                }
                target[tlen] = '\0';
                roff += tlen + 1;

                /* grow array if needed */
                if (nrelocs >= reloc_alloc) {
                    reloc_alloc = reloc_alloc ? reloc_alloc * 2 : 64;
                    relocs = (struct ht_reloc *)realloc(relocs,
                             reloc_alloc * sizeof(struct ht_reloc));
                }
                relocs[nrelocs].offset = reloff + cur_text_off;
                relocs[nrelocs].type = reltype;
                strcpy(relocs[nrelocs].target, target);
                nrelocs++;
            }
        } else if (rectype == HT_SYMBOL) {
            /* collect symbols */
            long soff = off;
            long send = off + reclen;

            symbol_off = off;
            symbol_len = reclen;

            while (soff < send) {
                unsigned long val;
                unsigned short flags;
                char psect[64], symname[64];
                int plen, nlen;

                if (soff + 7 > send) break;

                val = filebuf[soff] | (filebuf[soff+1] << 8) |
                      ((unsigned long)filebuf[soff+2] << 16) |
                      ((unsigned long)filebuf[soff+3] << 24);
                flags = filebuf[soff+4] | (filebuf[soff+5] << 8);
                soff += 6;

                for (plen = 0; plen < 63 && soff + plen < send; plen++) {
                    psect[plen] = filebuf[soff + plen];
                    if (psect[plen] == '\0') break;
                }
                psect[plen] = '\0';
                soff += plen + 1;

                for (nlen = 0; nlen < 63 && soff + nlen < send; nlen++) {
                    symname[nlen] = filebuf[soff + nlen];
                    if (symname[nlen] == '\0') break;
                }
                symname[nlen] = '\0';
                soff += nlen + 1;

                /* grow array if needed */
                if (nhtsyms >= htsym_alloc) {
                    htsym_alloc = htsym_alloc ? htsym_alloc * 2 : 32;
                    htsyms = (struct ht_sym *)realloc(htsyms,
                             htsym_alloc * sizeof(struct ht_sym));
                }
                htsyms[nhtsyms].value = val;
                htsyms[nhtsyms].flags = flags;
                strncpy(htsyms[nhtsyms].psect, psect, 31);
                htsyms[nhtsyms].psect[31] = '\0';
                strncpy(htsyms[nhtsyms].name, symname, 63);
                htsyms[nhtsyms].name[63] = '\0';
                nhtsyms++;
            }
        }

        off += reclen;
    }

    /* -g or -d mode: generate .s file (or stdout with hex dump) and return */
    if (gflag || dflag) {
        if (textbase == 0xffffffff) textbase = 0;
        if (database == 0xffffffff) database = 0;
        gen_ht_sfile(name, textbuf, textbase, textsize, databuf, database, datasize, bsssize, relocs, nrelocs, htsyms, nhtsyms);
        free(relocs);
        free(textbuf);
        free(databuf);
        free(htsyms);
        return;
    }

    /* display combined text section */
    if (textsize > 0) {
        if (vflag) {
            printf("\nTEXT: size=%lu\n", textsize);
        }
        if (bflag) {
            /* use textbuf directly for hexdump */
            dis_buf = textbuf;
            dis_base = 0;
            dis_limit = 0;
            hexdump("text", 0, (int)textsize, 0);
            dis_buf = 0;
        dis_limit = 0;
        }
    }

    /* second pass: display psects and other info */
    off = 0;
    while (off < filesize - 3) {
        reclen = filebuf[off] | (filebuf[off + 1] << 8);
        rectype = filebuf[off + 2];
        off += 3;

        if (off + reclen > filesize)
            break;

        if (vflag) {
            printf("[%04lx] rec type=%d len=%d\n", off - 3, rectype, reclen);
        }

        switch (rectype) {
        case HT_IDENT:
            if (reclen >= 10) {
                for (i = 0; i < 4 && i < reclen - 6; i++)
                    machine[i] = filebuf[off + 6 + i];
                machine[i] = '\0';
                if (vflag) {
                    printf("\nIDENT: machine=%s\n", machine);
                }
            }
            break;

        case HT_PSECT:
            if (vflag && reclen >= 3) {
                unsigned short flags;
                char psect[64];
                int plen;

                flags = filebuf[off] | (filebuf[off+1] << 8);

                for (plen = 0; plen < 63 && off + 2 + plen < filesize; plen++) {
                    psect[plen] = filebuf[off + 2 + plen];
                    if (psect[plen] == '\0') break;
                }
                psect[plen] = '\0';

                printf("\nPSECT: name=%s flags=0x%04x", psect[0] ? psect : "(empty)", flags);
                if (flags & HT_F_GLOBAL) printf(" GLOBAL");
                if (flags & HT_F_PURE) printf(" PURE");
                if (flags & HT_F_OVRLD) printf(" OVRLD");
                if (flags & HT_F_ABS) printf(" ABS");
                printf("\n");
            }
            break;

        case HT_START:
            if (vflag && reclen >= 4) {
                unsigned long start;
                char psect[64];
                int plen;

                start = filebuf[off] | (filebuf[off+1] << 8) |
                        ((unsigned long)filebuf[off+2] << 16) |
                        ((unsigned long)filebuf[off+3] << 24);

                for (plen = 0; plen < 63 && off + 4 + plen < filesize; plen++) {
                    psect[plen] = filebuf[off + 4 + plen];
                    if (psect[plen] == '\0') break;
                }
                psect[plen] = '\0';

                printf("\nSTART: addr=0x%08lx psect=%s\n", start, psect);
            }
            break;

        case HT_END:
            if (vflag) {
                printf("\nEND\n");
            }
            break;

        default:
            break;
        }

        off += reclen;
    }

    ht_report(relocs, nrelocs, symbol_off, symbol_len);

    free(relocs);
    free(textbuf);
    free(databuf);
    free(htsyms);

    printf("\n");
}
#endif

#ifdef DO_HITECH
/*
 * process HiTech library file (.LIB)
 */
void
procHTLib(name)
char *name;
{
    long off;
    unsigned short size_symbols, num_modules;
#ifdef notdef
    unsigned short symSize;
#endif
    unsigned short symCnt;
    unsigned long moduleSize;
    long modDataOff;
    char moduleName[256];
    char symName[256];
    int i, j, len;
    unsigned char symFlags;
    char *symTypes = "D?C???U";

    printf("=== %s (HiTech Library) ===\n", name);

    /* read header */
    size_symbols = filebuf[0] | (filebuf[1] << 8);
    num_modules = filebuf[2] | (filebuf[3] << 8);

    if (vflag) {
        printf("\nHeader:\n");
        printf("  Symbol directory size: %d bytes\n", size_symbols);
        printf("  Number of modules: %d\n", num_modules);
    }

    /* module data starts after header + symbol directory */
    modDataOff = 4 + size_symbols;

    /* process symbol directory */
    off = 4;
    for (i = 0; i < num_modules && off < filesize; i++) {
        if (off + 12 > filesize) break;

#ifdef notdef
        symSize = filebuf[off] | (filebuf[off+1] << 8);
#endif
        symCnt = filebuf[off+2] | (filebuf[off+3] << 8);
        moduleSize = filebuf[off+4] | (filebuf[off+5] << 8) |
                     ((unsigned long)filebuf[off+6] << 16) |
                     ((unsigned long)filebuf[off+7] << 24);
        off += 12;

        /* read module name */
        for (len = 0; len < 255 && off + len < filesize; len++) {
            moduleName[len] = filebuf[off + len];
            if (moduleName[len] == '\0') break;
        }
        moduleName[len] = '\0';
        off += len + 1;

        printf("\n%-15s  size=%ld  symbols=%d\n", moduleName, moduleSize, symCnt);

        /* read symbols */
        if (vflag || rflag) {
            for (j = 0; j < symCnt && off < filesize; j++) {
                symFlags = filebuf[off++];

                /* read symbol name */
                for (len = 0; len < 255 && off + len < filesize; len++) {
                    symName[len] = filebuf[off + len];
                    if (symName[len] == '\0') break;
                }
                symName[len] = '\0';
                off += len + 1;

                printf("  %c %s\n",
                       symFlags < 7 ? symTypes[symFlags] : '?',
                       symName);
            }
        } else {
            /* skip symbols */
            for (j = 0; j < symCnt && off < filesize; j++) {
                off++;  /* skip flags */
                while (off < filesize && filebuf[off]) off++;
                off++;  /* skip null terminator */
            }
        }

        /* optionally process module data */
        if (bflag && modDataOff + moduleSize <= filesize) {
            hexdump(moduleName, modDataOff, (int)moduleSize, 0);
        }

        modDataOff += moduleSize;
    }

    printf("\n");
}
#endif

/*
 * process archive file
 * format: 2-byte magic (0xFF75), then entries of:
 *   14-byte name, 2-byte length, file contents
 * terminated by entry with null name
 */
void
processAr(filename)
char *filename;
{
    int i;
    long off;
    char name[15];
    unsigned short len;

    printf("=== Archive: %s ===\n\n", filename);

    off = 2;  /* skip magic */
    while (off < filesize) {
        /* read 14-byte name */
        if (off + 16 > filesize) break;
        for (i = 0; i < 14; i++)
            name[i] = get_byte(off + i);
        name[14] = '\0';

        /* null name marks end */
        if (name[0] == '\0') break;

        len = get_word(off + 14);
        off += 16;

        /* process the object */
        if (off + len <= filesize) {
            processObj(name, off, len);
        }

        off += len;
    }
}

void
process_file(filename)
char *filename;
{
    unsigned short magic16;

    fp = fopen(filename, "rb");
    if (fp == NULL)
        error2("cannot open", filename);

    /* get file size */
    fseek(fp, 0L, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    /*
     * Nothing is read here.  get_byte pulls what it needs through its
     * buffer, so the file stays open until this one is finished with.
     */
    iobase = -1;
    iolen = 0;

    /* check for archive, HiTech, or Whitesmith object */
    magic16 = get_word(0);
    if (magic16 == AR_MAGIC) {
        processAr(filename);
#ifdef DO_HITECH
    } else if (filesize >= 13 && HT_IS_HITECH(filebuf)) {
        processHitech(filename);
#endif
    } else if (get_byte(0) == MAGIC) {
        processObj(filename, 0, filesize);
#ifdef DO_HITECH
    } else {
        /* check for HiTech library format */
        /* library header: 2-byte sym_size, 2-byte num_modules */
        /* module data starts at offset 4 + sym_size */
        unsigned short sym_size = get_word(0);
        unsigned short num_mods = get_word(2);
        long mod_data_off = 4 + sym_size;
        if (num_mods > 0 && num_mods < 1000 &&
            mod_data_off > 4 && mod_data_off < filesize &&
            filesize >= mod_data_off + 13 &&
            HT_IS_HITECH(filebuf + mod_data_off)) {
            procHTLib(filename);
        } else {
            error2("bad magic", filename);
        }
    }
#else
    } else {
        error2("bad magic", filename);
    }
#endif

    free(filebuf);

    fclose(fp);
    fp = 0;
}

void
usage()
{
    fprintf(stderr, "usage: nm [-bdgrv] file.o [...]\n");
    fprintf(stderr, "  -b    hex dump text/data segments\n");
    fprintf(stderr, "  -d    disassemble text segment\n");
    fprintf(stderr, "  -g    generate assemblable .s files\n");
    fprintf(stderr, "  -r    show relocations\n");
    fprintf(stderr, "  -v    show header\n");
    fprintf(stderr, "With no options, only symbol table is shown.\n");
    exit(1);
}

int
main(argc, argv)
int argc;
char **argv;
{
    int i;
    int nfiles = 0;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'b':
                bflag++;
                break;
            case 'd':
                dflag++;
                break;
            case 'g':
                gflag++;
                break;
            case 'r':
                rflag++;
                break;
            case 'v':
                vflag++;
                break;
            default:
                usage();
            }
        } else {
            process_file(argv[i]);
            nfiles++;
        }
    }

    if (nfiles == 0)
        usage();

    return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
