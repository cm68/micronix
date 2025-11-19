/*
 * Whitesmith's object file format 
 *
 * each object file has 5 sections: 
 * 16-byte header, text,  data, symbol table, relocation bytes 
 * 
 * /include/obj.h
 *
 * Changed: <2025-11-19 15:32:15 curt>
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

#define CONF_SYMLEN     0x07
#define CONF_INT32      0x08
#define CONF_LITTLE     0x10
#define CONF_ALIGN      0x60
#define CONF_NORELO     0x80

#define CONF_9  (CONF_LITTLE | (4 & CONF_SYMLEN))       /* 9 char syms */
#define CONF_15 (CONF_LITTLE | (7 & CONF_SYMLEN))       /* 15 char syms */

/*
 * symbol table
 * the name field is actually dependent on the conf byte
 *
 * but in micronix, the default is 9 characters, but we can have
 * up to 15, if the conf byte is 7. handle the worst case, and
 * swizzle the read count accordingly.
 * when we read in the symbol table, we'll need to swizzle the amount
 * we read.
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
	char name[15];
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
