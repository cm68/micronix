/*
 * a z80 disassembler - include file
 */
int format_instr(unsigned short addr, char *outbuf, 
	unsigned char (*get_byte)(unsigned short addr),
	char * (*get_sym)(unsigned short offset),
	unsigned long (*get_reloc)(unsigned short offset));
