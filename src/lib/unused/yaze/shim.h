/*
 * the glue from yaze to my simulator
 */

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int FASTREG;
typedef unsigned int FASTWORK;

extern BYTE get_byte(WORD a);
extern WORD get_word(WORD a);
extern void put_byte(WORD a, BYTE v);
extern void put_word(WORD a, WORD v);

// these just go straight across as macro expansions
#define	GetBYTE(a)	get_byte(a)
#define	GetWORD(a)	get_word(a)
#define	PutBYTE(a,v)	put_byte(a,v)
#define	PutWORD(a,v)	put_word(a,v)

// these need to do some work
extern BYTE GetOpCode(WORD pc);

