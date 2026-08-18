/*
 * rules.c - Code generation pattern rules
 */
/*
 * No <stddef.h>.  NULL was the only thing wanted from it, and a
 * plain 0 is a null pointer constant wherever one is expected - the
 * header costs 449 bytes of cpp heap for that one macro, in the
 * source that needed the most of it in the whole tree.
 */
/*
 * No expr.h: this file is a table of patterns, not a walker of
 * trees - it names the codes and the rule struct and nothing that
 * takes an expression apart.
 */
#include "pass2.h"
#include "lexops.h"
#include "opcodes.h"
#include "rules.h"

/* Common assembly templates for deduplication */
/*
 * Common instruction sequences are stored once, here, and named in a
 * template by a single byte with the high bit set - the index into
 * fragtab.  emitasm expands them before it interpolates anything, so
 * nothing downstream has to know.
 *
 * This is worth about eight kilobytes.  The templates are the largest
 * single thing in the compiler and they repeat heavily: "or a" alone
 * appeared a hundred times, and nothing pools identical literals.
 */
#define F_ORA          "\201"	/*  or a */
#define F_SBCHLDE      "\202"	/*  sbc hl,de */
#define F_LDLC         "\203"	/*  ld l,c */
#define F_JPPO5        "\204"	/*  jp po,$$+5 */
#define F_INCHL        "\205"	/*  inc hl */
#define F_LDHB         "\206"	/*  ld h,b */
#define F_EXDEHL       "\207"	/*  ex de,hl */
#define F_LDDER        "\210"	/*  ld de,$R */
#define F_ADDHLDE      "\211"	/*  add hl,de */
#define F_LDAH         "\212"	/*  ld a,h */
#define F_XOR80H       "\213"	/*  xor 80h */
#define F_LDLA         "\214"	/*  ld l,a */
#define F_LDALL        "\215"	/*  ld a,($LL) */
#define F_ADDHLHL      "\216"	/*  add hl,hl */
#define F_LDAHL        "\217"	/*  ld a,(hl) */
#define F_LDHA         "\220"	/*  ld h,a */
#define F_LDLA1        "\221"	/*  ld ($L),a */
#define F_PUSHBC       "\222"	/*  push bc */
#define F_JR3          "\223"	/*  jr $$+3 */
#define F_LDAL         "\224"	/*  ld a,l */
#define F_LDDH         "\225"	/*  ld d,h */
#define F_LDEL         "\226"	/*  ld e,l */
#define F_LDHLRH       "\227"	/*  ld (hl),$Rh */
#define F_LDHLRL       "\230"	/*  ld (hl),$Rl */
#define F_LDHLE        "\231"	/*  ld (hl),e */
#define F_POPBC        "\232"	/*  pop bc */
#define F_XORA         "\233"	/*  xor a */
#define F_LDHLL        "\234"	/*  ld hl,$L */
#define F_CPR          "\235"	/*  cp $R */
#define F_JPM5         "\236"	/*  jp m,$$+5 */
#define F_LDHLD        "\237"	/*  ld (hl),d */
#define F_LDDELO       "\240"	/*  ld de,$Lo */
#define F_LDLHL        "\241"	/*  ld ($L),hl */
#define F_SUBR         "\242"	/*  sub $R */
#define F_PUSHLR       "\243"	/*  push $Lr */
#define F_LDLL         "\244"	/*  ld ($L),l */
#define F_LDAL1        "\245"	/*  ld a,($L) */
#define F_LDHHL        "\246"	/*  ld h,(hl) */
#define F_SBCHLBC      "\247"	/*  sbc hl,bc */
#define F_LDAB         "\250"	/*  ld a,b */
#define F_POPHL        "\251"	/*  pop hl */
#define F_CALLLADEC    "\252"	/*  call qdec */
#define F_CALLLAINC    "\253"	/*  call qinc */
#define F_JRNZ3        "\254"	/*  jr nz,$$+3 */
#define F_LDLH         "\255"	/*  ld ($L+),h */
#define F_LDHLR        "\256"	/*  ld hl,$R */
#define F_DECHL        "\257"	/*  dec hl */
#define F_PUSHHL       "\260"	/*  push hl */
#define F_SBCAA        "\261"	/*  sbc a,a */
#define F_LDHLRL1      "\262"	/*  ld hl,$RL */
#define F_LDDB         "\263"	/*  ld d,b */
#define F_LDEC         "\264"	/*  ld e,c */
#define F_POPDE        "\265"	/*  pop de */
#define F_LDHLR2       "\266"	/*  ld (hl),$R2 */
#define F_LDHLR3       "\267"	/*  ld (hl),$R3 */
#define F_LDHLL1       "\270"	/*  ld h,($LL+) */
#define F_LDDEL        "\271"	/*  ld de,($L) */
#define F_LDHLL2       "\272"	/*  ld hl,($L) */
#define F_LDLLL        "\273"	/*  ld l,($LL) */
#define F_SUBE         "\274"	/*  sub e */
#define F_JPM7         "\275"	/*  jp m,$$+7 */
#define F_LDHLA        "\276"	/*  ld (hl),a */
#define F_LDA0         "\277"	/*  ld a,0 */
#define F_LDAC         "\300"	/*  ld a,c */
#define F_LDH0         "\301"	/*  ld h,0 */
#define F_CPE          "\302"	/*  cp e */
#define F_LDHLL3       "\303"	/*  ld hl,($L++) */
#define F_ORHL         "\304"	/*  or (hl) */
#define F_LDUL         "\305"	/*  ld $u,($L+) */
#define F_RLA          "\306"	/*  rla */
/*
 * The other half of a long.  A 32-bit value lives in HL':HL, so every
 * template that touches the high word brackets it in these - see
 * libsrc/libc/QLONG.md.  One byte, and it is the most repeated
 * fragment in the long rules, which is exactly what this table is for.
 */
#define F_EXX          "\307"	/*  exx */

char *fragtab[] = {
	0,
	"\tor a\n",	/* F_ORA */
	"\tsbc hl,de\n",	/* F_SBCHLDE */
	"\tld l,c\n",	/* F_LDLC */
	"\tjp po,$$+5\n",	/* F_JPPO5 */
	"\tinc hl\n",	/* F_INCHL */
	"\tld h,b\n",	/* F_LDHB */
	"\tex de,hl\n",	/* F_EXDEHL */
	"\tld de,$R\n",	/* F_LDDER */
	"\tadd hl,de\n",	/* F_ADDHLDE */
	"\tld a,h\n",	/* F_LDAH */
	"\txor 80h\n",	/* F_XOR80H */
	"\tld l,a\n",	/* F_LDLA */
	"\tld a,($LL)\n",	/* F_LDALL */
	"\tadd hl,hl\n",	/* F_ADDHLHL */
	"\tld a,(hl)\n",	/* F_LDAHL */
	"\tld h,a\n",	/* F_LDHA */
	"\tld ($L),a\n",	/* F_LDLA1 */
	"\tpush bc\n",	/* F_PUSHBC */
	"\tjr $$+3\n",	/* F_JR3 */
	"\tld a,l\n",	/* F_LDAL */
	"\tld d,h\n",	/* F_LDDH */
	"\tld e,l\n",	/* F_LDEL */
	"\tld (hl),$Rh\n",	/* F_LDHLRH */
	"\tld (hl),$Rl\n",	/* F_LDHLRL */
	"\tld (hl),e\n",	/* F_LDHLE */
	"\tpop bc\n",	/* F_POPBC */
	"\txor a\n",	/* F_XORA */
	"\tld hl,$L\n",	/* F_LDHLL */
	"\tcp $R\n",	/* F_CPR */
	"\tjp m,$$+5\n",	/* F_JPM5 */
	"\tld (hl),d\n",	/* F_LDHLD */
	"\tld de,$Lo\n",	/* F_LDDELO */
	"\tld ($L),hl\n",	/* F_LDLHL */
	"\tsub $R\n",	/* F_SUBR */
	"\tpush $Lr\n",	/* F_PUSHLR */
	"\tld ($L),l\n",	/* F_LDLL */
	"\tld a,($L)\n",	/* F_LDAL1 */
	"\tld h,(hl)\n",	/* F_LDHHL */
	"\tsbc hl,bc\n",	/* F_SBCHLBC */
	"\tld a,b\n",	/* F_LDAB */
	"\tpop hl\n",	/* F_POPHL */
	"\tcall qdec\n",	/* F_CALLLADEC */
	"\tcall qinc\n",	/* F_CALLLAINC */
	"\tjr nz,$$+3\n",	/* F_JRNZ3 */
	"\tld ($L+),h\n",	/* F_LDLH */
	"\tld hl,$R\n",	/* F_LDHLR */
	"\tdec hl\n",	/* F_DECHL */
	"\tpush hl\n",	/* F_PUSHHL */
	"\tsbc a,a\n",	/* F_SBCAA */
	"\tld hl,$RL\n",	/* F_LDHLRL1 */
	"\tld d,b\n",	/* F_LDDB */
	"\tld e,c\n",	/* F_LDEC */
	"\tpop de\n",	/* F_POPDE */
	"\tld (hl),$R2\n",	/* F_LDHLR2 */
	"\tld (hl),$R3\n",	/* F_LDHLR3 */
	"\tld h,($LL+)\n",	/* F_LDHLL1 */
	"\tld de,($L)\n",	/* F_LDDEL */
	"\tld hl,($L)\n",	/* F_LDHLL2 */
	"\tld l,($LL)\n",	/* F_LDLLL */
	"\tsub e\n",	/* F_SUBE */
	"\tjp m,$$+7\n",	/* F_JPM7 */
	"\tld (hl),a\n",	/* F_LDHLA */
	"\tld a,0\n",	/* F_LDA0 */
	"\tld a,c\n",	/* F_LDAC */
	"\tld h,0\n",	/* F_LDH0 */
	"\tcp e\n",	/* F_CPE */
	"\tld hl,($L++)\n",	/* F_LDHLL3 */
	"\tor (hl)\n",	/* F_ORHL */
	"\tld $u,($L+)\n",	/* F_LDUL */
	"\trla\n",	/* F_RLA */
	"\texx\n",	/* F_EXX */
};

#define T_SAVE_HL	F_LDDH F_LDEL
#define T_ADD_HL_DE	F_ADDHLDE
#define T_ADD_HL_HL	F_ADDHLHL
#define T_IDX_S_ST	F_LDLL F_LDLH
#define T_IDX_S_LD	"\tld l,($R)\n\tld h,($R+)\n"
#define T_IX_TEST	"\tld a,ixl\n\tor ixh\n"
#define T_BC_TEST	F_LDAC "\tor b\n"
#define T_HL_TEST	F_LDAL "\tor h\n"
/*
 * A long lives in HL':HL, so all four bytes have to be in the test.
 * A survives exx - only the three pairs are banked - so the fold just
 * carries on across it.
 */
#define T_HLDE_TEST	F_LDAH "\tor l\n" F_EXX "\tor h\n\tor l\n" F_EXX
#define T_BC_HL	F_LDLC F_LDHB
/*
 * Load a word through HL into HL.  A carries the low byte while HL is
 * walked to the high one, because the pointer and the result share the
 * register - which is also why anything that has to write back has to
 * save the address first.
 *
 * CLOBBERS A.  ld e,(hl) / inc hl / ld d,(hl) / ex de,hl is the same
 * four bytes and leaves A alone, but takes DE instead - and DE holds
 * the second operand of a word expression far more often than A holds
 * anything, so this is the safer default.  Anywhere A is live across a
 * word load, that is the form to reach for.
 */
#define T_LD_IHL	F_LDAHL F_INCHL F_LDHHL F_LDLA
/* the word a register-homed struct pointer points at, read and put
 * back: the member is at (ix+0) so neither needs an address */
#define T_IXP_LD	"\tld l,(ix+0)\n\tld h,(ix+1)\n"
#define T_IXP_ST	"\tld (ix+0),l\n\tld (ix+1),h\n"
#define T_SUB_DE	F_ORA F_SBCHLDE
#define T_SUB_BC	F_ORA F_SBCHLBC
/*
 * Turn the result of a signed subtraction into a sign flag: the answer
 * is sign exclusive-or overflow, so flip the top bit of the high byte
 * when P/V says the subtraction overflowed, then let or a set S from
 * it.  M means less than, P means greater or equal.
 */
#define T_SXORV	F_LDAH F_JPPO5 F_XOR80H F_ORA
/*
 * The same correction for a byte, where the difference is already in A
 * so there is nothing to fetch first.  Note the subtraction has to be
 * sub rather than cp: cp keeps A, but the top bit has to be flipped in
 * the value for or a to read the corrected sign back out.  Nothing
 * needs A afterwards - these only ever produce a flag.
 */
#define T_SXORA	F_JPPO5 F_XOR80H F_ORA
/*
 * "greater than" and "less or equal" are not one flag the way "less
 * than" is: they need the sign and the zero together.  Given S and Z
 * set from a corrected difference, send the negative side into an
 * xor a so that both paths arrive with Z meaning false.
 *
 *   J+0  jp m   3   negative
 *   J+3  jr     2   otherwise Z already says whether it was equal
 *   J+5  xor a  1
 *   J+6
 */
#define T_SZTAIL	F_JPM5 F_JR3 F_XORA
/* the address of a frame slot, worked out into HL - left path */
#define T_IDX_ADDR	F_PUSHLR F_POPHL F_LDDELO F_ADDHLDE
/* the same for the right operand, which is where address-of puts it */
#define T_IDX_R_ADDR	"\tpush $Rr\n" F_POPHL "\tld de,$Ro\n" F_ADDHLDE
/* four bytes of a constant written through the address in HL */
#define T_ST_IHL_N	F_LDHLR2 F_INCHL F_LDHLR3 F_INCHL F_LDHLRL F_INCHL F_LDHLRH
/* address on the stack, value in HL -> address in HL, value in DE */
#define T_SWAP_ADDR	F_POPDE F_EXDEHL
/* store DE through HL, then bring the value back to HL */
#define T_ST_IHL	F_LDHLE F_INCHL F_LDHLD F_EXDEHL
#define T_DE_TEST	"\tld a,e\n\tor d\n"

/*
 * llo and rlo are never both given - see struct rule - so they pack
 * into one byte, with RP_SUBR marking a right-hand one.
 */
#define R(o, lo, ro, llo, rlo, sfx, rep, l, r, d, f, tpl, dest) \
	{tpl, o, lo, ro, (llo) | (rlo), sfx, rep, \
	 (l) | ((r) << 2) | ((d) << 4) | ((rlo) ? RP_SUBR : 0), \
	 f, dest}


/*
 * The patterns and templates that more than one rule uses.
 * Each spelling was its own copy in the output - two hundred and
 * thirty-odd of them - and the table is the largest single thing
 * in pass2.  Named once, pointed at many times.
 */
static char RT0[] = "";
static char RT100[] = "\tld a,$R\n" F_LDLA1;
static char RT101[] = "\tld a,($RL)\n";
static char RT108[] = "\tld a,(hl)\n\tadd a,1\n" F_LDHLA F_INCHL "\tld a,(hl)\n\tadc a,0\n" F_LDHLA;
static char RT109[] = "\tld a,(hl)\n\tdec (hl)\n";
static char RT11[] = "$[\tld b,e\n\tinc b\n" "\tjr $$+4\n\tsla a\n\tdjnz $$-2\n$]";
static char RT111[] = "\tld a,(hl)\n\tinc (hl)\n";
static char RT113[] = "\tld a,(hl)\n\tsub 1\n" F_LDHLA F_INCHL "\tld a,(hl)\n\tsbc a,0\n" F_LDHLA;
static char RT12[] = "$[\tld b,e\n\tinc b\n" "\tjr $$+4\n\tsra a\n\tdjnz $$-2\n$]";
static char RT121[] = "\tld a,e\n";
static char RT128[] = "\tld bc,$R\n";
static char RT13[] = "$[\tld b,e\n\tinc b\n" "\tjr $$+4\n\tsrl a\n\tdjnz $$-2\n$]";
static char RT14[] = "$[\tld b,e\n\tld a,$L\n\tinc b\n" "\tjr $$+4\n\tsla a\n\tdjnz $$-2\n$]";
static char RT143[] = "\tld e,(hl)\n" F_INCHL "\tld d,(hl)\n";
static char RT162[] = "\tld ix,$R\n";
static char RT170[] = "\tor e\n";
static char RT174[] = "\tpush ix\n" F_POPHL;
static char RT178[] = "\tpush ix\n" F_POPHL "\tld de,$R\n" F_EXDEHL F_ORA F_SBCHLDE;
static char RT179[] = "\tpush ix\n" F_POPHL "\tld de,$R\n" F_ORA F_SBCHLDE;
static char RT183[] = "\tpush ix\n" F_POPHL F_LDDER F_ORA F_SBCHLDE;
static char RT184[] = "\tpush ix\n" F_POPHL F_ORA "\tsbc hl,bc\n";
static char RT185[] = "\tpush ix\n" F_POPHL F_ORA F_SBCHLBC;
static char RT186[] = "\tpush ix\n" F_POPHL F_ORA F_SBCHLDE;
static char RT187[] = "\tpush ix\n" F_POPHL T_HL_TEST;
static char RT193[] = "\tpush ix\n\tpop de\n" F_ORA F_SBCHLDE;
static char RT197[] = "\txor e\n";
static char RT199[] = F_CPE F_JRNZ3 "\tscf\n";
static char RT200[] = F_CPR;
static char RT201[] = F_CPR F_JRNZ3 "\tscf\n";
static char RT202[] = F_EXDEHL;
static char RT219[] = F_LDAB F_ORA;
static char RT220[] = F_LDAB F_ORA F_JPM7 F_LDAB "\tor c\n" F_JR3 F_XORA;
static char RT221[] = F_LDAC;
static char RT224[] = F_LDAC F_LDLA1;
static char RT226[] = F_LDAC F_ORA;
static char RT228[] = F_LDAH F_ORA;
static char RT229[] = F_LDAH F_ORA F_JPM7 F_LDAH "\tor l\n" F_JR3 F_XORA;
static char RT235[] = F_LDAL;
static char RT254[] = F_LDAL1;
static char RT26[] = "%(\tsra h\n\trr l\n)";
static char RT269[] = F_LDALL F_CPR F_JRNZ3 "\tscf\n";
static char RT271[] = F_LDALL F_SUBR T_SXORA;
static char RT272[] = F_LDALL F_SUBR T_SXORA T_SZTAIL;
static char RT275[] = F_LDDER;
static char RT28[] = "%(\tsrl h\n\trr l\n)";
static char RT285[] = F_LDDER F_EXDEHL T_SUB_DE T_SXORV;
static char RT287[] = F_LDDER F_ORA F_SBCHLDE;
static char RT288[] = F_LDDER F_ORA F_SBCHLDE F_JRNZ3 "\tscf\n";
static char RT290[] = F_LDDER T_SUB_DE T_SXORV;
static char RT300[] = F_LDHLL "\tdec (hl)\n";
static char RT302[] = F_LDHLL "\tinc (hl)\n";
static char RT31[] = "\tadd a,e\n";
static char RT312[] = F_LDHLL F_ORA F_SBCHLDE;
static char RT313[] = F_LDHLL F_CALLLADEC;
static char RT315[] = F_LDHLL F_CALLLAINC;
static char RT317[] = F_LDHLL T_SUB_BC T_SXORV;
static char RT318[] = F_LDHLL T_SUB_DE T_SXORV;
static char RT323[] = F_LDHLR;
static char RT324[] = F_LDHLR F_LDLHL;
static char RT325[] = F_LDHLR T_IDX_S_ST;
static char RT33[] = "\tand $R\n";
static char RT337[] = F_LDLA1;
static char RT35[] = "\tand e\n";
static char RT358[] = F_ORA;
static char RT359[] = F_ORA F_SBCHLBC;
static char RT360[] = F_ORA F_SBCHLDE;
static char RT361[] = F_ORA T_SZTAIL;
static char RT363[] = F_CALLLADEC;
static char RT364[] = F_CALLLAINC;
static char RT380[] = F_SUBE;
static char RT381[] = F_SUBE T_SXORA;
static char RT382[] = F_SUBE T_SXORA T_SZTAIL;
static char RT384[] = F_SUBR T_SXORA;
static char RT385[] = F_SUBR T_SXORA T_SZTAIL;
static char RT388[] = T_BC_HL;
static char RT390[] = T_BC_HL "%(\tsra h\n\trr l\n)";
static char RT391[] = T_BC_HL "%(\tsrl h\n\trr l\n)";
static char RT396[] = T_BC_HL "\tpush ix\n\tpop de\n" F_ORA F_SBCHLDE;
static char RT41[] = "\tcp b\n";
static char RT411[] = T_BC_HL F_LDDER F_ORA F_SBCHLDE;
static char RT412[] = T_BC_HL F_LDDER F_ORA F_SBCHLDE F_JRNZ3 "\tscf\n";
static char RT413[] = T_BC_HL F_LDDER T_SUB_DE T_SXORV;
static char RT415[] = T_BC_HL F_ORA F_SBCHLDE;
static char RT416[] = T_BC_HL F_CALLLADEC;
static char RT418[] = T_BC_HL F_CALLLAINC;
static char RT42[] = "\tcp c\n";
static char RT425[] = T_BC_HL T_SUB_DE T_SXORV;
static char RT426[] = T_BC_TEST;
static char RT427[] = T_DE_TEST;
static char RT429[] = T_HL_TEST;
static char RT430[] = T_IDX_ADDR F_CALLLADEC;
static char RT432[] = T_IDX_ADDR F_CALLLAINC;
static char RT44[] = "\tdec ($L)\n";
static char RT443[] = T_IX_TEST;
static char RT461[] = T_SUB_BC T_SXORV;
static char RT462[] = T_SUB_DE T_SXORV;
static char RT48[] = "\tdec b\n";
static char RT50[] = "\tdec bc\n";
static char RT52[] = "\tdec c\n";
static char RT54[] = "\tdec ix\n";
static char RT57[] = "\tinc ($L)\n";
static char RT61[] = "\tinc b\n";
static char RT63[] = "\tinc bc\n";
static char RT65[] = "\tinc c\n";
static char RT67[] = "\tinc ix\n";
static char RT69[] = "\tld $T,$L\n";
static char RT71[] = "\tld $t,($L)\n" F_LDUL;
/*
 * Storing a long, which lives in HL':HL with the low word in HL and
 * the HIGH word at the lower address - see QLONG.md and NUXI.  So the
 * exx comes first: the high word goes down, then the low word above
 * it, and the pair is left the way round it arrived, which is what
 * lets the value forms share these.
 */
static char RT85[] = F_EXX "\tld ($L),hl\n" F_EXX "\tld ($L++),hl\n";
/*
 * A long stored to a frame slot or through a struct pointer.  The
 * displacement rides inline after the call - see libsrc/libc/qldst.s -
 * which is four bytes against the fourteen the four byte-moves and the
 * exx pair used to take.  $Lr names the index register, so one
 * template serves IY and IX and the rule takes RF_IXIY.
 */
static char RT87[] = "\tcall qst$Lr\n\t.db $Lo\n";

#ifdef DEBUG
/* the patterns as written, for the rule trace */
char *rulepat[] = {
	"L",
	"L",
	"S",
	"V",
	"V",
	"V",
	"VF",
	"BF",
	"V:bF",
	"V:bF",
	"=(V,num):b",
	"=(V,num):b",
	"=(V,A):b",
	"=(V,A):b",
	"=(V,H):b",
	"=(V,H):b",
	"=(H,V):b",
	"=(H,V):b",
	"V:b",
	"V:b",
	"H:lF",
	"HF",
	"EF",
	"AF",
	"=(H,V)",
	"=(B,V)",
	"=(E,V)",
	"+(V,num)",
	"+(V,num)",
	"+(D(V),num)",
	"+(I,num)",
	"+(I,H)",
	"+(I,E)",
	"+(I,B)",
	"+(O,num)",
	"-(O,num)",
	"+(O,H)",
	"+(O,E)",
	"+(O,B)",
	"*(any,pow2)",
	"*(H,mul3)",
	"*(H,mul5)",
	"*(H,mul6)",
	"*(H,mul7)",
	"*(H,mul9)",
	"*(H,mul1)",
	"*(H,mul1)",
	"*(H,mul1)",
	"*(H,mul1)",
	"*(H,mul1)",
	"*(H,mul2)",
	"*(H,mul2)",
	"*(H,mul4)",
	"*(H,E)",
	"*(H,B)",
	"*(H,num)",
	"*(B,E)",
	"/(H,E)",
	"/(H,E)",
	"/(B,E)",
	"/(B,E)",
	"%(B,E)",
	"%(B,E)",
	"/(H,num)",
	"/(H,num)",
	"%(H,num)",
	"%(H,num)",
	"%(H,E)",
	"%(H,E)",
	"/(num,E)",
	"/(num,E)",
	"%(num,E)",
	"%(num,E)",
	"/(H,B)",
	"/(H,B)",
	"%(H,B)",
	"%(H,B)",
	"=(I,num):bV",
	"=(I,num):b",
	"=(I,num):sV",
	"=(I,num):s",
	"=(I,H):b",
	"=(I,H):s",
	"=(I,I):s",
	"=(I,E):s",
	"=(I,B):s",
	"=(O,A):b",
	"=(O,num):b",
	"=(D(num),num):s",
	"=(D(num),O):s",
	"=(D(num),num):b",
	"=(D(num),H):s",
	"=(D(num),A):b",
	"=(D(num),B):s",
	"=(D(num),E):s",
	"=(D(num),B):b",
	"=(D(num),H):lV",
	"=(D(num),H):l",
	"=(D(num),C):lV",
	"=(D(num),C):l",
	"=(D(num),num):l",
	"=(D(num),num):lV",
	"=(O,H):s",
	"=(O,H):b",
	"=(O,B):b",
	"=(O,B):s",
	"=(I,B):bV",
	"=(I,B):b",
	"=(O,num):s",
	"=(O,A):s",
	"=(I,A):s",
	"=(V,num)",
	"=(V,num)",
	"=(V,num)",
	"=(V,num)",
	"=(H,num):l",
	"=(B,num)",
	"=(E,num)",
	"=(H,num)",
	"=(V,O)",
	"=(O,V)",
	"=(I,V):s",
	"=(V,I)",
	"-(V,num)",
	"-(V,O)",
	"+(V,O)",
	"+(V,E)",
	"-(V,E)",
	"-(V,B)",
	"+(V,B)",
	"-(B,V)",
	"+(B,V)",
	"-(H,V)",
	"+(H,V)",
	"e(V,zero)",
	"n(V,zero)",
	"cmp(V,num)",
	"cmpx(V,num)",
	"cmp(V,O)",
	"cmpx(V,O)",
	"e(V,B)",
	"n(V,B)",
	"e(V,H)",
	"n(V,H)",
	"cmp(V,E)",
	"cmpx(V,E)",
	"cmp(H,V)",
	"cmpx(H,V)",
	"cmp(B,V)",
	"cmpx(B,V)",
	"cmp(V,B)",
	"cmpx(V,B)",
	"=(V,H)",
	"=(V,E)",
	"=(V,B)",
	"=(B,H)",
	"=(E,H)",
	"=(H,E)",
	"=(H,B)",
	"=(B,E)",
	"=(E,B)",
	"=(B,B)",
	"=(E,E)",
	"=(H,H)",
	";(any,H)",
	";(any,E)",
	";(any,B)",
	";(any,A)",
	"=(C,H)",
	"=(C,E)",
	"=(C,B)",
	"=(C,A)",
	"W(A):s",
	"W(H):s",
	"X(V):s",
	"W(V):s",
	"X(H):s",
	"X(B):s",
	"W(B):s",
	"X(I):s",
	"W(I):s",
	"X(O):s",
	"W(O):s",
	"W(H):s",
	"X(H):s",
	"X(A):s",
	"Z(H):s",
	"Z(H):b",
	"Z(H):b",
	"Z(B):b",
	"Z(E):b",
	"Z(A)",
	"Z(K)",
	"Z(H)",
	"X(H):l",
	"X(B):l",
	"X(A):l",
	"W(H):l",
	"W(B):l",
	"W(A):l",
	"=(O,H):lV",
	"=(I,H):lV",
	"=(O,C):lV",
	"=(I,C):lV",
	"=(O,H):l",
	"=(I,H):l",
	"=(I,C):l",
	"=(O,C):l",
	"=(I,num):lV",
	"=(I,num):l",
	"=(O,num):lV",
	"=(O,num):l",
	"=(D(O),num):bV",
	"=(D(O),num):sV",
	"=(D(O),num):lV",
	"=(H,C):l",
	"D(O):lF",
	"D(O):l",
	"D(I):l",
	"D(H):l",
	"D(B):l",
	"j(O):l",
	"k(O):l",
	"i(O):lS",
	"d(O):lS",
	"i(O):l",
	"d(O):l",
	"j(I):l",
	"k(I):l",
	"i(I):lS",
	"d(I):lS",
	"i(I):l",
	"d(I):l",
	"j(H):l",
	"k(H):l",
	"i(H):lS",
	"d(H):lS",
	"i(H):l",
	"d(H):l",
	"j(D(B)):l",
	"k(D(B)):l",
	"i(D(B)):lS",
	"d(D(B)):lS",
	"i(D(B)):l",
	"d(D(B)):l",
	"=(D(O),num):b",
	"=(D(O),num):s",
	"=(D(O),A):b",
	"=(D(O),H):b",
	"=(D(O),E):s",
	"=(D(O),H):s",
	"=(D(O),O):s",
	"=(D(O),B):s",
	"=(D(H),O):s",
	"=(D(I),O):s",
	"=(D(O),num):l",
	"=(D(H),num):lV",
	"=(D(H),num):l",
	"=(D(O),H):l",
	"=(D(I),H):l",
	"=(D(I),num):l",
	"=(D(V),H):l",
	"=(D(V),num):l",
	"=(D(B),H):l",
	"~(H):s",
	"~(B):s",
	"~(A):b",
	"!(H)",
	"!(H)",
	"!(H)",
	"!(A)",
	"!(B)",
	"!(B)",
	"!(E)",
	"!(K)",
	"!(V)",
	"=(B,A)",
	"=(H,A)",
	"=(E,A)",
	"+(B,num)",
	"+(B,smal)",
	"+(E,num)",
	"+(E,smal)",
	"m(A):b",
	"m(B)",
	"m(H)",
	"m(E)",
	"i(B)S",
	"d(B)S",
	"j(B)S",
	"k(B)S",
	"i(V):bS",
	"i(V):bS",
	"d(V):bS",
	"d(V):bS",
	"j(V):bS",
	"j(V):bS",
	"k(V):bS",
	"k(V):bS",
	"i(B)",
	"d(B)",
	"i(O):s",
	"d(O):s",
	"j(O):s",
	"k(O):s",
	"i(H):s",
	"d(H):s",
	"j(H):s",
	"k(H):s",
	"i(D(B)):b",
	"d(D(B)):b",
	"j(D(B)):b",
	"k(D(B)):b",
	"i(D(H)):b",
	"d(D(H)):b",
	"j(D(H)):b",
	"k(D(H)):b",
	"i(D(B)):s",
	"d(D(B)):s",
	"j(D(B)):s",
	"k(D(B)):s",
	"j(B)",
	"k(B)",
	"i(V)S",
	"d(V)S",
	"j(V)S",
	"k(V)S",
	"i(V)",
	"d(V)",
	"j(V)",
	"k(V)",
	"i(D(V)):s",
	"d(D(V)):s",
	"j(D(V)):s",
	"k(D(V)):s",
	"j(I):s",
	"k(I):s",
	"i(I):s",
	"d(I):s",
	"i(I):bS",
	"d(I):bS",
	"j(I):bS",
	"k(I):bS",
	"i(I):bF",
	"d(I):bF",
	"j(I):bF",
	"k(I):bF",
	"i(I):b",
	"d(I):b",
	"j(I):b",
	"k(I):b",
	"j(D(V)):b",
	"k(D(V)):b",
	"i(D(V)):b",
	"d(D(V)):b",
	"i(O):bF",
	"d(O):bF",
	"i(O):bS",
	"d(O):bS",
	"j(O):bS",
	"k(O):bS",
	"j(O):b",
	"k(O):b",
	"i(O):b",
	"d(O):b",
	"=(I,A)",
	"=(H,num)",
	"=(H,A):b",
	"=(H,V):b",
	"D(H):b",
	"D(H):s",
	"D(H):s",
	"D(B):b",
	"D(B):s",
	"D(B):s",
	"D(E):b",
	"D(E):s",
	"=(D(B),A):b",
	"=(D(E),A):b",
	"=(D(B),H):s",
	"=(D(E),H):s",
	"=(D(V),num):s",
	"=(D(V),H):s",
	"=(D(V),B):s",
	"=(D(V),num):b",
	"=(D(V),A):b",
	"=(D(V),H):b",
	"=(D(V),B):b",
	"=(D(V),E):b",
	"=(D(H),V):s",
	"=(D(H),V):b",
	"=(D(B),V):s",
	"=(D(I),V):s",
	"=(D(O),V):s",
	"=(D(I),num):s",
	"=(D(I),H):s",
	"=(D(I),num):b",
	"=(D(I),A):b",
	"=(D(I),H):b",
	"=(D(I),B):b",
	"=(D(I),B):s",
	"=(D(I),E):b",
	"=(D(I),E):s",
	"=(D(H),num):bV",
	"=(D(H),num):b",
	"=(D(H),E):sV",
	"=(D(H),E):s",
	"=(D(H),E):b",
	"=(D(H),A):b",
	"=(D(H),B):b",
	"=(D(H),H):b",
	"j(H):b",
	"k(H):b",
	"i(H):b",
	"d(H):b",
	"i(H):sS",
	"d(H):sS",
	"j(H):sS",
	"k(H):sS",
	"=(D(B),num):b",
	"=(D(B),H):b",
	"=(D(B),num):s",
	"=(D(E),num):s",
	"=(D(H),num):sV",
	"=(D(H),num):s",
	"=(D(H),B):s",
	"D(H):?F",
	"=(B,D(H)):s",
	"=(B,I)",
	"=(H,I)",
	"=(E,I)",
	"=(I,O)",
	"=(O,O)",
	"=(B,O)",
	"=(H,O)",
	"=(E,O)",
	"=(B,D(O)):s",
	"=(E,D(O)):s",
	"=(H,D(O)):s",
	"=(E,D(H)):s",
	"=(H,D(H)):s",
	"=(I,D(H)):s",
	"?OREQ(H,num):b",
	"?OREQ(I,K):b",
	"D(I):bF",
	"D(I):sF",
	"D(I):s",
	"D(num):s",
	"D(num):b",
	"D(num):l",
	"D(O):b",
	"D(O):b",
	"D(I):b",
	"D(I):b",
	"D(V):b",
	"D(V):s",
	"D(V):l",
	"D(O):s",
	"+(H,E)",
	"+(H,B)",
	"-(H,B)",
	"-(num,B)",
	"-(num,E)",
	"-(num,H)",
	"-(num,K):b",
	"-(num,A):b",
	"+(B,E)",
	"-(B,E)",
	"-(B,O)",
	"-(B,num)",
	"y(B,num)",
	"/(B,num)",
	"/(B,num)",
	"%(B,num)",
	"%(B,num)",
	"*(B,num)",
	"+(H,smal)",
	"-(H,smal)",
	"+(A,smal)",
	"-(A,smal)",
	"+(H,num)",
	"+(A,num):b",
	"+(D(I),num):b",
	"-(A,num):b",
	"-(D(I),num):b",
	"-(H,E)",
	"-(O,E)",
	"-(H,O)",
	"+(H,O)",
	"-(H,num)",
	"y(H,num)",
	"y(A,num):b",
	"y(num,K):b",
	"y(num,E):b",
	"y(num,H):b",
	"y(A,K):b",
	"y(A,E):b",
	"y(A,H):b",
	"w(A,K):b",
	"w(A,E):b",
	"w(A,H):b",
	"w(A,K):b",
	"w(A,E):b",
	"w(A,H):b",
	"w(A,num):b",
	"w(A,num):b",
	"w(H,eigh)",
	"w(H,eigh)",
	"y(H,eigh)",
	"w(H,smal)",
	"w(H,smal)",
	"w(B,smal)",
	"w(B,smal)",
	"w(H,num)",
	"w(H,num)",
	"y(H,E)",
	"y(H,B)",
	"w(H,E)",
	"w(H,E)",
	"w(H,B)",
	"w(H,B)",
	"w(B,num)",
	"w(B,num)",
	"=(A,D(I)):b",
	"=(A,D(O)):b",
	"=(A,A)",
	"=(A,num):b",
	"=(A,H):b",
	"=(A,B):b",
	"=(A,E):b",
	"i(V):bF",
	"i(V):bF",
	"d(V):bF",
	"d(V):bF",
	"i(V):b",
	"i(V):b",
	"d(V):b",
	"d(V):b",
	"j(V):b",
	"j(V):b",
	"k(V):b",
	"k(V):b",
	"i(A):b",
	"d(A):b",
	"=(O,E):s",
	"&(D(I),pow2):bF",
	"&(D(V),pow2):bF",
	"&(D(H),pow2):bF",
	"&(A,pow2):bF",
	"&(D(I),num):b",
	"|(D(I),num):b",
	"^(D(I),num):b",
	"+(A,D(O)):b",
	"-(A,D(O)):b",
	"&(A,D(O)):b",
	"|(A,D(O)):b",
	"^(A,D(O)):b",
	"+(A,D(I)):b",
	"-(A,D(I)):b",
	"&(A,D(I)):b",
	"|(A,D(I)):b",
	"^(A,D(I)):b",
	"+(A,K):b",
	"-(A,K):b",
	"&(A,K):b",
	"|(A,K):b",
	"^(A,K):b",
	"+(H,K):b",
	"-(H,K):b",
	"&(H,K):b",
	"|(H,K):b",
	"^(H,K):b",
	"+(A,E):b",
	"-(A,E):b",
	"&(A,E):b",
	"|(A,E):b",
	"^(A,E):b",
	"&(A,num):bF",
	"&(A,num):b",
	"&(A,K):bF",
	"|(A,num):b",
	"^(A,num):b",
	"&(H,num)",
	"|(H,num)",
	"^(H,num)",
	"&(B,num)",
	"|(B,num)",
	"^(B,num)",
	"&(H,E)",
	"|(H,E)",
	"^(H,E)",
	"&(H,B)",
	"|(H,B)",
	"^(H,B)",
	"&(B,E)",
	"|(B,E)",
	"^(B,E)",
	"<(H,zero)",
	"p(H,zero)",
	"<(B,zero)",
	"p(B,zero)",
	">(H,zero)",
	"q(H,zero)",
	">(B,zero)",
	"q(B,zero)",
	"<(H,E)",
	"p(H,E)",
	"cmpx(H,E)",
	"<(O,E)",
	"p(O,E)",
	"cmpx(O,E)",
	"<(O,B)",
	"p(O,B)",
	"cmpx(O,B)",
	"<(H,O)",
	"p(H,O)",
	"cmpx(H,O)",
	"<(H,B)",
	"p(H,B)",
	"cmpx(H,B)",
	"<(H,num)",
	"p(H,num)",
	"cmpx(H,num)",
	"e(C,E)",
	"n(C,E)",
	"e(C,O)",
	"n(C,O)",
	"e(H,E)",
	"cmpx(H,E)",
	"n(H,E)",
	"<(H,E)",
	"p(H,E)",
	"cmpx(O,E)",
	"<(O,E)",
	"p(O,E)",
	"cmpx(H,O)",
	"<(H,O)",
	"p(H,O)",
	"cmp(H,B)",
	"cmpx(H,B)",
	"e(B,E)",
	"n(B,E)",
	"<(B,E)",
	"p(B,E)",
	"cmpx(B,E)",
	"<(B,E)",
	"p(B,E)",
	"e(B,num)",
	"n(B,num)",
	"<(B,num)",
	"p(B,num)",
	"cmpx(B,num)",
	"<(B,num)",
	"p(B,num)",
	"e(H,O)",
	"n(H,O)",
	"cmp(H,num)",
	"<(A,zero)",
	"p(A,zero)",
	">(A,zero)",
	"q(A,zero)",
	"<(A,K)",
	"p(A,K)",
	">(A,K)",
	"q(A,K)",
	"<(A,num)",
	"p(A,num)",
	">(A,num)",
	"q(A,num)",
	"<(D(I),num):b",
	"p(D(I),num):b",
	">(D(I),num):b",
	"q(D(I),num):b",
	"cmp(A,K)",
	"e(A,num)",
	"n(A,num)",
	"e(A,V)",
	"n(A,V)",
	"e(A,V)",
	"n(A,V)",
	"<(A,V)",
	"q(A,V)",
	"<(A,V)",
	"q(A,V)",
	"<(A,V)",
	"q(A,V)",
	"<(A,V)",
	"q(A,V)",
	"<(A,num)",
	"p(A,num)",
	"cmp(D(I),num):b",
	">(A,K)",
	"q(A,K)",
	">(A,num)",
	"q(A,num)",
	">(D(I),num):b",
	"q(D(I),num):b",
	">(H,num)",
	"q(H,num)",
	">(B,num)",
	"q(B,num)",
	"cmpx(B,E)",
	"n(any,num)",
	"n",
	"=(O,I)",
	0,		/* the end: this array is SHORTER than rules[] */
};
#endif

/*
 * SORTED BY ROOT OPCODE, and it has to stay that way: step subscripts
 * ruleidx by the node's op to find where this op's run begins, and
 * walks it while the op holds.  Adding a rule means putting it with
 * its op, not at the end.  ruleindex() builds the index from the
 * table at startup, so the two cannot disagree, but a rule filed in
 * the wrong place would simply never be tried.
 *
 * Within a run the order is the priority: first match wins, and
 * tryrule can alter the node before it fails, so the order of a run
 * is not to be disturbed.  Between runs there is no relationship -
 * a rule rooted at one opcode can never match a node carrying
 * another.
 *
 * Which is why nothing is rooted at a pattern any more.  There were
 * 29 rows rooted at P_CMP and P_CMPX, one standing for four
 * comparisons or for two; they are written out per opcode now, 49
 * rows more, about 640 bytes.  A pattern at the root is a rule that
 * cannot be filed under an opcode, and it kept the table from being
 * an index - which is worth a great deal more than the rows it saved:
 * 666 million cycles to 209 million on our worst file.  Patterns in
 * the operand positions are untouched; only the root has to be
 * literal.
 */
struct rule rules[] = {

	/*
	 * Comma: both sides have already emitted their code, in order,
	 * and the value is the right one - so there is nothing left to do
	 * but say where it ended up.  ';' rather than ',' because the
	 * pattern parser uses the comma to separate children.
	 */
	R(COMMA,P_ANY,INHL,0,0,0, COMMA, P_L, P_R, P_NONE, 0, RT0, R_HL),
	R(COMMA,P_ANY,INDE,0,0,0, COMMA, P_L, P_R, P_NONE, 0, RT0, R_DE),
	R(COMMA,P_ANY,INBC,0,0,0, COMMA, P_L, P_R, P_NONE, 0, RT0, R_BC),
	R(COMMA,P_ANY,INA,0,0,0, COMMA, P_L, P_R, P_NONE, 0, RT0, R_A),

	/* bare SYM -> SYMREF+0, so the address rules below can see it */
	R(SYM,0,0,0,0,0, SYMREF, P_NONE, P_NONE, P_NONE, 0, 0, 0),
	/*
	 * The truth test.  "!x" is true when x is zero, so the answer is
	 * the zero flag once the value has been tested - and testing is
	 * all it takes, whatever the value is sitting in.
	 *
	 * A comparison already produces a flag and is handled before the
	 * rules run, by inverting it; this is the other half, where the
	 * operand is a value and nothing has set the flags.  It had no
	 * rule at any width but long, so every "!x" on an ordinary value
	 * reduced to nothing.
	 *
	 * The width that matters is the operand's: "!" yields an int
	 * whatever it was applied to.
	 */
	R(BANG,INHL,0,0,0,96, BANG, P_L, P_NONE, P_NONE, 0,
		F_LDAH "\tor l\n" F_EXX "\tor h\n\tor l\n" F_EXX, F_Z),
	R(BANG,INHL,0,0,0,64, BANG, P_L, P_NONE, P_NONE, 0, RT429, F_Z),
	R(BANG,INHL,0,0,0,32, BANG, P_L, P_NONE, P_NONE, 0, F_LDAL F_ORA, F_Z),
	R(BANG,INA,0,0,0,0, BANG, P_L, P_NONE, P_NONE, 0, RT358, F_Z),
	R(BANG,INBC,0,0,0,64, BANG, P_L, P_NONE, P_NONE, 0, RT426, F_Z),
	R(BANG,INBC,0,0,0,32, BANG, P_L, P_NONE, P_NONE, 0, RT226, F_Z),
	R(BANG,INDE,0,0,0,64, BANG, P_L, P_NONE, P_NONE, 0, RT427, F_Z),
	R(BANG,INE,0,0,0,0, BANG, P_L, P_NONE, P_NONE, 0, "\tld a,e\n" F_ORA, F_Z),
	R(BANG,REGVAR,0,0,0,0, BANG, P_L, P_NONE, P_L, RF_IX, RT443, F_Z),

	/* strength reduction */
	R(STAR,P_ANY,P_POW2,0,0,0, LSHIFT, P_L, P_R, P_NONE, RF_POW2, 0, 0),

	/* STAR by small constants */
	R(STAR,INHL,P_MUL3,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_DE, R_HL),
	R(STAR,INHL,P_MUL5,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_HL T_ADD_HL_DE, R_HL),
	R(STAR,INHL,P_MUL6,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL, R_HL),
	R(STAR,INHL,P_MUL7,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL T_ADD_HL_DE, R_HL),
	R(STAR,INHL,P_MUL9,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_HL T_ADD_HL_HL T_ADD_HL_DE, R_HL),
	R(STAR,INHL,P_MUL10,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL, R_HL),
	R(STAR,INHL,P_MUL11,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL T_ADD_HL_DE, R_HL),
	R(STAR,INHL,P_MUL12,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL T_ADD_HL_HL, R_HL),
	R(STAR,INHL,P_MUL14,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL, R_HL),
	R(STAR,INHL,P_MUL15,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL T_ADD_HL_DE, R_HL),
	R(STAR,INHL,P_MUL20,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL T_ADD_HL_HL, R_HL),
	R(STAR,INHL,P_MUL24,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL T_ADD_HL_HL T_ADD_HL_HL, R_HL),
	R(STAR,INHL,P_MUL40,0,0,0, STAR, P_L, P_NONE, P_NONE, 0,
		T_SAVE_HL T_ADD_HL_HL T_ADD_HL_HL T_ADD_HL_DE T_ADD_HL_HL T_ADD_HL_HL T_ADD_HL_HL, R_HL),

	/* runtime calls */
	/*
	 * The 16-bit helpers take the left operand in HL and the right in
	 * DE and return in HL.  a- is the signed form and l- the logical
	 * one - not long, whatever the letter suggests elsewhere.
	 *
	 * These were named __mul16, __div16 and __mod16, which the
	 * library has never defined.  Nothing noticed until a program was
	 * linked and run, because a call to a symbol that does not exist
	 * assembles perfectly well.
	 */
	R(STAR,INHL,INDE,0,0,0, STAR, P_L, P_R, P_NONE, 0, "\tcall amul\n", R_HL),
	/* the multiplier in BC: amul wants it in DE, and clobbers BC on
	 * the way, so the save the $[ $] pair makes is what puts a
	 * register variable back */
	R(STAR,INHL,INBC,0,0,0, STAR, P_L, P_R, P_NONE, 0,
		"\tld e,c\n\tld d,b\n$[\tcall amul\n$]", R_HL),
	/*
	 * Any other constant, which has to go through the helper like a
	 * variable would.  The shift-and-add forms above are cheaper and
	 * come first; this is what catches everything they do not name.
	 * There was nothing here at all, so "v * 100" emitted no multiply
	 * - and said so only after the marker learned to look below the
	 * root of a statement.
	 */
	R(STAR,INHL,P_NUM,0,0,0, STAR, P_L, P_R, P_NONE, 0,
		F_LDDER "\tcall amul\n", R_HL),
	/*
	 * The same with the left operand in BC, which a register variable
	 * puts it in.  There was a form for BC times a constant and none
	 * for BC times a value, so "i * a" with i in a register emitted
	 * no multiply at all - and no marker either, the store above it
	 * having matched.
	 *
	 * BC is saved across the call because amul takes its second
	 * operand off the stack with a pop bc and does not put it back.
	 * The register variable that was the left operand is still wanted
	 * afterwards, and when it was the subscript of the loop doing the
	 * multiplying the loop did not end.
	 */
	R(STAR,INBC,INDE,0,0,0, STAR, P_L, P_R, P_NONE, 0,
		T_BC_HL "\tcall amul\n", R_HL),
	R(STAR,INBC,P_NUM,0,0,0, STAR, P_L, P_R, P_NONE, 0,
		T_BC_HL F_LDDER "\tcall amul\n", R_HL),

	/* Address rules: IX+offset -> INDEX */
	R(PLUS,REGVAR,P_NUM,0,0,0, INDEX, P_NONE, P_NONE, P_L, RF_IX, 0, 0),

	/*
	 * The same, past the 7-bit (ix+d) window: form the address with
	 * 16-bit arithmetic (special-cased in tryrule).  Only reached
	 * when the INDEX rule above refuses.
	 *
	 * A struct big enough to need this is ordinary - v6's filsys is
	 * 478 bytes and its later members are all past 127 - and before
	 * this rule existed there was NOTHING for the shape: "p->hi = 2"
	 * with p in IX matched no rule, left an XXXXXX comment, and the
	 * assignment was simply gone from the output.  A comment is not
	 * an instruction and asz never saw it.
	 */
	R(PLUS,REGVAR,P_NUM,0,0,0, CODE, P_NONE, P_NONE, P_L, RF_IX, 0, 0),
	R(PLUS,DEREF,P_NUM,REGVAR,0,0, INDEX, P_NONE, P_NONE, P_LL, RF_IXIY, 0, 0),
	R(PLUS,INDEX,P_NUM,0,0,0, INDEX, P_NONE, P_NONE, P_L, 0, 0, 0),
	/*
	 * Array element with a variable subscript: the base is a frame
	 * slot and the scaled index is already in HL, so form the address
	 * rather than let (ix+d) do it.  A constant subscript never gets
	 * here - +(I,N) above folds it straight into the offset.
	 */
	R(PLUS,INDEX,INHL,0,0,0, PLUS, P_L, P_R, P_NONE, 0,
		F_PUSHLR F_POPDE F_ADDHLDE F_LDDELO F_ADDHLDE, R_HL),
	/* the same with the subscript already in DE, which is where the
	 * reorder leaves it when the index is the costlier side */
	R(PLUS,INDEX,INDE,0,0,0, PLUS, P_L, P_R, P_NONE, 0,
		F_PUSHLR F_POPHL F_ADDHLDE F_LDDELO F_ADDHLDE, R_HL),
	/* and in BC, which is where a register variable subscript sits */
	R(PLUS,INDEX,INBC,0,0,0, PLUS, P_L, P_R, P_NONE, 0,
		F_PUSHLR F_POPHL "\tadd hl,bc\n" F_LDDELO F_ADDHLDE, R_HL),

	/* symbol + constant offset folds into the SYMREF */
	R(PLUS,SYMREF,P_NUM,0,0,0, SYMREF, P_NONE, P_NONE, P_NONE, 0, 0, 0),
	/*
	 * The same for a global array, where the base is a link-time
	 * constant and the scaled subscript is in a register - one add,
	 * with the base loaded into whichever half is free.  A constant
	 * subscript never reaches here: +(O,N) above folds it away.
	 */
	R(PLUS,SYMREF,INHL,0,0,0, PLUS, P_L, P_R, P_NONE, 0, "\tld de,$L\n" F_ADDHLDE, R_HL),
	R(PLUS,SYMREF,INDE,0,0,0, PLUS, P_L, P_R, P_NONE, 0, F_LDHLL F_ADDHLDE, R_HL),
	/*
	 * The same with the subscript in BC, which a register variable
	 * puts it in - "buf[i]" over a global array with i in a register.
	 * There were forms for HL and DE and not for BC, so the address
	 * was never worked out and nothing was emitted for it.
	 */
	R(PLUS,SYMREF,INBC,0,0,0, PLUS, P_L, P_R, P_NONE, 0, F_LDHLL "\tadd hl,bc\n", R_HL),
	R(PLUS,REGVAR,SYMREF,0,0,0, PLUS, P_L, P_R, P_L, RF_IX,
		"\tpush ix\n" F_POPHL F_LDDER F_ADDHLDE, R_HL),
	/*
	 * The index register plus a value rather than a constant or a
	 * symbol - "p[i]" where p is a pointer register variable and i is
	 * worked out.  A constant offset folds into an INDEX and never
	 * reaches here, which is why this was missing: the shape only
	 * turns up when the subscript is not known.  Eleven places in the
	 * tools, counting the ones that then sign-extend the result.
	 */
	R(PLUS,REGVAR,INDE,0,0,0, PLUS, P_L, P_R, P_L, RF_IX,
		"\tpush ix\n" F_POPHL F_ADDHLDE, R_HL),
	R(PLUS,REGVAR,INBC,0,0,0, PLUS, P_L, P_R, P_L, RF_IX,
		"\tpush ix\n" F_POPHL "\tadd hl,bc\n", R_HL),
	R(PLUS,INBC,REGVAR,0,0,0, PLUS, P_L, P_R, P_R, RF_IX,
		T_BC_HL "\tpush ix\n\tpop de\n" F_ADDHLDE, R_HL),
	R(PLUS,INHL,REGVAR,0,0,0, PLUS, P_L, P_R, P_R, RF_IX,
		"\tpush ix\n\tpop de\n" F_ADDHLDE, R_HL),

	/* register base address calculations */
	R(PLUS,INBC,P_NUM,0,0,0, PLUS, P_L, P_R, P_NONE, 0, F_LDLC F_LDHB F_LDDER F_ADDHLDE, R_HL),
	R(PLUS,INBC,P_SMALL,0,0,0, PLUS, P_L, P_R, P_NONE, 0, F_LDLC F_LDHB "%(\tinc hl\n)", R_HL),
	R(PLUS,INDE,P_NUM,0,0,0, PLUS, P_L, P_R, P_NONE, 0, F_EXDEHL F_LDDER F_ADDHLDE, R_HL),
	R(PLUS,INDE,P_SMALL,0,0,0, PLUS, P_L, P_R, P_NONE, 0, F_EXDEHL "%(\tinc hl\n)", R_HL),

	/* 16-bit binary arithmetic */
	R(PLUS,INHL,INDE,0,0,0, PLUS, P_L, P_R, P_NONE, 0, T_ADD_HL_DE, R_HL),
	R(PLUS,INHL,INBC,0,0,0, PLUS, P_L, P_R, P_NONE, 0, "\tadd hl,bc\n", R_HL),
	R(PLUS,INBC,INDE,0,0,0, PLUS, P_L, P_R, P_NONE, 0, T_BC_HL T_ADD_HL_DE, R_HL),
	R(PLUS,INHL,P_SMALL,0,0,0, PLUS, P_L, P_R, P_NONE, 0, "%(\tinc hl\n)", R_HL),
	R(PLUS,INA,P_SMALL,0,0,0, PLUS, P_L, P_R, P_NONE, 0, "%(\tinc a\n)", R_A),
	R(PLUS,INHL,P_NUM,0,0,0, PLUS, P_L, P_R, P_NONE, 0, F_LDDER T_ADD_HL_DE, R_HL),
	/*
	 * A byte in A against a constant, once it is too big for the inc
	 * and dec runs above.  Only at byte width: at word width A holds
	 * the low half and the carry would have nowhere to go.
	 */
	R(PLUS,INA,P_NUM,0,0,1, PLUS, P_L, P_R, P_NONE, 0, "\tadd a,$R\n", R_A),
	R(PLUS,DEREF,P_NUM,INDEX,0,1, PLUS, P_L, P_R, P_NONE, 0, F_LDALL "\tadd a,$R\n", R_A),
	R(PLUS,INHL,SYMREF,0,0,0, PLUS, P_L, P_R, P_NONE, 0, F_LDDER F_ADDHLDE, R_HL),
	/*
	 * Byte arithmetic against a memory operand.  These match on the
	 * parent so that A is known to hold the left operand, which makes
	 * HL free to point at the right one - a rule matching the DEREF
	 * alone cannot know that, and would clobber a word left operand.
	 * The Z80 operates directly on (hl) and (iy+d), so no temporary
	 * register is needed at all.
	 */
	R(PLUS,INA,DEREF,0,SYMREF,1, PLUS, P_L, P_R, P_NONE, 0, F_LDHLRL1 "\tadd a,(hl)\n", R_A),
	R(PLUS,INA,DEREF,0,INDEX,1, PLUS, P_L, P_R, P_NONE, 0, "\tadd a,($RL)\n", R_A),

	/* byte arithmetic with both operands live: left in A, right in E */
	R(PLUS,INA,INE,0,0,1, PLUS, P_L, P_R, P_NONE, 0, RT31, R_A),
	/*
	 * The same five when the left operand is a WORD in HL and only
	 * its low byte is wanted - a short loaded through a pointer,
	 * meeting a byte at byte width.  The high byte never matters to
	 * a truncated result, so the answer is one ld away.  Without
	 * these, "np->flags |= e->flags & E_FUNARG" in the compiler's
	 * own expr.c reduced both operands and then had nowhere to go:
	 * the OR emitted nothing and the store above it was abandoned
	 * with a marker.
	 */
	R(PLUS,INHL,INE,0,0,1, PLUS, P_L, P_R, P_NONE, 0, F_LDAL "\tadd a,e\n", R_A),
	/*
	 * The same operators when the right operand arrived as a word in
	 * DE - a call result moved aside, mostly.  A byte operation only
	 * reads E, which is the low byte, which is the byte.  Without
	 * these "cnt += rec(n)" on a byte counter had no rule, and c0's
	 * cntCondLbls answered 0 under the self-build: every IF in the
	 * stream was emitted claiming no short-circuit labels.
	 */
	R(PLUS,INA,INDE,0,0,1, PLUS, P_L, P_R, P_NONE, 0, RT31, R_A),
	/*
	 * And the same going down.  "tab - 1" had no rule, so it left a
	 * marker and no instruction - rt_ixcmp2 has computed the right
	 * answer all along only because the value it dropped was
	 * reloaded by the comparison that followed.
	 */
	R(MINUS,SYMREF,P_NUM,0,0,0, SYMREF, P_NONE, P_NONE, P_NONE, 0, 0, 0),
	/*
	 * Arithmetic on it comes out in HL, which "=(V,H)" then puts
	 * back.  Only addition of a constant folds into an indexed
	 * location; subtraction has to be done.
	 */
	R(MINUS,REGVAR,P_NUM,0,0,0, MINUS, P_L, P_R, P_L, RF_IX, RT183, R_HL),
	R(MINUS,REGVAR,SYMREF,0,0,0, MINUS, P_L, P_R, P_L, RF_IX, RT183, R_HL),
	/*
	 * And the other way about: a symbol's address less a register
	 * variable.  The register forms above all carry the register on
	 * the left, and "&buffer[512] - bufp" - pr's "how much room is
	 * left" - has the address there instead, so it had no form at
	 * all.  ix into de, the address into hl, subtract the way the
	 * source wrote it.
	 */
	R(MINUS,SYMREF,REGVAR,0,0,0, MINUS, P_L, P_R, P_R, RF_IX,
		"\tpush ix\n" F_POPDE F_LDHLL F_ORA F_SBCHLDE, R_HL),
	/* and the difference, which is how a span is worked out when the
	 * far end is a local rather than the other register home */
	R(MINUS,REGVAR,INDE,0,0,0, MINUS, P_L, P_R, P_L, RF_IX, RT186, R_HL),
	/*
	 * And against the other register home: two pointers walking the
	 * same buffer land one in IX and one in BC, and "p - q" is how a
	 * span length is worked out - outf's literal spans first.
	 */
	R(MINUS,REGVAR,INBC,0,0,0, MINUS, P_L, P_R, P_L, RF_IX, RT185, R_HL),
	R(MINUS,INBC,REGVAR,0,0,0, MINUS, P_L, P_R, P_R, RF_IX, RT396, R_HL),
	/*
	 * And a value already worked out in HL against that home:
	 * "((char *)q - (char *)p) / sizeof(struct store)" in realloc,
	 * where the left side is a difference and only the right is a
	 * register variable.  Missing for the same reason as the pair
	 * above - the shape needs a left operand that is not itself a
	 * home, so nothing in the tree had written one until now.
	 */
	R(MINUS,INHL,REGVAR,0,0,0, MINUS, P_L, P_R, P_R, RF_IX, RT193, R_HL),
	R(MINUS,INHL,INBC,0,0,0, MINUS, P_L, P_R, P_NONE, 0, RT359, R_HL),
	/* a constant less a register variable - "5 - n".  The subtraction
	 * is not commutative, so normalize leaves the constant on the
	 * left and there was no form with it there. */
	R(MINUS,P_NUM,INBC,0,0,0, MINUS, P_L, P_R, P_NONE, 0,
		"\tld hl,$L\n" F_ORA F_SBCHLBC, R_HL),
	/*
	 * And with the value in DE or HL, which is what "0 - (n % 10)"
	 * reduces to: the helper's answer moved aside, the constant on
	 * the left where normalize keeps it.  outd() spells its digits
	 * exactly that way, so the self-built c1 printed every operand
	 * of every instruction as an empty string.
	 */
	R(MINUS,P_NUM,INDE,0,0,0, MINUS, P_L, P_R, P_NONE, 0,
		"\tld hl,$L\n" F_ORA F_SBCHLDE, R_HL),
	R(MINUS,P_NUM,INHL,0,0,0, MINUS, P_L, P_R, P_NONE, 0,
		"\tex de,hl\n\tld hl,$L\n" F_ORA F_SBCHLDE, R_HL),
	R(MINUS,P_NUM,INE,0,0,1, MINUS, P_L, P_R, P_NONE, 0,
		"\tld a,$L\n" F_SUBE, R_A),
	R(MINUS,P_NUM,INA,0,0,1, MINUS, P_L, P_R, P_NONE, 0,
		"\tld e,a\n\tld a,$L\n" F_SUBE, R_A),
	R(MINUS,INBC,INDE,0,0,0, MINUS, P_L, P_R, P_NONE, 0, RT415, R_HL),
	/*
	 * A pointer in BC less the address of an array - "s - buf", the
	 * ordinary pointer difference, when the array is declared with no
	 * size.  Given a size, pass1 puts a conversion over the symbol
	 * and this arrives as -(B,E) above; an array of unknown size has
	 * size zero, the conversion is not inserted because nothing looks
	 * wider than nothing, and the symbol reaches here bare.
	 */
	R(MINUS,INBC,SYMREF,0,0,0, MINUS, P_L, P_R, P_NONE, 0, RT411, R_HL),
	R(MINUS,INBC,P_NUM,0,0,0, MINUS, P_L, P_R, P_NONE, 0, RT411, R_HL),
	R(MINUS,INHL,P_SMALL,0,0,0, MINUS, P_L, P_R, P_NONE, 0, "%(\tdec hl\n)", R_HL),
	R(MINUS,INA,P_SMALL,0,0,0, MINUS, P_L, P_R, P_NONE, 0, "%(\tdec a\n)", R_A),
	R(MINUS,INA,P_NUM,0,0,1, MINUS, P_L, P_R, P_NONE, 0, F_SUBR, R_A),
	R(MINUS,DEREF,P_NUM,INDEX,0,1, MINUS, P_L, P_R, P_NONE, 0, F_LDALL F_SUBR, R_A),
	R(MINUS,INHL,INDE,0,0,0, MINUS, P_L, P_R, P_NONE, 0, RT360, R_HL),
	/*
	 * An address expression minus a pointer fetched from memory -
	 * "&v[3] - p" - reaches here as SYMREF minus INDE, and the
	 * SYMREF side is one immediate load.
	 */
	/*
	 * A symbol's address less a value in bc.  The de form is below
	 * and this was not here, so "&buf[33] - b" with b in bc did not
	 * reduce at all - and nothing said so: the long subtraction
	 * above it was emitted with an operand that had never been
	 * computed, pushing whatever hl happened to hold.
	 *
	 *	t = nd - (&buf[33] - b);	came out 65337, not 77
	 *
	 * The same expression with the pointer in ix compiles, which is
	 * the entry-2 rule beside this one: same shape, different
	 * register, and the table only spelled one of them.
	 */
	R(MINUS,SYMREF,INBC,0,0,0, MINUS, P_L, P_R, P_NONE, 0,
		F_LDHLL T_SUB_BC, R_HL),
	R(MINUS,SYMREF,INDE,0,0,0, MINUS, P_L, P_R, P_NONE, 0,
		"\tld hl,$L\n" F_ORA F_SBCHLDE, R_HL),
	/*
	 * And BOTH sides a symbol's address, which is how a program
	 * measures the gap between two things it was linked with.  The
	 * kernel sizes its buffer pool that way:
	 *
	 *	space = (UINT) (&usrtop) - (UINT) (blist);
	 *
	 * in main.c's binit.  Either half alone had a rule - SYMREF
	 * minus INDE above, INHL minus SYMREF below - and the pair had
	 * none, so the subtraction left a marker and the size came out
	 * as whatever was in HL.
	 */
	R(MINUS,SYMREF,SYMREF,0,0,0, MINUS, P_L, P_R, P_NONE, 0,
		"\tld hl,$L\n" F_LDDER F_ORA F_SBCHLDE, R_HL),
	/* less a symbol's address, which is one half of a pointer
	 * difference once the other half is in HL */
	R(MINUS,INHL,SYMREF,0,0,0, MINUS, P_L, P_R, P_NONE, 0, RT287, R_HL),
	R(MINUS,INHL,P_NUM,0,0,0, MINUS, P_L, P_R, P_NONE, 0, RT287, R_HL),
	R(MINUS,INA,DEREF,0,SYMREF,1, MINUS, P_L, P_R, P_NONE, 0, F_LDHLRL1 "\tsub (hl)\n", R_A),
	R(MINUS,INA,DEREF,0,INDEX,1, MINUS, P_L, P_R, P_NONE, 0, "\tsub ($RL)\n", R_A),
	R(MINUS,INA,INE,0,0,1, MINUS, P_L, P_R, P_NONE, 0, RT380, R_A),
	R(MINUS,INHL,INE,0,0,1, MINUS, P_L, P_R, P_NONE, 0, F_LDAL F_SUBE, R_A),
	R(MINUS,INA,INDE,0,0,1, MINUS, P_L, P_R, P_NONE, 0, RT380, R_A),
	R(DIV,INHL,INDE,0,0,0, DIV, P_L, P_R, P_NONE, RF_SIGNL, "\tcall adiv\n", R_HL),
	R(DIV,INHL,INDE,0,0,0, DIV, P_L, P_R, P_NONE, 0, "\tcall ldiv\n", R_HL),
	/* and with the left operand in BC, as a register variable puts it */
	R(DIV,INBC,INDE,0,0,0, DIV, P_L, P_R, P_NONE, RF_SIGNL,
		T_BC_HL "\tcall adiv\n", R_HL),
	R(DIV,INBC,INDE,0,0,0, DIV, P_L, P_R, P_NONE, 0,
		T_BC_HL "\tcall ldiv\n", R_HL),
	/* by a constant, which is what dividing a pointer difference by
	 * the element size looks like */
	R(DIV,INHL,P_NUM,0,0,0, DIV, P_L, P_R, P_NONE, RF_SIGNL,
		F_LDDER "\tcall adiv\n", R_HL),
	R(DIV,INHL,P_NUM,0,0,0, DIV, P_L, P_R, P_NONE, 0,
		F_LDDER "\tcall ldiv\n", R_HL),
	/*
	 * A constant dividend never got loaded - constants are left for
	 * the fused rules, and division had no fused form - and a
	 * divisor in a register home never moved to where the helpers
	 * read it.
	 */
	R(DIV,P_NUM,INDE,0,0,0, DIV, P_L, P_R, P_NONE, RF_SIGNL,
		"\tld hl,$L\n$[\tcall adiv\n$]", R_HL),
	R(DIV,P_NUM,INDE,0,0,0, DIV, P_L, P_R, P_NONE, 0,
		"\tld hl,$L\n$[\tcall ldiv\n$]", R_HL),
	R(DIV,INHL,INBC,0,0,0, DIV, P_L, P_R, P_NONE, RF_SIGNL,
		F_LDEC F_LDDB "\tcall adiv\n", R_HL),
	R(DIV,INHL,INBC,0,0,0, DIV, P_L, P_R, P_NONE, 0,
		F_LDEC F_LDDB "\tcall ldiv\n", R_HL),
	R(DIV,INBC,P_NUM,0,0,0, DIV, P_L, P_R, P_NONE, RF_SIGNL,
		T_BC_HL F_LDDER "\tcall adiv\n", R_HL),
	R(DIV,INBC,P_NUM,0,0,0, DIV, P_L, P_R, P_NONE, 0,
		T_BC_HL F_LDDER "\tcall ldiv\n", R_HL),
	R(MOD,INBC,INDE,0,0,0, MOD, P_L, P_R, P_NONE, RF_SIGNL,
		T_BC_HL "\tcall amod\n", R_HL),
	R(MOD,INBC,INDE,0,0,0, MOD, P_L, P_R, P_NONE, 0,
		T_BC_HL "\tcall lmod\n", R_HL),
	R(MOD,INHL,P_NUM,0,0,0, MOD, P_L, P_R, P_NONE, RF_SIGNL,
		F_LDDER "\tcall amod\n", R_HL),
	R(MOD,INHL,P_NUM,0,0,0, MOD, P_L, P_R, P_NONE, 0,
		F_LDDER "\tcall lmod\n", R_HL),
	R(MOD,INHL,INDE,0,0,0, MOD, P_L, P_R, P_NONE, RF_SIGNL, "\tcall amod\n", R_HL),
	R(MOD,INHL,INDE,0,0,0, MOD, P_L, P_R, P_NONE, 0, "\tcall lmod\n", R_HL),
	R(MOD,P_NUM,INDE,0,0,0, MOD, P_L, P_R, P_NONE, RF_SIGNL,
		"\tld hl,$L\n$[\tcall amod\n$]", R_HL),
	R(MOD,P_NUM,INDE,0,0,0, MOD, P_L, P_R, P_NONE, 0,
		"\tld hl,$L\n$[\tcall lmod\n$]", R_HL),
	R(MOD,INHL,INBC,0,0,0, MOD, P_L, P_R, P_NONE, RF_SIGNL,
		F_LDEC F_LDDB "\tcall amod\n", R_HL),
	R(MOD,INHL,INBC,0,0,0, MOD, P_L, P_R, P_NONE, 0,
		F_LDEC F_LDDB "\tcall lmod\n", R_HL),
	R(MOD,INBC,P_NUM,0,0,0, MOD, P_L, P_R, P_NONE, RF_SIGNL,
		T_BC_HL F_LDDER "\tcall amod\n", R_HL),
	R(MOD,INBC,P_NUM,0,0,0, MOD, P_L, P_R, P_NONE, 0,
		T_BC_HL F_LDDER "\tcall lmod\n", R_HL),
	R(RSHIFT,INA,INE,0,0,1, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL, RT12, R_A),
	R(RSHIFT,INA,INDE,0,0,1, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL, RT12, R_A),
	R(RSHIFT,INA,INHL,0,0,1, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL,
		"$[\tld b,l\n\tinc b\n"
		"\tjr $$+4\n\tsra a\n\tdjnz $$-2\n$]", R_A),
	R(RSHIFT,INA,INE,0,0,1, RSHIFT, P_L, P_R, P_NONE, 0, RT13, R_A),
	R(RSHIFT,INA,INDE,0,0,1, RSHIFT, P_L, P_R, P_NONE, 0, RT13, R_A),
	R(RSHIFT,INA,INHL,0,0,1, RSHIFT, P_L, P_R, P_NONE, 0,
		"$[\tld b,l\n\tinc b\n"
		"\tjr $$+4\n\tsrl a\n\tdjnz $$-2\n$]", R_A),
	/*
	 * A signed right shift keeps the sign: sra copies bit 7 back into
	 * itself where srl feeds in a zero.  The signed rule has to come
	 * first, since the unsigned pattern matches either width.
	 */
	R(RSHIFT,INA,P_NUM,0,0,1, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL, "%(\tsra a\n)", R_A),
	R(RSHIFT,INA,P_NUM,0,0,1, RSHIFT, P_L, P_R, P_NONE, 0, "%(\tsrl a\n)", R_A),
	/*
	 * A shift by a whole byte is a register move, not a loop - two
	 * bytes against the thirty-two the repeated form would emit.  The
	 * signed right shift has to put the sign back, since the byte
	 * that moved down carries it.
	 */
	R(RSHIFT,INHL,P_EIGHT,0,0,0, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL,
		"\tld l,h\n" F_LDAH F_RLA F_SBCAA F_LDHA, R_HL),
	R(RSHIFT,INHL,P_EIGHT,0,0,0, RSHIFT, P_L, P_R, P_NONE, 0, "\tld l,h\n" F_LDH0, R_HL),
	R(RSHIFT,INHL,P_SMALL,0,0,0, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL, RT26, R_HL),
	R(RSHIFT,INHL,P_SMALL,0,0,0, RSHIFT, P_L, P_R, P_NONE, 0, RT28, R_HL),
	R(RSHIFT,INBC,P_SMALL,0,0,0, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL, RT390, R_HL),
	R(RSHIFT,INBC,P_SMALL,0,0,0, RSHIFT, P_L, P_R, P_NONE, 0, RT391, R_HL),
	/*
	 * By any other count.  M is one to four and 8 has a form of its
	 * own, so a shift by five, six, seven or more than eight matched
	 * nothing at all - the left shifts have taken any count all
	 * along, and these stopped at four.
	 */
	R(RSHIFT,INHL,P_NUM,0,0,0, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL, RT26, R_HL),
	R(RSHIFT,INHL,P_NUM,0,0,0, RSHIFT, P_L, P_R, P_NONE, 0, RT28, R_HL),
	R(RSHIFT,INHL,INDE,0,0,0, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL,
		"$[\tld b,e\n\tinc b\n\tjr $$+6\n\tsra h\n\trr l\n\tdjnz $$-4\n$]", R_HL),
	R(RSHIFT,INHL,INDE,0,0,0, RSHIFT, P_L, P_R, P_NONE, 0,
		"$[\tld b,e\n\tinc b\n\tjr $$+6\n\tsrl h\n\trr l\n\tdjnz $$-4\n$]", R_HL),
	R(RSHIFT,INHL,INBC,0,0,0, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL,
		"$[\tld b,c\n\tinc b\n\tjr $$+6\n\tsra h\n\trr l\n\tdjnz $$-4\n$]", R_HL),
	R(RSHIFT,INHL,INBC,0,0,0, RSHIFT, P_L, P_R, P_NONE, 0,
		"$[\tld b,c\n\tinc b\n\tjr $$+6\n\tsrl h\n\trr l\n\tdjnz $$-4\n$]", R_HL),
	R(RSHIFT,INBC,P_NUM,0,0,0, RSHIFT, P_L, P_R, P_NONE, RF_SIGNL, RT390, R_HL),
	R(RSHIFT,INBC,P_NUM,0,0,0, RSHIFT, P_L, P_R, P_NONE, 0, RT391, R_HL),
	R(LSHIFT,INBC,P_NUM,0,0,0, LSHIFT, P_L, P_R, P_NONE, 0, T_BC_HL "%(" T_ADD_HL_HL ")", R_HL),
	/* and against a frame slot, which the other four widths of this
	 * had and the subtraction did not */
	/*
	 * No -(H,I).  It read the two bytes at the frame slot, but a bare
	 * INDEX operand is an address, not a value: a scalar's contents
	 * come through as D(I) and are loaded by the rules above long
	 * before a binary operator sees them.  The only thing that reaches
	 * here as a bare INDEX is a local array's name, whose value IS its
	 * address - so "q - buf" subtracted buf[0] and buf[1] from q.
	 * rewrite1 forms the address instead, at children_done.
	 */

	/* shifts */
	R(LSHIFT,INHL,P_NUM,0,0,0, LSHIFT, P_L, P_R, P_NONE, 0, "%(" T_ADD_HL_HL ")", R_HL),
	R(LSHIFT,INA,P_NUM,0,0,1, LSHIFT, P_L, P_R, P_NONE, 0, "%(\tsla a\n)", R_A),
	/*
	 * A byte shifted by a count only known at runtime - "1 << n",
	 * the ordinary way to make a mask.  Every other shift here
	 * spells itself out, which only works for a constant count.
	 *
	 * The count is in E and the value in A, so B does the counting
	 * and djnz walks it: one more than the count, then a jump
	 * straight to the djnz, so a count of nought runs the body no
	 * times rather than once.  Displacements are from each jump
	 * itself - sla a is two bytes, so the entry jump clears it and
	 * the djnz reaches back over it.
	 */
	R(LSHIFT,P_NUM,INE,0,0,1, LSHIFT, P_L, P_R, P_NONE, 0, RT14, R_A),
	/*
	 * The same count arriving as a word, which is what an expression
	 * count reduces to: "1 << (idx & 7)" works the AND out in HL and
	 * the target machinery may swap it to DE.  Only the low byte
	 * counts - a shift past eight of a byte is zero anyway, and the
	 * loop delivers exactly that.  Without these two forms the shift
	 * matched nothing, and matched nothing SILENTLY: the compound
	 * assignment above it stored the count itself through the
	 * address, so "map[i] |= 1 << (n & 7)" wrote n's low byte into
	 * the bitmap and pass1's else-if bookkeeping read garbage.
	 */
	R(LSHIFT,P_NUM,INDE,0,0,1, LSHIFT, P_L, P_R, P_NONE, 0, RT14, R_A),
	R(LSHIFT,P_NUM,INHL,0,0,1, LSHIFT, P_L, P_R, P_NONE, 0,
		"$[\tld b,l\n\tld a,$L\n\tinc b\n"
		"\tjr $$+4\n\tsla a\n\tdjnz $$-2\n$]", R_A),
	/*
	 * A byte VARIABLE shifted by a runtime count - the value already
	 * reduced into A, the count into E, DE or HL.  The docompound
	 * marker found these missing: "m[i++] >>= n" reduced its value
	 * and its count and then had no rule to put them together.
	 * Right shifts come in two kinds and the signed one must be
	 * first, RF_SIGNL deciding: sra copies the sign bit back in
	 * where srl feeds zero.
	 */
	R(LSHIFT,INA,INE,0,0,1, LSHIFT, P_L, P_R, P_NONE, 0, RT11, R_A),
	R(LSHIFT,INA,INDE,0,0,1, LSHIFT, P_L, P_R, P_NONE, 0, RT11, R_A),
	R(LSHIFT,INA,INHL,0,0,1, LSHIFT, P_L, P_R, P_NONE, 0,
		"$[\tld b,l\n\tinc b\n"
		"\tjr $$+4\n\tsla a\n\tdjnz $$-2\n$]", R_A),
	R(LSHIFT,INHL,P_EIGHT,0,0,0, LSHIFT, P_L, P_R, P_NONE, 0, "\tld h,l\n\tld l,0\n", R_HL),
	/*
	 * By a count that is not a constant: djnz over the one-shift
	 * body, entered at the test so a count of nought runs it not at
	 * all - the same shape the byte forms use.  The count arrives in
	 * E, or sits in C when it lives in the register home; B is the
	 * loop counter either way, safe under the $[ $] guard.
	 */
	R(LSHIFT,INHL,INDE,0,0,0, LSHIFT, P_L, P_R, P_NONE, 0,
		"$[\tld b,e\n\tinc b\n\tjr $$+3\n" T_ADD_HL_HL "\tdjnz $$-1\n$]", R_HL),
	R(LSHIFT,INHL,INBC,0,0,0, LSHIFT, P_L, P_R, P_NONE, 0,
		"$[\tld b,c\n\tinc b\n\tjr $$+3\n" T_ADD_HL_HL "\tdjnz $$-1\n$]", R_HL),

	/*
	 * Bit testing.  A single bit out of a byte is what bit does, in
	 * two bytes and without touching A or the carry - against and,
	 * which needs the byte in A first, costs two bytes itself and
	 * then a third to set the flags, because and leaves Z meaning
	 * the whole result rather than the bit.
	 *
	 * Only for a mask that is one bit: P matches a power of two, and
	 * RF_POW2 turns the mask into the bit number the instruction
	 * wants.  Neither admits 1, so bit 0 is still tested the long
	 * way - ispow2 answers 0 for it and both guards read that as no.
	 *
	 * The first two test the byte where it lies, through an index
	 * register or through HL, and are a byte shorter again than
	 * loading it into A first.  Reaching them takes more than a rule:
	 * an AND reduces its left operand before it is itself looked at,
	 * which loads the byte into A and leaves the address nowhere to
	 * be seen, so rewrite1 has a case that reduces the address and
	 * leaves the DEREF standing.  Without it the indexed rule sat
	 * here for a long time matching nothing at all.
	 *
	 * A global keeps the third form.  "ld a,(nn)" is the only direct
	 * absolute load the Z80 has and bit has no absolute form, so
	 * pointing HL at it first would cost what it saved.
	 */
	R(AND,DEREF,P_POW2,INDEX,0,9, AND, P_L, P_R, P_NONE, RF_POW2, "\tbit $R,($LL)\n", F_NZ),
	R(AND,DEREF,P_POW2,REGVAR,0,9, AND, P_L, P_R, P_LL, RF_POW2 | RF_IX,
		"\tbit $R,(ix+0)\n", F_NZ),
	R(AND,DEREF,P_POW2,INHL,0,9, AND, P_L, P_R, P_NONE, RF_POW2, "\tbit $R,(hl)\n", F_NZ),
	R(AND,INA,P_POW2,0,0,9, AND, P_L, P_R, P_NONE, RF_POW2, "\tbit $R,a\n", F_NZ),
	R(AND,DEREF,P_NUM,INDEX,0,1, AND, P_L, P_R, P_NONE, 0, F_LDALL "\tand $R\n", R_A),
	R(AND,INA,DEREF,0,SYMREF,1, AND, P_L, P_R, P_NONE, 0, F_LDHLRL1 "\tand (hl)\n", R_A),
	R(AND,INA,DEREF,0,INDEX,1, AND, P_L, P_R, P_NONE, 0, "\tand ($RL)\n", R_A),
	R(AND,INA,INE,0,0,1, AND, P_L, P_R, P_NONE, 0, RT35, R_A),
	R(AND,INHL,INE,0,0,1, AND, P_L, P_R, P_NONE, 0, F_LDAL "\tand e\n", R_A),
	R(AND,INA,INDE,0,0,1, AND, P_L, P_R, P_NONE, 0, RT35, R_A),
	/* the flag form first: and sets Z itself, so a test that only
	 * wants the flag must not pay for a result register */
	R(AND,INA,P_NUM,0,0,9, AND, P_L, P_R, P_NONE, 0, RT33, F_NZ),
	R(AND,INA,P_NUM,0,0,1, AND, P_L, P_R, P_NONE, 0, RT33, R_A),
	R(AND,INA,INE,0,0,9, AND, P_L, P_R, P_NONE, 0, RT35, F_NZ),
	/* no 16-bit and/or/xor on the Z80 - do it a byte at a time */
	R(AND,INHL,P_NUM,0,0,0, AND, P_L, P_R, P_NONE, 0,
		F_LDAL "\tand $Rl\n" F_LDLA F_LDAH "\tand $Rh\n" F_LDHA, R_HL),
	R(AND,INBC,P_NUM,0,0,0, AND, P_L, P_R, P_NONE, 0,
		T_BC_HL F_LDAL "\tand $Rl\n" F_LDLA F_LDAH "\tand $Rh\n" F_LDHA, R_HL),
	R(AND,INHL,INDE,0,0,0, AND, P_L, P_R, P_NONE, 0, F_LDAL "\tand e\n" F_LDLA F_LDAH "\tand d\n" F_LDHA, R_HL),
	/*
	 * The same with BC on one side or the other.  A word bitwise
	 * operator had a form for HL against DE and no other, so one of
	 * these on a register variable emitted nothing.  Against BC the
	 * accumulator can take b and c directly; with the value in BC it
	 * comes over to HL first, which is what the constant-count shifts
	 * do a few rules down.
	 */
	R(AND,INHL,INBC,0,0,0, AND, P_L, P_R, P_NONE, 0, F_LDAL "\tand c\n" F_LDLA F_LDAH "\tand b\n" F_LDHA, R_HL),
	/*
	 * The same the other way round.  and is commutative and the
	 * template does not care which operand it was handed, but the
	 * matcher does: with only the hl,bc spelling here, "w &= ~(-w)"
	 * - the register variable on the left and the reduced complement
	 * in hl on the right - had nothing to match.  od's power-of-two
	 * clamp is written that way.
	 */
	R(AND,INBC,INHL,0,0,0, AND, P_L, P_R, P_NONE, 0, F_LDAL "\tand c\n" F_LDLA F_LDAH "\tand b\n" F_LDHA, R_HL),
	R(AND,INBC,INDE,0,0,0, AND, P_L, P_R, P_NONE, 0, T_BC_HL F_LDAL "\tand e\n" F_LDLA F_LDAH "\tand d\n" F_LDHA, R_HL),
	R(OR,DEREF,P_NUM,INDEX,0,1, OR, P_L, P_R, P_NONE, 0, F_LDALL "\tor $R\n", R_A),
	R(OR,INA,DEREF,0,SYMREF,1, OR, P_L, P_R, P_NONE, 0, F_LDHLRL1 F_ORHL, R_A),
	R(OR,INA,DEREF,0,INDEX,1, OR, P_L, P_R, P_NONE, 0, "\tor ($RL)\n", R_A),
	R(OR,INA,INE,0,0,1, OR, P_L, P_R, P_NONE, 0, RT170, R_A),
	R(OR,INHL,INE,0,0,1, OR, P_L, P_R, P_NONE, 0, F_LDAL "\tor e\n", R_A),
	R(OR,INA,INDE,0,0,1, OR, P_L, P_R, P_NONE, 0, RT170, R_A),
	R(OR,INA,P_NUM,0,0,1, OR, P_L, P_R, P_NONE, 0, "\tor $R\n", R_A),
	R(OR,INHL,P_NUM,0,0,0, OR, P_L, P_R, P_NONE, 0,
		F_LDAL "\tor $Rl\n" F_LDLA F_LDAH "\tor $Rh\n" F_LDHA, R_HL),
	R(OR,INBC,P_NUM,0,0,0, OR, P_L, P_R, P_NONE, 0,
		T_BC_HL F_LDAL "\tor $Rl\n" F_LDLA F_LDAH "\tor $Rh\n" F_LDHA, R_HL),
	R(OR,INHL,INDE,0,0,0, OR, P_L, P_R, P_NONE, 0, F_LDAL "\tor e\n" F_LDLA F_LDAH "\tor d\n" F_LDHA, R_HL),
	R(OR,INHL,INBC,0,0,0, OR, P_L, P_R, P_NONE, 0, F_LDAL "\tor c\n" F_LDLA F_LDAH "\tor b\n" F_LDHA, R_HL),
	R(OR,INBC,INDE,0,0,0, OR, P_L, P_R, P_NONE, 0, T_BC_HL F_LDAL "\tor e\n" F_LDLA F_LDAH "\tor d\n" F_LDHA, R_HL),
	R(XOR,DEREF,P_NUM,INDEX,0,1, XOR, P_L, P_R, P_NONE, 0, F_LDALL "\txor $R\n", R_A),
	R(XOR,INA,DEREF,0,SYMREF,1, XOR, P_L, P_R, P_NONE, 0, F_LDHLRL1 "\txor (hl)\n", R_A),
	R(XOR,INA,DEREF,0,INDEX,1, XOR, P_L, P_R, P_NONE, 0, "\txor ($RL)\n", R_A),
	R(XOR,INA,INE,0,0,1, XOR, P_L, P_R, P_NONE, 0, RT197, R_A),
	R(XOR,INHL,INE,0,0,1, XOR, P_L, P_R, P_NONE, 0, F_LDAL "\txor e\n", R_A),
	R(XOR,INA,INDE,0,0,1, XOR, P_L, P_R, P_NONE, 0, RT197, R_A),
	R(XOR,INA,P_NUM,0,0,1, XOR, P_L, P_R, P_NONE, 0, "\txor $R\n", R_A),
	R(XOR,INHL,P_NUM,0,0,0, XOR, P_L, P_R, P_NONE, 0,
		F_LDAL "\txor $Rl\n" F_LDLA F_LDAH "\txor $Rh\n" F_LDHA, R_HL),
	R(XOR,INBC,P_NUM,0,0,0, XOR, P_L, P_R, P_NONE, 0,
		T_BC_HL F_LDAL "\txor $Rl\n" F_LDLA F_LDAH "\txor $Rh\n" F_LDHA, R_HL),
	R(XOR,INHL,INDE,0,0,0, XOR, P_L, P_R, P_NONE, 0, F_LDAL "\txor e\n" F_LDLA F_LDAH "\txor d\n" F_LDHA, R_HL),
	R(XOR,INHL,INBC,0,0,0, XOR, P_L, P_R, P_NONE, 0, F_LDAL "\txor c\n" F_LDLA F_LDAH "\txor b\n" F_LDHA, R_HL),
	R(XOR,INBC,INDE,0,0,0, XOR, P_L, P_R, P_NONE, 0, T_BC_HL F_LDAL "\txor e\n" F_LDLA F_LDAH "\txor d\n" F_LDHA, R_HL),
	/*
	 * Against zero, which is what "p == 0" on a pointer register
	 * variable comes to and had no form: testing the halves is
	 * shorter than loading nought into DE to subtract it.
	 */
	R(EQ,REGVAR,P_ZERO,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT187, F_Z),
	/*
	 * And against any other constant, or a symbol, which come to the
	 * same thing.  The whole comparison family, not just equality:
	 * "(unsigned)p < 0x100" asks an ordering question of a pointer
	 * and had no form, so it fell to the marker and branched on
	 * whatever flags were standing.  Unsigned, because IX holds
	 * pointers by allocation policy.
	 */
	R(EQ,REGVAR,P_NUM,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT179, F_CC),
	R(EQ,REGVAR,SYMREF,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT179, F_CC),
	R(EQ,REGVAR,INBC,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT184, F_Z),
	R(EQ,REGVAR,INHL,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT193, F_Z),
	/*
	 * And against DE, which had no form at all.  A pointer register
	 * variable compared with a local pointer emitted no code and the
	 * branch after it went wherever the flags happened to point.
	 */
	R(EQ,REGVAR,INDE,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT186, F_CC),
	/* the same with the index register on the other side, which
	 * normalize does not swap because equality is not a relation it
	 * reorders by operand kind */
	R(EQ,INHL,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_IX, RT193, F_CC),

	/*
	 * Both operands in register homes: BC against IX, which two
	 * register pointers walking the same buffer produce - outf's
	 * literal spans were the first ("p > q" with p in IX, q in BC).
	 * There was no rule at all, the comparison emitted NOTHING, and
	 * the branch went on whatever flags were lying around.
	 */
	R(EQ,INBC,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_IX, RT396, F_CC),
	/* and mirrored, the IX pointer on the left */
	R(EQ,REGVAR,INBC,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT185, F_CC),

	/* comparisons */
	/*
	 * An ADDRESS compared, rather than the thing at it.  "p->c" is
	 * an INDEX and the (ix+d) rules read through it, but "&p->c ==
	 * g" wants the address itself worked out, and until these rules
	 * existed CODE appeared in the table only as the source of an
	 * assignment.  So the conversion that forms an effective address
	 * in a register was never even attempted for a comparison: the
	 * left operand stayed an INDEX, nothing matched, and the test
	 * emitted a marker and then branched on whatever flags the
	 * preceding load happened to leave.
	 *
	 * The sequence is the one the INHL form uses - by the time these
	 * match, the address is in HL exactly as a loaded value would be.
	 */
	R(EQ,CODE,INDE,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT360, F_Z),
	R(EQ,CODE,SYMREF,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT287, F_Z),

	R(EQ,INHL,INDE,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT360, F_Z),
	/* BC operands: the Z80 has add/sbc hl,bc, so no shuffle needed */
	R(EQ,INHL,INBC,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT359, F_CC),
	R(EQ,INBC,INDE,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT415, F_Z),
	R(EQ,INBC,P_NUM,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT411, F_Z),

	/* against a symbol's address, which is what comparing a pointer
	 * with "&thing" comes to */
	R(EQ,INHL,SYMREF,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT287, F_Z),
	R(EQ,INHL,P_NUM,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT287, F_CC),

	/* byte comparisons */
	/* byte comparison against another byte, in E */
	R(EQ,INA,INE,0,0,0, EQ, P_L, P_R, P_NONE, 0, F_CPE, F_CC),
	R(EQ,INA,P_NUM,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT200, F_Z),
	/*
	 * Against a byte register variable, which lives in B or C.  cp
	 * takes either directly; there were forms for E and for a
	 * constant and none for these.
	 */
	R(EQ,INA,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_B, RT41, F_Z),
	R(EQ,INA,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_C, RT42, F_Z),
	R(EQ,DEREF,P_NUM,INDEX,0,1, EQ, P_L, P_R, P_NONE, 0, F_LDALL F_CPR, F_CC),
	R(NEQ,REGVAR,P_ZERO,0,0,0, NEQ, P_L, P_R, P_L, RF_IX, RT187, F_NZ),
	R(NEQ,REGVAR,P_NUM,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT179, F_CC),
	R(NEQ,REGVAR,SYMREF,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT179, F_CC),
	R(NEQ,REGVAR,INBC,0,0,0, NEQ, P_L, P_R, P_L, RF_IX, RT184, F_NZ),
	R(NEQ,REGVAR,INHL,0,0,0, NEQ, P_L, P_R, P_L, RF_IX, RT193, F_NZ),
	R(NEQ,REGVAR,INDE,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT186, F_CC),
	R(NEQ,INHL,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_IX, RT193, F_CC),
	R(NEQ,INBC,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_IX, RT396, F_CC),
	R(NEQ,REGVAR,INBC,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT185, F_CC),
	R(NEQ,CODE,INDE,0,0,0, NEQ, P_L, P_R, P_NONE, 0, RT360, F_NZ),
	R(NEQ,CODE,SYMREF,0,0,0, NEQ, P_L, P_R, P_NONE, 0, RT287, F_NZ),
	R(NEQ,INHL,INDE,0,0,0, NEQ, P_L, P_R, P_NONE, 0, RT360, F_NZ),
	R(NEQ,INHL,INBC,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT359, F_CC),
	R(NEQ,INBC,INDE,0,0,0, NEQ, P_L, P_R, P_NONE, 0, RT415, F_NZ),
	R(NEQ,INBC,P_NUM,0,0,0, NEQ, P_L, P_R, P_NONE, 0, RT411, F_NZ),
	R(NEQ,INHL,SYMREF,0,0,0, NEQ, P_L, P_R, P_NONE, 0, RT287, F_NZ),
	R(NEQ,INHL,P_NUM,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT287, F_CC),
	R(NEQ,INA,INE,0,0,0, EQ, P_L, P_R, P_NONE, 0, F_CPE, F_CC),
	R(NEQ,INA,P_NUM,0,0,0, NEQ, P_L, P_R, P_NONE, 0, RT200, F_NZ),
	R(NEQ,INA,REGVAR,0,0,0, NEQ, P_L, P_R, P_R, RF_B, RT41, F_NZ),
	R(NEQ,INA,REGVAR,0,0,0, NEQ, P_L, P_R, P_R, RF_C, RT42, F_NZ),
	R(NEQ,DEREF,P_NUM,INDEX,0,1, EQ, P_L, P_R, P_NONE, 0, F_LDALL F_CPR, F_CC),

	/* NEQ -> BANG(EQ) */
	R(NEQ,P_ANY,P_NUM,0,0,0, 0, P_NONE, P_NONE, P_NONE, RF_NOTEQ, 0, 0),
	R(NEQ,0,0,0,0,0, 0, P_NONE, P_NONE, P_NONE, RF_NOTEQ, 0, 0),
	R(LE,REGVAR,P_NUM,0,0,0, LE, P_L, P_R, P_L, RF_IX, RT178,
		F_CC),
	R(LE,REGVAR,SYMREF,0,0,0, LE, P_L, P_R, P_L, RF_IX, RT178,
		F_CC),
	R(LE,REGVAR,INDE,0,0,0, LE, P_L, P_R, P_L, RF_IX, "\tpush ix\n" F_POPHL F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(LE,INHL,REGVAR,0,0,0, LE, P_L, P_R, P_R, RF_IX, "\tpush ix\n\tpop de\n" F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(LE,INBC,REGVAR,0,0,0, LE, P_L, P_R, P_R, RF_IX, T_BC_HL "\tpush ix\n\tpop de\n" F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(LE,REGVAR,INBC,0,0,0, LE, P_L, P_R, P_L, RF_IX, RT396, F_CC),
	R(LE,INHL,P_ZERO,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, RT229, F_Z),
	R(LE,INBC,P_ZERO,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, RT220, F_Z),
	R(LE,INHL,INDE,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(LE,SYMREF,INDE,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, F_LDHLL F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(LE,SYMREF,INBC,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, F_LDHLL F_LDEC F_LDDB F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(LE,INHL,SYMREF,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, RT285, F_CC),
	R(LE,INHL,INBC,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, F_LDEC F_LDDB F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(LE,INHL,P_NUM,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, RT285, F_CC),
	/* LE/GT have no cheap flag of their own: swap the operands so the
	 * borrow from sbc answers the reversed question. */
	R(LE,INHL,INDE,0,0,0, LE, P_L, P_R, P_NONE, 0, F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	/* the same four with a symbol on the left - see the signed set */
	R(LE,SYMREF,INDE,0,0,0, LE, P_L, P_R, P_NONE, 0, F_LDHLL F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(LE,INHL,SYMREF,0,0,0, LE, P_L, P_R, P_NONE, 0, F_LDDER F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	/*
	 * LE and GT answer the reversed question, and there is no
	 * ex bc,hl - so copy BC into DE and swap that instead.
	 */
	R(LE,INHL,INBC,0,0,0, LE, P_L, P_R, P_NONE, 0, F_LDEC F_LDDB F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(LE,INBC,INDE,0,0,0, GT, P_L, P_R, P_NONE, RF_SIGNL, T_BC_HL F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(LE,INBC,P_NUM,0,0,0, GT, P_L, P_R, P_NONE, RF_SIGNL, T_BC_HL F_LDDER F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(LE,INA,P_ZERO,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, RT361, F_Z),
	R(LE,INA,INE,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, RT382, F_Z),
	R(LE,INA,P_NUM,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, RT385, F_Z),
	R(LE,DEREF,P_NUM,INDEX,0,1, LE, P_L, P_R, P_NONE, RF_SIGNL, RT272, F_Z),
	R(LE,INA,REGVAR,0,0,0, LE, P_L, P_R, P_R, RF_SIGNL|RF_B,
		"\tsub b\n" T_SXORA T_SZTAIL, F_Z),
	R(LE,INA,REGVAR,0,0,0, LE, P_L, P_R, P_R, RF_SIGNL|RF_C,
		"\tsub c\n" T_SXORA T_SZTAIL, F_Z),
	R(LE,INA,REGVAR,0,0,0, LE, P_L, P_R, P_R, RF_B,
		"\tcp b\n\tjr nz,$$+3\n\tscf\n", F_C),
	R(LE,INA,REGVAR,0,0,0, LE, P_L, P_R, P_R, RF_C,
		"\tcp c\n\tjr nz,$$+3\n\tscf\n", F_C),
	R(LE,INA,INE,0,0,0, LE, P_L, P_R, P_NONE, 0, RT199, F_C),
	R(LE,INA,P_NUM,0,0,0, LE, P_L, P_R, P_NONE, 0, RT201, F_C),
	R(LE,DEREF,P_NUM,INDEX,0,1, LE, P_L, P_R, P_NONE, 0, RT269, F_C),
	R(LE,INHL,P_NUM,0,0,0, LE, P_L, P_R, P_NONE, 0, RT288, F_C),
	R(LE,INBC,P_NUM,0,0,0, LE, P_L, P_R, P_NONE, 0, RT412, F_C),
	R(LE,INBC,INDE,0,0,0, GT, P_L, P_R, P_NONE, 0, T_BC_HL F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(LT,REGVAR,P_NUM,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT179, F_CC),
	R(LT,REGVAR,SYMREF,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT179, F_CC),
	R(LT,REGVAR,INDE,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT186, F_CC),
	R(LT,INHL,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_IX, RT193, F_CC),
	R(LT,INBC,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_IX, RT396, F_CC),
	R(LT,REGVAR,INBC,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT185, F_CC),

	/*
	 * Signed compare against zero is just the sign bit, and it has to
	 * be: sbc hl,de sets carry on an unsigned borrow, so the generic
	 * form below says "x < 0" is false for every x.  Must precede the
	 * T/Y(H,N) rules - zero is a subset of NUMBER and first match wins.
	 */
	R(LT,INHL,P_ZERO,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT228, F_M),
	R(LT,INBC,P_ZERO,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT219, F_M),

	/*
	 * Signed relational comparison.  Carry answers the unsigned
	 * question, so it cannot be used here: with a = -1 and b = 1 the
	 * subtraction does not borrow, and carry would report that -1 is
	 * not less than 1.
	 *
	 * The signed answer is sign exclusive-or overflow.  sbc hl,de
	 * leaves the sign in bit 7 of H and the overflow in P/V, so take
	 * the high byte, flip its top bit when the subtraction overflowed,
	 * and let or a set the sign from the result.  M is then "less
	 * than" and P is "greater or equal".
	 *
	 * Ten bytes against the three carry costs, which is why the
	 * unsigned forms below keep using it, and why comparing against
	 * zero stays on the sign-bit rules above - those are exact and
	 * cheaper.
	 */
	R(LT,INHL,INDE,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT462, F_M),
	/*
	 * A symbol compared against a register.  A SYMREF is left
	 * unreduced so the store and load rules can use it as an
	 * address, so where its *value* is wanted it has to be loaded -
	 * and here that is the whole difference.
	 *
	 * It only shows up on the left because "a > b" is canonicalised
	 * to "b < a", which is what puts the symbol there.  "s == buf"
	 * kept the symbol on the right, where it becomes DE and the
	 * ordinary rules match, so equality worked and ordering did not:
	 *
	 *	while (s > macbuffer && ...)	never ran
	 *
	 * which is macro.c's trailing-whitespace trim, so a macro body
	 * kept its trailing blanks - and an empty one walked off the end
	 * of the definition and ate the next line.
	 */
	R(LT,SYMREF,INDE,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT318, F_M),

	/*
	 * The same symbol-on-the-left shapes with the register operand
	 * living in BC - a register variable compared against a global
	 * array's address arrives exactly here, and the table stopping
	 * at (O,E) left "if (s > buf)" unreduced.
	 */
	R(LT,SYMREF,INBC,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT317, F_M),

	/* and with the symbol on the other side, where it becomes DE */
	R(LT,INHL,SYMREF,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT290, F_M),

	R(LT,INHL,INBC,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT461, F_M),
	R(LT,INHL,P_NUM,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT290, F_M),
	R(LT,INHL,INDE,0,0,0, LT, P_L, P_R, P_NONE, 0, RT360, F_C),
	R(LT,SYMREF,INDE,0,0,0, LT, P_L, P_R, P_NONE, 0, RT312, F_C),
	R(LT,INHL,SYMREF,0,0,0, LT, P_L, P_R, P_NONE, 0, RT287, F_C),
	R(LT,INHL,INBC,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT359, F_CC),
	/*
	 * A register variable compared, signed.  The rules below answer
	 * with carry, which is the unsigned question - the same fault the
	 * HL forms had, in the register that fix did not reach.  A
	 * variable that lives in BC and goes negative compared as though
	 * it were large: "i < 2" was false for i = -1.
	 *
	 * Greater-than and at-or-below have no flag of their own, so the
	 * operands are handed over the other way round, which is what the
	 * ex de,hl is doing.
	 */
	R(LT,INBC,INDE,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT425, F_M),
	R(LT,INBC,INDE,0,0,0, LT, P_L, P_R, P_NONE, 0, RT415, F_C),
	R(LT,INBC,P_NUM,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT413, F_M),
	R(LT,INBC,P_NUM,0,0,0, LT, P_L, P_R, P_NONE, 0, RT411, F_C),
	R(LT,INHL,P_NUM,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT287, F_CC),

	/*
	 * Signed byte comparisons.  These have to come before the
	 * unsigned forms below, which match either signedness and answer
	 * the unsigned question: cp sets carry on a borrow, and nothing
	 * borrows against zero, so "c < 0" was false for every char in
	 * the language.  Equality needs no signed form - the bits are
	 * either equal or they are not.
	 *
	 * Against zero the sign bit is the whole answer, and or a puts it
	 * in S for free.  Zero is a subset of NUMBER, so these must also
	 * precede the T/Y(A,N) rules: first match wins.
	 *
	 * None of these ask for flag context, unlike the unsigned rules
	 * below, which is what let a byte comparison used for its value
	 * fall through to no rule at all.  A flag becomes a number by the
	 * same path a word comparison uses.
	 */
	R(LT,INA,P_ZERO,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT358, F_M),
	/*
	 * Against anything else, the same sign-exclusive-or-overflow the
	 * word rules use.  Seven bytes against cp's two, which is why the
	 * unsigned forms below keep cp and why zero stays on the rules
	 * above.
	 *
	 * > and <= go the long way round rather than becoming >= and <
	 * against the constant plus one, the way the unsigned rules do.
	 * That trick has nowhere to go at 127, where the increment wraps
	 * to -128 and turns a test that is always false into one that is
	 * always true.
	 */
	R(LT,INA,INE,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT381, F_M),
	R(LT,INA,P_NUM,0,0,0, LT, P_L, P_R, P_NONE, RF_SIGNL, RT384, F_M),
	R(LT,DEREF,P_NUM,INDEX,0,1, LT, P_L, P_R, P_NONE, RF_SIGNL, RT271, F_M),
	R(LT,INA,INE,0,0,0, EQ, P_L, P_R, P_NONE, 0, F_CPE, F_CC),
	/*
	 * The ordered relationals against the same homes.  Signed first:
	 * the sign gate stops the search there, and an unsigned rule
	 * reached by a signed compare answers with the wrong flag.  Only
	 * LT and LE, because pass1 normalises GT and GE away by swapping.
	 */
	R(LT,INA,REGVAR,0,0,0, LT, P_L, P_R, P_R, RF_SIGNL|RF_B,
		"\tsub b\n" T_SXORA, F_M),
	R(LT,INA,REGVAR,0,0,0, LT, P_L, P_R, P_R, RF_SIGNL|RF_C,
		"\tsub c\n" T_SXORA, F_M),
	R(LT,INA,REGVAR,0,0,0, LT, P_L, P_R, P_R, RF_B, RT41, F_C),
	R(LT,INA,REGVAR,0,0,0, LT, P_L, P_R, P_R, RF_C, RT42, F_C),
	R(LT,INA,P_NUM,0,0,0, LT, P_L, P_R, P_NONE, 0, RT200, F_C),
	R(LT,DEREF,P_NUM,INDEX,0,1, EQ, P_L, P_R, P_NONE, 0, F_LDALL F_CPR, F_CC),
	R(GE,REGVAR,P_NUM,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT179, F_CC),
	R(GE,REGVAR,SYMREF,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT179, F_CC),
	R(GE,REGVAR,INDE,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT186, F_CC),
	R(GE,INHL,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_IX, RT193, F_CC),
	R(GE,INBC,REGVAR,0,0,0, EQ, P_L, P_R, P_R, RF_IX, RT396, F_CC),
	R(GE,REGVAR,INBC,0,0,0, EQ, P_L, P_R, P_L, RF_IX, RT185, F_CC),
	R(GE,INHL,P_ZERO,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT228, F_P),
	R(GE,INBC,P_ZERO,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT219, F_P),
	R(GE,INHL,INDE,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT462, F_P),
	R(GE,SYMREF,INDE,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT318, F_P),
	R(GE,SYMREF,INBC,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT317, F_P),
	R(GE,INHL,SYMREF,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT290, F_P),
	R(GE,INHL,INBC,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT461, F_P),
	R(GE,INHL,P_NUM,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT290, F_P),
	R(GE,INHL,INDE,0,0,0, GE, P_L, P_R, P_NONE, 0, RT360, F_NC),
	R(GE,SYMREF,INDE,0,0,0, GE, P_L, P_R, P_NONE, 0, RT312, F_NC),
	R(GE,INHL,SYMREF,0,0,0, GE, P_L, P_R, P_NONE, 0, RT287, F_NC),
	R(GE,INHL,INBC,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT359, F_CC),
	R(GE,INBC,INDE,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT425, F_P),
	R(GE,INBC,INDE,0,0,0, GE, P_L, P_R, P_NONE, 0, RT415, F_NC),
	R(GE,INBC,P_NUM,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT413, F_P),
	R(GE,INBC,P_NUM,0,0,0, GE, P_L, P_R, P_NONE, 0, RT411, F_NC),
	R(GE,INHL,P_NUM,0,0,0, EQ, P_L, P_R, P_NONE, 0, RT287, F_CC),
	R(GE,INA,P_ZERO,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT358, F_P),
	R(GE,INA,INE,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT381, F_P),
	R(GE,INA,P_NUM,0,0,0, GE, P_L, P_R, P_NONE, RF_SIGNL, RT384, F_P),
	R(GE,DEREF,P_NUM,INDEX,0,1, GE, P_L, P_R, P_NONE, RF_SIGNL, RT271, F_P),
	R(GE,INA,INE,0,0,0, EQ, P_L, P_R, P_NONE, 0, F_CPE, F_CC),
	R(GE,INA,P_NUM,0,0,0, GE, P_L, P_R, P_NONE, 0, RT200, F_NC),
	R(GE,DEREF,P_NUM,INDEX,0,1, EQ, P_L, P_R, P_NONE, 0, F_LDALL F_CPR, F_CC),
	R(GT,REGVAR,P_NUM,0,0,0, LE, P_L, P_R, P_L, RF_IX, RT178,
		F_CC),
	R(GT,REGVAR,SYMREF,0,0,0, LE, P_L, P_R, P_L, RF_IX, RT178,
		F_CC),
	R(GT,REGVAR,INDE,0,0,0, LE, P_L, P_R, P_L, RF_IX, "\tpush ix\n" F_POPHL F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(GT,INHL,REGVAR,0,0,0, LE, P_L, P_R, P_R, RF_IX, "\tpush ix\n\tpop de\n" F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(GT,INBC,REGVAR,0,0,0, LE, P_L, P_R, P_R, RF_IX, T_BC_HL "\tpush ix\n\tpop de\n" F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(GT,REGVAR,INBC,0,0,0, LE, P_L, P_R, P_L, RF_IX, RT396, F_CC),
	/*
	 * "> 0" and "<= 0" are not a single flag the way "< 0" is - they
	 * need the value to be non-negative AND non-zero.  Test the sign,
	 * and on the negative side fall into an xor a that forces Z, so
	 * both paths arrive with Z meaning false:
	 *
	 *   J+0  ld a,h   1
	 *   J+1  or a     1   sign of the high byte
	 *   J+2  jp m     3   negative, so false
	 *   J+5  ld a,h   1
	 *   J+6  or l     1   Z here means the whole value was zero
	 *   J+7  jr       2   past the forced-false
	 *   J+9  xor a    1
	 *   J+10
	 */
	R(GT,INHL,P_ZERO,0,0,0, GT, P_L, P_R, P_NONE, RF_SIGNL, RT229, F_NZ),
	R(GT,INBC,P_ZERO,0,0,0, GT, P_L, P_R, P_NONE, RF_SIGNL, RT220, F_NZ),
	R(GT,INHL,INDE,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(GT,SYMREF,INDE,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, F_LDHLL F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(GT,SYMREF,INBC,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, F_LDHLL F_LDEC F_LDDB F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(GT,INHL,SYMREF,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, RT285, F_CC),
	R(GT,INHL,INBC,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, F_LDEC F_LDDB F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(GT,INHL,P_NUM,0,0,0, LE, P_L, P_R, P_NONE, RF_SIGNL, RT285, F_CC),
	R(GT,INHL,INDE,0,0,0, LE, P_L, P_R, P_NONE, 0, F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(GT,SYMREF,INDE,0,0,0, LE, P_L, P_R, P_NONE, 0, F_LDHLL F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(GT,INHL,SYMREF,0,0,0, LE, P_L, P_R, P_NONE, 0, F_LDDER F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(GT,INHL,INBC,0,0,0, LE, P_L, P_R, P_NONE, 0, F_LDEC F_LDDB F_EXDEHL F_ORA F_SBCHLDE, F_CC),
	R(GT,INBC,INDE,0,0,0, GT, P_L, P_R, P_NONE, RF_SIGNL, T_BC_HL F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	R(GT,INBC,P_NUM,0,0,0, GT, P_L, P_R, P_NONE, RF_SIGNL, T_BC_HL F_LDDER F_EXDEHL T_SUB_DE T_SXORV, F_CC),
	/* or a sets S and Z together, which is what > 0 and <= 0 need */
	R(GT,INA,P_ZERO,0,0,0, GT, P_L, P_R, P_NONE, RF_SIGNL, RT361, F_NZ),
	R(GT,INA,INE,0,0,0, GT, P_L, P_R, P_NONE, RF_SIGNL, RT382, F_NZ),
	R(GT,INA,P_NUM,0,0,0, GT, P_L, P_R, P_NONE, RF_SIGNL, RT385, F_NZ),
	R(GT,DEREF,P_NUM,INDEX,0,1, GT, P_L, P_R, P_NONE, RF_SIGNL, RT272, F_NZ),

	/*
	 * Unsigned > and <=.  cp leaves the answer spread over two flags -
	 * carry says below, zero says equal - and "at or below" wants
	 * both.  Rather than branch twice, fold equality into the carry:
	 * when the two were equal, set it.  Carry then means "at or
	 * below" on its own, and its complement means "above".
	 *
	 *   J+0  cp     1   C = below, Z = equal
	 *   J+1  jr nz  2   not equal, so carry already answers
	 *   J+3  scf    1   equal, so make carry say so
	 *   J+4
	 *
	 * This replaces turning "> n" into ">= n+1", which has nowhere to
	 * go at 255: the increment wraps to zero and a test that is never
	 * true becomes one that always is.  Two bytes more, and right at
	 * both ends of the range.
	 */
	R(GT,INA,INE,0,0,0, GT, P_L, P_R, P_NONE, 0, RT199, F_NC),
	R(GT,INA,P_NUM,0,0,0, GT, P_L, P_R, P_NONE, 0, RT201, F_NC),
	R(GT,DEREF,P_NUM,INDEX,0,1, GT, P_L, P_R, P_NONE, 0, RT269, F_NC),

	/*
	 * The same fold at word width.  These turned "> n" into ">= n+1"
	 * until now, which the note above says has nowhere to go at the
	 * top of the range - it was fixed for bytes at 255 and left here,
	 * where the increment wraps at 65535 instead.  "u <= 0xffff" is
	 * true of every unsigned short and came out false for all of
	 * them.
	 */
	R(GT,INHL,P_NUM,0,0,0, GT, P_L, P_R, P_NONE, 0, RT288, F_NC),
	/* the same for a register variable, which had neither */
	R(GT,INBC,P_NUM,0,0,0, GT, P_L, P_R, P_NONE, 0, RT412, F_NC),
	R(GT,INBC,INDE,0,0,0, GT, P_L, P_R, P_NONE, 0, T_BC_HL F_EXDEHL F_ORA F_SBCHLDE, F_CC),

	/* arithmetic/logical on indexed */
	R(OREQ,INHL,P_NUM,0,0,1, OREQ, P_L, P_R, P_NONE, 0, F_LDAHL "\tor $R\n" F_LDHLA, R_A),
	R(OREQ,INDEX,INE,0,0,1, OREQ, P_L, P_R, P_NONE, 0, F_LDAL1 "\tor e\n" F_LDLA1, R_A),

	/* assign constant/A/HL to REGVAR C/B */
	R(ASSIGN,REGVAR,P_NUM,0,0,1, ASSIGN, P_L, P_R, P_L, RF_C, "\tld c,$R\n", R_A),
	R(ASSIGN,REGVAR,P_NUM,0,0,1, ASSIGN, P_L, P_R, P_L, RF_B, "\tld b,$R\n", R_A),
	R(ASSIGN,REGVAR,INA,0,0,1, ASSIGN, P_L, P_R, P_L, RF_C, "\tld c,a\n", R_A),
	R(ASSIGN,REGVAR,INA,0,0,1, ASSIGN, P_L, P_R, P_L, RF_B, "\tld b,a\n", R_A),
	R(ASSIGN,REGVAR,INHL,0,0,1, ASSIGN, P_L, P_R, P_L, RF_C, "\tld c,l\n", R_HL),
	R(ASSIGN,REGVAR,INHL,0,0,1, ASSIGN, P_L, P_R, P_L, RF_B, "\tld b,l\n", R_HL),

	/* load REGVAR C/B to HL (zero-extended) */
	R(ASSIGN,INHL,REGVAR,0,0,1, ASSIGN, P_L, P_R, P_R, RF_C, F_LDLC F_LDH0, R_HL),
	R(ASSIGN,INHL,REGVAR,0,0,1, ASSIGN, P_L, P_R, P_R, RF_B, "\tld l,b\n" F_LDH0, R_HL),
	/*
	 * The same for a value that ended up in IX.  HL, DE, BC, A and E
	 * each become a typed node before the table is reached; IX does
	 * not, so it arrives as CODE and matched nothing at all.  No rule
	 * matching normally leaves a marker, but this is a condition
	 * rather than a value - the caller wanted flags and took whatever
	 * was in them:
	 *
	 *	while ((top = fstack_top(&fs)) != NULL)
	 *
	 * with top in IX emitted push hl / pop ix, which sets no flags,
	 * and then branched on what the call had left behind.  cpp's
	 * filtbrace loops forever on any brace-less if, while or for.
	 */

	/* copy IX to HL/BC/DE */
	R(ASSIGN,INHL,REGVAR,0,0,0, ASSIGN, P_L, P_R, P_R, RF_IX, RT174, R_HL),
	R(ASSIGN,INBC,REGVAR,0,0,0, ASSIGN, P_L, P_R, P_R, RF_IX, "\tld c,ixl\n\tld b,ixh\n", R_BC),
	R(ASSIGN,INDE,REGVAR,0,0,0, ASSIGN, P_L, P_R, P_R, RF_IX, "\tld e,ixl\n\tld d,ixh\n", R_DE),

	/*
	 * Store to a frame slot.  A constant goes straight into the slot
	 * without touching a register, which is the right thing for a
	 * statement and the wrong thing for "i = k = 5": the assignment
	 * has a value, and there has to be one somewhere for the outer
	 * assignment to copy.  The :V forms pay for a register because
	 * something is going to read it.
	 */
	R(ASSIGN,INDEX,P_NUM,0,0,17, ASSIGN, P_L, P_R, P_NONE, 0, RT100, R_A),
	R(ASSIGN,INDEX,P_NUM,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, "\tld ($L),$R\n", 0),
	R(ASSIGN,INDEX,P_NUM,0,0,18, ASSIGN, P_L, P_R, P_NONE, 0, RT325, R_HL),
	R(ASSIGN,INDEX,P_NUM,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld ($L),$Rl\n\tld ($L+),$Rh\n", 0),
	R(ASSIGN,INDEX,INHL,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_LDLL, R_HL),
	/*
	 * These stored from a register and left the value in it, so they
	 * name it.  Claiming whatever register the node was aimed at
	 * would hand the parent one that was never written.
	 */
	R(ASSIGN,INDEX,INHL,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0, T_IDX_S_ST, R_HL),
	/*
	 * A frame slot assigned a frame slot's address - "p = &v".  A
	 * bare index is a place, not a value; reading one is a DEREF
	 * around it, and a copy between two locals comes through here as
	 * =(I,D(I)) with the load already done.  So this form only ever
	 * arises from address-of, and loading through the right operand
	 * made "p = &v" mean "p = v".
	 */
	R(ASSIGN,INDEX,INDEX,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		T_IDX_R_ADDR T_IDX_S_ST, R_HL),
	R(ASSIGN,INDEX,INDE,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld ($L),e\n\tld ($L+),d\n", R_DE),
	R(ASSIGN,INDEX,INBC,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld ($L),c\n\tld ($L+),b\n", R_BC),

	/* store to symref */
	R(ASSIGN,SYMREF,INA,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, RT337, R_A),
	R(ASSIGN,SYMREF,P_NUM,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, RT100, R_A),
	/*
	 * A byte in E, stored to a global.  E is where a byte read back
	 * through a pointer lands, so this is what "g = *p = 0" wants
	 * once pass1 is allowed to rewrite a chained assignment at byte
	 * width - without it that shape reduced to nothing while the
	 * word one beside it worked.  INE has two dozen rules that
	 * compute with it and, until now, none that put it anywhere.
	 */
	R(ASSIGN,SYMREF,INE,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld a,e\n" F_LDLA1, R_A),
	/*
	 * A literal address: "*(int *)0x50".  This is how a driver or a
	 * bootstrap talks to fixed hardware - v6's second level boot
	 * hands the disk controller its command address exactly so - and
	 * there was no rule for any of it, so the store compiled to a
	 * comment and vanished.  asz never sees a comment.
	 *
	 * A global is a SYMREF and needs no DEREF around it; a literal
	 * address arrives as DEREF(NUMBER), so these name the constant
	 * as a grandchild and interpolate it where the symbol would go.
	 * The Z80 addresses (nn) directly, so each is the one
	 * instruction the SYMREF form is.
	 */
	R(ASSIGN,DEREF,P_NUM,P_NUM,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld hl,$R\n\tld ($LLa),hl\n", R_HL),
	R(ASSIGN,DEREF,SYMREF,P_NUM,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld hl,$R\n\tld ($LLa),hl\n", R_HL),
	R(ASSIGN,DEREF,P_NUM,P_NUM,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld a,$R\n\tld ($LLa),a\n", R_A),
	R(ASSIGN,DEREF,INHL,P_NUM,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld ($LLa),hl\n", R_HL),
	R(ASSIGN,DEREF,INA,P_NUM,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld ($LLa),a\n", R_A),

	/*
	 * The same store from the OTHER two register pairs, because the
	 * value put there need not be in HL: a register variable lives
	 * in BC, and "*(int *)0xc000 = v" with v in BC is exactly the
	 * shape the second level boot writes.  The SYMREF family has
	 * carried ld (nn),bc and ld (nn),de for a while; a literal
	 * address reaches memory the same one instruction, and without
	 * these the store found no rule and left a marker.
	 */
	R(ASSIGN,DEREF,INBC,P_NUM,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld ($LLa),bc\n", R_BC),
	R(ASSIGN,DEREF,INDE,P_NUM,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld ($LLa),de\n", R_DE),
	/* the narrowing store: a word register keeps only its low byte */
	R(ASSIGN,DEREF,INBC,P_NUM,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
	    F_LDAC "\tld ($LLa),a\n", R_A),

	/*
	 * The long at a literal address.  Same four bytes the SYMREF
	 * forms write, with the constant where the name goes: the value
	 * lives in HL:HL', so the low word goes down, exx, the high word,
	 * exx back.  The CODE forms are here because the long helpers
	 * hand back a CODE that never passed through the step() loop
	 * that would have made it an INHL.
	 */
	R(ASSIGN,DEREF,INHL,P_NUM,0,19, ASSIGN, P_L, P_R, P_NONE, 0,
	    F_EXX "\tld ($LLa),hl\n" F_EXX "\tld ($LLa++),hl\n", R_HL),
	R(ASSIGN,DEREF,INHL,P_NUM,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
	    F_EXX "\tld ($LLa),hl\n" F_EXX "\tld ($LLa++),hl\n", 0),
	R(ASSIGN,DEREF,CODE,P_NUM,0,19, ASSIGN, P_L, P_R, P_NONE, 0,
	    F_EXX "\tld ($LLa),hl\n" F_EXX "\tld ($LLa++),hl\n", R_HL),
	R(ASSIGN,DEREF,CODE,P_NUM,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
	    F_EXX "\tld ($LLa),hl\n" F_EXX "\tld ($LLa++),hl\n", 0),
	/*
	 * A long constant.  There is no ld (nn),n on this machine - the
	 * word and byte forms above get away with ld (nn),hl and
	 * ld (nn),a, and there is no such instruction for an immediate -
	 * so point HL at the address and walk it, exactly as the SYMREF
	 * form does for a global.
	 */
	R(ASSIGN,DEREF,P_NUM,P_NUM,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld hl,$RW\n\tld ($LLa),hl\n"
	    "\tld hl,$Rw\n\tld ($LLa++),hl\n", R_HL),
	R(ASSIGN,DEREF,P_NUM,P_NUM,0,19, ASSIGN, P_L, P_R, P_NONE, 0,
	    "\tld hl,$LLa\n" F_LDHLR2 F_INCHL F_LDHLR3
	    F_INCHL F_LDHLRL F_INCHL F_LDHLRH
	    "\tld l,$Rl\n\tld h,$Rh\n" F_EXX "\tld l,$R2\n\tld h,$R3\n" F_EXX, R_HL),

	R(ASSIGN,SYMREF,INHL,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0, F_LDLHL, R_HL),
	/* narrowing store: a word result keeps only its low byte */
	R(ASSIGN,SYMREF,INHL,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_LDAL F_LDLA1, R_A),
	R(ASSIGN,SYMREF,INBC,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, RT224, R_A),
	/*
	 * A register variable stored to a global.  ld (nn),bc is four
	 * bytes and there was no rule for it at all, so "g = r" emitted
	 * nothing and said so in a marker nobody had run into.
	 */
	R(ASSIGN,SYMREF,INBC,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld ($L),bc\n", R_BC),
	R(ASSIGN,INDEX,INBC,0,0,17, ASSIGN, P_L, P_R, P_NONE, 0, RT224, R_A),
	R(ASSIGN,INDEX,INBC,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, "\tld ($L),c\n", 0),
	R(ASSIGN,SYMREF,P_NUM,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0, RT324, R_HL),
	/*
	 * A byte in A stored to a word.  This is what a truth test or a
	 * comparison used for its value comes to: the flag becomes a
	 * nought or a one in A, and what it goes into is wider than
	 * that.  Unsigned by construction, so the top half is zero;
	 * anything genuinely signed arrives wrapped in a SEXT.
	 */
	R(ASSIGN,SYMREF,INA,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDLA F_LDH0 F_LDLHL, R_HL),
	R(ASSIGN,INDEX,INA,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDLA F_LDH0 T_IDX_S_ST, R_HL),

	/* load constant to register variable */
	R(ASSIGN,REGVAR,P_NUM,0,0,0, ASSIGN, P_L, P_R, P_L, RF_IX, RT162, R_IX),
	R(ASSIGN,REGVAR,P_NUM,0,0,0, ASSIGN, P_L, P_R, P_L, RF_BC, RT128, R_BC),
	R(ASSIGN,REGVAR,P_NUM,0,0,0, ASSIGN, P_L, P_R, P_L, RF_DE, RT275, R_DE),
	R(ASSIGN,REGVAR,P_NUM,0,0,0, ASSIGN, P_L, P_R, P_L, RF_HL, RT323, R_HL),

	/* load constant to register (already converted) */
	/*
	 * A 32-bit constant first, since the rule below carries no width
	 * and would otherwise take it and keep the low half.  That is how
	 * a long constant passed as an argument arrived as its bottom two
	 * bytes with DE left holding whatever was there before.
	 */
	R(ASSIGN,INHL,P_NUM,0,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld l,$Rl\n\tld h,$Rh\n" F_EXX "\tld l,$R2\n\tld h,$R3\n" F_EXX, R_HL),
	R(ASSIGN,INBC,P_NUM,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT128, R_BC),
	R(ASSIGN,INDE,P_NUM,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT275, R_DE),
	R(ASSIGN,INHL,P_NUM,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT323, R_HL),

	/* assign to IX register variable */
	/*
	 * The index register holds a pointer that is used for field
	 * access, which is what it is chosen for - but the pointer is
	 * still a value, and gets assigned, compared and passed like any
	 * other.  Almost none of that had a rule.
	 *
	 * The index registers cannot be compared or arithmetic'd against
	 * anything except through HL, so those go via the stack.  Loading
	 * and storing them whole does not: ld ix,nn and ld (nn),ix exist.
	 */
	R(ASSIGN,REGVAR,SYMREF,0,0,0, ASSIGN, P_L, P_R, P_L, RF_IX, RT162, R_IX),
	R(ASSIGN,SYMREF,REGVAR,0,0,0, ASSIGN, P_L, P_R, P_R, RF_IX, "\tld ($L),ix\n", R_IX),
	/*
	 * There was a second copy of the rule above matching "C" instead
	 * of "V", because a value that had been worked out INTO IX
	 * stayed a bare CODE node - IX had no register node of its own,
	 * where HL, DE, BC and the byte registers each do.  It has one
	 * now: a result in IX reduces to the REGVAR naming it, so the
	 * rule above matches both and the copy is gone.  The "C:F" test
	 * that stood beside it went the same way.
	 */
	R(ASSIGN,INDEX,REGVAR,0,0,2, ASSIGN, P_L, P_R, P_R, RF_IX,
		"\tpush ix\n" F_POPHL T_IDX_S_ST, R_HL),
	/*
	 * "p + n" folds into an indexed location, so assigning one back
	 * is how a pointer step arrives here.  The base register of that
	 * location need not be the one being assigned to, so the address
	 * is worked out rather than added in place.
	 */
	R(ASSIGN,REGVAR,INDEX,0,0,0, ASSIGN, P_L, P_R, P_L, RF_IX,
		"\tpush $Rr\n" F_POPHL "\tld de,$Ro\n" F_ADDHLDE
		F_PUSHHL "\tpop ix\n", R_IX),

	R(ASSIGN,REGVAR,INHL,0,0,0, ASSIGN, P_L, P_R, P_L, RF_IX, F_PUSHHL "\tpop ix\n", R_IX),
	R(ASSIGN,REGVAR,INDE,0,0,0, ASSIGN, P_L, P_R, P_L, RF_IX, "\tpush de\n\tpop ix\n", R_IX),
	R(ASSIGN,REGVAR,INBC,0,0,0, ASSIGN, P_L, P_R, P_L, RF_IX, F_PUSHBC "\tpop ix\n", R_IX),

	/* register-to-register moves */
	R(ASSIGN,INBC,INHL,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, "\tld c,l\n\tld b,h\n", R_BC),
	R(ASSIGN,INDE,INHL,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT202, R_DE),
	R(ASSIGN,INHL,INDE,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT202, R_HL),
	R(ASSIGN,INHL,INBC,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, F_LDLC F_LDHB, R_HL),
	R(ASSIGN,INBC,INDE,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, "\tld c,e\n\tld b,d\n", R_BC),
	R(ASSIGN,INDE,INBC,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, F_LDEC F_LDDB, R_DE),
	R(ASSIGN,INBC,INBC,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT0, R_BC),
	R(ASSIGN,INDE,INDE,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT0, R_DE),
	R(ASSIGN,INHL,INHL,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT0, R_HL),

	/* assign to CODE result */
	R(ASSIGN,CODE,INHL,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT0, R_HL),
	R(ASSIGN,CODE,INDE,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT0, R_DE),
	R(ASSIGN,CODE,INBC,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT0, R_BC),
	R(ASSIGN,CODE,INA,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT0, R_A),

	/* storing one: a pair at a time to a global, a byte at a time to
	 * a local, since only (hl) takes an immediate */
	/*
	 * The value forms first, so they win when the value is wanted.
	 * The stores read HL:DE and write memory - the value is still
	 * sitting in the registers when they finish - so keeping it
	 * costs nothing but saying so.  Without these, a long
	 * assignment used as a value matched the plain form, which
	 * claims no register, and whatever was above it had an operand
	 * with no location: "if ((pos = off - ftell(f)) == 0)" in
	 * libcpm's fseek left its condition unreduced.
	 */
	R(ASSIGN,SYMREF,INHL,0,0,19, ASSIGN, P_L, P_R, P_NONE, 0, RT85, R_HL),
	R(ASSIGN,INDEX,INHL,0,0,19, ASSIGN, P_L, P_R, P_L, RF_IXIY, RT87, R_HL),
	R(ASSIGN,SYMREF,CODE,0,0,19, ASSIGN, P_L, P_R, P_NONE, 0, RT85, R_HL),
	R(ASSIGN,INDEX,CODE,0,0,19, ASSIGN, P_L, P_R, P_L, RF_IXIY, RT87, R_HL),
	R(ASSIGN,SYMREF,INHL,0,0,3, ASSIGN, P_L, P_R, P_NONE, 0, RT85, 0),
	R(ASSIGN,INDEX,INHL,0,0,3, ASSIGN, P_L, P_R, P_L, RF_IXIY, RT87, 0),
	/* the long helpers hand back a CODE that never passed through the
	 * step() loop that would have made it an INHL */
	R(ASSIGN,INDEX,CODE,0,0,3, ASSIGN, P_L, P_R, P_L, RF_IXIY, RT87, 0),
	R(ASSIGN,SYMREF,CODE,0,0,3, ASSIGN, P_L, P_R, P_NONE, 0, RT85, 0),
	/* the value form: store, then put the constant back in HL:DE */
	R(ASSIGN,INDEX,P_NUM,0,0,19, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld ($L),$R2\n\tld ($L+),$R3\n"
		"\tld ($L++),$Rl\n\tld ($L+++),$Rh\n"
		"\tld l,$Rl\n\tld h,$Rh\n" F_EXX "\tld l,$R2\n\tld h,$R3\n" F_EXX, R_HL),
	R(ASSIGN,INDEX,P_NUM,0,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld ($L),$R2\n\tld ($L+),$R3\n"
		"\tld ($L++),$Rl\n\tld ($L+++),$Rh\n", 0),
	/* no ld (nn),n, so point HL at the global and walk it */
	/* the value form first, so it wins when the value is wanted */
	R(ASSIGN,SYMREF,P_NUM,0,0,19, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDHLL F_LDHLR2 F_INCHL F_LDHLR3
		F_INCHL F_LDHLRL F_INCHL F_LDHLRH
		"\tld l,$Rl\n\tld h,$Rh\n" F_EXX "\tld l,$R2\n\tld h,$R3\n" F_EXX, R_HL),
	R(ASSIGN,SYMREF,P_NUM,0,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDHLL F_LDHLR2 F_INCHL F_LDHLR3
		F_INCHL F_LDHLRL F_INCHL F_LDHLRH, 0),
	/*
	 * A constant through a pointer that lives in memory, with the
	 * value kept.  The plain forms of these are older; the value
	 * forms exist because the matcher now refuses to hand a value
	 * consumer a store that produces none.
	 */
	R(ASSIGN,DEREF,P_NUM,SYMREF,0,17, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld hl,($LL)\n\tld (hl),$R\n\tld a,$R\n", R_A),
	R(ASSIGN,DEREF,P_NUM,SYMREF,0,18, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld hl,($LL)\n\tld (hl),$Rl\n" F_INCHL "\tld (hl),$Rh\n"
		"\tld hl,$R\n", R_HL),
	R(ASSIGN,DEREF,P_NUM,SYMREF,0,19, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld hl,($LL)\n\tld (hl),$R2\n" F_INCHL "\tld (hl),$R3\n"
		F_INCHL "\tld (hl),$Rl\n" F_INCHL "\tld (hl),$Rh\n"
		"\tld l,$Rl\n\tld h,$Rh\n" F_EXX "\tld l,$R2\n\tld h,$R3\n" F_EXX, R_HL),
	/* a long already in HL:DE is the return value as it stands */
	R(ASSIGN,INHL,CODE,0,0,3, ASSIGN, P_L, P_R, P_NONE, 0, RT0, R_HL),

	/*
	 * Storing a long constant through an address, which the four
	 * (hl) writes reach whether it came from a pointer variable or
	 * was worked out.  There is no ld (nn),n for any width, so the
	 * address has to be in HL either way.
	 */
	/*
	 * Storing through a pointer that is itself a global: load the
	 * pointer, then write through it.  Only the long form of this
	 * existed, so "p[0] = c" had nowhere to go once the subscript
	 * stopped being folded into the pointer's own address.
	 *
	 * The word form has to get the value out of HL first, since that
	 * is where the address has to end up.
	 */
	R(ASSIGN,DEREF,P_NUM,SYMREF,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld hl,($LL)\n\tld (hl),$R\n", 0),
	R(ASSIGN,DEREF,P_NUM,SYMREF,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld hl,($LL)\n\tld (hl),$Rl\n" F_INCHL "\tld (hl),$Rh\n", 0),
	R(ASSIGN,DEREF,INA,SYMREF,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld hl,($LL)\n\tld (hl),a\n", R_A),
	R(ASSIGN,DEREF,INHL,SYMREF,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDAL "\tld hl,($LL)\n\tld (hl),a\n", R_A),
	/*
	 * The same store with the value in BC, which is where a word
	 * register variable lives.  "*Curschar = nchar" in vi's normal.c
	 * is exactly this - a byte stored through a global char pointer
	 * from an int local the allocator put in BC - and it had no rule
	 * while the A and HL forms beside it did.
	 */
	R(ASSIGN,DEREF,INBC,SYMREF,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDAC "\tld hl,($LL)\n\tld (hl),a\n", R_A),
	R(ASSIGN,DEREF,INDE,SYMREF,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld hl,($LL)\n" F_LDHLE F_INCHL F_LDHLD, R_DE),
	R(ASSIGN,DEREF,INHL,SYMREF,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_EXDEHL "\tld hl,($LL)\n" F_LDHLE F_INCHL F_LDHLD F_EXDEHL, R_HL),
	/*
	 * Storing a symbol's own address - which is what a string
	 * literal is, since pass1 turns one into a SYMREF for the label
	 * it emitted.  A SYMREF is left unreduced so the load and store
	 * rules can use it as an address, so where its value is wanted
	 * it has to be loaded, and these are the store shapes that had
	 * no form for it:
	 *
	 *	arr[i] = "lit";		nothing stored, marker left
	 *	sp->f  = "lit";		the same
	 *
	 * A constant subscript folds to a plain symbol store and was
	 * always right, and any non-literal value is in a register
	 * already, so this only ever showed with a literal and a
	 * subscript that had to be worked out.
	 */
	R(ASSIGN,DEREF,SYMREF,SYMREF,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDDER "\tld hl,($LL)\n" F_LDHLE F_INCHL F_LDHLD, R_DE),
	/*
	 * And the same store one byte wide, which is the address
	 * TRUNCATED rather than kept.  The kernel builds a controller
	 * command block a byte at a time:
	 *
	 *	*p++ = ((unsigned) djcomm >> 0);
	 *	*p++ = ((unsigned) djcomm >> 8);
	 *
	 * and the first of those is this shape, the ">> 0" having folded
	 * away and left the bare address being stored into a byte.  The
	 * second and third are shifts and reduce to a register before
	 * they get here, so only the one that looks least like a
	 * truncation is the one that had no rule - sys/cus.c, djinit.
	 *
	 * The address goes to DE rather than HL because HL is where the
	 * POINTER has to end up, and it has to be loaded second: $LL is
	 * a pointer in memory and reading it would overwrite the value
	 * if the value were sitting in HL.
	 */
	R(ASSIGN,DEREF,SYMREF,SYMREF,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDDER "\tld a,e\n\tld hl,($LL)\n\tld (hl),a\n", R_A),
	/*
	 * A register variable stored through a pointer in memory:
	 * "*p = r" with r in BC.  The value needs no code, so the
	 * pointer can come to HL and take the store directly.
	 */
	R(ASSIGN,DEREF,INBC,SYMREF,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld hl,($LL)\n\tld (hl),c\n" F_INCHL "\tld (hl),b\n", 0),
	R(ASSIGN,DEREF,SYMREF,INHL,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDDER F_LDHLE F_INCHL F_LDHLD, R_DE),
	/*
	 * The byte of it, which is the address truncated.  This is the
	 * one sys/cus.c hits:
	 *
	 *	*p++ = ((unsigned) djcomm >> 0);
	 *
	 * with p a local the allocator put in HL.  The address goes to
	 * DE and not HL because HL already holds the pointer and is
	 * about to be stored through.
	 */
	R(ASSIGN,DEREF,SYMREF,INHL,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDDER "\tld a,e\n" F_LDHLA, R_A),
	/*
	 * The same with the pointer in BC instead of HL, which is where
	 * a word register variable lives.  "ichan[0] = &cmd" in the mw
	 * driver is exactly it: the channel array's address is a
	 * register variable and the value stored is a symbol's own
	 * address.  The INHL form above and the INDEX form below both
	 * existed; this one did not, so the store compiled to a comment.
	 *
	 * BC cannot be stored through, so the pointer moves to HL
	 * first.  The value is in DE by then and survives the move.
	 */
	R(ASSIGN,DEREF,SYMREF,INBC,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDDER F_LDLC F_LDHB F_LDHLE F_INCHL F_LDHLD, R_DE),
	R(ASSIGN,DEREF,SYMREF,INDEX,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDDER "\tld l,($LL)\n\tld h,($LL+)\n"
		F_LDHLE F_INCHL F_LDHLD, R_DE),

	R(ASSIGN,DEREF,P_NUM,SYMREF,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld hl,($LL)\n" T_ST_IHL_N, 0),
	/* the value form: store, then put the constant back in HL:DE */
	R(ASSIGN,DEREF,P_NUM,INHL,0,19, ASSIGN, P_L, P_R, P_NONE, 0,
		T_ST_IHL_N "\tld l,$Rl\n\tld h,$Rh\n" F_EXX "\tld l,$R2\n\tld h,$R3\n" F_EXX,
		R_HL),
	R(ASSIGN,DEREF,P_NUM,INHL,0,3, ASSIGN, P_L, P_R, P_NONE, 0, T_ST_IHL_N, 0),
	/*
	 * A long value stored through a pointer.  The value fills HL:DE,
	 * so there is nowhere to put the address except the stack - and
	 * that is exactly how lstde wants it: value in the pair, address
	 * pushed, which it consumes.
	 *
	 * ex (sp),hl does the swap without a spare register: push the
	 * high word, load the pointer over it, then trade.
	 */
	/*
	 * These called lstde inside a $[ $] pair, because lstde used to
	 * park its return address in BC - the register-variable home -
	 * and hand it back as the caller's variable.  That was fixed in
	 * lstde itself, which keeps the address in a static word now and
	 * touches BC nowhere, so the guard has been paying two bytes a
	 * site for a hazard that no longer exists.  The arithmetic
	 * helpers still need theirs: amul and adiv really do count in B.
	 */
	R(ASSIGN,DEREF,INHL,SYMREF,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
		F_PUSHHL "\tld hl,($LL)\n\tex (sp),hl\n\tcall qst\n",
		R_HL),
	R(ASSIGN,DEREF,INHL,INDEX,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
		F_PUSHHL "\tld l,($LL)\n\tld h,($LL+)\n\tex (sp),hl\n"
		"\tcall qst\n", R_HL),
	R(ASSIGN,DEREF,P_NUM,INDEX,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDLLL F_LDHLL1 T_ST_IHL_N, 0),
	/*
	 * And through the index register home.  "cp->value = value" in
	 * pass1's addCase, with cp a register pointer into the case
	 * array, is a long stored through IX and had no form at all: the
	 * store emitted nothing, so a self-hosted c0 gave every case
	 * label whatever the freshly grown array happened to hold.
	 *
	 * The dance is the one above with IX as the source of the
	 * address: push the high word, bring IX into HL, swap so the
	 * address is the stacked operand lstde takes and the high word
	 * is back in HL.
	 */
	R(ASSIGN,DEREF,INHL,REGVAR,0,3, ASSIGN, P_L, P_R, P_LL, RF_IX,
		F_PUSHHL "\tpush ix\n" F_POPHL "\tex (sp),hl\n"
		"\tcall qst\n", R_HL),
	R(ASSIGN,DEREF,P_NUM,REGVAR,0,3, ASSIGN, P_L, P_R, P_LL, RF_IX,
		"\tpush ix\n" F_POPHL T_ST_IHL_N, 0),
	/* and through the other register home - "*tp = t" in libu's time,
	 * writing the clock through the caller's pointer */
	R(ASSIGN,DEREF,INHL,INBC,0,3, ASSIGN, P_L, P_R, P_NONE, 0,
		F_PUSHHL T_BC_HL "\tex (sp),hl\n\tcall qst\n", R_HL),

	/* zero-extended loads */
	R(ASSIGN,INBC,INA,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, "\tld c,a\n\tld b,0\n", R_BC),
	R(ASSIGN,INHL,INA,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, F_LDLA F_LDH0, R_HL),
	R(ASSIGN,INDE,INA,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, "\tld e,a\n\tld d,0\n", R_DE),

	/* byte stores */
	R(ASSIGN,INDEX,INA,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT337, R_A),
	R(ASSIGN,INHL,P_NUM,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, 0, 0),
	R(ASSIGN,INHL,INA,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_LDLA, R_HL),
	R(ASSIGN,INHL,REGVAR,0,0,1, ASSIGN, P_L, P_R, P_R, RF_BC, "\tld (hl),c\n", 0),

	/* indirect stores via registers */
	R(ASSIGN,DEREF,INA,INBC,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_LDLC F_LDHB F_LDHLA, 0),
	R(ASSIGN,DEREF,INA,INDE,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_EXDEHL F_LDHLA F_EXDEHL, 0),
	R(ASSIGN,DEREF,INHL,INBC,0,2, ASSIGN, P_L, P_R, P_NONE, 0, F_PUSHHL F_LDLC F_LDHB F_POPDE F_LDHLE F_INCHL F_LDHLD, 0),
	R(ASSIGN,DEREF,INHL,INDE,0,2, ASSIGN, P_L, P_R, P_NONE, 0, F_EXDEHL "\tpush de\n" F_LDHLE F_INCHL F_LDHLD F_POPHL, 0),

	/*
	 * Store through a struct pointer in IX.  Only offset zero lands
	 * here - a non-zero member offset folds into an INDEX first via
	 * +(V,N), which is why these are the forms that were missing.
	 */
	R(ASSIGN,DEREF,P_NUM,REGVAR,0,2, ASSIGN, P_L, P_R, P_LL, RF_IX,
		"\tld (ix+0),$Rl\n\tld (ix+1),$Rh\n", 0),
	/* destval says so because it is true: the store reads HL and
	 * leaves it - which is what lets "head = p->next = q" chain */
	R(ASSIGN,DEREF,INHL,REGVAR,0,2, ASSIGN, P_L, P_R, P_LL, RF_IX,
		"\tld (ix+0),l\n\tld (ix+1),h\n", R_HL),
	/*
	 * A symbol's address stored through a register pointer.  "*p =
	 * buffer" with p a register char ** - pr's column-pointer reset
	 * - had the INHL and P_NUM forms beside it and not this one.
	 */
	R(ASSIGN,DEREF,SYMREF,REGVAR,0,2, ASSIGN, P_L, P_R, P_LL, RF_IX,
		F_LDDER "\tld (ix+0),e\n\tld (ix+1),d\n", R_DE),
	/*
	 * bc names its value like its siblings do.  The hl and de forms
	 * of this store say R_HL and R_DE; this one said nothing, and a
	 * store used as a value - if ((rep->ad2 = p) > reend), with rep
	 * the index register and p in bc - left the comparison over it
	 * with no rule.  The store does not touch bc.
	 */
	R(ASSIGN,DEREF,INBC,REGVAR,0,2, ASSIGN, P_L, P_R, P_LL, RF_IX,
		"\tld (ix+0),c\n\tld (ix+1),b\n", R_BC),
	/*
	 * The same with the value in de, which is where it sits when the
	 * store is one operand of a comparison: the other operand has
	 * taken hl.  The hl, bc and constant forms were all here and
	 * this was not, so
	 *
	 *	if ((rep->ad2 = p) > reend)
	 *
	 * left the whole condition unreduced - sed writes it that way,
	 * and GT normalizes to LT by exchanging its operands, which is
	 * what puts the store on the side that gets de.  Written the
	 * other way round it compiled, which made it look like a
	 * register-allocation quirk rather than a missing form.
	 *
	 * R_DE, not 0: the value of a store is what was stored, and
	 * LT,INHL,INDE above takes it from there.
	 */
	R(ASSIGN,DEREF,INDE,REGVAR,0,2, ASSIGN, P_L, P_R, P_LL, RF_IX,
		"\tld (ix+0),e\n\tld (ix+1),d\n", R_DE),
	R(ASSIGN,DEREF,P_NUM,REGVAR,0,1, ASSIGN, P_L, P_R, P_LL, RF_IX, "\tld (ix+0),$R\n", 0),
	R(ASSIGN,DEREF,INA,REGVAR,0,1, ASSIGN, P_L, P_R, P_LL, RF_IX, "\tld (ix+0),a\n", R_A),
	/* a word in HL narrowed on its way through the index register */
	R(ASSIGN,DEREF,INHL,REGVAR,0,1, ASSIGN, P_L, P_R, P_LL, RF_IX,
		F_LDAL "\tld (ix+0),a\n", R_A),
	/*
	 * And the same from BC, which is where a register variable
	 * lives - the third form these rules keep needing.  Without it
	 *
	 *	t->size = off;
	 *
	 * with size a byte member and off a local word left a marker
	 * and stored nothing, so type.c's member offsets stayed zero
	 * and every struct came out either empty or "too big".
	 */
	R(ASSIGN,DEREF,INBC,REGVAR,0,1, ASSIGN, P_L, P_R, P_LL, RF_IX,
		F_LDAC "\tld (ix+0),a\n", R_A),
	R(ASSIGN,DEREF,INDE,REGVAR,0,1, ASSIGN, P_L, P_R, P_LL, RF_IX,
		"\tld a,e\n\tld (ix+0),a\n", R_A),

	/*
	 * The index register stored through a pointer, rather than used
	 * as one.  It goes out through DE.
	 *
	 * The half-register forms are fine to use - they work on every
	 * Z80 - and this file uses them wherever they win: ld a,ixl for
	 * a single byte, and ld c,ixl / ld e,ixl to reach BC and DE.
	 * What they cannot do is reach HL, because the DD prefix renames
	 * H and L to IXH and IXL for the whole instruction, so "ld l,ixl"
	 * has no encoding at all - the assembler rejects it.  HL can only
	 * be had through the stack.
	 *
	 * For a pair the stack is smaller anyway: push ix / pop de and
	 * the three stores is six bytes, against seven for ld e,ixl /
	 * ld d,ixh, each half costing two for its prefix.  Going through
	 * A a byte at a time is seven as well.
	 */
	/*
	 * Storing a register variable in ix, by the address it goes to.
	 *
	 * The ones whose address is named in the instruction - a global,
	 * a register pointer - say where their value is, so that an
	 * assignment can be used as one:
	 *
	 *	if ((rep->ad2 = p) > reend)
	 *
	 * With destval 0 the rewrite has nothing to relabel the ASSIGN
	 * to, the operator over it never reduces, and the expression
	 * comes out as no rule though the store had a rule all along.
	 * Those that borrow hl for the address save it, because an
	 * assignment used as a value sits under an operator that has
	 * already evaluated its other operand into hl.
	 *
	 * The two whose address ARRIVES in hl stay valueless.  Whatever
	 * put it there - the shift and add of a subscript - ran before
	 * this template and outside any save it could make, and it ran
	 * through the same hl the operator above was using.  Naming a
	 * value buys a silent wrong answer in place of the refusal; the
	 * refusal stands until the ordering is fixed.
	 */
	R(ASSIGN,DEREF,REGVAR,INHL,0,2, ASSIGN, P_L, P_R, P_R, RF_IX,
		"\tpush ix\n\tpop de\n" F_LDHLE F_INCHL F_LDHLD, R_DE),
	R(ASSIGN,DEREF,REGVAR,INHL,0,1, ASSIGN, P_L, P_R, P_R, RF_IX,
		"\tld a,ixl\n" F_LDHLA, R_A),
	R(ASSIGN,DEREF,REGVAR,INBC,0,2, ASSIGN, P_L, P_R, P_R, RF_IX,
		F_PUSHHL T_BC_HL "\tpush ix\n\tpop de\n" F_LDHLE F_INCHL
		F_LDHLD F_POPHL, R_DE),
	R(ASSIGN,DEREF,REGVAR,INDEX,0,2, ASSIGN, P_L, P_R, P_R, RF_IX,
		F_LDLLL F_LDHLL1 "\tpush ix\n\tpop de\n"
		F_LDHLE F_INCHL F_LDHLD, R_DE),
	R(ASSIGN,DEREF,REGVAR,SYMREF,0,2, ASSIGN, P_L, P_R, P_R, RF_IX,
		F_PUSHHL "\tld hl,($LL)\n\tpush ix\n" F_POPDE
		F_LDHLE F_INCHL F_LDHLD F_POPHL, R_DE),

	/*
	 * Store through a pointer that itself lives in memory - a pointer
	 * parameter, say.  Load it first; the HL form has to shuffle
	 * through DE because HL is holding the value.
	 */
	R(ASSIGN,DEREF,P_NUM,INDEX,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDLLL F_LDHLL1 F_LDHLRL F_INCHL F_LDHLRH, 0),
	R(ASSIGN,DEREF,INHL,INDEX,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld e,($LL)\n\tld d,($LL+)\n" F_EXDEHL F_LDHLE F_INCHL F_LDHLD, 0),
	R(ASSIGN,DEREF,P_NUM,INDEX,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDLLL F_LDHLL1 "\tld (hl),$R\n", 0),
	/*
	 * R_A, and not 0: the value of a store is what was stored, and
	 * that is the byte in a.  With 0 the value came from e->tgt,
	 * which here was hl - the pointer this stored THROUGH - so
	 *
	 *		while (*p1 = p2[*p1])
	 *
	 * tested the address instead of the byte, and an address is
	 * never zero, so the loop never ended.  Silently: no diagnostic,
	 * correct-looking code, a program that hangs.  The SYMREF form
	 * of this store has always said R_A; this one did not, and which
	 * of the two fired depended on whether the allocator had left
	 * the pointer in a register or spilled it to the frame - so the
	 * same loop was right or wrong according to how many register
	 * variables the function declared.
	 */
	R(ASSIGN,DEREF,INA,INDEX,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDLLL F_LDHLL1 F_LDHLA, R_A),
	/*
	 * The same with the value in HL, narrowing to its low byte.  The
	 * word form above is here and the byte one was not, so storing a
	 * byte through a pointer in a frame slot emitted nothing.  Take
	 * the byte out of L before loading the pointer, which wants HL.
	 */
	R(ASSIGN,DEREF,INHL,INDEX,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDAL F_LDLLL F_LDHLL1 F_LDHLA, R_A),
	/*
	 * A register variable stored through a pointer in a frame slot,
	 * narrowing on the way - "where[0] = v" with v in BC.  Load the
	 * value out of BC before the pointer, since the pointer wants HL.
	 */
	R(ASSIGN,DEREF,INBC,INDEX,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDAC F_LDLLL F_LDHLL1 F_LDHLA, R_A),
	R(ASSIGN,DEREF,INBC,INDEX,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDLLL F_LDHLL1 "\tld (hl),c\n" F_INCHL "\tld (hl),b\n", R_BC),
	R(ASSIGN,DEREF,INDE,INDEX,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDLLL F_LDHLL1 F_LDHLE, R_DE),
	R(ASSIGN,DEREF,INDE,INDEX,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDLLL F_LDHLL1 F_LDHLE F_INCHL F_LDHLD, R_DE),

	/* indirect stores via HL */
	/*
	 * The value forms first.  Storing a constant through an address
	 * in HL leaves the address there, not the constant, so a chained
	 * assignment took the leftover pointer for the value:
	 *
	 *	a->left = a->right = NULL;
	 *
	 * in docall nulled the right and filled the left with the
	 * address of the right, one past.  freeexpr then walked into a
	 * node the loop had just handed to pusharg, and the self-hosted
	 * c1 died in free() with the stack in the heap.  Only when the
	 * address has to be worked out: a frame slot or a register home
	 * has a form of its own further up, which is why the same line
	 * elsewhere in the tree was fine.
	 */
	R(ASSIGN,DEREF,P_NUM,INHL,0,17, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld (hl),$R\n\tld a,$R\n", R_A),
	R(ASSIGN,DEREF,P_NUM,INHL,0,1, ASSIGN, P_L, P_R, P_NONE, 0, "\tld (hl),$R\n", 0),
	/*
	 * The stored value is the value of the assignment.  Where a
	 * chain consumes it - "d = ap->init = malloc(n)" - the store
	 * has to end with the VALUE in HL, not the address it went to:
	 * the bare form below leaves HL one past the slot, and the
	 * outer assignment filed that into d, which then walked the
	 * assigns table writing tokens over everything after it.  Value
	 * context pays the one-byte ex; the ordinary statement store
	 * keeps the short form.
	 */
	R(ASSIGN,DEREF,INDE,INHL,0,18, ASSIGN, P_L, P_R, P_NONE, 0, T_ST_IHL, R_HL),
	R(ASSIGN,DEREF,INDE,INHL,0,2, ASSIGN, P_L, P_R, P_NONE, 0, F_LDHLE F_INCHL F_LDHLD, 0),
	R(ASSIGN,DEREF,INDE,INHL,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_LDHLE, 0),
	/* a byte worked out in A, stored through an address in HL - which
	 * is where a compound assignment through a computed address ends
	 * up, the value in A and the address recovered from the stack */
	R(ASSIGN,DEREF,INA,INHL,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_LDHLA, R_A),
	/*
	 * The same byte in A, stored through a pointer, to a word.
	 *
	 * The symbol and subscript forms of this are up where the note
	 * about it is; the pointer form was missing, and
	 *
	 *	*(o->ovar) = ! *(o->ovar);
	 *
	 * had no rule - which is every boolean option toggle less
	 * writes.  Unsigned by construction like its siblings: a truth
	 * test leaves a nought or a one, and anything genuinely signed
	 * arrives wrapped in a SEXT.
	 *
	 * Through de rather than the four-byte "ld (hl),a / inc hl /
	 * ld (hl),0", because hl is the address here and the answer has
	 * to be somewhere afterwards.  A store with nowhere to leave
	 * its value cannot be used as one, and that is the shape that
	 * left the comparison above it unreducible in entry 9.
	 */
	R(ASSIGN,DEREF,INA,INHL,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tld e,a\n\tld d,0\n" F_LDHLE F_INCHL F_LDHLD, R_DE),
	/* a word narrowed to a byte on its way through an address */
	R(ASSIGN,DEREF,INBC,INHL,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_LDAC F_LDHLA, R_A),
	R(ASSIGN,DEREF,INHL,INHL,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_LDAL F_LDHLA, R_A),
	R(ASSIGN,DEREF,P_NUM,INBC,0,1, ASSIGN, P_L, P_R, P_NONE, 0, F_LDLC F_LDHB "\tld (hl),$R\n", 0),
	/* the value in HL, which has to be taken out of the way before the
	 * pointer comes over on top of it */
	R(ASSIGN,DEREF,INHL,INBC,0,1, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDAL F_LDLC F_LDHB F_LDHLA, R_A),
	R(ASSIGN,DEREF,P_NUM,INBC,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		T_BC_HL F_LDHLRL F_INCHL F_LDHLRH, 0),
	R(ASSIGN,DEREF,P_NUM,INDE,0,2, ASSIGN, P_L, P_R, P_NONE, 0,
		F_EXDEHL F_LDHLRL F_INCHL F_LDHLRH, 0),
	R(ASSIGN,DEREF,P_NUM,INHL,0,18, ASSIGN, P_L, P_R, P_NONE, 0,
		F_LDHLRL F_INCHL F_LDHLRH F_LDHLR, R_HL),
	R(ASSIGN,DEREF,P_NUM,INHL,0,2, ASSIGN, P_L, P_R, P_NONE, 0, F_LDHLRL F_INCHL F_LDHLRH, 0),
	R(ASSIGN,DEREF,INBC,INHL,0,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld (hl),c\n" F_INCHL "\tld (hl),b\n", 0),

	/* structured loads to BC/DE/HL */
	R(ASSIGN,INBC,DEREF,0,INHL,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld c,(hl)\n" F_INCHL "\tld b,(hl)\n", R_BC),
	/*
	 * A register given a frame slot's address - the other half of
	 * "p = &v", where p happens to live in a register.  As above, a
	 * bare index is a place: these loaded through it and turned every
	 * address-of into a read of what was there.
	 */
	R(ASSIGN,INBC,INDEX,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0,
		T_IDX_R_ADDR "\tld c,l\n\tld b,h\n", R_BC),
	R(ASSIGN,INHL,INDEX,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, T_IDX_R_ADDR, R_HL),
	R(ASSIGN,INDE,INDEX,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, T_IDX_R_ADDR F_EXDEHL, R_DE),
	R(ASSIGN,INDEX,SYMREF,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT325, R_HL),
	/* one symbol's address into another symbol's storage - "lp = &g" */
	R(ASSIGN,SYMREF,SYMREF,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT324, R_HL),
	R(ASSIGN,INBC,SYMREF,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT128, R_BC),
	R(ASSIGN,INHL,SYMREF,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT323, R_HL),
	R(ASSIGN,INDE,SYMREF,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT275, R_DE),
	R(ASSIGN,INBC,DEREF,0,SYMREF,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld a,($RL)\n\tld c,a\n\tld a,($RL+)\n\tld b,a\n", R_BC),
	R(ASSIGN,INDE,DEREF,0,SYMREF,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld de,($RL)\n", R_DE),
	R(ASSIGN,INHL,DEREF,0,SYMREF,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld hl,($RL)\n", R_HL),
	R(ASSIGN,INDE,DEREF,0,INHL,2, ASSIGN, P_L, P_R, P_NONE, 0, RT143, R_DE),
	R(ASSIGN,INHL,DEREF,0,INHL,2, ASSIGN, P_L, P_R, P_NONE, 0, F_LDAHL F_INCHL F_LDHHL F_LDLA, R_HL),
	R(ASSIGN,INDEX,DEREF,0,INHL,2, ASSIGN, P_L, P_R, P_NONE, 0, F_LDAHL F_LDLA1 F_INCHL F_LDAHL "\tld ($L+),a\n", 0),

	/* stores/loads with indexed/symref */
	R(ASSIGN,INA,DEREF,0,INDEX,1, ASSIGN, P_L, P_R, P_NONE, 0, RT101, R_A),
	R(ASSIGN,INA,DEREF,0,SYMREF,1, ASSIGN, P_L, P_R, P_NONE, 0, RT101, R_A),
	R(ASSIGN,INA,INA,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0, RT0, R_A),
	R(ASSIGN,INA,P_NUM,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, "\tld a,$R\n", R_A),
	/*
	 * A byte wanted in A that came back in HL, which is where the
	 * wrapper that lands a value puts it.  Only the low half is
	 * meaningful at this width, so it is one instruction - and it
	 * was the single most repeated thing the compiler could not do.
	 */
	R(ASSIGN,INA,INHL,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, RT235, R_A),
	R(ASSIGN,INA,INBC,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, RT221, R_A),
	R(ASSIGN,INA,INDE,0,0,1, ASSIGN, P_L, P_R, P_NONE, 0, RT121, R_A),
	/* storing a word already in DE to a global, so a nested
	 * assignment can be used for its value */
	R(ASSIGN,SYMREF,INDE,0,0,2, ASSIGN, P_L, P_R, P_NONE, 0, "\tld ($L),de\n", R_DE),

	/*
	 * Storing a register-relative address to a global: the INDEX
	 * form the address-combining rules mint is the value here, not
	 * a location - "g = a + 2" with a in IX.  Every other context
	 * (frame store, argument push, comparison) materializes it on
	 * some other path; this one had no rule at all, fell to the
	 * incomplete-rewrite marker, and the store was silently
	 * dropped - which is how cpp's -I list vanished on the Z80.
	 * add hl takes only bc/de/hl/sp, so the register goes through
	 * the stack.
	 */
	R(ASSIGN,SYMREF,INDEX,0,0,0, ASSIGN, P_L, P_R, P_NONE, 0,
		"\tpush $Rr\n\tpop hl\n\tld de,$Ro\n\tadd hl,de\n\tld ($L),hl\n",
		R_HL),

	/* test a long in memory for zero */
	R(DEREF,SYMREF,0,0,0,11, DEREF, P_L, P_NONE, P_NONE, 0,
		F_LDHLL F_LDAHL F_INCHL F_ORHL F_INCHL
		F_ORHL F_INCHL F_ORHL, F_NZ),

	/*
	 * Loading one, the mirror of the stores above: the low word lives
	 * at the lower address, so it comes back in HL and the high word
	 * in HL'.  A global can move a pair at a time; a frame slot goes a
	 * byte at a time, since (iy+d) is all that reaches it - and (iy+d)
	 * reads the same in either bank, IY not being one of the three.
	 */
	R(DEREF,SYMREF,0,0,0,3, DEREF, P_L, P_NONE, P_NONE, 0,
		F_EXX F_LDHLL2 F_EXX F_LDHLL3, R_HL),
	R(DEREF,INDEX,0,0,0,3, DEREF, P_L, P_NONE, P_L, RF_IXIY,
		"\tcall qld$Lr\n\t.db $Lo\n", R_HL),
	/* through a pointer already in HL */
	R(DEREF,INHL,0,0,0,3, DEREF, P_L, P_NONE, P_NONE, 0, "\tcall qld\n", R_HL),
	/* and through the other register home - doprnt reads its long
	 * argument through the pointer it walks the list with */
	R(DEREF,INBC,0,0,0,3, DEREF, P_L, P_NONE, P_NONE, 0, T_BC_HL "\tcall qld\n", R_HL),

	/* loads from register addresses */
	R(DEREF,INHL,0,0,0,1, DEREF, P_L, P_NONE, P_NONE, 0, F_LDAHL, R_A),
	/*
	 * Load a word through HL.  Where the result is wanted in DE it
	 * can go straight there - three bytes instead of four, no
	 * exchange, and A is left alone.  Only when it has to land back
	 * in HL does A get used as the carrier, because then the pointer
	 * and the result are the same register.
	 */
	R(DEREF,INHL,0,0,0,2, DEREF, P_L, P_NONE, P_NONE, RF_TDE, RT143, R_DE),
	R(DEREF,INHL,0,0,0,2, DEREF, P_L, P_NONE, P_NONE, 0, T_LD_IHL, R_HL),
	R(DEREF,INBC,0,0,0,1, DEREF, P_L, P_NONE, P_NONE, 0, F_LDLC F_LDHB F_LDAHL, R_A),
	/*
	 * Reading through BC when the answer is wanted in DE, which is
	 * where the right operand of a binary node is asked to go.  The
	 * rule below moves the pointer into HL to read through it and
	 * lands there, whatever it was asked for - so as the right
	 * operand of a comparison it overwrote the left one, and then
	 * nothing matched a comparison of HL against HL.  The label was
	 * right all along; the rule simply could not do as it was told.
	 *
	 * ld a,(bc) reads without HL at all.  BC steps forward for the
	 * high byte and back again, so the pointer is as it was.
	 */
	R(DEREF,INBC,0,0,0,2, DEREF, P_L, P_NONE, P_NONE, RF_TDE,
		"\tld a,(bc)\n\tld e,a\n\tinc bc\n"
		"\tld a,(bc)\n\tld d,a\n\tdec bc\n", R_DE),
	R(DEREF,INBC,0,0,0,2, DEREF, P_L, P_NONE, P_NONE, 0, F_LDLC F_LDHB F_LDAHL F_INCHL F_LDHHL F_LDLA, R_HL),
	R(DEREF,INDE,0,0,0,1, DEREF, P_L, P_NONE, P_NONE, 0, F_EXDEHL F_LDAHL, R_A),
	R(DEREF,INDE,0,0,0,2, DEREF, P_L, P_NONE, P_NONE, 0, F_EXDEHL "\tld e,(hl)\n" F_INCHL "\tld d,(hl)\n" F_EXDEHL, R_HL),

	/* pointer testing */
	R(DEREF,INHL,0,0,0,12, DEREF, P_L, P_NONE, P_NONE, 0, F_LDAHL "\tor (hl)\n", F_NZ),
	/*
	 * A frame variable as a truth value.  The flag named here is the
	 * one that means true, and true is "not zero" - these said Z, so
	 * "if (local)" ran its body exactly when the local was zero.
	 * The register-variable rule above has always said NZ, which is
	 * what made the disagreement visible.
	 */
	R(DEREF,INDEX,0,0,0,9, DEREF, P_L, P_NONE, P_NONE, 0, F_LDAL1 F_ORA, F_NZ),
	R(DEREF,INDEX,0,0,0,10, DEREF, P_L, P_NONE, P_NONE, 0, F_LDAL1 "\tor ($L+)\n", F_NZ),
	R(DEREF,INDEX,0,0,0,2, DEREF, P_L, P_NONE, P_NONE, 0, RT71, 0),
	/*
	 * Of the 8-bit registers only A can load from an absolute address,
	 * but the pairs all can, so ld de,(nn) reaches E without touching
	 * A or HL - either of which may hold the left operand.  It reads
	 * the following byte into D as well; D is dead here, and one byte
	 * of over-read is harmless in a flat memory model.
	 */
	/* and reading one back - see the store rules above */
	R(DEREF,P_NUM,0,0,0,2, DEREF, P_L, P_NONE, P_NONE, 0,
	    "\tld hl,($La)\n", R_HL),
	R(DEREF,P_NUM,0,0,0,1, DEREF, P_L, P_NONE, P_NONE, 0,
	    "\tld a,($La)\n", R_A),
	/* and the long, both words */
	R(DEREF,P_NUM,0,0,0,3, DEREF, P_L, P_NONE, P_NONE, 0,
	    F_EXX "\tld hl,($La)\n" F_EXX "\tld hl,($La++)\n", R_HL),

	R(DEREF,SYMREF,0,0,0,1, DEREF, P_L, P_NONE, P_NONE, RF_TDE, F_LDDEL, R_E),
	R(DEREF,SYMREF,0,0,0,1, DEREF, P_L, P_NONE, P_NONE, 0, RT254, R_A),
	R(DEREF,INDEX,0,0,0,1, DEREF, P_L, P_NONE, P_NONE, RF_TDE, "\tld e,($L)\n", R_E),
	/* the plain load: a byte local reaching A, as D(O):b does for a
	 * global.  Without it a byte local could only be read by the rules
	 * that match its parent too, so SEXT of one had nothing to widen */
	R(DEREF,INDEX,0,0,0,1, DEREF, P_L, P_NONE, P_NONE, 0, RT254, R_A),
	/* a byte through a struct pointer in IX at offset zero - a
	 * non-zero member offset folds into an INDEX first */
	R(DEREF,REGVAR,0,0,0,1, DEREF, P_L, P_NONE, P_L, RF_IX, "\tld a,(ix+0)\n", R_A),
	R(DEREF,REGVAR,0,0,0,2, DEREF, P_L, P_NONE, P_L, RF_IX,
		"\tld $t,(ix+0)\n\tld $u,(ix+1)\n", 0),
	/*
	 * And the long, which was the width nobody had written.  Reading
	 * one back through a register pointer - "sum += p->value" over an
	 * array of structs - emitted nothing whatever, and said nothing:
	 * the compound assignment pushed HL and DE as its right operand
	 * and called ladd on whatever the loop condition had left there.
	 * Low word from the lower address, as everywhere else.
	 *
	 * IX is not banked, so both halves are reached the same way from
	 * either side of the exx - which is what makes this four plain
	 * loads where the old layout needed A to shuffle the high word
	 * into HL past the pointer.
	 */
	R(DEREF,REGVAR,0,0,0,3, DEREF, P_L, P_NONE, P_L, RF_IX,
		F_EXX "\tld l,(ix+0)\n\tld h,(ix+1)\n" F_EXX
		"\tld l,(ix+2)\n\tld h,(ix+3)\n", R_HL),
	/* honour the target: as the right operand of a compare this has to
	 * land in DE, or it overwrites the left operand in HL */
	R(DEREF,SYMREF,0,0,0,2, DEREF, P_L, P_NONE, P_NONE, 0, "\tld $T,($L)\n", 0),

	/* negation */
	/* a byte in A negates in place - the Z80 has the instruction */
	R(NEG,INA,0,0,0,1, NEG, P_L, P_NONE, P_NONE, 0, "\tneg\n", R_A),
	R(NEG,INBC,0,0,0,0, NEG, P_L, P_NONE, P_NONE, 0, F_LDA0 "\tsub c\n" F_LDLA F_LDA0 "\tsbc a,b\n" F_LDHA, R_HL),
	R(NEG,INHL,0,0,0,0, NEG, P_L, P_NONE, P_NONE, 0, F_XORA "\tsub l\n" F_LDLA F_LDA0 "\tsbc a,h\n" F_LDHA, R_HL),
	R(NEG,INDE,0,0,0,0, NEG, P_L, P_NONE, P_NONE, 0, F_LDA0 F_SUBE F_LDLA F_LDA0 "\tsbc a,d\n" F_LDHA, R_HL),

	/* complement of a word; the long form is handled in rewrite.c,
	 * beside the long negation it shares its shape with */
	R(NOT,INHL,0,0,0,2, NOT, P_L, P_NONE, P_NONE, 0,
		F_LDAL "\tcpl\n" F_LDLA F_LDAH "\tcpl\n" F_LDHA, R_HL),
	/* the same of a register variable, brought over first */
	R(NOT,INBC,0,0,0,2, NOT, P_L, P_NONE, P_NONE, 0,
		T_BC_HL F_LDAL "\tcpl\n" F_LDLA F_LDAH "\tcpl\n" F_LDHA, R_HL),
	/*
	 * And of a word in de.  The hl and bc forms were here and this
	 * was not, so "a &= ~m" - the complement of a plain variable the
	 * allocator had put in de - reduced to nothing and c1 reported
	 * an expression it could not build.  2.11BSD's chmod writes the
	 * = case of symbolic modes that way.  A parenthesized operand
	 * like ~(a & b) compiled, which is what made it look like a
	 * quirk of the source rather than a missing rule.
	 *
	 * In place, and the result stays in de.  Six bytes, a the only
	 * scratch, hl untouched - and it lands where AND,INBC,INDE below
	 * can take it.  Going through ex de,hl instead would cost more,
	 * clobber hl, and leave the operands in an order no AND rule
	 * spells: there is an AND,INHL,INBC and no AND,INBC,INHL.
	 */
	R(NOT,INDE,0,0,0,2, NOT, P_L, P_NONE, P_NONE, 0,
		"\tld a,d\n\tcpl\n\tld d,a\n\tld a,e\n\tcpl\n\tld e,a\n",
		R_DE),
	R(NOT,INA,0,0,0,1, NOT, P_L, P_NONE, P_NONE, 0, "\tcpl\n", R_A),

	/*
	 * Narrowing, which is the other direction and is only ever a
	 * question of where the low half already is.
	 *
	 * Under HL':HL it is in HL, which is where a narrower reader
	 * looks anyway, so narrowing a long emits nothing at all - the
	 * high word is simply abandoned in HL'.  It used to be an
	 * ex de,hl, the low word having been in DE.  Same for everything
	 * else: the rule is here so that the cast emits nothing rather
	 * than matching nothing.
	 *
	 * pass1 used to write the cast's type over the node and leave no
	 * trace, so "(int)f()" on a long-returning f read HL - which was
	 * then the high word.  Every numeric escape in cpp came back
	 * zero, because escint() is "(int)getint(base)".
	 */
	R(NARROW,INHL,0,0,0,98, NARROW, P_L, P_NONE, P_NONE, 0, RT0, R_HL),
	R(NARROW,INHL,0,0,0,97, NARROW, P_L, P_NONE, P_NONE, 0, F_LDAL, R_A),
	R(NARROW,INHL,0,0,0,1, NARROW, P_L, P_NONE, P_NONE, 0, RT235, R_A),
	R(NARROW,INBC,0,0,0,1, NARROW, P_L, P_NONE, P_NONE, 0, RT221, R_A),
	R(NARROW,INDE,0,0,0,1, NARROW, P_L, P_NONE, P_NONE, 0, RT121, R_A),
	R(NARROW,INA,0,0,0,0, NARROW, P_L, P_NONE, P_NONE, 0, RT0, R_A),
	R(NARROW,INE,0,0,0,0, NARROW, P_L, P_NONE, P_NONE, 0, RT0, R_E),
	R(NARROW,INHL,0,0,0,0, NARROW, P_L, P_NONE, P_NONE, 0, RT0, R_HL),

	/*
	 * Widening a byte to a word.  Unsigned zero-extends; signed puts
	 * bit 7 into carry with rla, then sbc a,a turns that into 00 or
	 * ff.  Both honour the target, since the widened value is as
	 * often the right operand (DE) as the left (HL).
	 */
	R(WIDEN,INA,0,0,0,2, WIDEN, P_L, P_NONE, P_NONE, 0, "\tld $t,a\n\tld $u,0\n", 0),
	/*
	 * A byte that is already in HL rather than in A - which is where
	 * a ternary leaves its value, both arms having been landed in the
	 * one register so the expression has a value whichever way the
	 * branch went.  Only the low half is meaningful, so the high half
	 * is cleared or filled with the sign.
	 *
	 * The width has to be pinned on the operand, not just the result:
	 * a SEXT to short whose operand is already a short is a widening
	 * of a pointer, and filling H with the sign of L destroys it.
	 */
	R(WIDEN,INHL,0,0,0,34, WIDEN, P_L, P_NONE, P_NONE, 0, F_LDH0, R_HL),
	R(WIDEN,REGVAR,0,0,0,2, WIDEN, P_L, P_NONE, P_L, RF_IX, RT174, R_HL),
	R(WIDEN,INBC,0,0,0,66, WIDEN, P_L, P_NONE, P_NONE, 0, RT388, R_HL),
	R(WIDEN,INDEX,0,0,0,66, WIDEN, P_L, P_NONE, P_NONE, 0, RT71, 0),
	R(WIDEN,SYMREF,0,0,0,2, WIDEN, P_L, P_NONE, P_NONE, 0, RT69, 0),
	R(WIDEN,INHL,0,0,0,66, WIDEN, P_L, P_NONE, P_NONE, 0, RT0, R_HL),
	/* an unsigned word widened: the high half is nothing, which is
	 * the whole difference from X(H) */
	R(WIDEN,INHL,0,0,0,3, WIDEN, P_L, P_NONE, P_NONE, 0,
		F_EXX "\tld hl,0\n" F_EXX, R_HL),
	R(WIDEN,INBC,0,0,0,3, WIDEN, P_L, P_NONE, P_NONE, 0,
		T_BC_HL F_EXX "\tld hl,0\n" F_EXX, R_HL),
	R(WIDEN,INA,0,0,0,3, WIDEN, P_L, P_NONE, P_NONE, 0,
		F_LDLA F_LDH0 F_EXX "\tld hl,0\n" F_EXX, R_HL),
	/* widening a word to a word, which the usual conversions ask for
	 * between two pointers of the same size: nothing to do, except
	 * where the word is in the index register and has to come out */
	R(SEXT,REGVAR,0,0,0,2, SEXT, P_L, P_NONE, P_L, RF_IX, RT174, R_HL),
	R(SEXT,INHL,0,0,0,66, SEXT, P_L, P_NONE, P_NONE, 0, RT0, R_HL),
	/*
	 * The same for a word already in BC, where there is still nothing
	 * to extend but it does have to come over.  Without these, a
	 * pointer difference or a subscript on a register variable ran
	 * into a sign extension from short to short that no rule named,
	 * and stopped there.
	 */
	R(SEXT,INBC,0,0,0,66, SEXT, P_L, P_NONE, P_NONE, 0, RT388, R_HL),
	/* and from a frame slot, where it is the same load D(I):s makes */
	R(SEXT,INDEX,0,0,0,66, SEXT, P_L, P_NONE, P_NONE, 0, RT71, 0),
	/* a symbol's address widened to a word, which is what taking a
	 * function's address for a function pointer comes to */
	R(SEXT,SYMREF,0,0,0,2, SEXT, P_L, P_NONE, P_NONE, 0, RT69, 0),
	R(SEXT,INHL,0,0,0,34, SEXT, P_L, P_NONE, P_NONE, 0,
		F_LDAL F_RLA F_SBCAA F_LDHA, R_HL),
	R(SEXT,INA,0,0,0,2, SEXT, P_L, P_NONE, P_NONE, 0,
		"\tld $t,a\n" F_RLA F_SBCAA "\tld $u,a\n", 0),

	/*
	 * 32-bit values live in HL':HL, high word in HL'.
	 *
	 * Widening is where the layout earns its keep.  A word that is
	 * already in HL is already the low half of the long, so nothing
	 * has to move: the whole of the work is filling HL' in.  Under
	 * HL:DE the value had to be shifted into DE first, which is the
	 * ex de,hl these used to open with.
	 *
	 * Sign-extending fills the high word with sign bits - rla puts
	 * bit 15 (or bit 7 for a byte) into carry and sbc a,a spreads it.
	 */
	R(SEXT,INHL,0,0,0,3, SEXT, P_L, P_NONE, P_NONE, 0,
		F_LDAH F_RLA F_SBCAA F_EXX F_LDHA F_LDLA F_EXX, R_HL),
	R(SEXT,INBC,0,0,0,3, SEXT, P_L, P_NONE, P_NONE, 0,
		T_BC_HL F_LDAB F_RLA F_SBCAA F_EXX F_LDHA F_LDLA F_EXX, R_HL),
	R(SEXT,INA,0,0,0,3, SEXT, P_L, P_NONE, P_NONE, 0,
		F_LDLA F_RLA F_SBCAA F_LDHA F_EXX F_LDHA F_LDLA F_EXX, R_HL),
	R(PREINC,SYMREF,0,0,0,27, PREINC, P_L, P_NONE, P_NONE, 0, RT315, R_HL),
	R(PREINC,SYMREF,0,0,0,3, PREINC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL F_CALLLAINC F_EXX F_LDHLL2 F_EXX F_LDHLL3, R_HL),
	R(PREINC,INDEX,0,0,0,27, PREINC, P_L, P_NONE, P_NONE, 0, RT432, R_HL),
	R(PREINC,INDEX,0,0,0,3, PREINC, P_L, P_NONE, P_NONE, 0,
		T_IDX_ADDR F_CALLLAINC T_IDX_ADDR "\tcall qld\n", R_HL),
	R(PREINC,INHL,0,0,0,27, PREINC, P_L, P_NONE, P_NONE, 0, RT364, R_HL),
	R(PREINC,INHL,0,0,0,3, PREINC, P_L, P_NONE, P_NONE, 0,
		F_PUSHHL F_CALLLAINC F_POPHL
		"\tcall qld\n", R_HL),
	R(PREINC,DEREF,0,INBC,0,27, PREINC, P_L, P_NONE, P_NONE, 0, RT418, R_HL),
	R(PREINC,DEREF,0,INBC,0,3, PREINC, P_L, P_NONE, P_NONE, 0,
		T_BC_HL F_CALLLAINC T_BC_HL
		"\tcall qld\n", R_HL),

	/*
	 * Steps whose value nobody reads, for everything register-homed.
	 * The forms below materialise the answer in HL or A, and a bare
	 * "p++;" was paying those bytes to throw the answer away.  These
	 * must sit above the context-less forms: first match wins, and a
	 * pattern with no context suffix matches statement context too.
	 */
	R(PREINC,INBC,0,0,0,24, PREINC, P_L, P_NONE, P_NONE, 0, RT63, 0),
	R(PREINC,REGVAR,0,0,0,25, PREINC, P_L, P_NONE, P_L, RF_B, RT61, 0),
	R(PREINC,REGVAR,0,0,0,25, PREINC, P_L, P_NONE, P_L, RF_C, RT65, 0),

	/* pre-inc/dec */
	R(PREINC,INBC,0,0,0,0, PREINC, P_L, P_NONE, P_NONE, 0, "\tinc bc\n" F_LDLC F_LDHB, R_HL),
	/* inc/dec a word global in place */
	R(PREINC,SYMREF,0,0,0,2, PREINC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL2 F_INCHL F_LDLHL, R_HL),

	/*
	 * inc/dec through an address in HL.  Reading the word costs the
	 * pointer, so it goes on the stack first and comes back to do the
	 * store.  Postfix then undoes the update to get its old value,
	 * the same trick the memory forms use.
	 */
	R(PREINC,INHL,0,0,0,2, PREINC, P_L, P_NONE, P_NONE, 0,
		F_PUSHHL T_LD_IHL F_INCHL T_SWAP_ADDR T_ST_IHL, R_HL),
	/*
	 * The same through a pointer kept in a register, where the DEREF
	 * survives reduction and the address is in BC.  Without these,
	 * "(*p)++" on a register pointer loaded the pointer, took that
	 * for an address and stepped what it pointed at.
	 */
	/*
	 * The byte forms, which had never existed: "(*p)++" on a byte
	 * through a register pointer emitted a marker, and through a
	 * frame pointer too.  ld a,(bc) is one of the Z80's two
	 * one-byte indirect loads, so the BC forms cost nothing extra;
	 * the HL forms read and write through (hl) directly.  Postfix
	 * parks the old value in E, which nothing owns here.
	 */
	R(PREINC,DEREF,0,INBC,0,1, PREINC, P_L, P_NONE, P_NONE, 0,
		"\tld a,(bc)\n\tinc a\n\tld (bc),a\n", R_A),
	R(PREINC,DEREF,0,INHL,0,1, PREINC, P_L, P_NONE, P_NONE, 0,
		"\tinc (hl)\n\tld a,(hl)\n", R_A),
	R(PREINC,DEREF,0,INBC,0,2, PREINC, P_L, P_NONE, P_NONE, 0,
		T_BC_HL F_PUSHHL T_LD_IHL F_INCHL T_SWAP_ADDR T_ST_IHL, R_HL),
	/*
	 * Stepping a pointer the allocator homed in IX.  The BC pointer
	 * has had these all along; through-IX steps existed only fused
	 * with a deref, so "p++" standing alone matched nothing, and a
	 * scan loop over a register pointer read one byte forever - a
	 * marker in the listing, an infinite loop on the machine.  The
	 * statement forms come first: a step nobody wants the value of
	 * is inc ix and not a byte more.
	 */
	R(PREINC,REGVAR,0,0,0,24, PREINC, P_L, P_NONE, P_L, RF_IX, RT67, 0),
	R(PREINC,REGVAR,0,0,0,0, PREINC, P_L, P_NONE, P_L, RF_IX,
		"\tinc ix\n\tpush ix\n" F_POPHL, R_HL),
	/*
	 * And stepping the pointer that IX POINTS AT, which is the whole
	 * of stdio's idiom: "*f->_ptr++" and "*--f->_ptr" with the FILE
	 * homed in IX.  The rewriter leaves the DEREF standing for these
	 * so a load rule cannot turn the step into a fetch, and the shape
	 * it leaves is i(D(V)) - the ?(D(B)) forms name the same thing
	 * for a pointer in BC and were the only ones written.  filbuf,
	 * ungetc and fclose stepped nothing at all.
	 *
	 * At offset zero the member is (ix+0), so this needs no address
	 * arithmetic and no stack: read the pair, step it, put it back.
	 * A postfix wants the value from before, and undoing the step in
	 * HL afterwards is a byte against holding both.
	 */
	R(PREINC,DEREF,0,REGVAR,0,2, PREINC, P_L, P_NONE, P_LL, RF_IX,
		T_IXP_LD F_INCHL T_IXP_ST, R_HL),
	/*
	 * Prefix on a word in a frame slot.  Load, step, store - and the
	 * new value is the answer, so unlike the postfix forms above
	 * there is nothing to undo.
	 *
	 * Written through $t and $T so it lands wherever it was asked
	 * to.  Naming l and h outright, as this did, meant there was no
	 * rule at all when the answer was wanted in DE - which is what a
	 * relational asks for its right operand.  "if (++i >= n)" then
	 * had no reduction: pass2 loaded n into HL, loaded i on top of
	 * it, and jumped on whatever flags inc hl had left.  See
	 * PREINCBUG.
	 */
	R(PREINC,INDEX,0,0,0,2, PREINC, P_L, P_NONE, P_NONE, 0,
	  "\tld $t,($L)\n" F_LDUL "\tinc $T\n"
	  "\tld ($L),$t\n\tld ($L+),$u\n", 0),
	/*
	 * A byte step on a frame slot needs no register at all: inc/dec
	 * (iy+d) is one read-modify-write and it sets Z itself.  Where
	 * the value is thrown away that is the whole job, and where only
	 * the flags are wanted the prefix forms answer in NZ directly -
	 * "while (--n)" on a frame byte is dec (iy+d) / jp z, not the
	 * eleven-byte load, dec a, store, or a dance the value rules
	 * make.  A postfix in flag context answers with the value from
	 * before the step, so the load still happens and or a asks it.
	 */
	R(PREINC,INDEX,0,0,0,25, PREINC, P_L, P_NONE, P_NONE, 0, RT57, 0),
	R(PREINC,INDEX,0,0,0,9, PREINC, P_L, P_NONE, P_NONE, 0, RT57, F_NZ),
	R(PREINC,INDEX,0,0,0,1, PREINC, P_L, P_NONE, P_NONE, 0, F_LDAL1 "\tinc a\n" F_LDLA1, R_A),
	R(PREINC,DEREF,0,REGVAR,0,1, PREINC, P_L, P_NONE, P_LL, RF_IX,
		"\tinc (ix+0)\n\tld a,(ix+0)\n", R_A),

	/*
	 * Stepping a byte at a global.  There is no inc (nn), but there
	 * is inc (hl), so the address goes in HL and the step happens in
	 * memory - four bytes against the seven that loading, adding and
	 * storing would take.
	 *
	 * Which side of the step the load falls on is the whole
	 * difference between prefix and postfix, and a statement wants
	 * neither: it is only the step.
	 */
	R(PREINC,SYMREF,0,0,0,9, PREINC, P_L, P_NONE, P_NONE, 0, RT302, F_NZ),
	R(PREINC,SYMREF,0,0,0,25, PREINC, P_L, P_NONE, P_NONE, 0, RT302, 0),
	R(PREINC,SYMREF,0,0,0,1, PREINC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL "\tinc (hl)\n" F_LDAHL, R_A),
	R(PREINC,INHL,0,0,0,1, PREINC, P_L, P_NONE, P_NONE, 0,
		"\tld a,(hl)\n\tinc a\n" F_LDHLA, R_A),
	/*
	 * The word forms carry between the halves, so they go through A
	 * rather than inc (hl) and a branch.  They leave HL on the high
	 * byte and so say nothing about the value: statements only.
	 */
	R(PREINC,INHL,0,0,0,26, PREINC, P_L, P_NONE, P_NONE, 0, RT108, 0),
	/* a byte in A stepped in place */
	/*
	 * Stepping a byte that lives in B or C, in place.
	 *
	 * These have to come before the A forms below.  A byte register
	 * variable reduces into A to be worked on, and "k(A):b" then
	 * steps it there and calls that the answer - but A is a copy, and
	 * the variable never changed.  "while (--n)" on one of these did
	 * not terminate.
	 *
	 * dec b and dec c set Z themselves, so the flag forms need
	 * nothing after them.  That is also what djnz is: dec b and a
	 * relative jump, in one instruction and one byte less.
	 */
	R(PREINC,REGVAR,0,0,0,9, PREINC, P_L, P_NONE, P_L, RF_B, RT61, F_NZ),
	R(PREINC,REGVAR,0,0,0,9, PREINC, P_L, P_NONE, P_L, RF_C, RT65, F_NZ),
	R(PREINC,REGVAR,0,0,0,1, PREINC, P_L, P_NONE, P_L, RF_B, "\tinc b\n\tld a,b\n", R_A),
	R(PREINC,REGVAR,0,0,0,1, PREINC, P_L, P_NONE, P_L, RF_C, "\tinc c\n\tld a,c\n", R_A),
	R(PREINC,INA,0,0,0,1, PREINC, P_L, P_NONE, P_NONE, 0, "\tinc a\n", R_A),
	/*
	 * inc/dec a literal address - "*(char *)0x1000" - the same place
	 * the ASSIGN(DEREF,P_NUM) rules reach, only stepped.  Load the
	 * address and step through it: a byte is inc/dec (hl), a word is
	 * loaded, stepped and stored back.
	 */
	R(PREINC,P_NUM,0,0,0,1, PREINC, P_L, P_NONE, P_NONE, 0,
		"\tld hl,$La\n\tinc (hl)\n\tld a,(hl)\n", R_A),
	R(PREINC,P_NUM,0,0,0,2, PREINC, P_L, P_NONE, P_NONE, 0,
		"\tld hl,($La)\n\tinc hl\n\tld ($La),hl\n", R_HL),

	/*
	 * Stepping a long in memory.  The helper takes the address in HL,
	 * updates the value in place and hands back what was there before
	 * - which is what a postfix wants and a prefix does not, so a
	 * prefix that is used for its value reads the new one back.  As a
	 * statement, which is nearly always, there is nothing to read.
	 */
	R(POSTINC,SYMREF,0,0,0,3, POSTINC, P_L, P_NONE, P_NONE, 0, RT315, R_HL),
	/*
	 * The same for a frame slot, where the address has to be worked
	 * out: (iy+d) reaches a byte at a time, and the helper wants the
	 * whole address in HL.
	 */
	R(POSTINC,INDEX,0,0,0,3, POSTINC, P_L, P_NONE, P_NONE, 0, RT432, R_HL),
	/*
	 * And through a pointer, where the address is in HL already and
	 * there is nothing to work out - "(*lp)++", which the short forms
	 * had and these did not, so stepping a long through a pointer
	 * emitted nothing at all.
	 *
	 * A prefix used for its value has to read the long back, and the
	 * helper leaves the old one in HL:DE with the address gone, so the
	 * address is kept on the stack under the saved BC.  lld preserves
	 * BC, which is why it can come after the pop.
	 */
	R(POSTINC,INHL,0,0,0,3, POSTINC, P_L, P_NONE, P_NONE, 0, RT364, R_HL),
	/*
	 * And through a pointer kept in a register, where the DEREF is
	 * still standing because rewrite1 left it there - see the step
	 * branch that explains why.  The address is in BC and the helper
	 * wants it in HL, which is the only difference from the forms
	 * above.
	 */
	R(POSTINC,DEREF,0,INBC,0,3, POSTINC, P_L, P_NONE, P_NONE, 0, RT418, R_HL),
	R(POSTINC,INBC,0,0,0,24, POSTINC, P_L, P_NONE, P_NONE, 0, RT63, 0),
	R(POSTINC,REGVAR,0,0,0,25, POSTINC, P_L, P_NONE, P_L, RF_B, RT61, 0),
	R(POSTINC,REGVAR,0,0,0,25, POSTINC, P_L, P_NONE, P_L, RF_C, RT65, 0),
	R(POSTINC,SYMREF,0,0,0,2, POSTINC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL2 F_INCHL F_LDLHL F_DECHL, R_HL),
	R(POSTINC,INHL,0,0,0,2, POSTINC, P_L, P_NONE, P_NONE, 0,
		F_PUSHHL T_LD_IHL F_INCHL T_SWAP_ADDR T_ST_IHL
		F_DECHL, R_HL),
	R(POSTINC,DEREF,0,INBC,0,1, POSTINC, P_L, P_NONE, P_NONE, 0,
		"\tld a,(bc)\n\tld e,a\n\tinc a\n\tld (bc),a\n\tld a,e\n", R_A),
	R(POSTINC,DEREF,0,INHL,0,1, POSTINC, P_L, P_NONE, P_NONE, 0, RT111, R_A),
	R(POSTINC,DEREF,0,INBC,0,2, POSTINC, P_L, P_NONE, P_NONE, 0,
		T_BC_HL F_PUSHHL T_LD_IHL F_INCHL T_SWAP_ADDR T_ST_IHL
		F_DECHL, R_HL),

	/* postfix yields the old value, so read before updating */
	R(POSTINC,INBC,0,0,0,0, POSTINC, P_L, P_NONE, P_NONE, 0, F_LDLC F_LDHB "\tinc bc\n", R_HL),
	R(POSTINC,REGVAR,0,0,0,24, POSTINC, P_L, P_NONE, P_L, RF_IX, RT67, 0),
	R(POSTINC,REGVAR,0,0,0,0, POSTINC, P_L, P_NONE, P_L, RF_IX,
		"\tpush ix\n" F_POPHL "\tinc ix\n", R_HL),
	R(POSTINC,DEREF,0,REGVAR,0,2, POSTINC, P_L, P_NONE, P_LL, RF_IX,
		T_IXP_LD F_INCHL T_IXP_ST F_DECHL, R_HL),
	/*
	 * Postfix on a word in memory.  The old value is wanted as the
	 * result and the new one in store, and rather than hold both, the
	 * update is undone afterwards - one byte, against a push/pop pair
	 * or a shuffle through DE.
	 */
	R(POSTINC,INDEX,0,0,0,2, POSTINC, P_L, P_NONE, P_NONE, 0,
	  "\tld $t,($L)\n" F_LDUL "\tinc $T\n"
	  "\tld ($L),$t\n\tld ($L+),$u\n\tdec $T\n", 0),
	R(POSTINC,INDEX,0,0,0,25, POSTINC, P_L, P_NONE, P_NONE, 0, RT57, 0),
	R(POSTINC,INDEX,0,0,0,9, POSTINC, P_L, P_NONE, P_NONE, 0,
	  F_LDAL1 "\tinc ($L)\n" F_ORA, F_NZ),
	/* a postfix wants the value from before, so the step happens in
	 * memory and the load beats it there */
	R(POSTINC,INDEX,0,0,0,1, POSTINC, P_L, P_NONE, P_NONE, 0, F_LDAL1 "\tinc ($L)\n", R_A),

	/*
	 * The same through a pointer held in IX, where the member is the
	 * first one and so needs no offset to add - "m->parmcount++"
	 * with m in IX.  A member further in becomes +(D(V),N) and folds
	 * to an INDEX, which the two rules above already take; only the
	 * offset-free one arrived as a bare DEREF and matched nothing.
	 *
	 * It emitted no code and no marker, because the store above it
	 * matched anyway - which is how cpp's macdefine came to write a
	 * macro parameter through a stale HL.
	 */
	R(POSTINC,DEREF,0,REGVAR,0,1, POSTINC, P_L, P_NONE, P_LL, RF_IX,
		"\tld a,(ix+0)\n\tinc (ix+0)\n", R_A),
	R(POSTINC,SYMREF,0,0,0,25, POSTINC, P_L, P_NONE, P_NONE, 0, RT302, 0),
	R(POSTINC,SYMREF,0,0,0,1, POSTINC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL F_LDAHL "\tinc (hl)\n", R_A),
	/*
	 * Stepping what an address in HL points at - "p[i]++" once the
	 * subscript has been worked out.  A postfix wants the value from
	 * before, which is what is already in A after the load, so it can
	 * step memory directly and is a byte shorter.
	 */
	R(POSTINC,INHL,0,0,0,1, POSTINC, P_L, P_NONE, P_NONE, 0, RT111, R_A),
	R(POSTINC,INHL,0,0,0,26, POSTINC, P_L, P_NONE, P_NONE, 0, RT108, 0),
	/* postfix wants the old value, so take a copy before stepping */
	R(POSTINC,REGVAR,0,0,0,1, POSTINC, P_L, P_NONE, P_L, RF_B, "\tld a,b\n\tinc b\n", R_A),
	R(POSTINC,REGVAR,0,0,0,1, POSTINC, P_L, P_NONE, P_L, RF_C, "\tld a,c\n\tinc c\n", R_A),
	/* postfix step of a literal address: old value is the read */
	R(POSTINC,P_NUM,0,0,0,1, POSTINC, P_L, P_NONE, P_NONE, 0,
		"\tld hl,$La\n\tld a,(hl)\n\tinc (hl)\n", R_A),
	R(POSTINC,P_NUM,0,0,0,2, POSTINC, P_L, P_NONE, P_NONE, 0,
		"\tld hl,($La)\n\tinc hl\n\tld ($La),hl\n\tdec hl\n", R_HL),
	R(PREDEC,SYMREF,0,0,0,27, PREDEC, P_L, P_NONE, P_NONE, 0, RT313, R_HL),
	R(PREDEC,SYMREF,0,0,0,3, PREDEC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL F_CALLLADEC F_EXX F_LDHLL2 F_EXX F_LDHLL3, R_HL),
	R(PREDEC,INDEX,0,0,0,27, PREDEC, P_L, P_NONE, P_NONE, 0, RT430, R_HL),
	R(PREDEC,INDEX,0,0,0,3, PREDEC, P_L, P_NONE, P_NONE, 0,
		T_IDX_ADDR F_CALLLADEC T_IDX_ADDR "\tcall qld\n", R_HL),
	R(PREDEC,INHL,0,0,0,27, PREDEC, P_L, P_NONE, P_NONE, 0, RT363, R_HL),
	R(PREDEC,INHL,0,0,0,3, PREDEC, P_L, P_NONE, P_NONE, 0,
		F_PUSHHL F_CALLLADEC F_POPHL
		"\tcall qld\n", R_HL),
	R(PREDEC,DEREF,0,INBC,0,27, PREDEC, P_L, P_NONE, P_NONE, 0, RT416, R_HL),
	R(PREDEC,DEREF,0,INBC,0,3, PREDEC, P_L, P_NONE, P_NONE, 0,
		T_BC_HL F_CALLLADEC T_BC_HL
		"\tcall qld\n", R_HL),
	R(PREDEC,INBC,0,0,0,24, PREDEC, P_L, P_NONE, P_NONE, 0, RT50, 0),
	R(PREDEC,REGVAR,0,0,0,25, PREDEC, P_L, P_NONE, P_L, RF_B, RT48, 0),
	R(PREDEC,REGVAR,0,0,0,25, PREDEC, P_L, P_NONE, P_L, RF_C, RT52, 0),
	R(PREDEC,INBC,0,0,0,0, PREDEC, P_L, P_NONE, P_NONE, 0, "\tdec bc\n" F_LDLC F_LDHB, R_HL),
	R(PREDEC,SYMREF,0,0,0,2, PREDEC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL2 F_DECHL F_LDLHL, R_HL),
	R(PREDEC,INHL,0,0,0,2, PREDEC, P_L, P_NONE, P_NONE, 0,
		F_PUSHHL T_LD_IHL F_DECHL T_SWAP_ADDR T_ST_IHL, R_HL),
	R(PREDEC,DEREF,0,INBC,0,1, PREDEC, P_L, P_NONE, P_NONE, 0,
		"\tld a,(bc)\n\tdec a\n\tld (bc),a\n", R_A),
	R(PREDEC,DEREF,0,INHL,0,1, PREDEC, P_L, P_NONE, P_NONE, 0,
		"\tdec (hl)\n\tld a,(hl)\n", R_A),
	R(PREDEC,DEREF,0,INBC,0,2, PREDEC, P_L, P_NONE, P_NONE, 0,
		T_BC_HL F_PUSHHL T_LD_IHL F_DECHL T_SWAP_ADDR T_ST_IHL, R_HL),
	R(PREDEC,REGVAR,0,0,0,24, PREDEC, P_L, P_NONE, P_L, RF_IX, RT54, 0),
	R(PREDEC,REGVAR,0,0,0,0, PREDEC, P_L, P_NONE, P_L, RF_IX,
		"\tdec ix\n\tpush ix\n" F_POPHL, R_HL),
	R(PREDEC,DEREF,0,REGVAR,0,2, PREDEC, P_L, P_NONE, P_LL, RF_IX,
		T_IXP_LD F_DECHL T_IXP_ST, R_HL),
	R(PREDEC,INDEX,0,0,0,2, PREDEC, P_L, P_NONE, P_NONE, 0,
	  "\tld $t,($L)\n" F_LDUL "\tdec $T\n"
	  "\tld ($L),$t\n\tld ($L+),$u\n", 0),
	R(PREDEC,INDEX,0,0,0,25, PREDEC, P_L, P_NONE, P_NONE, 0, RT44, 0),
	R(PREDEC,INDEX,0,0,0,9, PREDEC, P_L, P_NONE, P_NONE, 0, RT44, F_NZ),
	R(PREDEC,INDEX,0,0,0,1, PREDEC, P_L, P_NONE, P_NONE, 0, F_LDAL1 "\tdec a\n" F_LDLA1, R_A),
	R(PREDEC,DEREF,0,REGVAR,0,1, PREDEC, P_L, P_NONE, P_LL, RF_IX,
		"\tdec (ix+0)\n\tld a,(ix+0)\n", R_A),
	R(PREDEC,SYMREF,0,0,0,9, PREDEC, P_L, P_NONE, P_NONE, 0, RT300, F_NZ),
	R(PREDEC,SYMREF,0,0,0,25, PREDEC, P_L, P_NONE, P_NONE, 0, RT300, 0),
	R(PREDEC,SYMREF,0,0,0,1, PREDEC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL "\tdec (hl)\n" F_LDAHL, R_A),
	R(PREDEC,INHL,0,0,0,1, PREDEC, P_L, P_NONE, P_NONE, 0,
		"\tld a,(hl)\n\tdec a\n" F_LDHLA, R_A),
	R(PREDEC,INHL,0,0,0,26, PREDEC, P_L, P_NONE, P_NONE, 0, RT113, 0),
	R(PREDEC,REGVAR,0,0,0,9, PREDEC, P_L, P_NONE, P_L, RF_B, RT48, F_NZ),
	R(PREDEC,REGVAR,0,0,0,9, PREDEC, P_L, P_NONE, P_L, RF_C, RT52, F_NZ),
	R(PREDEC,REGVAR,0,0,0,1, PREDEC, P_L, P_NONE, P_L, RF_B, "\tdec b\n\tld a,b\n", R_A),
	R(PREDEC,REGVAR,0,0,0,1, PREDEC, P_L, P_NONE, P_L, RF_C, "\tdec c\n\tld a,c\n", R_A),
	R(PREDEC,INA,0,0,0,1, PREDEC, P_L, P_NONE, P_NONE, 0, "\tdec a\n", R_A),
	/* prefix decrement of a literal address, as PREINC above */
	R(PREDEC,P_NUM,0,0,0,1, PREDEC, P_L, P_NONE, P_NONE, 0,
		"\tld hl,$La\n\tdec (hl)\n\tld a,(hl)\n", R_A),
	R(PREDEC,P_NUM,0,0,0,2, PREDEC, P_L, P_NONE, P_NONE, 0,
		"\tld hl,($La)\n\tdec hl\n\tld ($La),hl\n", R_HL),
	R(POSTDEC,SYMREF,0,0,0,3, POSTDEC, P_L, P_NONE, P_NONE, 0, RT313, R_HL),
	R(POSTDEC,INDEX,0,0,0,3, POSTDEC, P_L, P_NONE, P_NONE, 0, RT430, R_HL),
	R(POSTDEC,INHL,0,0,0,3, POSTDEC, P_L, P_NONE, P_NONE, 0, RT363, R_HL),
	R(POSTDEC,DEREF,0,INBC,0,3, POSTDEC, P_L, P_NONE, P_NONE, 0, RT416, R_HL),
	R(POSTDEC,INBC,0,0,0,24, POSTDEC, P_L, P_NONE, P_NONE, 0, RT50, 0),
	R(POSTDEC,REGVAR,0,0,0,25, POSTDEC, P_L, P_NONE, P_L, RF_B, RT48, 0),
	R(POSTDEC,REGVAR,0,0,0,25, POSTDEC, P_L, P_NONE, P_L, RF_C, RT52, 0),
	R(POSTDEC,SYMREF,0,0,0,2, POSTDEC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL2 F_DECHL F_LDLHL F_INCHL, R_HL),
	R(POSTDEC,INHL,0,0,0,2, POSTDEC, P_L, P_NONE, P_NONE, 0,
		F_PUSHHL T_LD_IHL F_DECHL T_SWAP_ADDR T_ST_IHL
		F_INCHL, R_HL),
	R(POSTDEC,DEREF,0,INBC,0,1, POSTDEC, P_L, P_NONE, P_NONE, 0,
		"\tld a,(bc)\n\tld e,a\n\tdec a\n\tld (bc),a\n\tld a,e\n", R_A),
	R(POSTDEC,DEREF,0,INHL,0,1, POSTDEC, P_L, P_NONE, P_NONE, 0, RT109, R_A),
	R(POSTDEC,DEREF,0,INBC,0,2, POSTDEC, P_L, P_NONE, P_NONE, 0,
		T_BC_HL F_PUSHHL T_LD_IHL F_DECHL T_SWAP_ADDR T_ST_IHL
		F_INCHL, R_HL),
	R(POSTDEC,INBC,0,0,0,0, POSTDEC, P_L, P_NONE, P_NONE, 0, F_LDLC F_LDHB "\tdec bc\n", R_HL),
	R(POSTDEC,REGVAR,0,0,0,24, POSTDEC, P_L, P_NONE, P_L, RF_IX, RT54, 0),
	R(POSTDEC,REGVAR,0,0,0,0, POSTDEC, P_L, P_NONE, P_L, RF_IX,
		"\tpush ix\n" F_POPHL "\tdec ix\n", R_HL),
	R(POSTDEC,DEREF,0,REGVAR,0,2, POSTDEC, P_L, P_NONE, P_LL, RF_IX,
		T_IXP_LD F_DECHL T_IXP_ST F_INCHL, R_HL),
	R(POSTDEC,INDEX,0,0,0,2, POSTDEC, P_L, P_NONE, P_NONE, 0,
	  "\tld $t,($L)\n" F_LDUL "\tdec $T\n"
	  "\tld ($L),$t\n\tld ($L+),$u\n\tinc $T\n", 0),
	R(POSTDEC,INDEX,0,0,0,25, POSTDEC, P_L, P_NONE, P_NONE, 0, RT44, 0),
	R(POSTDEC,INDEX,0,0,0,9, POSTDEC, P_L, P_NONE, P_NONE, 0,
	  F_LDAL1 "\tdec ($L)\n" F_ORA, F_NZ),
	R(POSTDEC,INDEX,0,0,0,1, POSTDEC, P_L, P_NONE, P_NONE, 0, F_LDAL1 "\tdec ($L)\n", R_A),
	R(POSTDEC,DEREF,0,REGVAR,0,1, POSTDEC, P_L, P_NONE, P_LL, RF_IX,
		"\tld a,(ix+0)\n\tdec (ix+0)\n", R_A),
	R(POSTDEC,SYMREF,0,0,0,25, POSTDEC, P_L, P_NONE, P_NONE, 0, RT300, 0),
	R(POSTDEC,SYMREF,0,0,0,1, POSTDEC, P_L, P_NONE, P_NONE, 0,
		F_LDHLL F_LDAHL "\tdec (hl)\n", R_A),
	R(POSTDEC,INHL,0,0,0,1, POSTDEC, P_L, P_NONE, P_NONE, 0, RT109, R_A),
	R(POSTDEC,INHL,0,0,0,26, POSTDEC, P_L, P_NONE, P_NONE, 0, RT113, 0),
	R(POSTDEC,REGVAR,0,0,0,1, POSTDEC, P_L, P_NONE, P_L, RF_B, "\tld a,b\n\tdec b\n", R_A),
	R(POSTDEC,REGVAR,0,0,0,1, POSTDEC, P_L, P_NONE, P_L, RF_C, "\tld a,c\n\tdec c\n", R_A),
	/* postfix decrement of a literal address */
	R(POSTDEC,P_NUM,0,0,0,1, POSTDEC, P_L, P_NONE, P_NONE, 0,
		"\tld hl,$La\n\tld a,(hl)\n\tdec (hl)\n", R_A),
	R(POSTDEC,P_NUM,0,0,0,2, POSTDEC, P_L, P_NONE, P_NONE, 0,
		"\tld hl,($La)\n\tdec hl\n\tld ($La),hl\n\tinc hl\n", R_HL),


	/* REGVAR -> IN* (value is in register) */
	R(REGVAR,0,0,0,0,0, INBC, P_NONE, P_NONE, P_NONE, RF_BC, 0, 0),
	R(REGVAR,0,0,0,0,0, INDE, P_NONE, P_NONE, P_NONE, RF_DE, 0, 0),
	R(REGVAR,0,0,0,0,0, INHL, P_NONE, P_NONE, P_NONE, RF_HL, 0, 0),

	/* REGVAR IX in flag context: test for zero */
	R(REGVAR,0,0,0,0,8, REGVAR, P_NONE, P_NONE, P_NONE, RF_IX, RT443, F_NZ),

	/* REGVAR byte C/B in flag context */
	R(REGVAR,0,0,0,0,9, REGVAR, P_NONE, P_NONE, P_NONE, RF_C, RT226, F_NZ),
	R(REGVAR,0,0,0,0,9, REGVAR, P_NONE, P_NONE, P_NONE, RF_B, RT219, F_NZ),

	/* REGVAR C/B -> INA (value in C/B, byte context) */
	R(REGVAR,0,0,0,0,1, INA, P_NONE, P_NONE, P_NONE, RF_C, RT221, R_A),
	R(REGVAR,0,0,0,0,1, INA, P_NONE, P_NONE, P_NONE, RF_B, F_LDAB, R_A),
	/* LOCALVAR -> INDEX */
	R(LOCALVAR,0,0,0,0,0, INDEX, P_NONE, P_NONE, P_NONE, 0, 0, 0),

	/* LOCALVAR past the 7-bit (iy+d) window (big-array bases live
	 * below the callee-save slots): form the address with 16-bit
	 * arithmetic (special-cased in tryrule).  Only reached when the
	 * INDEX rule above refuses. */
	R(LOCALVAR,0,0,0,0,0, CODE, P_NONE, P_NONE, P_NONE, 0, 0, 0),

	/*
	 * INHL/INDE/INA in flag context: test for zero.
	 *
	 * The long has to come first.  "H:F" carries no width, so it
	 * matches one too - and testing HL alone tests the high word,
	 * which is zero for every long that fits in an int.  "if (v)"
	 * on a long was false for 1 and true for 65536.
	 */
	R(INHL,0,0,0,0,11, INHL, P_NONE, P_NONE, P_NONE, 0, T_HLDE_TEST, F_NZ),
	R(INHL,0,0,0,0,8, INHL, P_NONE, P_NONE, P_NONE, 0, RT429, F_NZ),
	R(INDE,0,0,0,0,8, INDE, P_NONE, P_NONE, P_NONE, 0, RT427, F_NZ),
	R(INA,0,0,0,0,8, INA, P_NONE, P_NONE, P_NONE, 0, RT358, F_NZ),

	/* INBC in flag context: test for zero */
	R(INBC,0,0,0,0,8, INBC, P_NONE, P_NONE, P_NONE, 0, RT426, F_NZ),

	/* terminator */
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/*
 * Operands that are left alone: they reduce to themselves and the
 * rules read them where they stand.  One letter apiece, so a string
 * rather than a dozen strings and a dozen pointers to them.
 */
unsigned char preserve[] = {
	REGVAR, LOCALVAR, INDEX, P_NUM, SYM, SYMREF,
	INHL, INDE, INA, INE, INBC, CODE, 0
};
