/*
 * obj.h
 * 
 * Whitesmith's object file format The file has 5 sections: 16-byte header text
 * data symbol table relocation bytes The header structure is:
 */

struct obj {
    UINT8 ident;                /* see below */
    UINT8 conf;                 /* see below */
    UINT table;               /* symbol table size (bytes) */
    UINT text;                /* text segment size */
    UINT data;                /* data segment size */
    UINT bss;                 /* bss segment size (not in file) */
    UINT heap;                /* stack + heap size (not in file) */
    UINT textoff;             /* text offset (origin) */
    UINT dataoff;             /* data offset (origin) */
};

#define OBJECT	0x99            /* Whitesmith's standard */
#define RELOC	0x14            /* relocation bytes present */
#define NORELOC 0x94            /* no reloc bytes */

/*
 * symbol table
 * the name field is actually dependent on the conf byte
 * but in micronix, we only ever see the 8 byte symbols.
 * in addition, the conf byte could set the size of the value
 * to 4 bytes, and this does not come up in micronix for obvious
 * reasons
 */
struct ws_symbol {
    UINT value;
    UINT8 flag;
#define SF_SEG          0x03
#define         SF_UNDEF        0x00
#define         SF_TEXT         0x01
#define         SF_DATA         0x02
#define SF_DEF          0x04
#define SF_GLOBAL       0x08
    char name[9];
};

/*
 * relocation entry
 */
struct ws_reloc {
    UINT offset;
    UINT value;
    UINT8 type;
#define REL_TEXTOFF 1
#define REL_DATAOFF 2
#define REL_SYMBOL  4
};

#define REL_EXTEND  43

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
