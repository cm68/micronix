/*
 * Z80 instruction encoding
 * extracted from asm.c
 */
#ifdef linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stdio.h>
#endif

#include "asm.h"

/* instruction type codes */
#define IBASIC      1
#define IBASIC_EXT  2
#define IARITH      3
#define IINCR       4
#define IBITSH      5
#define ISTACK      6
#define IRET        7
#define IJMP        8
#define IJRL        9
#define ICALL       10
#define IRST        11
#define IIN         12
#define IOUT        13
#define IEXCH       14
#define IINTMODE    15
#define ILOAD       16
#define IEND        0

/* arithmetic sub-types */
#define ADD     0
#define UNARY   1
#define CARRY   2

struct instruct {
	unsigned char type;
	char *mnem;
	unsigned char opcode;
	unsigned char arg;
};

struct instruct isr_table[] = {
	/* basic instructions */
	{ IBASIC, "nop", 0x00, 0 },
	{ IBASIC, "rlca", 0x07, 0 },
	{ IBASIC, "rrca", 0x0F, 0 },
	{ IBASIC, "rla", 0x17, 0 },
	{ IBASIC, "rra", 0x1F, 0 },
	{ IBASIC, "daa", 0x27, 0 },
	{ IBASIC, "cpl", 0x2F, 0 },
	{ IBASIC, "scf", 0x37, 0 },
	{ IBASIC, "ccf", 0x3F, 0 },
	{ IBASIC, "halt", 0x76, 0 },
	{ IBASIC, "exx", 0xD9, 0 },
	{ IBASIC, "di", 0xF3, 0 },
	{ IBASIC, "ei", 0xFB, 0 },
	
	/* extended basic instructions */
	{ IBASIC_EXT, "neg", 0x44, 0xED },
	{ IBASIC_EXT, "retn", 0x44, 0xED },
	{ IBASIC_EXT, "reti", 0x4D, 0xED },
	{ IBASIC_EXT, "rrd", 0x67, 0xED },
	{ IBASIC_EXT, "rld", 0x6F, 0xED },
	{ IBASIC_EXT, "ldi", 0xA0, 0xED },
	{ IBASIC_EXT, "cpi", 0xA1, 0xED },
	{ IBASIC_EXT, "ini", 0xA2, 0xED },
	{ IBASIC_EXT, "outi", 0xA3, 0xED },
	{ IBASIC_EXT, "ldd", 0xA8, 0xED },
	{ IBASIC_EXT, "cpd", 0xA9, 0xED },
	{ IBASIC_EXT, "ind", 0xAA, 0xED },
	{ IBASIC_EXT, "outd", 0xAB, 0xED },
	{ IBASIC_EXT, "ldir", 0xB0, 0xED },
	{ IBASIC_EXT, "cpir", 0xB1, 0xED },
	{ IBASIC_EXT, "inir", 0xB2, 0xED },
	{ IBASIC_EXT, "otir", 0xB3, 0xED },
	{ IBASIC_EXT, "lddr", 0xB8, 0xED },
	{ IBASIC_EXT, "cpdr", 0xB9, 0xED },
	{ IBASIC_EXT, "indr", 0xBA, 0xED },
	{ IBASIC_EXT, "otdr", 0xBB, 0xED },
	
	/* arithmetic */
	{ IARITH, "add", 0x80, ADD },
	{ IARITH, "adc", 0x88, CARRY },
	{ IARITH, "sub", 0x90, UNARY },
	{ IARITH, "sbc", 0x98, CARRY },
	{ IARITH, "and", 0xA0, UNARY },
	{ IARITH, "xor", 0xA8, UNARY },
	{ IARITH, "or", 0xB0, UNARY },
	{ IARITH, "cp", 0xB8, UNARY },
	
	/* inc / dec */
	{ IINCR, "inc", 0x04, 0x03 },
	{ IINCR, "dec", 0x05, 0x0B },
	
	/* bit / shift */
	{ IBITSH, "rlc", 0x00, 0 },
	{ IBITSH, "rrc", 0x08, 0 },
	{ IBITSH, "rl", 0x10, 0 },
	{ IBITSH, "rr", 0x18, 0 },
	{ IBITSH, "sla", 0x20, 0 },
	{ IBITSH, "sra", 0x28, 0 },
	{ IBITSH, "sll", 0x30, 0 },
	{ IBITSH, "srl", 0x38, 0 },
	{ IBITSH, "bit", 0x40, 1 },
	{ IBITSH, "res", 0x80, 1 },
	{ IBITSH, "set", 0xC0, 1 },
	
	/* stack ops */
	{ ISTACK, "pop", 0xC1, 0 },
	{ ISTACK, "push", 0xC5, 0 },
	
	/* return */
	{ IRET, "ret", 0xC0, 0xC9 },
	
	/* jump */
	{ IJMP, "jp", 0xC2, 0xE9 },
	
	/* jump relative */
	{ IJRL, "jr", 0x18, 1 },
	{ IJRL, "djnz", 0x10, 0},
	
	/* call */
	{ ICALL, "call", 0xC4, 0xCD },
	
	/* rst */
	{ IRST, "rst", 0xC7, 0 },
	
	/* in */
	{ IIN, "in", 0xDB, 0x40 },
	
	/* out */
	{ IOUT, "out", 0xD3, 0x41 },
	
	/* exchange */
	{ IEXCH, "ex", 0xE3, 0x08 },
	
	/* interrupt mode */
	{ IINTMODE, "im", 0x46, 0x5E },
	
	/* load instructions */
	{ ILOAD, "ld", 0x00, 0x00 },
	
	{ IEND, "", 0x00, 0x00}
};

/* Token definitions needed */
#define T_BIAS  0x80

#define T_B     (T_BIAS + 0)
#define T_C     (T_BIAS + 1)
#define T_D     (T_BIAS + 2)
#define T_E     (T_BIAS + 3)
#define T_H     (T_BIAS + 4)
#define T_L     (T_BIAS + 5)
#define T_HL_I  (T_BIAS + 6)
#define T_A     (T_BIAS + 7)

#define T_BC    (T_BIAS + 8)
#define T_DE    (T_BIAS + 9)
#define T_HL    (T_BIAS + 10)
#define T_SP    (T_BIAS + 11)
#define T_AF    (T_BIAS + 12)
#define T_IX    (T_BIAS + 13)
#define T_IY    (T_BIAS + 14)

#define T_NZ    (T_BIAS + 15)
#define T_Z     (T_BIAS + 16)
#define T_NC    (T_BIAS + 17)
#define T_CR    (T_BIAS + 18)
#define T_PO    (T_BIAS + 19)
#define T_PE    (T_BIAS + 20)
#define T_P     (T_BIAS + 21)
#define T_M     (T_BIAS + 22)

#define T_IXH   (T_BIAS + 23)
#define T_IXL   (T_BIAS + 24)
#define T_IX_D  (T_BIAS + 25)
#define T_IYH   (T_BIAS + 26)
#define T_IYL   (T_BIAS + 27)
#define T_IY_D  (T_BIAS + 28)

#define T_PLAIN (T_BIAS + 29)
#define T_INDIR (T_BIAS + 30)

#define T_SP_I  (T_BIAS + 31)
#define T_BC_I  (T_BIAS + 32)
#define T_DE_I  (T_BIAS + 33)
#define T_IX_I  (T_BIAS + 34)
#define T_IY_I  (T_BIAS + 35)

#define T_I     (T_BIAS + 40)
#define T_R     (T_BIAS + 41)

/* External functions from asm.c */
extern void need();
extern unsigned char operand();
extern void emitbyte();
extern void emit_exp();
extern void emit_imm();
extern unsigned char tok2reg();
extern char match();
extern void gripe();
extern unsigned short cur_address;
extern char pass;
extern char no_relax;
extern void add_jump();
extern int is_relaxed();   /* the jump add_jump has just counted */
extern unsigned char peekchar();
extern unsigned char skipwhite();

#define T_NUM   (T_BIAS + 43)
#define T_C_I   (T_BIAS + 36)

/*
 * store indirect
 * load indirect
 */
int
do_stax(vp)
struct expval *vp;
{
	unsigned char arg;
	struct expval value;

	need(',');
	arg = operand(&value);

	switch (arg) {
	case T_HL:					/* ld (nn), hl */
		emitbyte(0x22);
		break;

	case T_A:					/* ld (nn), a */
		emitbyte(0x32);
		break;

	case T_IX:					/* ld (nn), ix */
		emitbyte(0xDD);
		emitbyte(0x22);
		break;

	case T_IY:					/* ld (nn), iy */
		emitbyte(0xFD);
		emitbyte(0x22);
		break;

	case T_BC:					/* ld (nn), bc */
	case T_DE:					/* ld (nn), de */
	case T_SP:					/* ld (nn), sp */
		emitbyte(0xED);
		emitbyte(0x43 + ((arg - T_BC) << 4));
		break;

	default:
		return 1;
	}
	emit_exp(2, vp);
	return 0;
}

/*
 * 16 bit load
 */
int
do_16i(reg)
unsigned char reg;
{
	unsigned char arg;
	struct expval value;

	/*
	 * correct for ix,iy into hl
	 */
	if (reg == T_IX) {
		emitbyte(0xDD);
		reg = T_HL;
	} else if (reg == T_IY) {
		emitbyte(0xFD);
		reg = T_HL;
	}

	/*
	 * grab a direct or deferred word
	 */
	need(',');
	arg = operand(&value);

	if (arg == T_PLAIN) {
		/*
		 * ld bc|de|hl|sp, nn
		 */
		emitbyte(0x01 + ((reg - T_BC) << 4));
		emit_exp(2, &value);
	} else if (arg == T_INDIR) {
		if (reg == T_HL) {
			emitbyte(0x2A);
		} else {
			/*
			 * ld bc|de|sp, (nn)
			 */
			emitbyte(0xED);
			emitbyte(0x4B + ((reg - T_BC) << 4));
		}
		emit_exp(2, &value);
	} else if (reg == T_SP) {
		/*
		 * ld sp,hl|ix|iy specials
		 */
		switch (arg) {
		case T_HL:
			emitbyte(0xF9);
			break;
		case T_IX:
			emitbyte(0xDD);
			emitbyte(0xF9);
			break;
		case T_IY:
			emitbyte(0xFD);
			emitbyte(0xF9);
			break;
		default:
			return 1;
		}
	} else
		return 1;
	return 0;
}

/*
 * if there is a passed in expval, it's a displacement for the first arg
 * cases:
 * ld a|b|c|d|e|h|l|(hl)|(ix+d)|(iy+d), a|b|c|d|e|h|l|(hl)|(ix+d)|(iy+d)
 * ld a,(bc)|(de)|(nnnn)|i|r
 */
int
do_ldr8(arg, disp)
unsigned char arg;
struct expval *disp;
{
	unsigned char reg;
    struct expval value;
	struct expval *disp_ptr;
	unsigned char arg_reg, reg_reg;
    value.sym = 0;
    disp->sym = 0;

	disp_ptr = 0;

	if (arg == T_IX_D || arg == T_IY_D) {
        disp_ptr = disp;
	}
	need(',');

	reg = operand(&value);

	if (arg >= T_IXH && arg <= T_IY_D) {
		if (arg <= T_IX_D) {
			emitbyte(0xDD);
            /* lose on ld ix*, iy* or ld ix[hl], (ix+d) */
            if (reg >= T_IYH && reg <= T_IY_D)
                return 1;
            if (arg != T_IX_D && reg == T_IX_D)
                return 1;
		} else {
			emitbyte(0xFD);
            /* lose on ld iy*, ix* or ld iy[hl], (iy+d) */
            if (reg >= T_IXH && reg <= T_IX_D)
                return 1;
            if (arg != T_IY_D && reg == T_IY_D)
                return 1;
		}
	} else if (reg >= T_IXH && reg <= T_IY_D) {
		if (arg == T_HL_I)
			return 1;

		if (reg <= T_IX_D) {
			emitbyte(0xDD);
		} else {
			emitbyte(0xFD);
		}
		if (reg == T_IX_D || reg == T_IY_D) {
            disp_ptr = &value;
		} else if (tok2reg(arg) == 4 || tok2reg(arg) == 5)
            /* lose on ld [hl], ix[hl] */
			return 1;
	}

	/*
	 * no (hl),(hl)
	 */
	if (arg == T_HL_I && reg == T_HL_I)
		return 1;

	/* Convert tokens to register numbers for opcode calculation */
	arg_reg = tok2reg(arg);
	reg_reg = tok2reg(reg);

	if (arg_reg <= 7 && reg_reg <= 7) {
		/* reg8->reg8 */
		emitbyte(0x40 + (arg_reg << 3) + reg_reg);
		if (disp_ptr)
			emit_imm(disp_ptr);
	} else if (arg_reg <= 7 && (reg == T_PLAIN)) {
		/* ld reg8, n */
		emitbyte(0x06 + (arg_reg << 3));
		if (disp_ptr)
			emit_imm(disp_ptr);
		emit_imm(&value);
	} else if (arg == T_A) {
		/*
		 * special a loads
		 */
		switch (reg) {
		case T_BC_I:
			emitbyte(0x0A);
			break;

		case T_DE_I:
			emitbyte(0x1A);
			break;

		case T_INDIR:
			emitbyte(0x3A);
			emit_exp(2, &value);
			break;

		case T_I:
			emitbyte(0xED);
			emitbyte(0x57);
			break;

		case T_R:
			emitbyte(0xED);
			emitbyte(0x5F);
			break;

		default:
			return 1;
		}
	} else
		return 1;
	return 0;
}

static char
do_basic(isr)
struct instruct *isr;
{
	emitbyte(isr->opcode);
	return 0;
}

static char
do_basic_ext(isr)
struct instruct *isr;
{
	emitbyte(isr->arg);
	emitbyte(isr->opcode);
	return 0;
}

static char
do_arith(isr)
struct instruct *isr;
{
	unsigned char prim = 0, arg, reg;
	struct expval value;

	arg = operand(&value);

	if (isr->arg == CARRY) {
		if (arg == T_HL) {
			prim = 1;
		} else if (arg != T_A)
			return 1;

		need(',');
		arg = operand(&value);
	} else if (isr->arg == ADD) {
		if (arg == T_HL) {
			prim = 2;
		} else if (arg == T_IX || arg == T_IY) {
			prim = 3;
			reg = arg;
		} else if (arg != T_A)
			return 1;

		need(',');
		arg = operand(&value);

		if (prim == 3 && arg == T_HL)
			return 1;

		if (prim == 3 && arg == reg)
			arg = T_HL;
	} else if (skipwhite() == ',') {
		/*
		 * sub, and, xor, or and cp take one operand - the
		 * accumulator is implicit in Z80 syntax.  "cp a,32" is
		 * the 8080-ish spelling and is not accepted, but it used
		 * to be read as "cp a" with the 32 dropped in silence,
		 * which assembles cleanly and compares the accumulator
		 * with itself.  Two of those were sitting in this tree:
		 * one made every character look like a space to the CP/M
		 * argv parser, the other took the bound off perror's
		 * error-table lookup.  Say so instead.
		 */
		return 1;
	}

	if (prim == 0) {
		if (arg <= T_A) {
			emitbyte(isr->opcode + (arg - T_B));
		} else if (arg >= T_IXH && arg <= T_IX_D) {
			emitbyte(0xDD);
			emitbyte(isr->opcode + (arg - T_IXH) + 4);
			if (arg == T_IX_D)
				emitbyte(value.num.b);
		} else if (arg >= T_IYH && arg <= T_IY_D) {
			emitbyte(0xFD);
			emitbyte(isr->opcode + (arg - T_IYH) + 4);
			if (arg == T_IY_D)
				emitbyte(value.num.b);
		} else if (arg == T_PLAIN) {
			emitbyte(isr->opcode + 0x46);
			emitbyte(value.num.b);
		} else
			return 1;
	} else if (prim == 1) {
		if (arg >= T_BC && arg <= T_SP) {
			emitbyte(0xED);
			emitbyte((0x42 + (isr->opcode == 0x88 ? 8 : 0)) +
					 ((arg - 8) << 4));
		} else
			return 1;
	} else if (prim == 2) {
		if (arg >= T_BC && arg <= T_SP) {
			emitbyte(0x09 + ((arg - 8) << 4));
		} else
			return 1;
	} else if (prim == 3) {
		if (arg == T_HL)
			arg = reg;
		if (arg == reg)
			arg = T_HL;

		if (reg == T_IX)
			emitbyte(0xDD);
		else
			emitbyte(0xFD);

		if (arg >= T_BC && arg <= T_SP) {
			emitbyte(0x09 + ((arg - 8) << 4));
		} else
			return 1;
	}
	return 0;
}

static char
do_incr(isr)
struct instruct *isr;
{
	unsigned char arg;
	struct expval value;

	arg = operand(&value);

	if (arg <= T_A) {
		emitbyte(isr->opcode + ((arg) << 3));
	} else if (arg <= T_SP) {
		emitbyte(isr->arg + ((arg - T_BC) << 4));
	} else if (arg == T_IX) {
		emitbyte(0xDD);
		emitbyte(isr->arg + 0x20);
	} else if (arg == T_IY) {
		emitbyte(0xFD);
		emitbyte(isr->arg + 0x20);
	} else if (arg >= T_IXH && arg <= T_IX_D) {
		emitbyte(0xDD);
		emitbyte(isr->opcode + ((arg - T_IXH + 4) << 3));
		if (arg == T_IX_D)
			emitbyte(value.num.b);
	} else if (arg >= T_IYH && arg <= T_IY_D) {
		emitbyte(0xFD);
		emitbyte(isr->opcode + ((arg - T_IYH + 4) << 3));
		if (arg == T_IY_D)
			emitbyte(value.num.b);
	} else
		return 1;
	return 0;
}

static char
do_bitsh(isr)
struct instruct *isr;
{
	unsigned char arg, reg;
	struct expval value;

	arg = operand(&value);

	reg = 0;
	if (isr->arg) {
		if (arg != T_PLAIN || value.sym)
			return 1;

		if (value.num.b > 7)
			return 1;

		reg = value.num.b;

		need(',');
		arg = operand(&value);
	}

	if (arg == T_IX_D || arg == T_IY_D) {

		if (arg == T_IX_D)
			emitbyte(0xDD);
		else
			emitbyte(0xFD);

		emitbyte(0xCB);

		emitbyte(value.num.b);

		arg = T_HL_I;
		if (peekchar() == ',') {
			need(',');
			arg = operand(&value);

			if (arg == 6)
				arg = 8;
		}
	} else
		emitbyte(0xCB);

	/* Convert register token to register code (0-7) */
	if (arg >= T_B && arg <= T_A)
		arg -= T_B;
	else if (arg == T_HL_I)
		arg = 6;
	else if (arg > 7)
		return 1;

	emitbyte(isr->opcode + arg + (reg << 3));
	return 0;
}

static char
do_stack(isr)
struct instruct *isr;
{
	unsigned char arg;
	struct expval value;

	arg = operand(&value);
	if (arg == T_AF)
		arg = T_SP;

	if (arg >= T_BC && arg <= T_SP) {
		emitbyte(isr->opcode + ((arg - T_BC) << 4));
	} else if (arg == T_IX) {
		emitbyte(0xDD);
		emitbyte(isr->opcode + 0x20);
	} else if (arg == T_IY) {
		emitbyte(0xFD);
		emitbyte(isr->opcode + 0x20);
	} else
		return 1;
	return 0;
}

static char
do_ret(isr)
struct instruct *isr;
{
	unsigned char arg;
	struct expval value;

	arg = operand(&value);

	if (arg == T_C) arg = T_CR;  /* 'c' means carry, not register C */
	if (arg >= T_NZ && arg <= T_M) {
		emitbyte(isr->opcode + ((arg - T_NZ) << 3));
	} else if (arg == 255) {
		emitbyte(isr->arg);
	} else
		return 1;
	return 0;
}

static char
do_jmp(isr)
struct instruct *isr;
{
	unsigned char arg, cond;
	struct expval value;
	unsigned short addr;
	int target, dist;

	arg = operand(&value);

	if (arg == T_C) arg = T_CR;
	if (arg >= T_NZ && arg <= T_M) {
		cond = arg;
		need(',');
		arg = operand(&value);

		/* record jump for relaxation */
		addr = cur_address;
		add_jump(addr, value.sym, value.num.w, cond);

		/* check if relaxed to jr */
		if (is_relaxed()) {
			/* emit jr cc, offset */
			/* jr nz=20, z=28, nc=30, c=38 */
			emitbyte(0x20 + ((cond - T_NZ) << 3));
			/* calculate relative offset */
			if (value.sym)
				target = value.sym->value + value.num.w;
			else
				target = value.num.w;
			dist = target - (cur_address + 1);
			emitbyte(dist & 0xff);
		} else {
			emitbyte(isr->opcode + ((cond - T_NZ) << 3));
			emit_exp(2, &value);
		}
	} else if (arg == T_NUM || arg == T_PLAIN) {
		/* unconditional jp */
		addr = cur_address;
		add_jump(addr, value.sym, value.num.w, 0);

		if (is_relaxed()) {
			/* emit jr offset */
			emitbyte(0x18);
			if (value.sym)
				target = value.sym->value + value.num.w;
			else
				target = value.num.w;
			dist = target - (cur_address + 1);
			emitbyte(dist & 0xff);
		} else {
			emitbyte(isr->opcode + 1);
			emit_exp(2, &value);
		}
	} else if (arg == T_HL_I) {
		emitbyte(isr->arg);
	} else if (arg == T_IX_I) {
		emitbyte(0xDD);
		emitbyte(isr->arg);
	} else if (arg == T_IY_I) {
		emitbyte(0xFD);
		emitbyte(isr->arg);
	} else
		return 1;
	return 0;
}

static char
do_jrl(isr)
struct instruct *isr;
{
	unsigned char arg, reg;
	struct expval value;
	int target, dist;

	arg = operand(&value);

	reg = 0;
	if (isr->arg) {
		if (arg == T_C) arg = T_CR;  /* 'c' means carry, not register C */
		if (arg >= T_NZ && arg <= T_CR) {
			/* conditional jr: base opcode 0x20 + (cond * 8) */
			reg = 0x08 + ((arg - T_NZ) << 3);
			need(',');
			arg = operand(&value);
		} else if (arg != T_NUM && arg != T_PLAIN)
			return 1;
	}

	if (arg != T_PLAIN)
		return 1;

	emitbyte(isr->opcode + reg);
	/* compute PC-relative offset: target - (PC after 2-byte jr) */
	if (value.sym)
		target = value.sym->value + value.num.w;
	else
		target = value.num.w;
	dist = target - (cur_address + 1);
	if (pass == 1 && (dist < -128 || dist > 127))
		gripe("relative jump out of range");
	emitbyte(dist & 0xff);
	return 0;
}

static char
do_call(isr)
struct instruct *isr;
{
	unsigned char arg;
	struct expval value;

	arg = operand(&value);

	if (arg == T_C) arg = T_CR;  /* 'c' means carry, not register C */
	if (arg == 1) arg = T_CR;
	if (arg >= T_NZ && arg <= T_M) {
		emitbyte(isr->opcode + ((arg - T_NZ) << 3));
		need(',');
		operand(&value);  /* get the address */
		emit_exp(2, &value);
	} else if (arg == T_PLAIN) {
		emitbyte(isr->arg);
		emit_exp(2, &value);
	} else
		return 1;
	return 0;
}

static char
do_rst(isr)
struct instruct *isr;
{
	unsigned char arg;
	struct expval value;

	arg = operand(&value);

	if (arg != T_PLAIN || value.num.b & 0x7 || value.num.b > 0x38)
		return 1;

	emitbyte(isr->opcode + value.num.b);
	return 0;
}

static char
do_in(isr)
struct instruct *isr;
{
	unsigned char arg, reg;
	struct expval value;

	arg = operand(&value);

	if (arg == T_C_I) {
		emitbyte(0xED);
		emitbyte(0x70);
		return 0;
	}

	if (arg == T_HL_I || arg > T_A)
		return 1;

	reg = arg;
	need(',');
	arg = operand(&value);

	if (reg == T_A && arg == T_INDIR) {
		emitbyte(isr->opcode);
		emitbyte(value.num.b);
	} else if (arg == T_C_I) {
		emitbyte(0xED);
		emitbyte(0x40 + (reg << 3));
	} else
		return 1;
	return 0;
}

static char
do_out(isr)
struct instruct *isr;
{
	unsigned char arg, reg;
	struct expval value;

	arg = operand(&value);

	if (arg == T_INDIR) {
		reg = value.num.b;
		need(',');
		arg = operand(&value);

		if (arg != T_A)
			return 1;

		emitbyte(isr->opcode);
		emitbyte(reg);
	} else if (arg == T_C_I) {
		need(',');
		arg = operand(&value);

		if (arg == T_HL_I)
			return 1;
		if (arg == T_PLAIN && !value.num.w)
			arg = T_HL_I;

		if (arg > T_A)
			return 1;

		emitbyte(0xED);
		emitbyte(0x41 + (arg << 3));
	} else
		return 1;
	return 0;
}

static char
do_exch(isr)
struct instruct *isr;
{
	unsigned char arg, reg;
	struct expval value;

	reg = operand(&value);
	need(',');
	arg = operand(&value);

	if (reg == T_AF) {
		if (arg == T_AF) {
			need('\'');
			emitbyte(isr->arg);
		} else
			return 1;
	}
	else if (reg == T_DE) {
		if (arg == T_HL) {
			emitbyte(isr->opcode + 0x08);
		} else
			return 1;
	}
	else if (reg == T_SP_I) {
		switch (arg) {
		case T_HL:
			break;
		case T_IX:
			emitbyte(0xDD);
			break;
		case T_IY:
			emitbyte(0xFD);
			break;
		default:
			return 1;
		}
		emitbyte(isr->opcode);
	}
	return 0;
}

static char
do_intmode(isr)
struct instruct *isr;
{
	unsigned char arg;
	struct expval value;

	arg = operand(&value);

	if (arg != T_PLAIN)
		return 1;

	emitbyte(0xED);
	switch (value.num.w) {
	case 0:
	case 1:
		emitbyte(isr->opcode + (value.num.b << 4));
		break;

	case 2:
		emitbyte(isr->arg);
		break;

	default:
		return 1;
	}
	return 0;
}

static char
do_load(isr)
struct instruct *isr;
{
	unsigned char arg, reg;
	struct expval value;

	arg = operand(&value);

	if (arg == T_INDIR) {
		return do_stax(&value);
	}

	if (arg <= T_A || (arg >= T_IXH && arg <= T_IY_D)) {
		return do_ldr8(arg, &value);
	}

	if ((arg >= T_BC && arg <= T_SP) || (arg == T_IX || arg == T_IY)) {
		return do_16i(arg);
	}

	if (arg >= T_BC_I && arg <= T_R) {
		need(',');
		reg = operand(&value);
		if (reg != T_A)
			return 1;

		switch (arg) {
		case T_BC_I:
			emitbyte(0x02);
			break;

		case T_DE_I:
			emitbyte(0x12);
			break;

		case T_I:
			emitbyte(0xED);
			emitbyte(0x47);
			break;

		case T_R:
			emitbyte(0xED);
			emitbyte(0x4F);
			break;
		}
	} else
		return 1;
	return 0;
}

static char (*isr_handlers[])() = {
	0,
	do_basic,
	do_basic_ext,
	do_arith,
	do_incr,
	do_bitsh,
	do_stack,
	do_ret,
	do_jmp,
	do_jrl,
	do_call,
	do_rst,
	do_in,
	do_out,
	do_exch,
	do_intmode,
	do_load
};

/*
 * attempts to assemble an instruction assuming a symbol has just been tokenized
 *
 * in = pointer to string
 * returns 0 if an instruction is not matched, 1 if it is
 */
char
asm_instr(in)
char *in;
{
	register struct instruct *isr;

	/*
	 * The first character decides sixty-eight of the sixty-nine.
	 * This walked the whole table for every instruction in the
	 * file and called match on each entry to be told no; the test
	 * match makes first is the one made here, without the call.
	 */
	for (isr = isr_table; isr->type != IEND; isr++) {
		if (*in != *isr->mnem)
			continue;
		if (match(in, isr->mnem)) {
			if ((*isr_handlers[isr->type])(isr))
				gripe("invalid operand");
			return 1;
		}
	}
	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
