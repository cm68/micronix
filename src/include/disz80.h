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
 * An address by itself does not say where it is.  0x100b is one thing in
 * task 1 and another in the supervisor's on board memory, and while a
 * trap sequence runs the cpu is not fetching from the pc at all - it is
 * reading a sixteen byte window of rom while the pc counts past whatever
 * happens to be underneath.  Printing the bare number invites the reader
 * to look up the wrong bytes, which is exactly what happened here.
 *
 * dis_space writes the space and the address within it into buf and
 * returns buf.  In the trap window the address is the offset into the
 * window, 0 through 15, because that is the only number that means
 * anything there.
 */
extern char *dis_space(unsigned short addr, char *buf, int len);

/*
 * The other direction: turn something a person typed - sys:0100,
 * tsk1:100b, trap:05 - into exactly the string dis_space would print for
 * that place, so the two can simply be compared.  Returns 0 if the spec
 * makes no sense for this machine.  It lives beside dis_space so that
 * only one file has to know how a space is spelled.
 */
extern int dis_parse(char *spec, char *buf, int len);

/*
 * format an instruction and return the bytes consumed
 */
int format_instr(unsigned short addr, char *outbuf);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
