/*
 * whitesmith's object file
 */
typedef unsigned char UCHAR;
typedef unsigned short UINT;
typedef unsigned int ULONG;

#include "../include/obj.h"

/*
 * symbol table
 */
struct symtab {
        unsigned short  value;
        unsigned char   flag;
#define SF_SEG          0x03
#define         SF_UNDEF        0x00
#define         SF_TEXT         0x01
#define         SF_DATA         0x02
#define SF_DEF          0x04
#define SF_GLOBAL       0x08
        char    name[9];
};

/*
 * relocation entry
 */
struct reloc {
	unsigned short offset;
	unsigned short value;
	unsigned char type;
#define	REL_TEXTOFF	1
#define	REL_DATAOFF	2
#define	REL_SYMBOL	4
};

#define	REL_EXTEND	43

extern struct reloc *getreloc(char **pp);
extern unsigned short segoffset;

