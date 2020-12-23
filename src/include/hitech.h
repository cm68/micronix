/*
 * the hitech object file
 *
 * this file is derived purely from inspection of a number of object
 * files, and there are large gaps in understanding the format.  doubtless
 * the format has support for different word lengths, byte orders, relocation
 * types, and so on.
 */
#define	HITECH_MAGIC			0x000a

#pragma pack(1)

struct hitechobj {
	unsigned short magic;		/* 0x0a */
	unsigned char len;			/* 7 */
	unsigned int bytesex;		/* 0x03020100 */
	unsigned short bsize;		/* 0x100 */
	char arch[4];				/* Z80 */
};

/*
 * hitech record types
 */
#define	HIREC_UNK0	0
#define	HIREC_BLK	1
#define	HIREC_UNK2	2
#define	HIREC_RELOC	3
#define	HIREC_SYM	4
#define	HIREC_UNK5	5
#define	HIREC_END	6

/*
 * record prefix sharred by all types
 */
struct hipre {
	unsigned short reclen;
	unsigned char code;
};

/*
 * HIREC_BLK has this, a zero-terminated segment name, 
 * followed by payload bytes
 */
struct hiblkrec {
	unsigned short addr;
	unsigned short i0;
};

/*
 * there is an array of these in every symbol record, with each
 * entry followed by a 2 null-terminated strings: seg, name
 */
struct hisymrec {
	unsigned short addr;
	unsigned short i0;
	unsigned short flags;
};
#define	HTSYM_DEF	0x10
#define	HTSYM_UNDEF	0x16
#define	HTSYM_EQU	0x00

/*
 * relocation entries - these also have a string appended (seg, sym)
 */
struct hirelrec {
	unsigned short addr;	/* offset in previous block */
	unsigned char flags;
};
#define	HTREL_SYM	0x22	/* add the symbol address */
#define	HTREL_SEG	0x12	/* add the segment base */

/*
 * these are of unknown use, and have a null-terminated segment name
 * appended
 */
struct hiunkrec {			/* type 2 */
	unsigned short i0;
};

/*
 * this is different in that it has an extra short.
 */
struct hiunkrec5 {
	unsigned short i0;		/* might be a forced origin or phase */
	unsigned short i1;
};

