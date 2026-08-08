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
 * Reading a byte to show someone is not the same operation as reading
 * one because the cpu asked for it, and the disassembler wants the
 * first.  get_byte is the second: in hwsim it is the bus read, which
 * honours the trap sequence redirection and counts it down, and in
 * usersim it trips read watchpoints.  Disassembling through either one
 * changes the machine it is describing.
 *
 * dis_byte reads the address space the cpu would see, through whatever
 * mapping is in force, and disturbs nothing.
 */
extern unsigned char dis_byte(unsigned short addr);

/*
 * format an instruction and return the bytes consumed
 */
int format_instr(unsigned short addr, char *outbuf);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
