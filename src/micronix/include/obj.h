/*
 * Whitesmith's object file format
 *
 * The one header for the object format.  asz writes it, ld reads and
 * rewrites it, nm and size read it, and the kernel's exec and the boot
 * loader read the header to load a program.  Any source that needs to
 * know the format - the structures or the constants - includes this.
 *
 * include/obj.h
 */

#ifndef OBJ_H
#define OBJ_H

/*
 * The object file header - 16 bytes, first thing in the file.
 */
struct obj {
	unsigned char ident;			/* OBJECT / MAGIC, below */
	unsigned char conf;			/* config byte, see CONF_* */
	unsigned short table;			/* symbol table size (bytes) */
	unsigned short text;			/* text segment size */
	unsigned short data;			/* data segment size */
	unsigned short bss;			/* bss segment size (not in file) */
	unsigned short heap;			/* stack + heap size (not in file) */
	unsigned short textoff;			/* text offset (origin) */
	unsigned short dataoff;			/* data offset (origin) */
};

/*
 * object file magic number (first byte)
 */
#define OBJECT	0x99			/* Whitesmith's standard */
#define MAGIC	0x99			/* the tools' name for OBJECT */

/*
 * archive magic number (first 2 bytes, little-endian)
 * 0177565 octal
 */
#define AR_MAGIC    0xFF75      /* whitesmiths: name, 2-byte length */
#define V7_MAGIC    0xFF65      /* v7, which is what ar(1) writes */

/*
 * A v7 member header is 26 bytes and the name is the first 14 of
 * them, so both formats start the same way and only the rest differs:
 *
 *	date 4  uid 1  gid 1  mode 2  size 4
 *
 * The size is the four bytes at the end of it.
 */
#define V7_HDRSIZ   26
#define V7_SIZEOFF  8           /* within the 12 bytes after the name */

/*
 * configuration byte flags (second byte of object file)
 */
#define CONF_SYMASK	0x07    /* mask: symbol length = (conf & 7) * 2 + 1 */
#define CONF_INT32  0x08    /* 32-bit integers */
#define CONF_LITTLE 0x10    /* little-endian */
#define CONF_ALIGN  0x60    /* alignment mask */
#define CONF_NORELO 0x80    /* no relocations */

/*
 * common configurations
 */
#define CONF_9      (CONF_LITTLE | (4 & CONF_SYMASK))   /* 9 char syms */
#define CONF_15     (CONF_LITTLE | (7 & CONF_SYMASK))   /* 15 char syms */

/*
 * maximum symbol length
 */
#define SYMLEN      15

/*
 * symbol table entry
 *
 * the name field is actually dependent on the conf byte.  In micronix
 * the default is 9 characters, but we can have up to 15 if the conf
 * byte is 7.  Handle the worst case and swizzle the read count
 * accordingly when reading the symbol table.
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
 * segment types - used in symbol table and relocations
 *
 * In object file symbol table entries (type byte):
 *   bits 0-2: segment (4=abs, 5=text, 6=data, 7=bss)
 *   bit 3: global flag (0x08)
 *
 * Internal segment numbers (used in assembler):
 */
#define SEG_UNDEF   0       /* undefined (extern reference) */
#define SEG_TEXT    1       /* text segment */
#define SEG_DATA    2       /* data segment */
#define SEG_BSS     3       /* bss segment */
#define SEG_ABS     4       /* absolute (not relocatable) */
#define SEG_EXT     5       /* external reference */

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
 * Relocation encoding constants
 *
 * Bump encoding (distance to next relocation):
 *   0x01-0x1f: bump value 1-31
 *   0x20-0x3f: high byte (value = ((b-32)<<8) + next_byte + 32)
 *   max single bump = 0x3f,0xff = 8223
 *
 * Control bytes (relocation type):
 *   0x40: absolute (SEG_ABS)
 *   0x44: text segment (SEG_TEXT)
 *   0x48: data segment (SEG_DATA)
 *   0x4c: bss segment (SEG_BSS)
 *   0x50-0xfb: symbol index = (b - 0x50) >> 2
 *   0xfc: extended symbol, followed by:
 *     0x00-0x7f: index = b + 43
 *     0x80-0xff: index = ((b-0x80)<<8) + next_byte + 171
 */
#define REL_BUMP_MAX    31      /* max simple bump (0x01-0x1f) */
#define REL_BUMP_EXT    32      /* extended bump threshold (0x20) */
#define REL_BUMP_LIM    8223    /* max single bump value (0x3f,0xff) */

/* Segment relocation base values (add hilo: 0=word, 1=lo, 2=hi) */
#define REL_ABS         0x40    /* absolute relocation */
#define REL_TEXT        0x44    /* text segment relocation */
#define REL_DATA        0x48    /* data segment relocation */
#define REL_BSS         0x4c    /* bss segment relocation */

/* Relocation size modifiers (added to segment base) */
#define REL_WORD        0       /* full 16-bit relocation */
#define REL_LO          1       /* low byte only */
#define REL_HI          2       /* high byte only */
#define REL_SYM_BASE    0x50    /* start of symbol index range */
#define REL_SYM_END     0xfc    /* end of symbol index range (exclusive) */
#define REL_SYM_EXT     0xfc    /* extended symbol marker */
#define REL_EXT_LONG    0x80    /* extended symbol long form flag */

/* symbol index encoding thresholds */
#define REL_SYM_OFS     4       /* offset added to symbol index */
#define REL_SYM_SHIFT   16      /* shift for inline encoding */
#define REL_EXT_THR1    47      /* threshold for 1-byte extended */
#define REL_EXT_THR2    175     /* threshold for 2-byte extended */

/*
 * Object file header offsets
 */
#define HDR_MAGIC       0       /* 1 byte: magic (0x99) */
#define HDR_CONFIG      1       /* 1 byte: config byte */
#define HDR_SYMTAB      2       /* 2 bytes: symbol table size */
#define HDR_TEXT        4       /* 2 bytes: text segment size */
#define HDR_DATA        6       /* 2 bytes: data segment size */
#define HDR_BSS         8       /* 2 bytes: bss segment size */
#define HDR_HEAP        10      /* 2 bytes: heap size */
#define HDR_TEXTOFF     12      /* 2 bytes: text start offset */
#define HDR_DATAOFF     14      /* 2 bytes: data start offset */
#define HDR_SIZE        16      /* total header size */

/*
 * Archive entry header
 */
#define AR_NAMELEN      14      /* archive member name length */
#define AR_HDRSIZE      16      /* name(14) + length(2) */

/*
 * Object file header layout:
 *
 *   Offset  Size  Description
 *   0       1     Magic (0x99)
 *   1       1     Config byte
 *   2       2     Symbol table size (bytes)
 *   4       2     Text segment size
 *   6       2     Data segment size
 *   8       2     BSS segment size
 *   10      2     Heap size (text + data in memory)
 *   12      2     Text start offset (usually 0)
 *   14      2     Data start offset (usually text_size)
 *
 * After header:
 *   - Text segment data (text_size bytes)
 *   - Data segment data (data_size bytes)
 *   - Symbol table entries (symtab_size bytes)
 *   - Text relocation records (variable, 0-terminated)
 *   - Data relocation records (variable, 0-terminated)
 *
 * Symbol table entry (symlen+3 bytes each):
 *   - 2 bytes: value (segment-relative offset)
 *   - 1 byte: type (segment in bits 0-2, global flag in bit 3)
 *   - N bytes: symbol name (N = symlen from config)
 *
 * Relocation record:
 *   - 2 bytes: address within segment to patch
 *   - 2 bytes: symbol index (or segment marker)
 *
 * Archive format:
 *   - 2 bytes: magic (0xFF75)
 *   - Entries: 14-byte name + 2-byte length + object data
 *   - End marker: entry with null name
 */

#endif /* OBJ_H */

/* vim: tabstop=4 shiftwidth=4 noexpandtab: */
