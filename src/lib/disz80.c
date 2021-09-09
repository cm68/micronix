/*
 * disz80.c
 *
 * a z80 disassembler - brute force with symbols
 */

#include <stdio.h>
#include <string.h>

#include "../include/disz80.h"

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
#define	OP_IDX1	0x80	// ddcb fdcb wierdness, index, then opcode
};

struct optab base_optab[256] = {
	{"NOP", 0}, {"LD BC,%s", OP_DATA}, {"LD (BC),A", 0}, {"INC BC", 0},
	{"INC B", 0}, {"DEC B", 0}, {"LD B,%s", OP_IMM8}, {"RLC A", 0},
	{"EX AF, AF'", 0}, {"ADD HL,BC", 0}, {"LD A,(BC)", 0}, {"DEC BC", 0},
	{"INC C", 0}, {"DEC C", 0}, {"LD C,%s", OP_IMM8}, {"RRC A", 0},

	{"DJNZ %s", OP_PC8}, {"LD DE,%s", OP_DATA}, {"LD (DE),A", 0}, {"INC DE", 0},
	{"INC D", 0}, {"DEC D", 0}, {"LD D,%s", OP_IMM8}, {"RL A", 0},
	{"JR %s", OP_PC8}, {"ADD HL,DE", 0}, {"LD A,(DE)", 0}, {"DEC DE", 0},
	{"INC E", 0}, {"DEC E", 0}, {"LD E,%s", OP_IMM8}, {"RR A", 0},

	{"JR NZ,%s", OP_PC8}, {"LD HL,%s", OP_DATA}, {"LD (%s),HL", OP_DATA|OP_IND}, {"INC HL", 0},
	{"INC H", 0}, {"DEC H", 0}, {"LD H,%s", OP_IMM8}, {"DAA", 0},
	{"JR Z,%s", OP_PC8}, {"ADD HL,HL", 0}, {"LD HL,(%s)", OP_DATA|OP_IND}, {"DEC HL", 0},
	{"INC L", 0}, {"DEC L", 0}, {"LD L,%s", OP_IMM8}, {"CPL", 0},

	{"JR NC,%s", OP_PC8}, {"LD SP,%s", OP_DATA}, {"LD (%s),A", OP_DATA|OP_IND}, {"INC SP", 0},
	{"INC (HL)", 0}, {"DEC (HL)", 0}, {"LD (HL),%s", OP_IMM8}, {"SCF", 0},
	{"JR C,%s", 0}, {"ADD HL,SP", 0}, {"LD A,(%s)", OP_DATA|OP_IND}, {"DEC SP", 0},
	{"INC A", 0}, {"DEC A", 0}, {"LD A,%s", OP_IMM8}, {"CCF", 0},

	{"LD B,B", 0}, {"LD B,C", 0}, {"LD B,D", 0}, {"LD B,E", 0},
	{"LD B,H", 0}, {"LD B,L", 0}, {"LD B,(HL)", 0}, {"LD B,A", 0},
	{"LD C,B", 0}, {"LD C,C", 0}, {"LD C,D", 0}, {"LD C,E", 0},
	{"LD C,H", 0}, {"LD C,L", 0}, {"LD C,(HL)", 0}, {"LD C,A", 0},

	{"LD D,B", 0}, {"LD D,C", 0}, {"LD D,D", 0}, {"LD D,E", 0},
	{"LD D,H", 0}, {"LD D,L", 0}, {"LD D,(HL)", 0}, {"LD D,A", 0},
	{"LD E,B", 0}, {"LD E,C", 0}, {"LD E,D", 0}, {"LD E,E", 0},
	{"LD E,H", 0}, {"LD E,L", 0}, {"LD E,(HL)", 0}, {"LD E,A", 0},

	{"LD H,B", 0}, {"LD H,C", 0}, {"LD H,D", 0}, {"LD H,E", 0},
	{"LD H,H", 0}, {"LD H,L", 0}, {"LD H,(HL)", 0}, {"LD H,A", 0},
	{"LD L,B", 0}, {"LD L,C", 0}, {"LD L,D", 0}, {"LD L,E", 0},
	{"LD L,H", 0}, {"LD L,L", 0}, {"LD L,(HL)", 0}, {"LD L,A", 0},

	{"LD (HL),B", 0}, {"LD (HL),C", 0}, {"LD (HL),D", 0}, {"LD (HL),E", 0},
	{"LD (HL),H", 0}, {"LD (HL),L", 0}, {"HALT", 0}, {"LD (HL),A", 0},
	{"LD A,B", 0}, {"LD A,C", 0}, {"LD A,D", 0}, {"LD A,E", 0},
	{"LD A,H", 0}, {"LD A,L", 0}, {"LD A,(HL)", 0}, {"LD A,A", 0},
	
	{"ADD A,B", 0}, {"ADD A,C", 0}, {"ADD A,D", 0}, {"ADD A,E", 0},
	{"ADD A,H", 0}, {"ADD A,H", 0}, {"ADD A,(HL)", 0}, {"ADD A,A", 0},
	{"ADC A,B", 0}, {"ADC A,C", 0}, {"ADC A,D", 0}, {"ADC A,E", 0},
	{"ADC A,H", 0}, {"ADC A,H", 0}, {"ADC A,(HL)", 0}, {"ADC A,A", 0},

	{"SUB A,B", 0}, {"SUB A,C", 0}, {"SUB A,D", 0}, {"SUB A,E", 0},
	{"SUB A,H", 0}, {"SUB A,H", 0}, {"SUB A,(HL)", 0}, {"SUB A,A", 0},
	{"SBC A,B", 0}, {"SBC A,C", 0}, {"SBC A,D", 0}, {"SBC A,E", 0},
	{"SBC A,H", 0}, {"SBC A,H", 0}, {"SBC A,(HL)", 0}, {"SBC A,A", 0},

	{"AND A,B", 0}, {"AND A,C", 0}, {"AND A,D", 0}, {"AND A,E", 0},
	{"AND A,H", 0}, {"AND A,H", 0}, {"AND A,(HL)", 0}, {"AND A,A", 0},
	{"XOR A,B", 0}, {"XOR A,C", 0}, {"XOR A,D", 0}, {"XOR A,E", 0},
	{"XOR A,H", 0}, {"XOR A,H", 0}, {"XOR A,(HL)", 0}, {"XOR A,A", 0},

	{"OR A,B", 0}, {"OR A,C", 0}, {"OR A,D", 0}, {"OR A,E", 0},
	{"OR A,H", 0}, {"OR A,H", 0}, {"OR A,(HL)", 0}, {"OR A,A", 0},
	{"CP A,B", 0}, {"CP A,C", 0}, {"CP A,D", 0}, {"CP A,E", 0},
	{"CP A,H", 0}, {"CP A,H", 0}, {"CP A,(HL)", 0}, {"CP A,A", 0},
	
	{"RET NZ", 0}, {"POP BC", 0}, {"JP NZ,%s", OP_TEXT}, {"JP %s", OP_TEXT},
	{"CALL NZ,%s", OP_TEXT}, {"PUSH BC", 0}, {"ADD A,%s", OP_IMM8}, {"RST 0", 0},
	{"RET Z", 0}, {"RET", 0}, {"JP Z,%s", OP_TEXT}, {0, 0},
	{"CALL Z,%s", OP_TEXT}, {"CALL %s", OP_TEXT}, {"ADC A,%s", OP_IMM8}, {"SYS %s", OP_SYS},

	{"RET NC", 0}, {"POP DE", 0}, {"JP NC,%s", OP_TEXT}, {"OUT (%s),A", OP_IMM8|OP_IND},
	{"CALL NC,%s", OP_TEXT}, {"PUSH DE", 0}, {"SUB A,%s", OP_IMM8}, {"RST 10", 0},
	{"RET C", 0}, {"EXX", 0}, {"JP C,%s", OP_TEXT}, {"IN A,(%s)", OP_IMM8|OP_IND},
	{"CALL C,%s", OP_TEXT}, {0, 0}, {"SBC A,%s", OP_IMM8}, {"RST 18", 0},

	{"RET PO", 0}, {"POP HL", 0}, {"JP PO,%s", OP_TEXT}, {"EX (SP),HL", 0},
	{"CALL PO,%s", OP_TEXT}, {"PUSH HL", 0}, {"AND A,%s", OP_IMM8}, {"RST 20", 0},
	{"RET PE", 0}, {"JP (HL)", 0}, {"JP PE,%s", OP_TEXT}, {"EX DE,HL", 0},
	{"CALL PE,%s", OP_TEXT}, {0, 0}, {"XOR A,%s", OP_IMM8}, {"RST 28", 0},

	{"RET P", 0}, {"POP AF", 0}, {"JP P,%s", OP_TEXT}, {"DI", 0},
	{"CALL P,%s", OP_TEXT}, {"PUSH AF", 0}, {"OR A,%s", OP_IMM8}, {"RST 30", 0},
	{"RET M", 0}, {"LD SP,HL", 0}, {"JP M,%s", OP_TEXT}, {"EI", 0},
	{"CALL M,%s", OP_TEXT}, {0, 0}, {"CP A,%s", OP_IMM8}, {"RST 38", 0}
};

struct optab dd_optab[256] = {
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {"ADD IX,BC", 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {"ADD IX,DE", 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {"LD IX,%s", OP_DATA}, {"LD (%s),IX", OP_DATA|OP_IND}, {"INC IX", 0}, 
	{"INC IXH", 0}, {"DEC IXH", 0}, {"LD IXH,%s", OP_IMM8}, {0, 0}, 
	{0, 0}, {"ADD IX,IX", 0}, {"LD IX,(%s)", OP_DATA|OP_IND}, {"DEC IX", 0}, 
	{"INC IXL", 0}, {"DEC IXL", 0}, {"LD IXL,%s", OP_IMM8}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"INC (IX%s)", OP_IDX}, {"DEC (IX%s)", OP_IDX}, {"LD (IX%s),%s", OP_IDX|OP_IMM8}, {0, 0}, 
	{0, 0}, {"ADD IX,SP", 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD B,IXH", 0}, {"LD B,IXL", 0}, {"LD B,(IX%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD C,IXH", 0}, {"LD C,IXL", 0}, {"LD C,(IX%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD D,IXH", 0}, {"LD D,IXL", 0}, {"LD D,(IX%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD E,IXH", 0}, {"LD E,IXL", 0}, {"LD E,(IX%s)", OP_IDX}, {0, 0}, 

	{"LD IXH, B", 0}, {"LD IXH, C", 0}, {"LD IXH, D", 0}, {"LD IXH, E", 0}, 
	{"LD IXH, IXH", 0}, {"LD IXH, IXL", 0}, {"LD H, (IX%s)", OP_IDX}, {"LD IXH, A", 0}, 
	{"LD IXL, B", 0}, {"LD IXL, C", 0}, {"LD IXL, D", 0}, {"LD IXL, E", 0}, 
	{"LD IXL, IXL", 0}, {"LD IXL, IXL", 0}, {"LD L, (IX%s)", OP_IDX}, {"LD IXL, A", 0}, 

	{"LD (IX%s), B", OP_IDX}, {"LD (IX%s), C", OP_IDX}, 
	{"LD (IX%s), D", OP_IDX}, {"LD (IX%s), E", OP_IDX}, 
	{"LD (IX%s), H", OP_IDX}, {"LD (IX%s), L", OP_IDX}, 
	{0, 0}, {"LD (IX%s), A", OP_IDX}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD A, IXH", 0}, {"LD A, IXL", 0}, {"LD A, (IX%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"ADD A, IXH", 0}, {"ADD A, IXL", 0}, {"ADD A, (IX%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"ADC A, IXH", 0}, {"ADC A, IXL", 0}, {"ADC A, (IX%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"SUB A, IXH", 0}, {"SUB A, IXL", 0}, {"SUB A, (IX%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"SBC A, IXH", 0}, {"SBC A, IXL", 0}, {"SBC A, (IX%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"AND A, IXH", 0}, {"AND A, IXL", 0}, {"AND A, (IX%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"XOR A, IXH", 0}, {"XOR A, IXL", 0}, {"XOR A, (IX%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"OR A, IXH", 0}, {"OR A, IXL", 0}, {"OR A, (IX%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"CP A, IXH", 0}, {"CP A, IXL", 0}, {"CP A, (IX%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {"POP IX", 0}, {0, 0}, {"EX (SP),IX", 0}, 
	{0, 0}, {"PUSH IX", 0}, {0, 0}, {0, 0}, 
	{0, 0}, {"JP (IX)", 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {"LD SP,IX", 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
};

struct optab fd_optab[256] = {
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {"ADD IY,BC", 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {"ADD IY,DE", 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {"LD IY,%s", OP_DATA}, {"LD (%s),IY", OP_DATA|OP_IND}, {"INC IY", 0}, 
	{"INC IYH", 0}, {"DEC IYH", 0}, {"LD IYH,%s", OP_IMM8}, {0, 0}, 
	{0, 0}, {"ADD IY,IY", 0}, {"LD IY,(%s)", OP_DATA|OP_IND}, {"DEC IY", 0}, 
	{"INC IYL", 0}, {"DEC IYL", 0}, {"LD IYL,%s", OP_IMM8}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"INC (IY%s)", OP_IDX}, {"DEC (IY%s)", OP_IDX}, {"LD (IY%s),%s", OP_IDX|OP_IMM8}, {0, 0}, 
	{0, 0}, {"ADD IY,SP", 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD B,IYH", 0}, {"LD B,IYL", 0}, {"LD B,(IY%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD C,IYH", 0}, {"LD C,IYL", 0}, {"LD C,(IY%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD D,IYH", 0}, {"LD D,IYL", 0}, {"LD D,(IY%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD E,IYH", 0}, {"LD E,IYL", 0}, {"LD E,(IY%s)", OP_IDX}, {0, 0}, 

	{"LD IYH, B", 0}, {"LD IYH, C", 0}, {"LD IYH, D", 0}, {"LD IYH, E", 0}, 
	{"LD IYH, IYH", 0}, {"LD IYH, IYL", 0}, {"LD H, (IY%s)", OP_IDX}, {"LD IYH, A", 0}, 
	{"LD IYL, B", 0}, {"LD IYL, C", 0}, {"LD IYL, D", 0}, {"LD IYL, E", 0}, 
	{"LD IYL, IYL", 0}, {"LD IYL, IYL", 0}, {"LD L, (IY%s)", OP_IDX}, {"LD IYL, A", 0}, 

	{"LD (IY%s), B", OP_IDX}, {"LD (IY%s), C", OP_IDX}, 
	{"LD (IY%s), D", OP_IDX}, {"LD (IY%s), E", OP_IDX}, 
	{"LD (IY%s), H", OP_IDX}, {"LD (IY%s), L", OP_IDX}, 
	{0, 0}, {"LD (IY%s), A", OP_IDX}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LD A, IYH", 0}, {"LD A, IYL", 0}, {"LD A, (IY%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"ADD A, IYH", 0}, {"ADD A, IYL", 0}, {"ADD A, (IY%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"ADC A, IYH", 0}, {"ADC A, IYL", 0}, {"ADC A, (IY%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"SUB A, IYH", 0}, {"SUB A, IYL", 0}, {"SUB A, (IY%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"SBC A, IYH", 0}, {"SBC A, IYL", 0}, {"SBC A, (IY%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"AND A, IYH", 0}, {"AND A, IYL", 0}, {"AND A, (IY%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"XOR A, IYH", 0}, {"XOR A, IYL", 0}, {"XOR A, (IY%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"OR A, IYH", 0}, {"OR A, IYL", 0}, {"OR A, (IY%s)", OP_IDX}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"CP A, IYH", 0}, {"CP A, IYL", 0}, {"CP A, (IY%s)", OP_IDX}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {"POP IY", 0}, {0, 0}, {"EX (SP),IY", 0}, 
	{0, 0}, {"PUSH IY", 0}, {0, 0}, {0, 0}, 
	{0, 0}, {"JP (IY)", 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {"LD SP,IY", 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
};

struct optab cb_optab[256] = {
	{"RLC B", 0}, {"RLC C", 0}, {"RLC D", 0}, {"RLC E", 0}, 
	{"RLC H", 0}, {"RLC L", 0}, {"RLC (HL)", 0}, {"RLC A", 0}, 
	{"RRC B", 0}, {"RRC C", 0}, {"RRC D", 0}, {"RRC E", 0}, 
	{"RRC H", 0}, {"RRC L", 0}, {"RRC (HL)", 0}, {"RRC A", 0}, 

	{"RL B", 0}, {"RL C", 0}, {"RL D", 0}, {"RL E", 0}, 
	{"RL H", 0}, {"RL L", 0}, {"RL (HL)", 0}, {"RL A", 0}, 
	{"RR B", 0}, {"RR C", 0}, {"RR D", 0}, {"RR E", 0}, 
	{"RR H", 0}, {"RR L", 0}, {"RR (HL)", 0}, {"RR A", 0}, 

	{"SLA B", 0}, {"SLA C", 0}, {"SLA D", 0}, {"SLA E", 0}, 
	{"SLA H", 0}, {"SLA L", 0}, {"SLA (HL)", 0}, {"SLA A", 0}, 
	{"SRA B", 0}, {"SRA C", 0}, {"SRA D", 0}, {"SRA E", 0}, 
	{"SRA H", 0}, {"SRA L", 0}, {"SRA (HL)", 0}, {"SRA A", 0}, 

	{"SLL B", 0}, {"SLL C", 0}, {"SLL D", 0}, {"SLL E", 0}, 
	{"SLL H", 0}, {"SLL L", 0}, {"SLL (HL)", 0}, {"SLL A", 0}, 
	{"SRL B", 0}, {"SRL C", 0}, {"SRL D", 0}, {"SRL E", 0}, 
	{"SRL H", 0}, {"SRL L", 0}, {"SRL (HL)", 0}, {"SRL A", 0}, 

	{"BIT 0,B", 0}, {"BIT 0,C", 0}, {"BIT 0,D", 0},    {"BIT 0,E", 0}, 
	{"BIT 0,H", 0}, {"BIT 0,L", 0}, {"BIT 0,(HL)", 0}, {"BIT 0,A", 0}, 
	{"BIT 1,B", 0}, {"BIT 1,C", 0}, {"BIT 1,D", 0},    {"BIT 1,E", 0}, 
	{"BIT 1,H", 0}, {"BIT 1,L", 0}, {"BIT 1,(HL)", 0}, {"BIT 1,A", 0}, 
	{"BIT 2,B", 0}, {"BIT 2,C", 0}, {"BIT 2,D", 0},    {"BIT 2,E", 0}, 
	{"BIT 2,H", 0}, {"BIT 2,L", 0}, {"BIT 2,(HL)", 0}, {"BIT 2,A", 0}, 
	{"BIT 3,B", 0}, {"BIT 3,C", 0}, {"BIT 3,D", 0},    {"BIT 3,E", 0}, 
	{"BIT 3,H", 0}, {"BIT 3,L", 0}, {"BIT 3,(HL)", 0}, {"BIT 3,A", 0}, 
	{"BIT 4,B", 0}, {"BIT 4,C", 0}, {"BIT 4,D", 0},    {"BIT 4,E", 0}, 
	{"BIT 4,H", 0}, {"BIT 4,L", 0}, {"BIT 4,(HL)", 0}, {"BIT 4,A", 0}, 
	{"BIT 5,B", 0}, {"BIT 5,C", 0}, {"BIT 5,D", 0},    {"BIT 5,E", 0}, 
	{"BIT 5,H", 0}, {"BIT 5,L", 0}, {"BIT 5,(HL)", 0}, {"BIT 5,A", 0}, 
	{"BIT 6,B", 0}, {"BIT 6,C", 0}, {"BIT 6,D", 0},    {"BIT 6,E", 0}, 
	{"BIT 6,H", 0}, {"BIT 6,L", 0}, {"BIT 6,(HL)", 0}, {"BIT 6,A", 0}, 
	{"BIT 7,B", 0}, {"BIT 7,C", 0}, {"BIT 7,D", 0},    {"BIT 7,E", 0}, 
	{"BIT 7,H", 0}, {"BIT 7,L", 0}, {"BIT 7,(HL)", 0}, {"BIT 7,A", 0}, 

	{"RES 0,B", 0}, {"RES 0,C", 0}, {"RES 0,D", 0},    {"RES 0,E", 0}, 
	{"RES 0,H", 0}, {"RES 0,L", 0}, {"RES 0,(HL)", 0}, {"RES 0,A", 0}, 
	{"RES 1,B", 0}, {"RES 1,C", 0}, {"RES 1,D", 0},    {"RES 1,E", 0}, 
	{"RES 1,H", 0}, {"RES 1,L", 0}, {"RES 1,(HL)", 0}, {"RES 1,A", 0}, 
	{"RES 2,B", 0}, {"RES 2,C", 0}, {"RES 2,D", 0},    {"RES 2,E", 0}, 
	{"RES 2,H", 0}, {"RES 2,L", 0}, {"RES 2,(HL)", 0}, {"RES 2,A", 0}, 
	{"RES 3,B", 0}, {"RES 3,C", 0}, {"RES 3,D", 0},    {"RES 3,E", 0}, 
	{"RES 3,H", 0}, {"RES 3,L", 0}, {"RES 3,(HL)", 0}, {"RES 3,A", 0}, 
	{"RES 4,B", 0}, {"RES 4,C", 0}, {"RES 4,D", 0},    {"RES 4,E", 0}, 
	{"RES 4,H", 0}, {"RES 4,L", 0}, {"RES 4,(HL)", 0}, {"RES 4,A", 0}, 
	{"RES 5,B", 0}, {"RES 5,C", 0}, {"RES 5,D", 0},    {"RES 5,E", 0}, 
	{"RES 5,H", 0}, {"RES 5,L", 0}, {"RES 5,(HL)", 0}, {"RES 5,A", 0}, 
	{"RES 6,B", 0}, {"RES 6,C", 0}, {"RES 6,D", 0},    {"RES 6,E", 0}, 
	{"RES 6,H", 0}, {"RES 6,L", 0}, {"RES 6,(HL)", 0}, {"RES 6,A", 0}, 
	{"RES 7,B", 0}, {"RES 7,C", 0}, {"RES 7,D", 0},    {"RES 7,E", 0}, 
	{"RES 7,H", 0}, {"RES 7,L", 0}, {"RES 7,(HL)", 0}, {"RES 7,A", 0}, 

	{"SET 0,B", 0}, {"SET 0,C", 0}, {"SET 0,D", 0},    {"SET 0,E", 0}, 
	{"SET 0,H", 0}, {"SET 0,L", 0}, {"SET 0,(HL)", 0}, {"SET 0,A", 0}, 
	{"SET 1,B", 0}, {"SET 1,C", 0}, {"SET 1,D", 0},    {"SET 1,E", 0}, 
	{"SET 1,H", 0}, {"SET 1,L", 0}, {"SET 1,(HL)", 0}, {"SET 1,A", 0}, 
	{"SET 2,B", 0}, {"SET 2,C", 0}, {"SET 2,D", 0},    {"SET 2,E", 0}, 
	{"SET 2,H", 0}, {"SET 2,L", 0}, {"SET 2,(HL)", 0}, {"SET 2,A", 0}, 
	{"SET 3,B", 0}, {"SET 3,C", 0}, {"SET 3,D", 0},    {"SET 3,E", 0}, 
	{"SET 3,H", 0}, {"SET 3,L", 0}, {"SET 3,(HL)", 0}, {"SET 3,A", 0}, 
	{"SET 4,B", 0}, {"SET 4,C", 0}, {"SET 4,D", 0},    {"SET 4,E", 0}, 
	{"SET 4,H", 0}, {"SET 4,L", 0}, {"SET 4,(HL)", 0}, {"SET 4,A", 0}, 
	{"SET 5,B", 0}, {"SET 5,C", 0}, {"SET 5,D", 0},    {"SET 5,E", 0}, 
	{"SET 5,H", 0}, {"SET 5,L", 0}, {"SET 5,(HL)", 0}, {"SET 5,A", 0}, 
	{"SET 6,B", 0}, {"SET 6,C", 0}, {"SET 6,D", 0},    {"SET 6,E", 0}, 
	{"SET 6,H", 0}, {"SET 6,L", 0}, {"SET 6,(HL)", 0}, {"SET 6,A", 0}, 
	{"SET 7,B", 0}, {"SET 7,C", 0}, {"SET 7,D", 0},    {"SET 7,E", 0}, 
	{"SET 7,H", 0}, {"SET 7,L", 0}, {"SET 7,(HL)", 0}, {"SET 7,A", 0}
};

struct optab ddcb_optab[256] = {
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RLC (IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RRC (IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RL (IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RR (IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SLA (IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SRA (IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SRL (IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 0,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 1,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 2,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 3,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 4,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 5,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 6,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 7,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 0,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 1,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 2,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 3,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 4,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 5,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 6,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 7,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 0,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 1,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 2,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 3,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 4,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 5,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 6,(IX%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 7,(IX%s)", OP_IDX1}, {0, 0}
};

struct optab fdcb_optab[256] = {
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RLC (IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RRC (IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RL (IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RR (IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SLA (IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SRA (IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SRL (IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 0,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 1,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 2,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 3,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 4,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 5,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 6,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"BIT 7,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 0,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 1,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 2,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 3,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 4,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 5,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 6,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"RES 7,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 0,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 1,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 2,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 3,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 4,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 5,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 6,(IY%s)", OP_IDX1}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {"SET 7,(IY%s)", OP_IDX1}, {0, 0}
};

struct optab ed_optab[256] = {
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{"IN B,(c)", 0}, {"OUT (C),B", 0}, {"SBC HL,BC", 0}, {"LD (%s),BC", OP_DATA}, 
	{"NEG", 0}, {"RETN", 0}, {"IM 0", 0}, {"LD I,A", 0}, 
	{"IN C,(c)", 0}, {"OUT (C),C", 0}, {"ADC HL,BC", 0}, {"LD BC,(%s)", OP_DATA}, 
	{0, 0}, {"RETI", 0}, {0, 0}, {"LD R,A", 0}, 

	{"IN D,(c)", 0}, {"OUT (C),D", 0}, {"SBC HL,DE", 0}, {"LD (%s),DE", OP_DATA}, 
	{0, 0}, {"RETN", 0}, {"IM 1", 0}, {"LD A,I", 0}, 
	{"IN E,(c)", 0}, {"OUT (C),E", 0}, {"ADC HL,DE", 0}, {"LD DE,(%s)", OP_DATA}, 
	{0, 0}, {"RETN", 0}, {"IM 2", 0}, {"LD A,R", 0}, 

	{"IN H,(c)", 0}, {"OUT (C),H", 0}, {"SBC HL,HL", 0}, {0, 0}, 
	{0, 0}, {"RETN", 0}, {"IM 0", 0}, {"RRD", 0}, 
	{"IN L,(c)", 0}, {"OUT (C),L", 0}, {"ADC HL,HL", 0}, {0, 0}, 
	{0, 0}, {"RETN", 0}, {0, 0}, {"RLD", 0}, 

	{0, 0}, {0, 0}, {"SBC HL,SP", 0}, {"LD (%s),sp", OP_DATA}, 
	{0, 0}, {"RETN", 0}, {"IM 1", 0}, {0, 0}, 
	{"IN A,(c)", 0}, {"OUT (C),A", 0}, {"ADC HL,SP", 0}, {"LD SP,(%s)", OP_DATA}, 
	{0, 0}, {"RETN", 0}, {"IM 2", 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{"LDI", 0}, {"CPI", 0}, {"INI", 0}, {"OUTI", 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LDD", 0}, {"CPD", 0}, {"IND", 0}, {"OUTD", 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{"LDIR", 0}, {"CPIR", 0}, {"INIR", 0}, {"OTIR", 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{"LDDR", 0}, {"CPDR", 0}, {"INDR", 0}, {"OTDR", 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 

	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
};

/*
 * format an instruction
 */
int					/* how many bytes we consumed */
format_instr(
	unsigned short addr,
	char *outbuf, 			/* where to put the output */
	unsigned char (*get_byte)(unsigned short addr),	/* how to read bytes */
	char * (*get_sym)(symaddr_t offset),
	unsigned int (*get_reloc)(symaddr_t offset),
	int (*sc)(unsigned short addr, unsigned char (*gb)(unsigned short a), char *out))
{
	char bcount = 0;
	unsigned char opcode;
	char signed_byte;
	unsigned short value;
	unsigned long reloc;
	int oflags;

	char *symname = 0;
	struct optab *o;
	char i;
	char valbuf[60];
	char disp[6];

	char o1, o2, o3, o4;

	o1 = o2 = o3 = o4 = 0xff;

	o1 = 
	opcode = (*get_byte)(addr + bcount++);

	switch (opcode) {
	case 0xcb:
		o2 = 
		opcode = (*get_byte)(addr + bcount++);
		o = &cb_optab[opcode];
		break;
	case 0xed:
		o2 = 
		opcode = (*get_byte)(addr + bcount++);
		o = &ed_optab[opcode];
		break;
	case 0xdd:
		o2 = 
		opcode = (*get_byte)(addr + bcount++);
		if (opcode == 0xcb) {
			signed_byte = (char)(*get_byte)(addr + bcount++);
			opcode = (*get_byte)(addr + bcount++);
			o = &ddcb_optab[opcode];
		} else {
			o = &dd_optab[opcode];
		}
		break;
	case 0xfd:
		o2 = 
		opcode = (*get_byte)(addr + bcount++);
		if (opcode == 0xcb) {
			signed_byte = (char)(*get_byte)(addr + bcount++);
			opcode = (*get_byte)(addr + bcount++);
			o = &fdcb_optab[opcode];
		} else {
			o = &fd_optab[opcode];
		}
		break;
	default:
		o = &base_optab[opcode];
		break;
	}

	/*
	 * we now have advanced over the opcode and now need to read operand bytes
	 */
	oflags = o->flags;

	/* micronix system call RST8 - variable length */
	if (oflags & OP_SYS) {
		if (sc) {
			bcount += (*sc)(addr, get_byte, valbuf);
		} else {
			oflags = 0;
		}
	}

	if (oflags & OP_IDX) {
		signed_byte = (char)(*get_byte)(addr + bcount++);
	}

	if (oflags & (OP_IDX | OP_IDX1)) {
		value = signed_byte;
		if (signed_byte < 0) {
			sprintf(disp, "-%d", -signed_byte);
		} else {
			sprintf(disp, "+%d", signed_byte);
		}
	}

	if (oflags & OP_IMM8) {
		o4 =
		opcode = (*get_byte)(addr + bcount++);
		value = opcode;
		sprintf(valbuf, "0x%x", value);
	}

	if (oflags & OP_PC8) {
		signed_byte = (char)(*get_byte)(addr + bcount++);
		if (signed_byte < 0) {
			sprintf(valbuf, ".-%d", -signed_byte);
		} else {
			sprintf(valbuf, ".+%d", signed_byte);
		}
	}

	if (oflags & (OP_TEXT | OP_DATA)) {
		if (get_reloc) {
			reloc = (*get_reloc)(addr + bcount);
			switch (RELTYPE(reloc)) {
			case RL_SYMBOL:
				symname = (*get_sym)(reloc);
				break;
			case RL_TEXT:
				break;
			case RL_DATA:
				break;
			default:
				symname = 0;
				break;
			}
		}
		value = (unsigned char)(*get_byte)(addr + bcount++);
		value += (*get_byte)(addr + bcount++) << 8;
		if (!symname) {
			symname = (*get_sym)(value);
			if (symname) {
				value = 0;
			}
		}
		if (symname) {
			if (value) {
				sprintf(valbuf, "%s+0x%x", symname, value);
			} else {
				strcpy(valbuf, symname);
			}
		} else {
			sprintf(valbuf, "0x%x", value);
		}
	}

	if (oflags & (OP_SYS|OP_DATA|OP_TEXT|OP_PC8|OP_IMM8|OP_IDX|OP_IDX1)) {
		if ((oflags & (OP_IMM8|OP_IDX)) == (OP_IMM8|OP_IDX)) {
			sprintf(outbuf, o->op, disp, valbuf);
		} else if (oflags & (OP_IDX|OP_IDX1)) {
			sprintf(outbuf, o->op, disp);
		} else {
			sprintf(outbuf, o->op, valbuf);
		}
	} else {
		if (o->op) {
			strcpy(outbuf, o->op);
		} else {
			sprintf(outbuf, "foo: %x %x %x %x", o1, o2, o3, o4);
		}
	}
	// printf("format_instr: %x -> %d\n", addr, bcount);
	return bcount;
}
