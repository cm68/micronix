/*
 * dump out the symbol table and optionally the relocation entries
 * of an object file, or if an archive, of each file in the archive
 * just for grins, it can disassemble too.
 */

#include "ws.h"
#include <fcntl.h>


int verbose;
int rflag;
int dflag;
struct ws_symbol *syms;
int nsyms;

unsigned char relocbuf[16384];
unsigned char *relocp;

unsigned short location;

struct member {
	char fname[14];
	unsigned char len[2];
} member;

struct reloc {
	unsigned short seg;
	struct ws_reloc rl;
	struct reloc *next;
} *rlhead;
#define	SEG_TEXT	0
#define	SEG_DATA	1

struct reloc *
lookup(int i)
{
	struct reloc *r;
	for (r = rlhead; r; r = r->next) {
		if (r->rl.offset == i) {
			break;
		}
	}
	return r;
}

void 
render(char *buf, struct reloc *r, unsigned short value)
{
	switch(r->rl.type) {
	case REL_TEXTOFF:
		sprintf(buf, "T+%x", value);
		break;
	case REL_DATAOFF:
		sprintf(buf, "D+%x", value);
		break;
	case REL_SYMBOL:
		sprintf(buf, syms[r->rl.value].name);
		break;
	default:
		sprintf(buf, "BZZZT");
	}
}

/*
 * the current object header
 */
struct obj head;

/*
 * file offsets to deal with disassembly inside an archive, since the disassembly is two-pass
 */
int textoff;
int dataoff;
int endoff;
int fd;

struct optab {
	char *op;
	unsigned char flags;
#define	OP_PC8	0x01	// here +1 is a pc-rel 8 bit signed offset
#define	OP_TEXT	0x02	// here +1 is a 16 bit absolute text address
#define	OP_IDX	0x04	// here +1 is an 8 bit signed index register bias
#define	OP_DATA	0x08	// here +1 is a 16 bit absolute address
#define	OP_IMM8	0x10	// here +1 is a 8 bit immediate
#define	OP_IND	0x20	// here +1 is a 16 bit indirect address
#define	OP_SYS	0x40	// system call
};

struct optab base_optab[256] = {
	{ "NOP", 0 }, { "LD BC,%s", OP_DATA }, { "LD (BC),A", 0 }, { "INC BC", 0 },
	{ "INC B", 0 }, { "DEC B", 0 }, { "LD B,%s", OP_IMM8 }, { "RLC A", 0 },
	{ "EX AF, AF'", 0 }, { "ADD HL,BC", 0 }, { "LD A,(BC)", 0 }, { "DEC BC", 0 },
	{ "INC C", 0 }, { "DEC C", 0 }, { "LD C,%s", OP_IMM8 }, { "RRC A", 0 },

	{ "DJNZ %s", OP_PC8 }, { "LD DE,%s", OP_DATA }, { "LD (DE),A", 0 }, { "INC DE", 0 },
	{ "INC D", 0 }, { "DEC D", 0 }, { "LD D,%s", OP_IMM8 }, { "RL A", 0 },
	{ "JR %s", OP_PC8 }, { "ADD HL,DE", 0 }, { "LD A,(DE)", 0 }, { "DEC DE", 0 },
	{ "INC E", 0 }, { "DEC E", 0 }, { "LD E,%s", OP_IMM8 }, { "RR A", 0 },

	{ "JR NZ,%s", OP_PC8 }, { "LD HL,%s", OP_DATA }, { "LD (%s),HL", OP_DATA|OP_IND }, { "INC HL", 0 },
	{ "INC H", 0 }, { "DEC H", 0 }, { "LD H,%s", OP_IMM8 }, { "DAA", 0 },
	{ "JR Z,%s", OP_PC8 }, { "ADD HL,HL", 0 }, { "LD HL,(%s)", OP_DATA|OP_IND }, { "DEC HL", 0 },
	{ "INC L", 0 }, { "DEC L", 0 }, { "LD L,%s", OP_IMM8 }, { "CPL", 0 },

	{ "JR NC,%s", 0 }, { "LD SP,%s", OP_DATA }, { "LD (%s),A", OP_DATA|OP_IND }, { "INC SP", 0 },
	{ "INC (HL)", 0 }, { "DEC (HL)", 0 }, { "LD (HL),", OP_IMM8 }, { "SCF", 0 },
	{ "JR C,%s", 0 }, { "ADD HL,SP", 0 }, { "LD A,(%s)", OP_DATA|OP_IND }, { "DEC SP", 0 },
	{ "INC A", 0 }, { "DEC A", 0 }, { "LD A,", OP_IMM8 }, { "CCF", 0 },

	{ "LD B,B", 0 }, { "LD B,C", 0 }, { "LD B,D", 0 }, { "LD B,E", 0 },
	{ "LD B,H", 0 }, { "LD B,L", 0 }, { "LD B,(HL)", 0 }, { "LD B,A", 0 },
	{ "LD C,B", 0 }, { "LD C,C", 0 }, { "LD C,D", 0 }, { "LD C,E", 0 },
	{ "LD C,H", 0 }, { "LD C,L", 0 }, { "LD C,(HL)", 0 }, { "LD C,A", 0 },

	{ "LD D,B", 0 }, { "LD D,C", 0 }, { "LD D,D", 0 }, { "LD D,E", 0 },
	{ "LD D,H", 0 }, { "LD D,L", 0 }, { "LD D,(HL)", 0 }, { "LD D,A", 0 },
	{ "LD E,B", 0 }, { "LD E,C", 0 }, { "LD E,D", 0 }, { "LD E,E", 0 },
	{ "LD E,H", 0 }, { "LD E,L", 0 }, { "LD E,(HL)", 0 }, { "LD E,A", 0 },

	{ "LD H,B", 0 }, { "LD H,C", 0 }, { "LD H,D", 0 }, { "LD H,E", 0 },
	{ "LD H,H", 0 }, { "LD H,L", 0 }, { "LD H,(HL)", 0 }, { "LD H,A", 0 },
	{ "LD L,B", 0 }, { "LD L,C", 0 }, { "LD L,D", 0 }, { "LD L,E", 0 },
	{ "LD L,H", 0 }, { "LD L,L", 0 }, { "LD L,(HL)", 0 }, { "LD L,A", 0 },

	{ "LD (HL),B", 0 }, { "LD (HL),C", 0 }, { "LD (HL),D", 0 }, { "LD (HL),E", 0 },
	{ "LD (HL),H", 0 }, { "LD (HL),L", 0 }, { "HALT", 0 }, { "LD (HL),A", 0 },
	{ "LD A,B", 0 }, { "LD A,C", 0 }, { "LD A,D", 0 }, { "LD A,E", 0 },
	{ "LD A,H", 0 }, { "LD A,L", 0 }, { "LD A,(HL)", 0 }, { "LD A,A", 0 },
	
	{ "ADD A,B", 0 }, { "ADD A,C", 0 }, { "ADD A,D", 0 }, { "ADD A,E", 0 },
	{ "ADD A,H", 0 }, { "ADD A,H", 0 }, { "ADD A,(HL)", 0 }, { "ADD A,A", 0 },
	{ "ADC A,B", 0 }, { "ADC A,C", 0 }, { "ADC A,D", 0 }, { "ADC A,E", 0 },
	{ "ADC A,H", 0 }, { "ADC A,H", 0 }, { "ADC A,(HL)", 0 }, { "ADC A,A", 0 },

	{ "SUB A,B", 0 }, { "SUB A,C", 0 }, { "SUB A,D", 0 }, { "SUB A,E", 0 },
	{ "SUB A,H", 0 }, { "SUB A,H", 0 }, { "SUB A,(HL)", 0 }, { "SUB A,A", 0 },
	{ "SBC A,B", 0 }, { "SBC A,C", 0 }, { "SBC A,D", 0 }, { "SBC A,E", 0 },
	{ "SBC A,H", 0 }, { "SBC A,H", 0 }, { "SBC A,(HL)", 0 }, { "SBC A,A", 0 },

	{ "AND A,B", 0 }, { "AND A,C", 0 }, { "AND A,D", 0 }, { "AND A,E", 0 },
	{ "AND A,H", 0 }, { "AND A,H", 0 }, { "AND A,(HL)", 0 }, { "AND A,A", 0 },
	{ "XOR A,B", 0 }, { "XOR A,C", 0 }, { "XOR A,D", 0 }, { "XOR A,E", 0 },
	{ "XOR A,H", 0 }, { "XOR A,H", 0 }, { "XOR A,(HL)", 0 }, { "XOR A,A", 0 },

	{ "OR A,B", 0 }, { "OR A,C", 0 }, { "OR A,D", 0 }, { "OR A,E", 0 },
	{ "OR A,H", 0 }, { "OR A,H", 0 }, { "OR A,(HL)", 0 }, { "OR A,A", 0 },
	{ "CP A,B", 0 }, { "CP A,C", 0 }, { "CP A,D", 0 }, { "CP A,E", 0 },
	{ "CP A,H", 0 }, { "CP A,H", 0 }, { "CP A,(HL)", 0 }, { "CP A,A", 0 },
	
	{ "RET NZ", 0 }, { "POP BC", 0 }, { "JP NZ,%s", OP_TEXT}, { "JP %s", OP_TEXT},
	{ "CALL NZ,%s", OP_TEXT }, { "PUSH BC", 0 }, { "ADD A,%s", OP_IMM8}, { "RST 0", 0},
	{ "RET Z", 0 }, { "RET", 0 }, { "JP Z,%s", OP_TEXT}, { 0, 0 },
	{ "CALL Z,%s", OP_TEXT }, { "CALL %s", OP_TEXT }, { "ADC A,%s", OP_IMM8}, { "SYS %s", OP_SYS},

	{ "RET NC", 0 }, { "POP DE", 0 }, { "JP NC,%s", OP_TEXT}, { "OUT (%s),A", OP_IMM8|OP_IND},
	{ "CALL NC,", OP_TEXT }, { "PUSH DE", 0 }, { "SUB A,", OP_IMM8}, { "RST 10", 0},
	{ "RET C", 0 }, { "EXX", 0 }, { "JP C,%s", OP_TEXT}, { "IN A,(%s)", OP_IMM8|OP_IND},
	{ "CALL C,%s", OP_TEXT }, { 0, 0 }, { "SBC A,%s", OP_IMM8}, { "RST 18", 0},

	{ "RET PO", 0 }, { "POP HL", 0 }, { "JP PO,%s", OP_TEXT}, { "EX (SP),HL", 0},
	{ "CALL PO,%s", OP_TEXT }, { "PUSH HL", 0 }, { "AND A,%s", OP_IMM8}, { "RST 20", 0},
	{ "RET PE", 0 }, { "JP (HL)", 0 }, { "JP PE,%s", OP_TEXT}, { "EX DE,HL", 0},
	{ "CALL PE,%s", OP_TEXT }, { 0, 0 }, { "XOR A,%s", OP_IMM8}, { "RST 28", 0},

	{ "RET P", 0 }, { "POP AF", 0 }, { "JP P,%s", OP_TEXT}, { "DI", 0},
	{ "CALL P,%s", OP_TEXT }, { "PUSH AF", 0 }, { "OR A,%s", OP_IMM8}, { "RST 30", 0},
	{ "RET M", 0 }, { "LD SP,HL", 0 }, { "JP M,%s", OP_TEXT}, { "EI", 0},
	{ "CALL M,%s", OP_TEXT }, { 0, 0 }, { "CP A,%s", OP_IMM8}, { "RST 38", 0}
};

struct optab dd_optab[256] = {
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ 0, 0}, { "ADD IX,BC", 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ 0, 0}, { "ADD IX,DE", 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 

	{ 0, 0}, { "LD IX,%s", OP_DATA}, { "LD (%s),IX", OP_DATA|OP_IND}, { "INC IX", 0}, 
	{ "INC IXH", 0}, { "DEC IXH", 0}, { "LD IXH,%s", OP_IMM8}, { 0, 0}, 
	{ 0, 0}, { "ADD IX,IX", 0}, { "LD IX,(%s)", OP_DATA|OP_IND}, { "DEC IX", 0}, 
	{ "INC IXL", 0}, { "DEC IXL", 0}, { "LD IXL,%s", OP_IMM8}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "INC (IX%s)", OP_IDX}, { "DEC (IX%s)", OP_IDX}, { "LD (IX%s),%s", OP_IDX|OP_IMM8}, { 0, 0}, 
	{ 0, 0}, { "ADD IX,SP", 0}, { 0, 0}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "LD B,IXH", 0}, { "LD B,IXL", 0}, { "LD B,(IX%s)", OP_IDX}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "LD C,IXH", 0}, { "LD C,IXL", 0}, { "LD C,(IX%s)", OP_IDX}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "LD D,IXH", 0}, { "LD D,IXL", 0}, { "LD D,(IX%s)", OP_IDX}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "LD E,IXH", 0}, { "LD E,IXL", 0}, { "LD E,(IX%s)", OP_IDX}, { 0, 0}, 

	{ "LD IXH, B", 0}, { "LD IXH, C", 0}, { "LD IXH, D", 0}, { "LD IXH, E", 0}, 
	{ "LD IXH, IXH", 0}, { "LD IXH, IXL", 0}, { "LD H, (IX%s)", OP_IDX}, { "LD IXH, A", 0}, 
	{ "LD IXL, B", 0}, { "LD IXL, C", 0}, { "LD IXL, D", 0}, { "LD IXL, E", 0}, 
	{ "LD IXL, IXL", 0}, { "LD IXL, IXL", 0}, { "LD L, (IX%s)", OP_IDX}, { "LD IXL, A", 0}, 

	{ "LD (IX%s), B", OP_IDX}, { "LD (IX%s), C", OP_IDX}, 
	{ "LD (IX%s), D", OP_IDX}, { "LD (IX%s), E", OP_IDX}, 
	{ "LD (IX%s), H", OP_IDX}, { "LD (IX%s), L", OP_IDX}, 
	{ 0, 0}, { "LD (IX%s), A", OP_IDX}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "LD A, IXH", 0}, { "LD A, IXL", 0}, { "LD A, (IX%s)", OP_IDX}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "ADD A, IXH", 0}, { "ADD A, IXL", 0}, { "ADD A, (IX%s)", OP_IDX}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "ADC A, IXH", 0}, { "ADC A, IXL", 0}, { "ADC A, (IX%s)", OP_IDX}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "SUB A, IXH", 0}, { "SUB A, IXL", 0}, { "SUB A, (IX%s)", OP_IDX}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "SBC A, IXH", 0}, { "SBC A, IXL", 0}, { "SBC A, (IX%s)", OP_IDX}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "AND A, IXH", 0}, { "AND A, IXL", 0}, { "AND A, (IX%s)", OP_IDX}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "XOR A, IXH", 0}, { "XOR A, IXL", 0}, { "XOR A, (IX%s)", OP_IDX}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "OR A, IXH", 0}, { "OR A, IXL", 0}, { "OR A, (IX%s)", OP_IDX}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ "CP A, IXH", 0}, { "CP A, IXL", 0}, { "CP A, (IX%s)", OP_IDX}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 

	{ 0, 0}, { "POP IX", 0}, { 0, 0}, { "EX (SP),IX", 0}, 
	{ 0, 0}, { "PUSH IX", 0}, { 0, 0}, { 0, 0}, 
	{ 0, 0}, { "JP (IX)", 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 

	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, 
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { "LD SP,IX", 0}, { 0, 0}, { 0, 0}, 
};

struct optab ddcb_optab[256] = { };
struct optab fd_optab[256] = { };
struct optab fdcb_optab[256] = { };
struct optab ed_optab[256] = { };
struct optab cb_optab[256] = { };


/*
0xffff          0x00cf          4       "SYS INDIR"             imm161
0xffff          0x01cf          2       "SYS EXIT"
0xffff          0x02cf          2       "SYS FORK"
0xffff          0x06cf          2       "SYS CLOSE"
0xffff          0x07cf          2       "SYS WAIT"
0xffff          0x0dcf          2       "SYS TIME"
0xffff          0x14cf          2       "SYS GETPID"
0xffff          0x17cf          2       "SYS SETUID"
0xffff          0x18cf          2       "SYS GETUID"
0xffff          0x19cf          2       "SYS STIME"
0xffff          0x22cf          2       "SYS NICE"
0xffff          0x23cf          2       "SYS SLEEP"
0xffff          0x24cf          2       "SYS SYNC"
0xffff          0x29cf          2       "SYS DUP"
0xffff          0x2acf          2       "SYS PIPE"
0x00ff          0x00cf          2       "SYS UNDEC"
 */
struct sys {
	int n;
	char *name;
	int bc;
} sc[] = {
	{ 0x0, "INDIR", 2 },
	{ 0x1, "EXIT", 0 },
	{ 0x2, "FORK", 0 },
	{ 0x6, "CLOSE", 0 },
	{ 0x7, "WAIT", 0 },
	{ 0xd, "TIME", 0 },
	{ 0x14, "GETPID", 0 },
	{ 0x17, "SETUID", 0 },
	{ 0x18, "GETUID", 0 },
	{ 0x19, "STIME", 0 },
	{ 0x22, "NICE", 0 },
	{ 0x23, "SLEEP", 0 },
	{ 0x24, "SYNC", 0 },
	{ 0x29, "DUP", 0 },
	{ 0x2a, "PIPE", 0 }
};

/*
 * grunge through the object code and create xrefs and labels
 */
process_instr(int len)
{
	struct optab *o;

	unsigned char opcode;
	unsigned char a[2];
	unsigned short value;
	char b;
	char disp[6];
	char valbuf[10];
	int i, j;
	unsigned char barray[6];
	int bcount;
	unsigned char ibuf[30];
	struct reloc *r;

	location = 0;
	while (location < len) {
		bcount = 0;
		
		/* find the right opcode table entry */
		read(fd, &opcode, 1);
		barray[bcount++] = opcode;

		if (opcode == 0xcb) {
			read(fd, &opcode, 1);
			barray[bcount++] = opcode;
			o = &cb_optab[opcode];
		} else if (opcode == 0xed) {
			read(fd, &opcode, 1);
			barray[bcount++] = opcode;
			o = &ed_optab[opcode];
		} else if (opcode == 0xdd) {
			read(fd, &opcode, 1);
			barray[bcount++] = opcode;
			if (opcode == 0xcb) {
				read(fd, &opcode, 1);
				barray[bcount++] = opcode;
				o = &ddcb_optab[opcode];
			} else {
				o = &dd_optab[opcode];
			}
		} else if (opcode == 0xfd) {
			read(fd, &opcode, 1);
			barray[bcount++] = opcode;
			if (opcode == 0xcb) {
				read(fd, &opcode, 1);
				barray[bcount++] = opcode;
				o = &fdcb_optab[opcode];
			} else {
				o = &fd_optab[opcode];
			}
		} else {
			o = &base_optab[opcode];
		}
		printf("%04x: ", location);

		/* now read operand bytes */
		if (o->flags & OP_SYS) {
			read(fd, &opcode, 1);
			barray[bcount++] = opcode;
			for (i = sizeof(sc) / sizeof(sc[0]) - 1; i >= 0; i--) {
				if (sc[i].n == opcode) {
					break;
				}
			}
			for (j = 0; j < sc[i].bc; j++) {
				read(fd, &opcode, 1);
				barray[bcount++] = opcode;
			}
			if (i < 0) {
				sprintf(valbuf, "%d", opcode);		
			} else {
				sprintf(valbuf, "%s", sc[i].name);		
			}
		}
		if (o->flags & OP_IDX) {
			read(fd, &a, 1);
			barray[bcount++] = a[0];
			b = a[0];
			value = b;
			if (b < 0) {
				sprintf(disp, "-%d", -b);
			} else {
				sprintf(disp, "+%d", b);
			}
		}
		if (o->flags & OP_IMM8) {
			read(fd, &a, 1);
			barray[bcount++] = a[0];
			value = a[0];
			sprintf(valbuf, "%d", value);
		}
		if (o->flags & OP_PC8) {
			read(fd, &a, 1);
			barray[bcount++] = a[0];
			b = a[0];
			if (b < 0) {
				sprintf(valbuf, ".-%d", -b);
			} else {
				sprintf(valbuf, ".+%d", b);
			}
		}
		if (o->flags & OP_TEXT) {
			r = lookup(location+bcount);
			read(fd, &a, 2);
			barray[bcount++] = a[0];
			barray[bcount++] = a[1];
			value = a[0] + (a[1] << 8);
			sprintf(valbuf, "%x", value);
			if (r) {
				render(valbuf, r, value);
			}
		}
		if (o->flags & OP_DATA) {
			read(fd, &a, 2);
			barray[bcount++] = a[0];
			barray[bcount++] = a[1];
			value = a[0] + (a[1] << 8);
			sprintf(valbuf, "%x", value);
		}

		if (o->flags & (OP_SYS|OP_DATA|OP_TEXT|OP_PC8|OP_IMM8|OP_IDX)) {
			if ((o->flags & (OP_IMM8|OP_IDX)) == (OP_IMM8|OP_IDX)) {
				sprintf(ibuf, o->op, disp, valbuf);
			} else if (o->flags & OP_IDX) {
				sprintf(ibuf, o->op, disp);
			} else {
				sprintf(ibuf, o->op, valbuf);
			}
		} else {
			sprintf(ibuf, o->op);
		}
		printf("%-20s ; ", ibuf);
		for (i = 0; i < bcount; i++) {
			printf("%02x ", barray[i]);	
		}
		printf("\n");
		location += bcount;
	}
}

dumpseg(int bc, int base)
{
	unsigned int i;
	unsigned char b;
	int c = 0;

	for (i = 0; i < bc; i++) {
		read(fd, &b, 1);
		if (c == 0) {
			printf("%04x: ", i + base);
		}
		printf("%02x ", b);	
		c++;
		if (c == 16) {
			c = 0;
			printf("\n");
		}
	}
	if (c) {
		printf("\n");
	}
}

disassem()
{
	/* dump out hex */
	lseek(fd, textoff, SEEK_SET);
	if (verbose) {
		if (head.text) {
			printf("text:\n");
			dumpseg(head.text, head.textoff);
		}
		if (head.data) {
			printf("data:\n");
			dumpseg(head.data, head.dataoff);
		}
	}
	/* process instructions to build xrefs and generate labels */
	lseek(fd, textoff, SEEK_SET);
	process_instr(head.text);	
	if (head.data) {
		printf("data:\n");
		dumpseg(head.data, head.dataoff);
	}
	/* list */
}

void
makerelocs(unsigned char seg)
{
	struct ws_reloc *rp;
	struct reloc *r;

	location = 0;

	while ((rp = getreloc(&relocp)) != 0) {
		r = malloc(sizeof(struct reloc));		
		r->seg = seg;
		r->rl.offset = rp->offset;
		r->rl.value = rp->value;
		r->rl.type = rp->type;
		r->next = rlhead;
		rlhead = r;
	}
}

void
dumprelocs()
{
	struct reloc *r;

	for (r = rlhead; r; r = r->next) {
		printf("%s:%04x ", r->seg ? "DATA" : "TEXT", r->rl.offset);
		switch (r->rl.type) {
		case REL_TEXTOFF:
			printf("text relative\n");
			break;
		case REL_DATAOFF:
			printf("data relative\n");
			break;
		case REL_SYMBOL:
			if (r->rl.value > nsyms) {
				printf("out of bounds symbol reference %d\n",r->rl.value);
			} else {
				printf("symbol reference %s\n", syms[r->rl.value].name);
			}
			break;
		default:
			printf("getreloc lose type %d\n", r->rl.type);
			break;
		}
	}
}

void
freerelocs()
{
	struct reloc *r;

	while (rlhead) {
		r = rlhead;
		rlhead = r->next;
		free(r);
	}
}

/*
 * process a file from the command line.  if it's an archive, iterate over the members
 */
void
nm(char *oname)
{
	unsigned char magic;
	int i;

	fd = open(oname, 0);
	if (fd < 0) {
		printf("cannot open input\n");
		exit(1);
	}
	read(fd, &magic, sizeof(magic));
	if (magic == OBJECT) {
		printf("%s:\n", oname);
		lseek(fd, 0, SEEK_SET);
		do_object(fd, 0);
		close(fd);
		return;
	} else if (magic != 0x75) {
		printf("bad magic: %x\n", magic);
		close(fd);
		return;
	}
	lseek(fd, 2, SEEK_SET);
	while (1) {
		i = read(fd, &member, sizeof(member));
		if (i < sizeof(member)) {
			close(fd);
			return;
		}
		i = member.len[0] + (member.len[1] << 8);
		if (i == 0) {
			close(fd);
			return;
		}
		printf("%s %s:\n", oname, member.fname);
		do_object(fd, i);
	}
}

void
do_object(int fd, int limit)
{
	struct ws_symbol *sym;
	int i;
	unsigned short value;
	unsigned char flag;

	read(fd, &head, sizeof(head));
	if (verbose) {
		printf("magic %x sym:%d text:%d data:%d textoff:%x dataoff:%x\n",
			(head.ident << 8) + head.conf, 
			head.table, head.text, head.data, head.textoff, head.dataoff);
	}
	if (head.ident != OBJECT) {
		printf("bad object file magic number %x\n", head.ident);
		exit(1);
	}

	/* mark the file positions */
	textoff = lseek(fd, 0, SEEK_CUR);
	dataoff = lseek(fd, head.text, SEEK_CUR);
	lseek(fd, head.data, SEEK_CUR);

	/* read symbol table */
	nsyms = head.table / 12;
	if (nsyms == 0) {
		printf("no name list\n");
		exit(1);
	}
	syms = malloc(nsyms * sizeof(*syms));
	read(fd, syms, nsyms * sizeof(*syms));

	sym = syms;
	for(i = 0; i < nsyms; i++) {
		printf("%5d %9s: ", i, sym->name);
		value = sym->value;
		flag = sym->flag;

		printf("%04x ", value);
		if (flag & SF_GLOBAL)
			printf("global ");
		if (flag & SF_DEF)
			printf("defined ");
		switch (flag & SF_SEG) {
		case SF_TEXT:
			printf("code ");
			break;
		case SF_DATA:
			printf("data ");
			break;
		case 0:
			break;
		default:
			printf("unknown segment");
			break;
		}
		printf("\n");
		sym++;
	}

	/* read in the relocs, which are from the symbol table to logical eof  */
	if (limit == 0) {
		limit = sizeof(relocbuf);
	} else {
		limit -= sizeof(head) + head.text + head.data + head.table;
	}

	i = read(fd, relocbuf, limit);
	if (i < 0) {
		printf("error reading relocs");
		goto out;
	}
	if (head.conf == RELOC) {
		relocp = relocbuf;
		makerelocs(SEG_TEXT);
		makerelocs(SEG_DATA);
	}
	endoff = lseek(fd, 0, SEEK_CUR);

	if (rflag) {
		dumprelocs();
	}
	if (dflag) {
		disassem();
	}
out:
	lseek(fd, endoff, SEEK_SET);
	freerelocs();
	free(syms);
}

int
main(argc, argv)
char **argv;
{
	int i;
	unsigned short value;
	unsigned char flag;

	if (--argc > 0 && *argv[1] == '-') {
		argv++;
		while (*++*argv) switch (**argv) {
		case 'v':
			verbose++;
			continue;
		case 'd':
			dflag++;
			continue;
		case 'r':
			rflag++;
			continue;
		default:
			continue;
		}
		argc--;
	}
	if (argc == 0) {
		nm("a.out");
	} else {
		while (argc--) {
			nm(*++argv);
		}
	}
}
