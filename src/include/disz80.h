/*
 * a z80 disassembler interface
 *
 * include/disz80.h
 *
 * Changed: <2023-06-16 00:06:59 curt>
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
 * callers to the dissassembler need to define these
 */
extern unsigned int get_reloc(unsigned short addr);
extern unsigned char get_byte(unsigned short addr);
extern char *get_symname(unsigned short addr);
extern int fmt_syscall(unsigned short addr, char *dest);

/*
 * format an instruction and return the bytes consumed
 */
int format_instr(unsigned short addr, char *outbuf);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
