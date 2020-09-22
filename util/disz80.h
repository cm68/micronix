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

/*
 * format an instruction and return the bytes consumed
 */
int format_instr(
	int addr, 
	char *outbuf,
	char (*get_byte)(int addr),		
	char *(*get_sym)(int symaddr),
	int (*get_reloc)(int offset),
	int (*syscall)(int addr, char (*gb)(int a), char *d));

extern int mnix_sc(int addr, char (*gb)(int a), char *d);
