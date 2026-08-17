/*
 * ld - Whitesmith's object file linker
 *
 * Two-pass linker:
 * Pass 1: assign addresses, resolve symbols
 * Pass 2: write output with relocations applied
 */
#include <stdio.h>

#ifdef linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/*
 * for creat's mode argument, in write_output: the output of a linker
 * is a program and has to be created executable.  On micronix creat
 * is in libu and needs no declaration; here the prototype has to be
 * in scope or gcc treats the call as an error rather than a guess.
 */
#include <sys/stat.h>
#include <fcntl.h>

/*
 * __malloc is the allocation that may come back empty.  On Micronix
 * malloc reports and exits, so asking "is there room" has to go to the
 * entry point underneath it; on the host, malloc is already that.
 */
#ifdef linux
#define __malloc malloc
#endif
#endif

#include "wsobj.h"
#ifdef DO_HITECH
#include "hiobj.h"
#endif

/* use wsSegNames from wsobj.c */

char verbose;
char Vflag;            /* -V: list object files */
char rflag;            /* -r: emit relocatable output */
char sflag;            /* -s: strip symbols */
char out_symlen;       /* output symbol length (0=15, set by -9) */

/*
 * segment base addresses (command line settable)
 */
unsigned short text_base;
unsigned short data_base;
unsigned short bss_base;
int data_set;                   /* -Tdata given, so it is absolute */
int bss_set;                    /* -Tbss given, so it is absolute */

/*
 * running totals for segment layout
 */
unsigned short text_pos;
unsigned short data_pos;
unsigned short bss_pos;

/*
 * final segment sizes
 */
unsigned short total_text;
unsigned short total_data;
unsigned short total_bss;

/*
 * symbol table entry
 */
/*
 * A SYMBOL COSTS WHAT ITS NAME COSTS.  The name was a fixed char[16]
 * at the front, so every symbol paid for sixteen bytes whether it
 * needed them or not, and the average name in what this links is six
 * characters.  It is a tail now: last member, one byte declared, the
 * rest allocated behind the structure by whoever makes the symbol.
 *
 * name[1] and not name[0] or name[], deliberately.  sizeof then
 * carries the terminator, so permalloc(sizeof(struct symbol) + len)
 * is exactly right for a name of len characters on both compilers -
 * ccc gives a flexible member one byte and gcc gives it none - and
 * "char name[0]" is a GNU extension that -pedantic -Werror rejects.
 *
 * This is what LD.md has described this structure as being for some
 * time, and it was not: it said the name is a variable-length tail
 * "so a symbol costs what its name costs" while the code carried the
 * array.  Now it does.
 */
struct symbol {
    unsigned short value;   /* final resolved address */
    unsigned short size;    /* bss: how big, from the object that owns it */
    unsigned char type;     /* original type byte */
    unsigned char seg;      /* decoded segment */
    struct object *obj;     /* defining object (NULL for extern) */
    struct symbol *next;
    char name[1];           /* variable length tail */
};

struct symbol *symbols;
int num_globals;

/*
 * object file info
 */
/*
 * A range of an object's bss that merging took away, because an
 * earlier object already defines the symbol that occupied it.
 * Everything above it moves down by len.
 */
struct bssexc {
    unsigned short off;             /* bss-relative, as it was */
    unsigned short len;
    struct bssexc *next;
};

struct object {
    char *name;
    char *path;                     /* the file to reopen for pass 2 */
    FILE *fp;
    long file_base;                 /* base offset in file (for archives) */
    unsigned char config;
    unsigned char symlen;
    unsigned short symtab_size;
    unsigned short text_size;
    unsigned short data_size;
    unsigned short bss_size;
    unsigned short heap_size;
    unsigned short hdr_text_off;    /* from header */
    unsigned short hdr_data_off;    /* from header */
    unsigned short num_syms;
    /* assigned during pass 1 */
    unsigned short text_off;        /* offset in final text segment */
    unsigned short data_off;        /* offset in final data segment */
    unsigned short bss_off;         /* offset in final bss segment */
    /* relocation table offsets in file */
    long textRelocOff;            /* file offset of text relocs */
    long dataRelocOff;            /* file offset of data relocs */
    /* local symbol table for relocation lookups */
    struct symbol **symtab;         /* array indexed by symbol index */
    /*
     * The value each symbol had IN THIS OBJECT.  A merged bss symbol
     * belongs to whichever object defined it first and its struct
     * holds that object's offset, so a later object cannot ask the
     * symbol where its own copy was.  This can.
     */
    unsigned short *symval;
    /*
     * And the segment THIS object gave it.  symtab[] points at the
     * shared symbol, whose segment is whatever the defining object
     * said - so an object that merely refers to a bss global looks,
     * through symtab[], exactly like one that defines it.
     */
    unsigned char *symseg;
    struct bssexc *exc;             /* what merging removed, in order */
#ifdef DO_HITECH
    /*
     * Hi-Tech specific fields.  Guarded with the code that uses them:
     * five members on every object record is not something to carry
     * for a format nothing in the tree has emitted in years.
     */
    char is_hitech;                 /* 1 if Hi-Tech format */
    unsigned char *ht_text;         /* collected text segment data */
    unsigned char *ht_data;         /* collected data segment data */
    struct ht_reloc *ht_relocs;     /* relocation array */
    int ht_nrelocs;                 /* number of relocations */
#endif
    struct object *next;
};

struct object *objects;
struct object *objects_tail;

struct infile {
    char *name;
    int is_archive;
    FILE *fp;           /* archives: opened once, kept for pass 2 */
    struct infile *next;
};

struct infile *infiles;
struct infile *infiles_tail;

/*
 * pending relocation for -r output
 */
struct outreloc {
    unsigned short offset;      /* offset in merged segment */
    struct symbol *sym;         /* symbol (for ext) or NULL (for seg) */
    unsigned char seg;          /* segment type if sym is NULL */
    unsigned char hilo;         /* REL_WORD/LO/HI */
    struct outreloc *next;
};

struct outreloc *text_relocs;
struct outreloc *textRelocTl;
struct outreloc *data_relocs;
struct outreloc *dataRelocTl;

#ifdef DO_HITECH
/*
 * Hi-Tech relocation entry for linking
 */
struct ht_reloc {
    unsigned short offset;      /* offset within segment */
    unsigned char type;         /* HT_RPSECT/HT_RNAME | HT_RBYTE/HT_RWORD */
    unsigned char seg;          /* SEG_TEXT or SEG_DATA */
    char target[16];            /* target name (symbol or psect) */
};
#endif

char *outfile = "a.out";
FILE *outfp;

/* library search paths */
#define MAX_LIBPATHS 64
char *libpaths[MAX_LIBPATHS];
int nlibpaths;

/* linker-defined symbol patching */
#define LSYM_LTEXT  0
#define LSYM_HTEXT  1
#define LSYM_LDATA  2
#define LSYM_HDATA  3
#define LSYM_LBSS   4
#define LSYM_HBSS   5
#define LSYM_COUNT  6

struct lnksym {
    char *name;
    struct object *obj;
    unsigned short off;
} lnksyms[] = {
    { "__Ltext", 0, 0 },
    { "__Htext", 0, 0 },
    { "__Ldata", 0, 0 },
    { "__Hdata", 0, 0 },
    { "__Lbss",  0, 0 },
    { "__Hbss",  0, 0 },
};

void
usage()
{
    fprintf(stderr, "usage: ld [-vV9rs] [-o outfile] [-L<dir>] [-l<lib>] [-Ttext=addr] [-Tdata=addr] [-Tbss=addr] file...\n");
    fprintf(stderr, "  -V            list object files linked\n");
    fprintf(stderr, "  -r            emit relocatable output (for subsequent links)\n");
    fprintf(stderr, "  -s            strip symbol table from output\n");
    fprintf(stderr, "  -9            use 9-char symbols in output (default 15)\n");
    fprintf(stderr, "  -L<dir>       add <dir> to library search path\n");
    fprintf(stderr, "  -l<lib>       link with library lib<lib>.a\n");
    fprintf(stderr, "  -Ttext=addr   set text segment base address\n");
    fprintf(stderr, "  -Tdata=addr   set data segment base address\n");
    fprintf(stderr, "  -Tbss=addr    set bss segment base address\n");
    exit(1);
}

/*
 * find library file in search paths
 * given "foo", searches for "libfoo.a" in each -L directory
 * returns allocated path string or NULL if not found
 */
/*
 * Allocation that cannot come back empty.
 *
 * Not one of the malloc call sites here looked at what came back;
 * every one dereferenced it at once - memset over a struct object,
 * sprintf into a member name, strcpy of a library path.  On this
 * machine the first thing written through a null lands in page zero,
 * where the rst 08 syscall trap lives.
 *
 * Destroying that trap is not a crash, which is what makes it so hard
 * to find.  The next write() does not trap: execution runs forward
 * through page zero, now zeros, to 0x0100 - the entry point - and the
 * program starts again.  It runs out of memory again, and again, each
 * pass eating another frame, until the stack has walked down through
 * the heap.  What you see is a linker that has consumed 39K of stack
 * at a call depth of four, with no diagnostic anywhere near the cause.
 *
 * cpp had the same thirteen-sites-no-checks problem and the same
 * symptom; see xalloc in cpp/util.c.  There is nothing useful to do
 * with a failed allocation, so say so and stop.
 */
char *permalloc();

char *
xalloc(n)
unsigned n;
{
    return permalloc(n);
}

/*
 * permalloc - the permanent arena
 *
 * NOTHING xalloc HANDS OUT IS EVER FREED.  Symbols, objects, their
 * symtab index maps and the display names of archive members all live
 * from the moment they are made until ld exits - the only free() in
 * this file is segbuf, which comes from __malloc and not from here.
 *
 * Paying malloc for that is expensive on the machine this links for:
 * the allocator in its libc keeps a three-byte header per block and
 * rounds the payload up to a three-byte granule, so the header alone
 * was costing more than the type byte and the segment byte of every
 * symbol put together.  Here it is paid once per chunk.
 *
 * PERMCHUNK is a multiple of the 129-byte granule that libc's malloc
 * grabs from sbrk, so a chunk is a whole number of them.
 *
 * The chunks are not linked and there is no way to give one back.
 * That is the point: this is the allocator for things that last as
 * long as the program does, and ld links one output and exits.
 */
#define PERMCHUNK   (8 * 129)       /* 1032 */

static char *permp;                 /* next free byte */
static unsigned permleft;           /* how many are left there */

/*
 * A symbol with room for its name.  Names are truncated to 15
 * characters, the width of the field in the object file, so that is
 * the most this ever allocates a tail for.
 */
struct symbol *
newsym(name)
char *name;
{
    struct symbol *s;
    register char *d;
    unsigned n;

    for (n = 0; n < 15 && name[n]; n++)
        ;

    s = (struct symbol *)permalloc(sizeof(struct symbol) + n);
    d = s->name;
    while (n--)
        *d++ = *name++;
    *d = '\0';
    s->size = 0;
    return s;
}

char *
permalloc(n)
unsigned n;
{
    char *p;

    /*
     * Even, so the shorts and pointers in what comes back are laid
     * out the way the compiler expects them.
     */
    n = (n + 1) & ~1;

    if (n > permleft) {
        /*
         * What is left of the old chunk is abandoned - at most one
         * node's worth, and chasing it would cost more than it saves.
         */
        permleft = n > PERMCHUNK ? n : PERMCHUNK;
        permp = malloc(permleft);
        if (!permp) {
            fprintf(stderr, "ld: out of memory\n");
            exit(1);
        }
    }
    p = permp;
    permp += n;
    permleft -= n;
    return p;
}

char *
findlib(name)
char *name;
{
    char path[1024];
    char *result;
    int i;
    FILE *fp;

    for (i = 0; i < nlibpaths; i++) {
        sprintf(path, "%s/lib%s.a", libpaths[i], name);
        fp = fopen(path, "rb");
        if (fp) {
            fclose(fp);
            result = xalloc(strlen(path) + 1);
            strcpy(result, path);
            return result;
        }
    }
    return 0;
}

void
error(msg)
char *msg;
{
    fprintf(stderr, "ld: %s\n", msg);
    exit(1);
}

void
error2(msg, arg)
char *msg;
char *arg;
{
    fprintf(stderr, "ld: %s: %s\n", msg, arg);
    exit(1);
}

unsigned short
read_word(fp)
FILE *fp;
{
    unsigned char buf[2];
    if (fread(buf, 1, 2, fp) != 2)
        error("read error");
    return buf[0] | (buf[1] << 8);
}

unsigned char
read_byte(fp)
FILE *fp;
{
    int c;
    c = fgetc(fp);
    if (c == EOF)
        error("read error");
    return c;
}

void
write_byte(b)
unsigned char b;
{
    if (fputc(b, outfp) == EOF)
        error("write error");
}

void
write_word(w)
unsigned short w;
{
    write_byte(w & 0xff);
    write_byte(w >> 8);
}

/*
 * decode segment from type byte
 */
unsigned char
decode_seg(type)
unsigned char type;
{
    switch (type & 0x07) {
    case 4: return SEG_ABS;
    case 5: return SEG_TEXT;
    case 6: return SEG_DATA;
    case 7: return SEG_BSS;
    default: return SEG_EXT;
    }
}

/*
 * lookup symbol by name in global table
 */
struct symbol *
sym_lookup(name)
char *name;
{
    struct symbol *s;
    for (s = symbols; s; s = s->next) {
        if (strcmp(s->name, name) == 0)
            return s;
    }
    return 0;
}

/*
 * A symbol that belongs to one object and is not offered to the link.
 *
 * Bit 3 of the type byte is the global flag, and until now nothing
 * here looked at it: every symbol table entry went into the global
 * table, so a local symbol in two objects collided exactly as two
 * globals would.  That is what the segment sentinels asz emits ran
 * into - .edata and .ebss are in every object by construction.
 *
 * It still gets a symbol struct and still goes in obj->symtab[],
 * because relocations name symbols by index and a local one can be
 * the target of a relocation like any other.  It just never joins
 * the chain the rest of the link searches.
 */
struct symbol *
sym_local(name, value, seg, type, obj)
char *name;
unsigned short value;
unsigned char seg;
unsigned char type;
struct object *obj;
{
    struct symbol *s;

    s = newsym(name);
    s->value = value;
    s->seg = seg;
    s->type = type;
    s->obj = obj;
    s->next = 0;
    return s;
}

/*
 * add or update symbol in global table
 */
struct symbol *
sym_define(name, value, seg, type, obj)
char *name;
unsigned short value;
unsigned char seg;
unsigned char type;
struct object *obj;
{
    struct symbol *s;

    s = sym_lookup(name);
    if (s) {
        /* already exists */
        if (s->seg != SEG_EXT && seg != SEG_EXT) {
            /*
             * Two definitions of one uninitialised global is how C of
             * this vintage is written: a header says "struct user u;"
             * and every file that includes it defines it.  They are
             * the same object - the first wins the address and
             * bss_merge() takes the duplicate space back, having
             * first checked they agree about the size.
             */
            if (s->seg == SEG_BSS && seg == SEG_BSS)
                return s;
            fprintf(stderr, "ld: duplicate symbol: %s\n", name);
            fprintf(stderr, "  defined in %s and %s\n",
                    s->obj ? s->obj->name : "?", obj ? obj->name : "?");
            exit(1);
        }
        if (seg != SEG_EXT) {
            /* this is the definition */
            s->value = value;
            s->seg = seg;
            s->type = type;
            s->obj = obj;
        }
        return s;
    }

    /* new symbol */
    s = newsym(name);
    s->value = value;
    s->seg = seg;
    s->type = type;
    s->obj = obj;
    s->next = symbols;
    symbols = s;
    num_globals++;
    return s;
}

/*
 * read object file header and symbols
 */
void
read_object(name)
char *name;
{
    struct object *obj;
    struct symbol *gsym;
    FILE *fp;
    unsigned char magic;
    unsigned short val;
    unsigned char type, seg;
    char symname[16];
    int i;

    fp = fopen(name, "rb");
    if (fp == NULL)
        error2("cannot open", name);

    magic = read_byte(fp);
    if (magic != MAGIC)
        error2("bad magic", name);

    obj = (struct object *)xalloc(sizeof(struct object));
    memset(obj, 0, sizeof(struct object));
    obj->name = name;
    obj->path = name;
    obj->fp = fp;

    obj->config = read_byte(fp);
    obj->symlen = (obj->config & CONF_SYMASK) * 2 + 1;
    obj->symtab_size = read_word(fp);
    obj->text_size = read_word(fp);
    obj->data_size = read_word(fp);
    obj->bss_size = read_word(fp);
    obj->heap_size = read_word(fp);
    obj->hdr_text_off = read_word(fp);
    obj->hdr_data_off = read_word(fp);

    obj->num_syms = obj->symtab_size / (obj->symlen + 3);

    if (Vflag)
        printf("%s\n", name);

    if (verbose) {
        printf("%s: symlen=%d text=%d data=%d bss=%d syms=%d\n",
               name, obj->symlen,
               obj->text_size, obj->data_size, obj->bss_size, obj->num_syms);
    }

    /* add to list */
    if (!objects) {
        objects = objects_tail = obj;
    } else {
        objects_tail->next = obj;
        objects_tail = obj;
    }

    /* allocate local symbol table for relocation lookups */
    obj->symtab = (struct symbol **)xalloc(obj->num_syms * sizeof(struct symbol *));
    obj->symval = (unsigned short *)xalloc(obj->num_syms * sizeof(unsigned short));
    obj->symseg = (unsigned char *)xalloc(obj->num_syms);

    /* skip to symbol table: header(16) + text + data */
    fseek(fp, (long)(16 + obj->text_size + obj->data_size), SEEK_SET);

    /* read symbols */
    for (i = 0; i < obj->num_syms; i++) {
        val = read_word(fp);
        type = read_byte(fp);
        fread(symname, 1, obj->symlen, fp);
        symname[obj->symlen] = '\0';
        seg = decode_seg(type);

        /* add to global symbol table, get back pointer */
        gsym = (type & 0x08)
                ? sym_define(symname, val, seg, type, obj)
                : sym_local(symname, val, seg, type, obj);
        obj->symtab[i] = gsym;
        obj->symval[i] = val;
        obj->symseg[i] = seg;

        if (verbose > 1) {
            printf("  [%d] %s: val=0x%04x seg=%s%s\n",
                   i, symname, val, wsSegNames[seg],
                   (type & 0x08) ? " global" : "");
        }
    }

    /* record relocation table positions */
    obj->textRelocOff = ftell(fp);

    /* skip text relocs to find data relocs */
    while (1) {
        unsigned char b = read_byte(fp);
        if (b == 0) break;
        if (b >= 32 && b < 64) read_byte(fp);  /* extended bump */
        else if (b == 0xfc) {
            b = read_byte(fp);
            if (b >= 0x80) read_byte(fp);  /* extended symbol */
        }
    }
    obj->dataRelocOff = ftell(fp);

    if (verbose > 1) {
        printf("  text_reloc@0x%lx data_reloc@0x%lx\n",
               obj->textRelocOff, obj->dataRelocOff);
    }

    /*
     * Done with it until pass 2, which opens it again by name.  Held
     * open, fourteen objects and three libraries is more files than a
     * program has - and holding them bought nothing, because every
     * offset that matters is recorded above.
     */
    fclose(fp);
    obj->fp = NULL;
}

/*
 * check if symbol name is currently undefined
 */
int
is_undefined(name)
char *name;
{
    struct symbol *s = sym_lookup(name);
    return s && s->seg == SEG_EXT;
}

/*
 * check if there are any undefined symbols
 */
int
has_undefined()
{
    struct symbol *s;
    for (s = symbols; s; s = s->next) {
        if (s->seg == SEG_EXT)
            return 1;
    }
    return 0;
}

int ar_byindex();

/*
 * read object from archive at given offset
 * name is the archive member name for display
 */
void
read_ar_obj(arname, fp, base, membername)
char *arname;
FILE *fp;
long base;
char *membername;
{
    struct object *obj;
    struct symbol *gsym;
    unsigned char magic;
    unsigned short val;
    unsigned char type, seg;
    char symname[16];
    char *fullname;
    int i;

    fseek(fp, (long)(base), SEEK_SET);

    magic = read_byte(fp);
    if (magic != MAGIC) {
        fprintf(stderr, "ld: %s(%s): bad magic\n", arname, membername);
        return;
    }

    /* create display name "archive(member)" */
    fullname = (char *)xalloc(strlen(arname) + strlen(membername) + 3);
    sprintf(fullname, "%s(%s)", arname, membername);

    obj = (struct object *)xalloc(sizeof(struct object));
    memset(obj, 0, sizeof(struct object));
    obj->name = fullname;
    obj->path = arname;
    obj->fp = fp;
    obj->file_base = base;

    obj->config = read_byte(fp);
    obj->symlen = (obj->config & CONF_SYMASK) * 2 + 1;
    obj->symtab_size = read_word(fp);
    obj->text_size = read_word(fp);
    obj->data_size = read_word(fp);
    obj->bss_size = read_word(fp);
    obj->heap_size = read_word(fp);
    obj->hdr_text_off = read_word(fp);
    obj->hdr_data_off = read_word(fp);

    obj->num_syms = obj->symtab_size / (obj->symlen + 3);

    if (Vflag)
        printf("%s\n", fullname);

    if (verbose) {
        printf("%s: symlen=%d text=%d data=%d bss=%d syms=%d\n",
               fullname, obj->symlen,
               obj->text_size, obj->data_size, obj->bss_size, obj->num_syms);
    }

    /* add to list */
    if (!objects) {
        objects = objects_tail = obj;
    } else {
        objects_tail->next = obj;
        objects_tail = obj;
    }

    /* allocate local symbol table for relocation lookups */
    obj->symtab = (struct symbol **)xalloc(obj->num_syms * sizeof(struct symbol *));
    obj->symval = (unsigned short *)xalloc(obj->num_syms * sizeof(unsigned short));
    obj->symseg = (unsigned char *)xalloc(obj->num_syms);

    /* seek to symbol table */
    fseek(fp, (long)(base + 16 + obj->text_size + obj->data_size), SEEK_SET);

    /* read symbols */
    for (i = 0; i < obj->num_syms; i++) {
        val = read_word(fp);
        type = read_byte(fp);
        fread(symname, 1, obj->symlen, fp);
        symname[obj->symlen] = '\0';
        seg = decode_seg(type);

        gsym = (type & 0x08)
                ? sym_define(symname, val, seg, type, obj)
                : sym_local(symname, val, seg, type, obj);
        obj->symtab[i] = gsym;
        obj->symval[i] = val;
        obj->symseg[i] = seg;

        if (verbose > 1) {
            printf("  [%d] %s: val=0x%04x seg=%s%s\n",
                   i, symname, val, wsSegNames[seg],
                   (type & 0x08) ? " global" : "");
        }
    }

    /* record relocation table positions (relative to file, not archive) */
    obj->textRelocOff = ftell(fp);

    /* skip text relocs to find data relocs */
    while (1) {
        unsigned char b = read_byte(fp);
        if (b == 0) break;
        if (b >= 32 && b < 64) read_byte(fp);
        else if (b == 0xfc) {
            b = read_byte(fp);
            if (b >= 0x80) read_byte(fp);
        }
    }
    obj->dataRelocOff = ftell(fp);

    if (verbose > 1) {
        printf("  text_reloc@0x%lx data_reloc@0x%lx\n",
               obj->textRelocOff, obj->dataRelocOff);
    }

}

/*
 * process archive file - include only objects that satisfy undefined symbols
 * returns number of objects added
 */
int
read_archive(f)
struct infile *f;
{
    char *name = f->name;
    FILE *fp;
    unsigned char buf[2];
    unsigned short magic16;
    int count;
    int v7;

    /*
     * Once for the whole link.  The caller's loop can call this again
     * for circularity between archives, and the members taken out of
     * an archive keep its handle for pass 2 - so reopening leaked a
     * FILE and its 512 byte buffer per call, and _NFILE is twelve.
     * Linking anything that needed three passes over libc ran out of
     * files on an archive that was already open.
     */
    if (!f->fp) {
        struct infile *o;

        /*
         * A library can be named more than once - the driver passes
         * "-lc -lu -lc" so that libc and libu can satisfy each other -
         * and each mention is its own record.  They are the same file
         * and must share the one handle: three opens of libc.a used
         * three of the twelve a program has, and the output file had
         * nowhere left to go.
         */
        for (o = infiles; o; o = o->next)
            if (o != f && o->fp && strcmp(o->name, name) == 0)
                break;
        if (o) {
            f->fp = o->fp;
        } else {
            f->fp = fopen(name, "rb");
            if (f->fp == NULL)
                error2("cannot open", name);
        }
    }
    fp = f->fp;
    if (fseek(fp, 0L, SEEK_SET) != 0)
        error2("seek error", name);

    if (fread(buf, 1, 2, fp) != 2)
        error2("read error", name);

    /*
     * Two archive formats, told apart by the magic and differing only
     * in the member header.  ar(1) writes the v7 one and is becoming
     * the librarian here; the whitesmiths one is what wslib wrote and
     * what every library on disk still is.
     *
     * They start the same way - fourteen bytes of name - so only the
     * rest of the header, the way the end is found, and the padding
     * are per-format.
     */
    magic16 = buf[0] | (buf[1] << 8);
    if (magic16 == AR_MAGIC)
        v7 = 0;
    else if (magic16 == V7_MAGIC)
        v7 = 1;
    else
        error2("bad archive magic", name);

    if (verbose) {
        printf("scanning %s archive %s\n", v7 ? "v7" : "ws", name);
    }

    /*
     * EVERY ARCHIVE HAS AN INDEX.  ar writes one whenever it writes an
     * archive and maintains it through delete, move and quick-append,
     * and its offsets are three bytes, so there is no size at which it
     * declines - sixteen megabytes is past anything this machine can
     * hold, let alone link.
     *
     * So an archive without an index is not a slower kind of archive,
     * it is a broken one, and saying so is better than quietly taking
     * an hour to do what should take a moment.
     *
     * What used to be here was the walk: open every member in turn,
     * read its whole symbol table, ask whether it defined anything
     * still wanted, and go round again because a member taken can
     * want one already passed.  That is gone, and ar_needed with it.
     */
    count = ar_byindex(name, fp, v7);
    if (count < 0)
        error2("no symbol index in", name);

    /* the handle stays on the infile record; pass 2 still needs it */
    return count;
}

/*
 * Take members named by the archive's symbol index.
 *
 * The index is a member called __.SYMDEF and it is the first one -
 * see the long comment in ar.c for what is in it.  Two bytes of
 * count, then per symbol THREE bytes of offset and a NUL terminated
 * name, the offset naming the member's HEADER so a reader can seek
 * there and carry on as though it had walked to it.
 *
 * Three because two capped a usable archive at 64K and libc.a was
 * already 40K.  Three reaches 16M and still costs no long arithmetic
 * to assemble - a load and two shifts - which is the whole reason
 * this format is not v7's four.
 *
 * What this replaces is ar_needed(), which opens every member in the
 * archive and reads its whole symbol table to ask one question.  With
 * an index the question is asked of one member's worth of bytes read
 * once, and the members that are not wanted are never touched.
 *
 * Returns the number of members taken, or -1 for "no index here",
 * which is not an error: an archive from before this existed, or one
 * too big to index, is read the old way by the caller.
 *
 * The pass repeats while it keeps taking, for the same reason the
 * caller's loop does: a member pulled in can want something defined
 * by a member EARLIER in the index, which this pass has already gone
 * by.  Each repeat costs a walk of the index and no file reads at
 * all, so draining an archive this way is cheap where draining it by
 * ar_needed() is what made the linker slow.
 */
int
ar_byindex(name, fp, v7)
char *name;
FILE *fp;
int v7;
{
    unsigned char hdr[26];
    char membername[16];
    long idxbase, idxend, pos;
    long base;
    unsigned short nsym, i;
    long memboff;
    char symname[64];
    int j, c, count, added;

    if (fseek(fp, 2L, SEEK_SET) != 0)
        return -1;
    if (fread(membername, 1, 14, fp) != 14)
        return -1;
    membername[14] = '\0';
    if (strcmp(membername, "__.SYMDEF") != 0)
        return -1;

    /* the index member's own length, per format */
    if (v7) {
        if (fread(hdr, 1, 12, fp) != 12)
            return -1;
        /*
         * Three bytes of the v7 size, which is four.  An index for a
         * 16M archive - the most ar will write one for - cannot need
         * the fourth, and reading three keeps this off long
         * arithmetic it does not need.
         */
        idxend = (long)(hdr[V7_SIZEOFF] & 0xff)
               | ((long)(hdr[V7_SIZEOFF + 1] & 0xff) << 8)
               | ((long)(hdr[V7_SIZEOFF + 2] & 0xff) << 16);
        idxbase = 2 + V7_HDRSIZ;
    } else {
        if (fread(hdr, 1, 2, fp) != 2)
            return -1;
        idxend = hdr[0] | (hdr[1] << 8);
        idxbase = 2 + 16;
    }
    if (idxend < 2)
        return -1;
    idxend += idxbase;

    /*
     * Read from the file rather than into a buffer.
     *
     * The index is a few kilobytes - libc's is 253 symbols - and this
     * linker has sixty-four of them for everything.  Buffering it
     * would cost that much for as long as the archive is being
     * scanned, and for nothing: the passes below are few, stdio is
     * already buffering the reads, and the walk this replaces read
     * every member of the archive rather than every byte of one.
     */
    count = 0;
    do {
        added = 0;
        if (fseek(fp, idxbase, SEEK_SET) != 0)
            break;
        if (fread(hdr, 1, 2, fp) != 2)
            break;
        nsym = hdr[0] | (hdr[1] << 8);

        for (i = 0; i < nsym; i++) {
            if (ftell(fp) >= idxend)
                break;
            if (fread(hdr, 1, 3, fp) != 3)
                break;
            memboff = (long)(hdr[0] & 0xff)
                    | ((long)(hdr[1] & 0xff) << 8)
                    | ((long)(hdr[2] & 0xff) << 16);
            for (j = 0; (c = getc(fp)) != EOF && c; ) {
                if (j < (int)sizeof(symname) - 1)
                    symname[j++] = c;
            }
            symname[j] = '\0';
            if (c == EOF)
                break;

            if (!is_undefined(symname))
                continue;

            /*
             * Wanted.  The place in the index has to be kept: the
             * member is read through the same handle, and the scan
             * carries on from here afterwards.
             */
            pos = ftell(fp);

            if (fseek(fp, memboff, SEEK_SET) != 0)
                break;
            if (fread(membername, 1, 14, fp) != 14)
                break;
            membername[14] = '\0';
            /*
             * The header is read to be sure it is there, not to learn
             * the length: read_ar_obj() works from the base and the
             * object's own header says how big it is.  A walk needs
             * the length to find the NEXT member; an index does not.
             */
            if (v7) {
                if (fread(hdr, 1, 12, fp) != 12)
                    break;
                base = memboff + V7_HDRSIZ;
            } else {
                if (fread(hdr, 1, 2, fp) != 2)
                    break;
                base = memboff + 16;
            }

            if (verbose)
                printf("including %s(%s) [index]\n", name, membername);
            read_ar_obj(name, fp, base, membername);
            count++;
            added++;

            if (fseek(fp, pos, SEEK_SET) != 0)
                break;
        }
    } while (added);

    return count;
}

/*
 * map Hi-Tech psect name to Whitesmith segment
 */
unsigned char
map_psect(psect)
char *psect;
{
    if (strcmp(psect, "text") == 0)
        return SEG_TEXT;
    if (strcmp(psect, "data") == 0)
        return SEG_DATA;
    if (strcmp(psect, "bss") == 0)
        return SEG_BSS;
    /* empty or unknown psect maps to absolute */
    return SEG_ABS;
}

/*
 * read string from file (null-terminated, max len-1 chars)
 */
int
read_str(fp, buf, maxlen)
FILE *fp;
char *buf;
int maxlen;
{
    int i;
    int c;
    for (i = 0; i < maxlen - 1; i++) {
        c = fgetc(fp);
        if (c == EOF || c == '\0') {
            buf[i] = '\0';
            return i;
        }
        buf[i] = c;
    }
    buf[i] = '\0';
    /* skip rest of string if truncated */
    while ((c = fgetc(fp)) != EOF && c != '\0')
        ;
    return i;
}

#ifdef DO_HITECH

/*
 * read Hi-Tech object file
 */
void
read_ht_object(name)
char *name;
{
    struct object *obj;
    FILE *fp;
    unsigned char hdr[3];
    int reclen, rectype;
    long off, size;
    unsigned char *textbuf = NULL;
    unsigned char *databuf = NULL;
    unsigned short textsize = 0, textalloc = 0;
    unsigned short datasize = 0, dataalloc = 0;
    unsigned short bsssize = 0;
    struct ht_reloc *htrelocs = NULL;
    int htreloc_alloc = 0, nhtrelocs = 0;
    unsigned char cur_seg = SEG_TEXT;
    unsigned short cur_off = 0;

    fp = fopen(name, "rb");
    if (fp == NULL)
        error2("cannot open", name);

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    obj = (struct object *)xalloc(sizeof(struct object));
    memset(obj, 0, sizeof(struct object));
    obj->name = name;
    obj->path = name;
    obj->fp = fp;
    obj->is_hitech = 1;

    if (Vflag)
        printf("%s (HiTech)\n", name);

    /* add to object list */
    if (!objects) {
        objects = objects_tail = obj;
    } else {
        objects_tail->next = obj;
        objects_tail = obj;
    }

    /* parse records */
    off = 0;
    while (off < size - 3) {
        fseek(fp, (long)(off), SEEK_SET);
        if (fread(hdr, 1, 3, fp) != 3)
            break;

        reclen = hdr[0] | (hdr[1] << 8);
        rectype = hdr[2];
        off += 3;

        if (off + reclen > size)
            break;

        switch (rectype) {
        case HT_IDENT:
        case HT_PSECT:
        case HT_START:
            /* skip - not needed for linking */
            break;

        case HT_TEXT:
            if (reclen >= 5) {
                /* TEXT: 4-byte offset + psect name + data */
                unsigned long toff;
                char psect[64];
                int plen, dlen;
                unsigned char seg;
                unsigned short endoff;

                toff = read_word(fp);
                toff |= (unsigned long)read_word(fp) << 16;
                plen = read_str(fp, psect, 64);

                dlen = reclen - 4 - plen - 1;
                seg = map_psect(psect);
                cur_seg = seg;
                cur_off = (unsigned short)toff;

                /* collect data into appropriate buffer */
                if (seg == SEG_TEXT && dlen > 0) {
                    endoff = cur_off + dlen;
                    if (endoff > textsize) textsize = endoff;
                    if (endoff > textalloc) {
                        textalloc = endoff + 256;
                        textbuf = realloc(textbuf, textalloc);
                    }
                    fread(textbuf + cur_off, 1, dlen, fp);
                } else if (seg == SEG_DATA && dlen > 0) {
                    endoff = cur_off + dlen;
                    if (endoff > datasize) datasize = endoff;
                    if (endoff > dataalloc) {
                        dataalloc = endoff + 256;
                        databuf = realloc(databuf, dataalloc);
                    }
                    fread(databuf + cur_off, 1, dlen, fp);
                } else if (seg == SEG_BSS) {
                    endoff = cur_off + dlen;
                    if (endoff > bsssize) bsssize = endoff;
                }
            }
            break;

        case HT_RELOC:
            /* RELOC records apply to most recent TEXT segment */
            fseek(fp, (long)(off), SEEK_SET);
            {
                long rend = off + reclen;
                while (ftell(fp) < rend) {
                    unsigned short reloff;
                    unsigned char reltype;
                    char target[64];

                    reloff = read_word(fp);
                    reltype = read_byte(fp);
                    read_str(fp, target, 64);

                    /* grow reloc array if needed */
                    if (nhtrelocs >= htreloc_alloc) {
                        htreloc_alloc = htreloc_alloc ? htreloc_alloc * 2 : 32;
                        htrelocs = realloc(htrelocs,
                            htreloc_alloc * sizeof(struct ht_reloc));
                    }

                    /* record relocation */
                    htrelocs[nhtrelocs].offset = cur_off + reloff;
                    htrelocs[nhtrelocs].type = reltype;
                    htrelocs[nhtrelocs].seg = cur_seg;
                    strncpy(htrelocs[nhtrelocs].target, target, 15);
                    htrelocs[nhtrelocs].target[15] = '\0';
                    nhtrelocs++;
                }
            }
            break;

        case HT_SYMBOL:
            /* parse symbol record */
            fseek(fp, (long)(off), SEEK_SET);
            {
                long send = off + reclen;
                while (ftell(fp) < send) {
                    unsigned long val;
                    unsigned short flags;
                    char psect[64], symname[64];
                    unsigned char seg, type;

                    val = read_word(fp);
                    val |= (unsigned long)read_word(fp) << 16;
                    flags = read_word(fp);
                    read_str(fp, psect, 64);
                    read_str(fp, symname, 64);

                    if (symname[0] == '\0')
                        continue;

                    /* overflow check for 32->16 bit */
                    if (val > 0xFFFF && verbose) {
                        fprintf(stderr, "ld: warning: %s value truncated\n",
                                symname);
                    }

                    /* determine segment */
                    if ((flags & 0x0f) == 6) {
                        seg = SEG_EXT;  /* undefined/external */
                    } else if (flags & HT_F_ABS) {
                        seg = SEG_ABS;
                    } else {
                        seg = map_psect(psect);
                    }

                    /* construct type byte for WS compatibility */
                    type = 0;
                    if (flags & HT_F_GLOBAL)
                        type |= 0x08;  /* global flag */
                    switch (seg) {
                    case SEG_ABS:  type |= 4; break;
                    case SEG_TEXT: type |= 5; break;
                    case SEG_DATA: type |= 6; break;
                    case SEG_BSS:  type |= 7; break;
                    }

                    /* register symbol (only globals for now) */
                    if (flags & HT_F_GLOBAL) {
                        sym_define(symname, (unsigned short)val, seg, type, obj);
                    }

                    if (verbose > 1) {
                        printf("  %s: val=0x%04x seg=%s%s\n",
                               symname, (unsigned short)val,
                               wsSegNames[seg],
                               (flags & HT_F_GLOBAL) ? " global" : "");
                    }
                }
            }
            break;

        case HT_END:
            goto done_parsing;
        }

        off += reclen;
    }

done_parsing:
    /* store collected data in object */
    obj->ht_text = textbuf;
    obj->ht_data = databuf;
    obj->text_size = textsize;
    obj->data_size = datasize;
    obj->bss_size = bsssize;
    obj->ht_relocs = htrelocs;
    obj->ht_nrelocs = nhtrelocs;

    /* close file - data is now in memory buffers */
    fclose(fp);
    obj->fp = NULL;

    if (verbose) {
        printf("%s: text=%d data=%d bss=%d relocs=%d\n",
               name, textsize, datasize, bsssize, nhtrelocs);
    }
}

/*
 * read Hi-Tech object from memory buffer (library member)
 */
void
read_ht_ar_obj(arname, fp, base, msize, membername)
char *arname;
FILE *fp;
long base;
long msize;
char *membername;
{
    struct object *obj;
    unsigned char hdr[3];
    int reclen, rectype;
    long off, endpos;
    unsigned char *textbuf = NULL;
    unsigned char *databuf = NULL;
    unsigned short textsize = 0, textalloc = 0;
    unsigned short datasize = 0, dataalloc = 0;
    unsigned short bsssize = 0;
    struct ht_reloc *htrelocs = NULL;
    int htreloc_alloc = 0, nhtrelocs = 0;
    unsigned char cur_seg = SEG_TEXT;
    unsigned short cur_off = 0;
    char *fullname;

    /* create "archive(member)" name */
    fullname = xalloc(strlen(arname) + strlen(membername) + 3);
    sprintf(fullname, "%s(%s)", arname, membername);

    obj = (struct object *)xalloc(sizeof(struct object));
    memset(obj, 0, sizeof(struct object));
    obj->name = fullname;
    obj->path = arname;
    obj->fp = fp;
    obj->file_base = base;
    obj->is_hitech = 1;

    if (Vflag)
        printf("%s (HiTech)\n", fullname);

    /* add to object list */
    if (!objects) {
        objects = objects_tail = obj;
    } else {
        objects_tail->next = obj;
        objects_tail = obj;
    }

    endpos = base + msize;
    off = base;

    /* parse records */
    while (off < endpos - 3) {
        fseek(fp, (long)(off), SEEK_SET);
        if (fread(hdr, 1, 3, fp) != 3)
            break;

        reclen = hdr[0] | (hdr[1] << 8);
        rectype = hdr[2];
        off += 3;

        if (off + reclen > endpos)
            break;

        switch (rectype) {
        case HT_IDENT:
        case HT_PSECT:
        case HT_START:
            break;

        case HT_TEXT:
            if (reclen >= 5) {
                unsigned long toff;
                char psect[64];
                int plen, dlen;
                unsigned char seg;
                unsigned short endoff;

                toff = read_word(fp);
                toff |= (unsigned long)read_word(fp) << 16;
                plen = read_str(fp, psect, 64);

                dlen = reclen - 4 - plen - 1;
                seg = map_psect(psect);
                cur_seg = seg;
                cur_off = (unsigned short)toff;

                if (seg == SEG_TEXT && dlen > 0) {
                    endoff = cur_off + dlen;
                    if (endoff > textsize) textsize = endoff;
                    if (endoff > textalloc) {
                        textalloc = endoff + 256;
                        textbuf = realloc(textbuf, textalloc);
                    }
                    fread(textbuf + cur_off, 1, dlen, fp);
                } else if (seg == SEG_DATA && dlen > 0) {
                    endoff = cur_off + dlen;
                    if (endoff > datasize) datasize = endoff;
                    if (endoff > dataalloc) {
                        dataalloc = endoff + 256;
                        databuf = realloc(databuf, dataalloc);
                    }
                    fread(databuf + cur_off, 1, dlen, fp);
                } else if (seg == SEG_BSS) {
                    endoff = cur_off + dlen;
                    if (endoff > bsssize) bsssize = endoff;
                }
            }
            break;

        case HT_RELOC:
            fseek(fp, (long)(off), SEEK_SET);
            {
                long rend = off + reclen;
                while (ftell(fp) < rend) {
                    unsigned short reloff;
                    unsigned char reltype;
                    char target[64];

                    reloff = read_word(fp);
                    reltype = read_byte(fp);
                    read_str(fp, target, 64);

                    if (nhtrelocs >= htreloc_alloc) {
                        htreloc_alloc = htreloc_alloc ? htreloc_alloc * 2 : 32;
                        htrelocs = realloc(htrelocs,
                            htreloc_alloc * sizeof(struct ht_reloc));
                    }

                    htrelocs[nhtrelocs].offset = cur_off + reloff;
                    htrelocs[nhtrelocs].type = reltype;
                    htrelocs[nhtrelocs].seg = cur_seg;
                    strncpy(htrelocs[nhtrelocs].target, target, 15);
                    htrelocs[nhtrelocs].target[15] = '\0';
                    nhtrelocs++;
                }
            }
            break;

        case HT_SYMBOL:
            fseek(fp, (long)(off), SEEK_SET);
            {
                long send = off + reclen;
                while (ftell(fp) < send) {
                    unsigned long val;
                    unsigned short flags;
                    char psect[64], symname[64];
                    unsigned char seg, type;

                    val = read_word(fp);
                    val |= (unsigned long)read_word(fp) << 16;
                    flags = read_word(fp);
                    read_str(fp, psect, 64);
                    read_str(fp, symname, 64);

                    if (symname[0] == '\0')
                        continue;

                    if ((flags & 0x0f) == 6) {
                        seg = SEG_EXT;
                    } else if (flags & HT_F_ABS) {
                        seg = SEG_ABS;
                    } else {
                        seg = map_psect(psect);
                    }

                    type = 0;
                    if (flags & HT_F_GLOBAL)
                        type |= 0x08;
                    switch (seg) {
                    case SEG_ABS:  type |= 4; break;
                    case SEG_TEXT: type |= 5; break;
                    case SEG_DATA: type |= 6; break;
                    case SEG_BSS:  type |= 7; break;
                    }

                    if (flags & HT_F_GLOBAL) {
                        sym_define(symname, (unsigned short)val, seg, type, obj);
                    }
                }
            }
            break;

        case HT_END:
            goto done_ar_parsing;
        }

        off += reclen;
    }

done_ar_parsing:
    obj->ht_text = textbuf;
    obj->ht_data = databuf;
    obj->text_size = textsize;
    obj->data_size = datasize;
    obj->bss_size = bsssize;
    obj->ht_relocs = htrelocs;
    obj->ht_nrelocs = nhtrelocs;

    /* don't store fp - library manages it, data is in memory */
    obj->fp = NULL;

    if (verbose) {
        printf("%s: text=%d data=%d bss=%d relocs=%d\n",
               fullname, textsize, datasize, bsssize, nhtrelocs);
    }
}

/*
 * check if Hi-Tech library module defines any currently undefined symbol
 * reads from symbol directory in library header
 */
int
ht_ar_needed(fp, symoff, symcnt)
FILE *fp;
long symoff;
int symcnt;
{
    int i;
    unsigned char symflag;
    char symname[64];

    fseek(fp, (long)(symoff), SEEK_SET);

    for (i = 0; i < symcnt; i++) {
        symflag = fgetc(fp);

        /* read symbol name */
        read_str(fp, symname, 64);

        /* check if this is a defined symbol that we need */
        if (symflag == HT_SYM_DEF || symflag == HT_SYM_COMMON) {
            if (is_undefined(symname)) {
                if (verbose) {
                    printf("  %s satisfies undefined\n", symname);
                }
                return 1;
            }
        }
    }
    return 0;
}

/*
 * process Hi-Tech library file
 * returns number of modules included
 */
int
read_ht_ar(name)
char *name;
{
    FILE *fp;
    unsigned char buf[16];
    unsigned short sym_size, num_mods;
    long symoff, modoff;
    int i, count = 0;

    fp = fopen(name, "rb");
    if (fp == NULL)
        error2("cannot open", name);

    /* read library header */
    if (fread(buf, 1, 4, fp) != 4)
        error2("read error", name);

    sym_size = buf[0] | (buf[1] << 8);
    num_mods = buf[2] | (buf[3] << 8);

    if (verbose) {
        printf("scanning HiTech library %s: %d modules\n", name, num_mods);
    }

    modoff = 4 + sym_size;  /* module data starts after symbol directory */
    symoff = 4;             /* symbol directory starts after header */

    /* scan each module */
    for (i = 0; i < num_mods; i++) {
#ifdef notdef
        unsigned short symSize;
#endif
		unsigned short symCnt;
        unsigned long moduleSize;
        char moduleName[256];
        long mod_symoff;
        int j;

        fseek(fp, (long)(symoff), SEEK_SET);
        if (fread(buf, 1, 12, fp) != 12)
            break;

#ifdef notdef
        symSize = buf[0] | (buf[1] << 8);
#endif
        symCnt = buf[2] | (buf[3] << 8);
        moduleSize = buf[4] | (buf[5] << 8) |
                     ((unsigned long)buf[6] << 16) |
                     ((unsigned long)buf[7] << 24);
        symoff += 12;

        /* read module name */
        read_str(fp, moduleName, 256);
        symoff += strlen(moduleName) + 1;

        mod_symoff = symoff;  /* remember symbol entries offset */

        /* check if this module is needed */
        if (ht_ar_needed(fp, mod_symoff, symCnt)) {
            if (verbose) {
                printf("including %s(%s)\n", name, moduleName);
            }
            read_ht_ar_obj(name, fp, modoff, moduleSize, moduleName);
            count++;
        }

        /* skip past symbol entries */
        fseek(fp, (long)(mod_symoff), SEEK_SET);
        for (j = 0; j < symCnt; j++) {
            fgetc(fp);  /* flags */
            while (fgetc(fp) != '\0')  /* name */
                ;
        }
        symoff = ftell(fp);

        modoff += moduleSize;
    }

    /* always close - objects have data in memory buffers */
    fclose(fp);

    return count;
}

#endif /* DO_HITECH */

/*
 * Pass 1: assign segment addresses to each object
 */

/*
 * How much of this object's bss was taken away below a given offset.
 * The offset is as it was in the object; the ranges are in order.
 */
unsigned short
bss_shift(obj, off)
struct object *obj;
unsigned short off;
{
    struct bssexc *e;
    unsigned short n = 0;

    for (e = obj->exc; e; e = e->next)
        if (off >= e->off + e->len)
            n += e->len;
    return n;
}

/*
 * MERGE THE UNINITIALISED GLOBALS.
 *
 * A header of this vintage writes "struct user u;" and every file
 * that includes it defines it, so forty objects each carry their own
 * u.  They are one object, and the C of the day expected the linker
 * to say so: the first definition wins the address and the rest are
 * that same storage under that same name.
 *
 * Merging the addresses alone is not enough - the image would still
 * carry thirty-nine copies of everything the headers declare.  The
 * bytes come back too: each later object has the range cut out of its
 * bss and everything above it moves down, which is what exc records
 * and bss_shift answers questions about.  Two objects of 1K, sharing
 * 500 bytes of one name, come to 1500 and not 2000.
 *
 * Sizes have to agree.  Two objects that disagree about how big u is
 * were compiled against different headers, and merging them would lay
 * one file's idea of the layout over another's.  That is said out
 * loud rather than resolved.
 *
 * A symbol's size is the distance to whatever comes next, which is
 * why asz names every byte of bss: .ebss closes the last one, and the
 * local symbol of a static stops an unreferenced one from being
 * folded into the global before it.
 */
void
bss_merge()
{
    struct object *obj;
    struct symbol *s;
    unsigned short *off, *siz;
    struct symbol **sym;
    int i, j, n;
    unsigned short base, shift;
    struct bssexc *e, **tail;
    int phase;
    unsigned short maxsyms;

    /*
     * ONE SET OF SCRATCH BUFFERS FOR THE WHOLE FUNCTION, sized to the
     * largest object.  These used to be allocated inside the loop, so
     * once per object per phase - 126 allocations linking cpp - out of
     * an allocator that never gives anything back.  They held 12,612
     * bytes by the end, more than the symbols and more than twice the
     * per-object index maps, and every one of them was dead the moment
     * its object's iteration finished.
     *
     * They are working state for sorting one object's bss symbols by
     * offset.  Nothing in them outlives the loop body, so nothing has
     * to be kept: the buffers are reused, and only the largest object
     * decides how big they are.
     */
    maxsyms = 0;
    for (obj = objects; obj; obj = obj->next) {
        if (obj->num_syms > maxsyms)
            maxsyms = obj->num_syms;
    }
    if (maxsyms == 0)
        return;

    off = (unsigned short *)xalloc(maxsyms * sizeof(*off));
    siz = (unsigned short *)xalloc(maxsyms * sizeof(*siz));
    sym = (struct symbol **)xalloc(maxsyms * sizeof(*sym));

    /*
     * Two passes over the objects.  The first records, for every bss
     * symbol, how big the object that DEFINES it says it is; the
     * second merges the duplicates and checks them against that.
     *
     * They cannot be one pass.  The object that defines a symbol is
     * not necessarily the first one the link reaches - norm.o defines
     * szq and cfold.o only refers to it, but cfold.o is read first -
     * so a single pass compares a duplicate against a size nobody has
     * filled in yet, and calls a 2888-byte array a disagreement with
     * zero.
     */
    for (phase = 0; phase < 2; phase++)
    for (obj = objects; obj; obj = obj->next) {
        if (obj->num_syms == 0)
            continue;
        base = obj->text_size + obj->data_size;   /* asz biases by this */

        n = 0;
        for (i = 0; i < obj->num_syms; i++) {
            s = obj->symtab[i];
            if (!s || obj->symseg[i] != SEG_BSS)
                continue;
            off[n] = obj->symval[i] - base;
            sym[n] = s;
            n++;
        }
        for (i = 1; i < n; i++)                   /* by offset */
            for (j = i; j > 0 && off[j] < off[j - 1]; j--) {
                unsigned short t = off[j]; off[j] = off[j-1]; off[j-1] = t;
                s = sym[j]; sym[j] = sym[j-1]; sym[j-1] = s;
            }
        for (i = 0; i < n; i++)
            siz[i] = (i + 1 < n) ? off[i + 1] - off[i]
                                 : obj->bss_size - off[i];

        tail = &obj->exc;
        shift = 0;
        for (i = 0; i < n; i++) {
            s = sym[i];
            if (!(s->type & 0x08) || s->obj == obj) {
                /* ours: it says how big the thing is, and on the
                 * second pass it moves down by what went before it */
                if (phase == 0) {
                    if (s->type & 0x08)
                        s->size = siz[i];
                } else {
                    s->value -= shift;
                }
                continue;
            }
            if (phase == 0)
                continue;
            /*
             * No storage of its own here, so there is nothing to take
             * back and nothing to disagree about - two symbols at one
             * offset, which is what a zero-length delta means.  The
             * address still merges; that happened in sym_define.
             */
            if (siz[i] == 0)
                continue;
            if (s->size != siz[i]) {
                fprintf(stderr,
                    "ld: %s is %u bytes in %s but %u in %s\n",
                    s->name, s->size, s->obj ? s->obj->name : "?",
                    siz[i], obj->name);
                exit(1);
            }
            e = (struct bssexc *)xalloc(sizeof(*e));
            e->off = off[i];
            e->len = siz[i];
            e->next = 0;
            *tail = e;
            tail = &e->next;
            shift += siz[i];
        }
        if (phase == 1) {
            if (shift && verbose)
                printf("%s: %u bytes of bss merged away\n",
                       obj->name, shift);
            obj->bss_size -= shift;
        }
    }
}

void
pass1_layout()
{
    struct object *obj;
    struct symbol *s;

    bss_merge();

    /* assign segment offsets to each object */
    for (obj = objects; obj; obj = obj->next) {
        obj->text_off = text_pos;
        obj->data_off = data_pos;
        obj->bss_off = bss_pos;

        text_pos += obj->text_size;
        data_pos += obj->data_size;
        bss_pos += obj->bss_size;

        if (verbose) {
            printf("%s: text@0x%04x data@0x%04x bss@0x%04x\n",
                   obj->name, obj->text_off, obj->data_off, obj->bss_off);
        }
    }

    total_text = text_pos;
    total_data = data_pos;
    total_bss = bss_pos;

    /*
     * data and bss bases are absolute addresses, like text_base.  When
     * neither was given, data follows the text and bss follows the data -
     * the ordinary layout.  An explicit -Tdata/-Tbss is used as-is, which
     * is how upm loads its CCP at 0x0100 below the text.
     */
    if (!data_set)
        data_base = text_base + total_text;
    if (!bss_set)
        bss_base = data_base + total_data;

    /* find linker-defined symbols and save original offsets BEFORE resolution */
    {
        int i;
        for (i = 0; i < LSYM_COUNT; i++) {
            s = sym_lookup(lnksyms[i].name);
            if (s && s->seg == SEG_DATA && s->obj) {
                lnksyms[i].obj = s->obj;
                /* original value is text_size + data_offset */
                lnksyms[i].off = s->value - s->obj->text_size;
            }
        }
    }

    /* now resolve all symbol addresses */
    {
    int undef_count = 0;
    for (s = symbols; s; s = s->next) {
        if (s->seg == SEG_EXT) {
            fprintf(stderr, "ld: undefined symbol: %s\n", s->name);
            undef_count++;
            continue;
        }
        if (s->seg == SEG_ABS)
            continue;

        /* adjust value based on object's segment offset */
        if (s->obj) {
            switch (s->seg) {
            case SEG_TEXT:
                s->value = text_base + s->obj->text_off + s->value;
                break;
            case SEG_DATA:
                s->value = data_base + s->obj->data_off +
                           (s->value - s->obj->text_size);
                break;
            case SEG_BSS:
                s->value = bss_base + s->obj->bss_off +
                           (s->value - s->obj->text_size - s->obj->data_size);
                break;
            }
        }

        if (verbose > 1) {
            printf("resolved %s = 0x%04x\n", s->name, s->value);
        }
    }
    if (undef_count && !rflag)
        exit(1);
    }

    /* set linker symbol values and change to absolute */
    {
        unsigned short vals[LSYM_COUNT];
        int i;

        vals[LSYM_LTEXT] = text_base;
        vals[LSYM_HTEXT] = text_base + total_text;
        vals[LSYM_LDATA] = data_base;
        vals[LSYM_HDATA] = data_base + total_data;
        vals[LSYM_LBSS]  = bss_base;
        vals[LSYM_HBSS]  = bss_base + total_bss;

        for (i = 0; i < LSYM_COUNT; i++) {
            s = sym_lookup(lnksyms[i].name);
            if (s) {
                s->value = vals[i];
                s->seg = SEG_ABS;
                if (verbose)
                    printf("%s = 0x%04x\n", lnksyms[i].name, vals[i]);
            }
        }
    }
}

/*
 * get relocation target value for a symbol index
 */
unsigned short
getRelocVal(obj, ctrl)
struct object *obj;
int ctrl;
{
    struct symbol *s;
    int idx;

    /* decode control byte to get segment or symbol */
    switch (ctrl) {
    case 0x40:  /* absolute - no change */
        return 0;
    case 0x44:  /* text segment */
        return text_base + obj->text_off;
    case 0x48:  /* data segment */
        return data_base + obj->data_off;
    case 0x4c:  /* bss segment */
        return bss_base + obj->bss_off;
    default:
        /* symbol reference */
        if (ctrl >= 0x50 && ctrl < 0xfc) {
            idx = ((ctrl - 0x50) >> 2);
        } else {
            /* extended encoding - not handled yet */
            error("extended reloc encoding not supported");
            return 0;
        }
        if (idx >= obj->num_syms) {
            error("reloc symbol index out of range");
        }
        s = obj->symtab[idx];
        return s->value;
    }
}

/*
 * add a pending relocation for -r output
 */
void
add_outreloc(list, tail, offset, sym, seg, hilo)
struct outreloc **list;
struct outreloc **tail;
unsigned short offset;
struct symbol *sym;
unsigned char seg;
unsigned char hilo;
{
    struct outreloc *r = (struct outreloc *)xalloc(sizeof(struct outreloc));
    r->offset = offset;
    r->sym = sym;
    r->seg = seg;
    r->hilo = hilo;
    r->next = NULL;
    if (*tail)
        (*tail)->next = r;
    else
        *list = r;
    *tail = r;
}

/*
 * find symbol index in output symbol table
 */
int
findSymIdx(sym)
struct symbol *sym;
{
    struct symbol *s;
    int idx = 0;
    for (s = symbols; s; s = s->next, idx++) {
        if (s == sym)
            return idx;
    }
    return -1;
}

/*
 * write a relocation table using shared wsobj functions
 */
void
write_relocs(rlist)
struct outreloc *rlist;
{
    struct outreloc *r;
    int last = 0;
    int bump;

    for (r = rlist; r; r = r->next) {
        bump = r->offset - last;
        wsEncBump(outfp, bump);

        if (r->sym) {
            /* symbol reference */
            int idx = findSymIdx(r->sym);
            wsEncReloc(outfp, -1, idx, r->hilo);
        } else {
            /* segment reference */
            wsEncReloc(outfp, r->seg, 0, r->hilo);
        }
        last = r->offset + (r->hilo ? 1 : 2);
    }
    wsEndReloc(outfp);
}

/*
 * apply relocations to segment data
 * reloc_off: file offset of relocation table
 * buf: segment data buffer
 * seg_size: size of segment
 * seg_base: base address adjustment for this object's segment
 * is_text: 1 for text segment, 0 for data segment (for -r reloc collection)
 */
/*
 * The segment being relocated, reached one byte at a time.
 *
 * There are two ways to hold it.  If the whole segment fits in memory
 * it is read in, patched, and written once - one read and one write,
 * which is what a link on a machine with room should cost.  If it does
 * not fit, the raw bytes are copied to the output first and patched
 * where they lie, through a window that is written back when it moves.
 *
 * Both look the same to apply_relocs, which is the point: the
 * relocation logic is delicate and there is only one copy of it.  The
 * memory case is simply a window as big as the segment, and the
 * fallback is a write-back cache that spills.
 *
 * Relocation positions only ever move forward - the stream is a series
 * of bumps along the segment - so one window is enough and it is never
 * asked to go back.
 */
#define WINSZ 512

static unsigned char *segbuf;       /* whole segment, or 0 when spilled */
static unsigned char winbuf[WINSZ];
static FILE *winfp;                 /* what the window is a window onto */
static long winbase = -1;           /* file offset of winbuf[0] */
static long winlo, winhi;           /* the segment, so a short window */
static int winlen, windirty;        /* cannot write past the end of it */

static void
winflush()
{
    if (winbase >= 0 && windirty) {
        if (fseek(winfp, winbase, SEEK_SET) != 0)
            error("seek error");
        if (fwrite(winbuf, 1, winlen, winfp) != winlen)
            error("write error");
    }
    windirty = 0;
}

static void
winload(off)
long off;
{
    long start;

    if (winbase >= 0 && off >= winbase && off < winbase + winlen)
        return;
    winflush();
    start = off - ((off - winlo) % WINSZ);
    winlen = WINSZ;
    if (start + winlen > winhi)
        winlen = (int)(winhi - start);
    if (fseek(winfp, start, SEEK_SET) != 0)
        error("seek error");
    if (fread(winbuf, 1, winlen, winfp) != winlen)
        error("read error");
    winbase = start;
}

static unsigned char
bget(pos)
int pos;
{
    if (segbuf)
        return segbuf[pos];
    winload(winlo + pos);
    return winbuf[winlo + pos - winbase];
}

static void
bput(pos, v)
int pos;
unsigned char v;
{
    if (segbuf) {
        segbuf[pos] = v;
        return;
    }
    winload(winlo + pos);
    winbuf[winlo + pos - winbase] = v;
    windirty = 1;
}

void
apply_relocs(obj, reloc_off, seg_size, seg_base, is_text)
struct object *obj;
long reloc_off;
int seg_size;
unsigned short seg_base;
int is_text;
{
    int pos = 0;
    unsigned char b;
    int bump, idx;
    unsigned short val, add;
    struct symbol *s;
    int need_reloc;     /* for -r: does this reloc need to be preserved? */
    int bssrel;         /* addend is a bss offset, so merging moves it */
    unsigned char outseg;
    int hilo;           /* 0=word, 1=lo, 2=hi */
    int size;           /* relocation size: 2 for word, 1 for lo/hi */

    fseek(obj->fp, (long)(reloc_off), SEEK_SET);

    while (1) {
        b = read_byte(obj->fp);
        if (b == 0) break;  /* end of relocs */

        /* decode bump */
        if (b < 32) {
            bump = b;
        } else if (b < 64) {
            bump = ((b - 32) << 8) + read_byte(obj->fp) + 32;
        } else {
            /* control byte - determine relocation type */
            add = 0;
            need_reloc = 0;
            bssrel = 0;
            s = NULL;
            outseg = 0;
            hilo = 0;

            /* segment relocations: 0x40-0x4f range */
            if (b >= 0x40 && b < 0x50) {
                hilo = b & 3;
                switch (b & ~3) {
                case 0x40:  /* absolute - no adjustment */
                    add = 0;
                    break;
                case 0x44:  /* text segment */
                    add = text_base + obj->text_off;
                    if (rflag) {
                        need_reloc = 1;
                        outseg = SEG_TEXT;
                    }
                    break;
                case 0x48:  /* data segment */
                    add = data_base + obj->data_off;
                    if (rflag) {
                        need_reloc = 1;
                        outseg = SEG_DATA;
                    }
                    break;
                case 0x4c:  /* bss segment */
                    add = bss_base + obj->bss_off;
                      bssrel = 1;
                    if (rflag) {
                        need_reloc = 1;
                        outseg = SEG_BSS;
                    }
                    break;
                }
            } else if (b >= 0x50 && b < 0xfc) {
                /* symbol reference - low 2 bits are hilo */
                hilo = b & 3;
                idx = (b - 0x50) >> 2;
                if (idx < obj->num_syms) {
                    s = obj->symtab[idx];
                    if (hilo && s->obj == obj) {
                        /* hi/lo on symbol in same object: use segment base */
                        if (s->seg == SEG_TEXT)
                            add = text_base + obj->text_off;
                        else if (s->seg == SEG_DATA)
                            add = data_base + obj->data_off - obj->text_size;
                        else if (s->seg == SEG_BSS)
                            add = bss_base + obj->bss_off
                                  - obj->text_size - obj->data_size;
                        else
                            add = s->value;
                        /* in -r mode, preserve as symbol relocation */
                        if (rflag && hilo)
                            need_reloc = 1;
                    } else {
                        /* word reloc or external symbol: use full value */
                        add = s->value;
                        /* in -r mode, preserve symbol hi/lo relocations */
                        if (rflag && (s->seg == SEG_EXT || hilo))
                            need_reloc = 1;
                    }
                }
            } else if ((b & ~3) == 0xfc) {
                /* extended symbol encoding - low 2 bits are hilo */
                hilo = b & 3;
                b = read_byte(obj->fp);
                if (b < 0x80) {
                    idx = b + 47 - 4;
                } else {
                    idx = ((b - 0x80) << 8) + read_byte(obj->fp) + 175 - 4;
                }
                if (idx < obj->num_syms) {
                    s = obj->symtab[idx];
                    if (hilo && s->obj == obj) {
                        /* hi/lo on symbol in same object: use segment base */
                        if (s->seg == SEG_TEXT)
                            add = text_base + obj->text_off;
                        else if (s->seg == SEG_DATA)
                            add = data_base + obj->data_off - obj->text_size;
                        else if (s->seg == SEG_BSS)
                            add = bss_base + obj->bss_off
                                  - obj->text_size - obj->data_size;
                        else
                            add = s->value;
                        /* in -r mode, preserve as symbol relocation */
                        if (rflag && hilo)
                            need_reloc = 1;
                    } else {
                        /* word reloc or external symbol: use full value */
                        add = s->value;
                        /* in -r mode, preserve symbol hi/lo relocations */
                        if (rflag && (s->seg == SEG_EXT || hilo))
                            need_reloc = 1;
                    }
                }
            }

            /*
             * A byte half of a bss address cannot be shifted: the
             * offset is split across two relocations and neither
             * half is the number that needs moving.  Nothing emits
             * one - the compiler addresses bss with word
             * relocations - so this is a refusal, not a gap.
             */
            if (bssrel && hilo) {
                fprintf(stderr,
                    "ld: %s: byte relocation on a bss offset\n",
                    obj->name);
                exit(1);
            }

            /* determine relocation size */
            size = (hilo == 0) ? 2 : 1;

            /* apply relocation at current position */
            if (pos < seg_size) {
                if (hilo == 0) {
                    /* word relocation */
                    if (pos + 1 < seg_size) {
                        val = bget(pos) | (bget(pos + 1) << 8);
                        if (bssrel)
                            val -= bss_shift(obj, (unsigned short)val);
                        val += add;
                        bput(pos, val & 0xff);
                        bput(pos + 1, val >> 8);
                    }
                } else if (hilo == 1) {
                    /* lo byte relocation */
                    val = bget(pos) + (add & 0xff);
                    bput(pos, val & 0xff);
                } else {
                    /* hi byte relocation */
                    val = bget(pos) + (add >> 8);
                    bput(pos, val & 0xff);
                }

                if (verbose > 2) {
                    printf("    reloc @%04x += %04x -> %02x (%s)\n",
                           seg_base + pos, add,
                           hilo ? bget(pos) : (bget(pos) | (bget(pos+1)<<8)),
                           hilo == 0 ? "word" : hilo == 1 ? "lo" : "hi");
                }
            }

            /* collect reloc for -r output */
            if (need_reloc) {
                if (is_text)
                    add_outreloc(&text_relocs, &textRelocTl,
                                 seg_base + pos, s, outseg, hilo);
                else
                    add_outreloc(&data_relocs, &dataRelocTl,
                                 seg_base + pos, s, outseg, hilo);
            }

            pos += size;
            continue;
        }

        pos += bump;
    }
}

#ifdef DO_HITECH
/*
 * apply Hi-Tech relocations to segment data
 */
void
apply_htrel(obj, buf, seg, seg_size, seg_base)
struct object *obj;
unsigned char *buf;
unsigned char seg;           /* SEG_TEXT or SEG_DATA */
unsigned short seg_size;
unsigned short seg_base;
{
    int i;
    struct symbol *s;
    unsigned short target_val;
    unsigned short pos;
    int rsize;

    for (i = 0; i < obj->ht_nrelocs; i++) {
        struct ht_reloc *r = &obj->ht_relocs[i];

        if (r->seg != seg)
            continue;

        pos = r->offset;
        if (pos >= seg_size)
            continue;

        rsize = r->type & HT_RSIZE_MASK;

        /* get target value */
        if (r->type & HT_RPSECT) {
            /* psect-relative: target is segment name */
            unsigned char tseg = map_psect(r->target);
            switch (tseg) {
            case SEG_TEXT:
                target_val = text_base + obj->text_off;
                break;
            case SEG_DATA:
                target_val = data_base + obj->data_off;
                break;
            case SEG_BSS:
                target_val = bss_base + obj->bss_off;
                break;
            default:
                target_val = 0;
            }
        } else {
            /* symbol reference: lookup target name */
            s = sym_lookup(r->target);
            if (s == NULL) {
                fprintf(stderr, "ld: undefined symbol: %s\n", r->target);
                target_val = 0;
            } else {
                target_val = s->value;
            }
        }

        /* apply relocation */
        if (rsize == HT_RWORD && pos + 1 < seg_size) {
            unsigned short val = buf[pos] | (buf[pos+1] << 8);
            val += target_val;
            buf[pos] = val & 0xff;
            buf[pos+1] = val >> 8;
        } else if (rsize == HT_RBYTE) {
            buf[pos] += target_val & 0xff;
        }

        if (verbose > 2) {
            printf("  ht_reloc @%04x += %04x (%s)\n",
                   seg_base + pos, target_val, r->target);
        }
    }
}
#endif

/*
 * patch linker-defined symbols in data segment
 * these are symbols like __Lbss/__Hbss that need their values written
 * to the data location where they are defined
 */
void
patch_lnksyms(obj, seg_size)
struct object *obj;
int seg_size;
{
    unsigned short vals[LSYM_COUNT];
    int i;

    vals[LSYM_LTEXT] = text_base;
    vals[LSYM_HTEXT] = text_base + total_text;
    vals[LSYM_LDATA] = data_base;
    vals[LSYM_HDATA] = data_base + total_data;
    vals[LSYM_LBSS]  = bss_base;
    vals[LSYM_HBSS]  = bss_base + total_bss;

    for (i = 0; i < LSYM_COUNT; i++) {
        if (lnksyms[i].obj == obj && lnksyms[i].off + 1 < seg_size) {
            bput(lnksyms[i].off, vals[i] & 0xff);
            bput(lnksyms[i].off + 1, vals[i] >> 8);
            if (verbose > 1)
                printf("  patched %s at data+0x%04x = 0x%04x\n",
                       lnksyms[i].name, lnksyms[i].off, vals[i]);
        }
    }
}

/*
 * copy segment data with relocations applied
 */
void
copy_segment(obj, seg_start, seg_size, reloc_off, seg_base, is_text, dest)
struct object *obj;
int seg_start;
int seg_size;
long reloc_off;
unsigned short seg_base;
int is_text;
FILE *dest;
{
    static unsigned char cbuf[WINSZ];
    long obase;
    int done, want;

    if (seg_size == 0)
        return;

    /*
     * Ask for the whole segment.  __malloc rather than malloc: coming
     * back empty is not a failure here, it is the answer to "is there
     * room", and the spilled path below is what happens when there is
     * not.
     */
    segbuf = (unsigned char *)__malloc(seg_size);

#ifdef DO_HITECH
    if (obj->is_hitech) {
        if (!segbuf)
            error("out of memory");     /* hi-tech has no spilled path */
        if (is_text) {
            if (obj->ht_text)
                memcpy(segbuf, obj->ht_text, seg_size);
            else
                memset(segbuf, 0, seg_size);
            apply_htrel(obj, segbuf, SEG_TEXT, seg_size, seg_base);
        } else {
            if (obj->ht_data)
                memcpy(segbuf, obj->ht_data, seg_size);
            else
                memset(segbuf, 0, seg_size);
            apply_htrel(obj, segbuf, SEG_DATA, seg_size, seg_base);
            patch_lnksyms(obj, seg_size);
        }
        if (fwrite(segbuf, 1, seg_size, dest) != seg_size)
            error("write error");
        free(segbuf);
        segbuf = 0;
        return;
    }
#endif

    if (fseek(obj->fp, (long)(obj->file_base + seg_start), SEEK_SET) != 0)
        error("seek error");

    if (segbuf) {
        /* it fits: one read, patch it, one write */
        if (fread(segbuf, 1, seg_size, obj->fp) != seg_size)
            error("read error");
        apply_relocs(obj, reloc_off, seg_size, seg_base, is_text);
        if (!is_text)
            patch_lnksyms(obj, seg_size);
        if (fwrite(segbuf, 1, seg_size, dest) != seg_size)
            error("write error");
        free(segbuf);
        segbuf = 0;
        return;
    }

    /*
     * It does not fit.  Copy the segment out as it stands and patch it
     * where it lies, through the window.  Costs a second pass over the
     * bytes and a seek per window, and is what makes a link possible at
     * all when the segment is bigger than the room left.
     */
    obase = ftell(dest);
    for (done = 0; done < seg_size; done += want) {
        want = seg_size - done;
        if (want > WINSZ)
            want = WINSZ;
        if (fread(cbuf, 1, want, obj->fp) != want)
            error("read error");
        if (fwrite(cbuf, 1, want, dest) != want)
            error("write error");
    }

    winfp = dest;
    winlo = obase;
    winhi = obase + seg_size;
    winbase = -1;
    windirty = 0;

    apply_relocs(obj, reloc_off, seg_size, seg_base, is_text);
    if (!is_text)
        patch_lnksyms(obj, seg_size);

    winflush();
    winbase = -1;

    /* the window left it wherever it last wrote; the next segment appends */
    if (fseek(dest, obase + seg_size, SEEK_SET) != 0)
        error("seek error");
}

/*
 * Pass 2: write output file
 */
void
pass2_output()
{
    struct object *obj;
    unsigned char config;
    int symlen;
    FILE *datafp;
    char tmpname[32];
    unsigned char xferbuf[512];
    int n;

    /* default to 15-char symbols */
    symlen = out_symlen ? out_symlen : 15;

    /*
     * CREATE IT WITH THE MODE IT SHOULD HAVE.  What a linker writes is
     * a program, and fopen has no way to say so - it creates under the
     * umask, 0664, and nothing here was putting the execute bit back.
     * creat takes the mode, and fopen below does not disturb the mode
     * of a file that already exists, so this is where it gets decided.
     * There is no fdopen in this libc or the one on micronix, which is
     * why it is two calls and not open() with a mode.
     *
     * It went unnoticed while every install copied ONTO a file that
     * was already executable, because cp keeps the mode of a
     * destination that exists.  The first install to remove the old
     * name first produced a /bin full of programs that would not run,
     * and make(1) found it: it checks access(path, 1) before it will
     * exec, so it walked past everything in /bin and said "could not
     * locate ccc" about a file sitting right there.
     *
     * -r output is not a program.  It is a relocatable object to be
     * linked again, and it gets 0644 like any other .o.
     */
#ifdef linux
    close(creat(outfile, rflag ? 0600 : 0700));	/* host: no group bits */
#else
    close(creat(outfile, rflag ? 0644 : 0755));
#endif

    /*
     * "w+b", not "wb": a segment too big to hold is copied out and
     * then patched where it lies, which means reading back what was
     * just written.
     */
    outfp = fopen(outfile, "w+b");
    if (outfp == NULL)
        error2("cannot create", outfile);

    /* write header */
    write_byte(MAGIC);
    config = CONF_LITTLE | ((symlen - 1) / 2);
    if (!rflag)
        config |= CONF_NORELO;
    write_byte(config);
    write_word(sflag ? 0 : num_globals * (symlen + 3));   /* symtab size */
    write_word(total_text);
    write_word(total_data);
    write_word(total_bss);
    write_word(0);              /* heap */
    write_word(text_base);      /* text offset */
    write_word(data_base);      /* data offset */

    /*
     * One pass over the inputs.
     *
     * The output wants all the text and then all the data, but an
     * object carries both - so walking the objects once per segment
     * kind means opening every input twice, or holding them all open,
     * and _NFILE is twelve.  The data goes to a second stream instead
     * and is appended when the text is done, which costs one temporary
     * file and lets the link hold one input at a time.
     */
    sprintf(tmpname, "/tmp/ld%d", getpid());
    datafp = fopen(tmpname, "w+b");
    if (datafp == NULL)
        error2("cannot create", tmpname);
    unlink(tmpname);            /* it lives only as long as the handle */

    for (obj = objects; obj; obj = obj->next) {
        obj->fp = fopen(obj->path, "rb");
        if (obj->fp == NULL)
            error2("cannot reopen", obj->path);

        copy_segment(obj, 16, obj->text_size,
                     obj->textRelocOff,
                     text_base + obj->text_off, 1, outfp);
        copy_segment(obj, 16 + obj->text_size, obj->data_size,
                     obj->dataRelocOff,
                     data_base + obj->data_off, 0, datafp);

        fclose(obj->fp);
        obj->fp = NULL;
    }

    /* and now the data, onto the end of the text */
    if (fseek(datafp, 0L, SEEK_SET) != 0)
        error("seek error");
    while ((n = fread(xferbuf, 1, sizeof(xferbuf), datafp)) > 0)
        if (fwrite(xferbuf, 1, n, outfp) != n)
            error("write error");
    fclose(datafp);

    /* write symbol table (after text and data) unless stripped */
    if (!sflag) {
        struct symbol *s;
        int i;
        unsigned char type;

        for (s = symbols; s; s = s->next) {
            /* warn if symbol name will be truncated */
            if (strlen(s->name) > symlen) {
                fprintf(stderr, "ld: symbol truncated: %s\n", s->name);
            }
            /* symbol entry: value, type, name */
            write_word(s->value);
            /* encode segment in type byte */
            switch (s->seg) {
            case SEG_ABS:  type = 0x04 | 0x08; break;
            case SEG_TEXT: type = 0x05 | 0x08; break;
            case SEG_DATA: type = 0x06 | 0x08; break;
            case SEG_BSS:  type = 0x07 | 0x08; break;
            default:       type = 0x08; break;
            }
            write_byte(type);
            /*
             * The field in the object file is fixed width and the
             * name is not any more, so the zeroes that used to come
             * free out of a padded array get written here.
             */
            {
                register char *np = s->name;

                for (i = 0; i < symlen; i++)
                    write_byte(*np ? *np++ : 0);
            }
        }
    }

    /* write text relocs */
    if (rflag) {
        write_relocs(text_relocs);
    }

    /* write data relocs */
    if (rflag) {
        write_relocs(data_relocs);
    }

    fclose(outfp);
}

/*
 * print load map to stderr
 */
void
print_map()
{
    struct object *obj;
    struct symbol *s;

    fprintf(stderr, "\nLoad map:\n");
    fprintf(stderr, "  text: 0x%04x - 0x%04x (%d bytes)\n",
            text_base, text_base + total_text - 1, total_text);
    fprintf(stderr, "  data: 0x%04x - 0x%04x (%d bytes)\n",
            data_base,
            data_base + total_data - 1, total_data);
    fprintf(stderr, "  bss:  0x%04x - 0x%04x (%d bytes)\n",
            bss_base,
            bss_base + total_bss - 1, total_bss);

    fprintf(stderr, "\nObjects:\n");
    for (obj = objects; obj; obj = obj->next) {
        fprintf(stderr, "  %-20s text@%04x data@%04x bss@%04x\n",
                obj->name,
                text_base + obj->text_off,
                data_base + obj->data_off,
                bss_base + obj->bss_off);
    }

    fprintf(stderr, "\nSymbols:\n");
    for (s = symbols; s; s = s->next) {
        fprintf(stderr, "  %04x %c %s\n",
                s->value,
                s->seg == SEG_TEXT ? 'T' :
                s->seg == SEG_DATA ? 'D' :
                s->seg == SEG_BSS ? 'B' :
                s->seg == SEG_ABS ? 'A' : 'U',
                s->name);
    }
}

unsigned short
parse_addr(s)
char *s;
{
    unsigned short val = 0;
    int base = 10;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
    } else if (s[0] == '0') {
        base = 8;
    }

    while (*s) {
        val *= base;
        if (*s >= '0' && *s <= '9')
            val += *s - '0';
        else if (base == 16 && *s >= 'a' && *s <= 'f')
            val += *s - 'a' + 10;
        else if (base == 16 && *s >= 'A' && *s <= 'F')
            val += *s - 'A' + 10;
        else
            error2("bad address", s);
        s++;
    }
    return val;
}

#ifdef DO_HITECH

/*
 * check if file is Hi-Tech object format
 * looks for IDENT record: 0x0A 0x00 0x07 (len=10, type=7)
 */
int
is_hitech_obj(name)
char *name;
{
    FILE *fp;
    unsigned char buf[3];
    int result = 0;

    fp = fopen(name, "rb");
    if (fp == NULL)
        return 0;

    if (fread(buf, 1, 3, fp) == 3) {
        result = HT_IS_HITECH(buf);
    }
    fclose(fp);
    return result;
}

/*
 * check if file is Hi-Tech library
 * libraries have: 4-byte header, then module data starting with IDENT record
 */
int
is_hitech_lib(name)
char *name;
{
    FILE *fp;
    unsigned char buf[20];
    unsigned short sym_size, num_mods;
    long size, mod_off;
    int result = 0;

    fp = fopen(name, "rb");
    if (fp == NULL)
        return 0;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    if (fread(buf, 1, 4, fp) == 4) {
        sym_size = buf[0] | (buf[1] << 8);
        num_mods = buf[2] | (buf[3] << 8);
        mod_off = 4 + sym_size;

        /* sanity check and verify first module is HT object */
        if (num_mods > 0 && num_mods < 1000 &&
            mod_off > 4 && mod_off < size) {
            unsigned char ident[3];
            fseek(fp, (long)(mod_off), SEEK_SET);
            if (fread(ident, 1, 3, fp) == 3 && HT_IS_HITECH(ident)) {
                result = 1;
            }
        }
    }
    fclose(fp);
    return result;
}

#endif /* DO_HITECH */

/*
 * check if file is an archive
 * returns: 0 = not archive, 1 = WS archive, 2 = HT library
 */
int
is_archive(name)
char *name;
{
    FILE *fp;
    unsigned char buf[2];
    unsigned short magic16;

    /* check WS archive first (quick magic check) */
    fp = fopen(name, "rb");
    if (fp == NULL)
        return 0;

    if (fread(buf, 1, 2, fp) != 2) {
        fclose(fp);
        return 0;
    }
    fclose(fp);

    magic16 = buf[0] | (buf[1] << 8);
    if (magic16 == AR_MAGIC || magic16 == V7_MAGIC)
        return 1;               /* read_archive tells the two apart */

#ifdef DO_HITECH
    /* check Hi-Tech library */
    if (is_hitech_lib(name))
        return 2;
#endif

    return 0;
}

/*
 * input file list for archive re-processing
 */

void
add_infile(name, is_ar)
char *name;
int is_ar;
{
    struct infile *f = (struct infile *)xalloc(sizeof(struct infile));
    f->name = name;
    f->is_archive = is_ar;
    f->fp = NULL;
    f->next = 0;

    if (!infiles) {
        infiles = infiles_tail = f;
    } else {
        infiles_tail->next = f;
        infiles_tail = f;
    }
}

int
main(argc, argv)
int argc;
char **argv;
{
    int i;
    char *arg;
    int nfiles = 0;
    struct infile *f;
    int added, pass;

    /* first pass: parse options and collect input files */
    for (i = 1; i < argc; i++) {
        arg = argv[i];

        if (arg[0] == '-') {
            switch (arg[1]) {
            case 'v':
                verbose++;
                break;

            case 'V':
                Vflag++;
                break;

            case 'r':
                rflag++;
                break;

            case 's':
                sflag++;
                break;

            case '9':
                out_symlen = 9;
                break;

            case 'o':
                if (arg[2])
                    outfile = &arg[2];
                else if (++i < argc)
                    outfile = argv[i];
                else
                    usage();
                break;

            case 'L':
                /* -L<dir>: add to library search path */
                if (arg[2]) {
                    if (nlibpaths < MAX_LIBPATHS)
                        libpaths[nlibpaths++] = &arg[2];
                } else {
                    usage();
                }
                break;

            case 'l':
                /* -l<lib>: find and add library */
                if (arg[2]) {
                    char *libpath = findlib(&arg[2]);
                    if (libpath == 0)
                        error2("library not found", &arg[2]);
                    add_infile(libpath, is_archive(libpath));
                    nfiles++;
                } else {
                    usage();
                }
                break;

            case 'T':
                /* -Ttext, -Tdata, -Tbss */
                if (strncmp(&arg[2], "text", 4) == 0) {
                    if (arg[6] == '=')
                        text_base = parse_addr(&arg[7]);
                    else if (++i < argc)
                        text_base = parse_addr(argv[i]);
                    else
                        usage();
                } else if (strncmp(&arg[2], "data", 4) == 0) {
                    if (arg[6] == '=')
                        data_base = parse_addr(&arg[7]);
                    else if (++i < argc)
                        data_base = parse_addr(argv[i]);
                    else
                        usage();
                    data_set = 1;
                } else if (strncmp(&arg[2], "bss", 3) == 0) {
                    if (arg[5] == '=')
                        bss_base = parse_addr(&arg[6]);
                    else if (++i < argc)
                        bss_base = parse_addr(argv[i]);
                    else
                        usage();
                    bss_set = 1;
                } else {
                    usage();
                }
                break;

            default:
                usage();
            }
        } else {
            /* input file - check if archive or object */
            add_infile(arg, is_archive(arg));
            nfiles++;
        }
    }

    if (nfiles == 0)
        usage();

    /* process input files, handling archives specially */
    /* first load all .o files unconditionally */
    for (f = infiles; f; f = f->next) {
        if (f->is_archive == 0) {
            /* object file - auto-detect format */
#ifdef DO_HITECH
            if (is_hitech_obj(f->name))
                read_ht_object(f->name);
            else
#endif
                read_object(f->name);
        }
    }

    /*
     * Now the archives.  Each one drains itself - ar_byindex repeats
     * while it keeps taking, and the walk it falls back to does the
     * same - so this loop asks each once and is left only with
     * genuine circularity BETWEEN two archives.
     *
     * That draining is what makes naming a library first mean
     * something.  A member is only taken if something already
     * undefined needs it when the scan arrives, so a single pass
     * misses whatever a member taken late asks of one passed early;
     * an archive that has not finished answering lets the next one
     * answer instead.  With "-lc -lu -lc" naming two different libcs
     * that decided which of them a program got, and it was the wrong
     * one - see the commit that moved the driver's own libraries to
     * the end of the link.
     */
    pass = 0;
    do {
        added = 0;
        for (f = infiles; f; f = f->next) {
            if (f->is_archive == 1) {
                /* Whitesmith or v7 archive */
                added += read_archive(f);
#ifdef DO_HITECH
            } else if (f->is_archive == 2) {
                /* Hi-Tech library */
                added += read_ht_ar(f->name);
#endif
            }
        }
        pass++;
        if (verbose && added) {
            printf("archive pass %d: added %d objects\n", pass, added);
        }
    } while (added > 0 && has_undefined());

    /* Pass 1: assign addresses and resolve symbols */
    pass1_layout();


    /* Pass 2: write output */
    pass2_output();

    /*
     * The map and the symbol list are a diagnostic, not a product.
     * A link of any size buries whatever the user was actually told
     * under a few hundred lines of it, and on this machine every one
     * of those lines is a write() through the simulator.
     */
    if (verbose)
        print_map();

    /* close all object files (avoid double-close for archive members) */
    {
        struct object *obj, *obj2;
        for (obj = objects; obj; obj = obj->next) {
            if (obj->fp) {
                fclose(obj->fp);
                /* clear fp from any other objects sharing the same handle */
                for (obj2 = obj->next; obj2; obj2 = obj2->next) {
                    if (obj2->fp == obj->fp)
                        obj2->fp = NULL;
                }
                obj->fp = NULL;
            }
        }
    }

    return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
