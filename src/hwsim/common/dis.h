/*
 * dis.h
 *
 * a z80 disassembler
 */
#define	REL_SYMBOL	3	// low word is arg to get_sym

int format_instr(unsigned short addr, char *outbuf, 
	unsigned char (*get_byte)(unsigned short addr),
	char * (*get_sym)(unsigned short offset),
	unsigned int (*get_reloc)(unsigned short offset));
