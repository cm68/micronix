/*
 * disz80.h
 *
 * a z80 disassembler interface
 */

/*
 * the get_reloc interface returns the address of a symbol,
 * with a potential tag.  these tags are handled by get_sym.
 */
#define	RELTYPE(i)	((i) >> 16)
#define	RL_SYMBOL	1
#define	RL_TEXT		2
#define	RL_DATA		3
#define	RELNUM(i)	((i) & 0xffff)

typedef unsigned int symaddr_t;

/*
 * format an instruction and return the bytes consumed
 */
int format_instr(
	unsigned short addr, 
	char *outbuf,
	char (*get_byte)(unsigned short addr),		
	char *(*get_sym)(symaddr_t symaddr),
	int (*get_reloc)(symaddr_t offset),
	int (*syscall)(unsigned short addr, char (*gb)(unsigned short a), char *d));

extern int mnix_sc(unsigned short addr, char (*gb)(unsigned short a), char *d);
