/*
 * Whitesmith's object file format 
 *
 * each object file has 5 sections: 
 * 16-byte header, text,  data, symbol table, relocation bytes 
 * 
 * /include/obj.h
 *
 * Changed: <2023-07-04 11:25:30 curt>
 */

/*
 * The object file header
 */
struct obj {
	unsigned char ident;				/* see below */
	unsigned char conf;					/* see below */
	unsigned short table;				/* symbol table size (bytes) */
	unsigned short text;				/* text segment size */
	unsigned short data;				/* data segment size */
	unsigned short bss;					/* bss segment size (not in file) */
	unsigned short heap;				/* stack + heap size (not in file) */
	unsigned short textoff;				/* text offset (origin) */
	unsigned short dataoff;				/* data offset (origin) */
};

#define OBJECT	0x99			/* Whitesmith's standard */
#define RELOC	0x14			/* relocation bytes present */
#define NORELOC 0x94			/* no reloc bytes */

/*
 * symbol table
 * the name field is actually dependent on the conf byte
 * but in micronix, we only ever see the 8 byte symbols.
 * in addition, the conf byte could set the size of the value
 * to 4 bytes, and this does not come up in micronix for obvious
 * reasons
 */
struct ws_symbol {
	unsigned short value;
	unsigned char flag;
#define SF_SEG          0x03
#define         SF_UNDEF        0x00
#define         SF_TEXT         0x01
#define         SF_DATA         0x02
#define         SF_BSS          0x03
#define SF_DEF          0x04
#define SF_GLOBAL       0x08
	char name[9];
};

/*
 * relocation entry
 */
struct ws_reloc {
	unsigned short offset;
	unsigned short value;
	unsigned char type;
#define REL_TEXTOFF 1
#define REL_DATAOFF 2
#define REL_BSSOFF  3
#define REL_SYMBOL  4
};

#define REL_EXTEND  43

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
