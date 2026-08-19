/*
 * z80 assembler guts
 *
 * substantially rewritten to remove stuff not needed for a compiler backend
 * or an assembler that is used in conjunction with a preprocessor
 * things removed:  the type machinery, and the odd defl, def syntax
 * the most significant thing removed is any notion of arithmetic expressions
 *
 * another messy feature removed is the local label stuff.
 * 
 * /usr/src/cmd/asz/asm.c 
 *
 * Changed: <2025-12-22 10:17:38 curt>
 *
 */
#ifdef linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stdio.h>
#endif

#include "asm.h"
#include <obj.h>

/*
 * segment names and the relocation writer, both in wsobj.c
 */
extern char *wsSegNames[];
extern void wsEncBump();
extern void wsEncReloc();
extern void wsEndReloc();

#ifdef DEBUG
extern char verbose;
/*
 * verbosity levels:
 * 1 = file 
 * 2 = pass progress
 * 3 = instructions
 * 4 = allocations/symbols/relocs
 * 5 - tokens
 */
#endif

extern FILE *infp;
extern FILE *inbuffp;
extern int lineNum;
extern char *infile;
extern char m_flag;

void asm_reset();
void assemble();
unsigned char peekchar();
unsigned char nextchar();
void outbyte();
void outtmp();
unsigned char operand();

/*
 * all token numbers are biased by 0x80, and are unsigned
 * this means that 7 bit ascii characters are literally
 * matched
 */
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

#define T_PLAIN (T_BIAS + 29)   /* an immediate value */
#define T_INDIR (T_BIAS + 30)   /* in indirect immediate */

#define T_SP_I  (T_BIAS + 31)
#define T_BC_I  (T_BIAS + 32)
#define T_DE_I  (T_BIAS + 33)
#define T_IX_I  (T_BIAS + 34)
#define T_IY_I  (T_BIAS + 35)

#define T_C_I   (T_BIAS + 36)
#define T_I     (T_BIAS + 37)
#define T_R     (T_BIAS + 38)

#define T_NAME  (T_BIAS + 39)
#define T_NUM   (T_BIAS + 40)
#define T_STR   (T_BIAS + 41)
#define T_EOF   (T_BIAS + 42)
#define T_LOCAL (T_BIAS + 43)  /* local label ref: Nf or Nb */

#ifdef DEBUG
char *tokname[] = {
    /*  0 */ "B", "C", "D", "E", "H", "L", "(HL)", "A",
    /*  8 */ "BC", "DE", "HL", "SP", "AF", "IX", "IY",
    /* 15 */ "NZ", "Z", "NC", "CR", "PO", "PE", "P", "M",
    /* 23 */ "IXH", "IXL", "(IX+d)", "IYH", "IYL", "(IY+d)",
    /* 29 */ "SYMREF", "INDIR",
    /* 31 */ "(SP)", "(BC)", "(DE)", "(IX)", "(IY)", "(C)", "I", "R",
    /* 39 */ "NAME", "NUM", "STR", "EOF", "LOCAL"
};
#endif

extern char asm_instr();

/*
 * operand table
 */
struct oprnd {
	unsigned char token;
	char *mnem;
};

struct oprnd op_table[] = {
	{ T_B, "b" },
	{ T_C, "c" },
	{ T_D, "d" },
	{ T_E, "e" },
	{ T_H, "h" },
	{ T_L, "l" },
	{ T_A, "a" },
	{ T_BC, "bc" },
	{ T_DE, "de" },
	{ T_HL, "hl" },
	{ T_SP, "sp" },
	{ T_AF, "af" },
	{ T_NZ, "nz" },
	{ T_Z, "z" },
	{ T_NC, "nc" },
	{ T_CR, "cr" },
	{ T_PO, "po" },
	{ T_PE, "pe" },
	{ T_P, "p" },
	{ T_M, "m" },
	{ T_M, "alt" },		/* Hi-Tech: arithmetic less than = negative */
	{ T_CR, "llt" },	/* Hi-Tech: logical less than = carry */
	{ T_P, "age" },		/* Hi-Tech: arithmetic greater or equal = positive */
	{ T_NC, "lge" },	/* Hi-Tech: logical greater or equal = no carry */
	{ T_NZ, "anz" },	/* Hi-Tech: arithmetic not zero */
	{ T_Z, "az" },		/* Hi-Tech: arithmetic zero */
	{ T_P, "fge" },		/* Hi-Tech: float greater or equal */
	{ T_M, "flt" },		/* Hi-Tech: float less than */
	{ T_NZ, "fnz" },	/* Hi-Tech: float not zero */
	{ T_Z, "fz" },		/* Hi-Tech: float zero */
	{ T_NZ, "lnz" },	/* Hi-Tech: logical not zero */
	{ T_Z, "lz" },		/* Hi-Tech: logical zero */
	{ T_PO, "no" },		/* Hi-Tech: no overflow */
	{ T_PE, "o" },		/* Hi-Tech: overflow */
	{ T_IX, "ix" },
	{ T_IY, "iy" },
	{ T_IXH, "ixh" },
	{ T_IXL, "ixl" },
	{ T_IYH, "iyh" },
	{ T_IYL, "iyl" },
	{ T_I, "i" },
	{ T_R, "r" },
	{ 255, "" }
};

#define TOKLEN 19

/* use wsSegNames from wsobj.c */
#define segname wsSegNames

/*
 * symbols come in a couple of flavors that are driven by
 * the assembler semantics:
 *
 * global symbols are exported to the object file, but can
 * have relocations referring to them.
 *
 * extern symbols are also found in the object file, and
 * are very likely to have relocations referring to them
 *
 * static symbols are not exported to the object file, but
 * are also likely to have relocations applied to them. these
 * relocations in the object file are implemented at segment
 * offsets.  they also are likely to start out unresolved
 * until they find definitions
 *
 * symbols are created when encountered, and usually it's a
 * forward reference without any information other than the
 * name.
 *
 * symbols that are intended to be in the object file get 
 * assigned an index in pass 1 of 0, otherwise 0xffff.
 *
 */
/*
 * relocs are chained off of headers and need to stay
 * ordered.
 */
#define RELOC_WORD  0       /* full 16-bit relocation */
#define RELOC_LO    1       /* low byte only */
#define RELOC_HI    2       /* high byte only */

struct reloc {
    unsigned short addr;    /* where the fixup goes */
    struct symbol *sym;     /* what it contains */
    unsigned char hilo;     /* RELOC_WORD/LO/HI */
    struct reloc *next;
};

struct rhead {
    char *segment;
    struct reloc *head;
    struct reloc *tail;
};

/*
 * Relocations go to a scratch file, not into memory.
 *
 * There is one of these for every call, jump and 16-bit immediate
 * that names a symbol, and nm.c makes about three thousand: twenty
 * kilobytes of list on a machine with thirty-two to spend, on top of
 * the symbol table, which is why the assembler ran out of memory on
 * the largest source in the tree and the file simply did not build.
 *
 * They were only in memory to be held until the end.  The encoding
 * needs sym->index, which is not settled until every symbol is known,
 * so a relocation cannot be encoded where it is made - but it does
 * not have to be kept in core to wait, and the encoded stream already
 * goes to a scratch file of its own.  This is the same trick one step
 * earlier.  The symbol pointer is written as-is: the symbols are
 * permalloc'd and outlive the assembly, so the pointer read back is
 * the pointer written.
 */
struct relrec {
    struct rhead *tab;
    unsigned short addr;
    struct symbol *sym;
    unsigned char hilo;
};

extern FILE *relfp;             /* the scratch file, opened in asz.c */
long nrel;                      /* records written to it */

extern unsigned char *lineptr;
extern unsigned char linebuf[];

/*
 * token buffer 
 */
char token_buf[TOKLEN];
char sym_name[TOKLEN];
unsigned long token_val;
unsigned char cur_token;

/*
 * current assembly address 
 */
unsigned short cur_address;
/*
 * Address of the statement being assembled.  cur_address advances as
 * each byte is emitted, so by the time an operand is evaluated it has
 * already stepped past the opcode - $ has to mean where the
 * instruction started, so that jr $+2 lands on the next one.
 */
unsigned short insn_address;

/*
 * Set when the statement's operand mentioned $.  Such a jump must not
 * be relaxed from jp to jr: the offset was written against the size
 * this instruction has now, and shrinking it moves the target.
 */
char used_dollar;

/*
 * $ is an address in the segment being assembled, not a constant, so
 * the linker has to add the segment base to it exactly as it does for
 * a label.  That means the operand needs a relocation, and a
 * relocation record names a symbol.  These stand in for one: a symbol
 * whose segment is the one $ appeared in and whose value is zero, so
 * that the address $ contributes rides in the operand's addend.
 *
 * They are deliberately not on the symbol list.  Nothing ever makes
 * them visible, so they keep index 0xffff, which is what marks a
 * relocation as segment-relative and what keeps them out of the
 * object's symbol table.
 */
/*
 * Two bytes of name, "$", and they are statics rather than allocated,
 * so the tail has to be declared here rather than asked for.
 */
struct { struct symbol s; char pad[1]; } dollarsym[2];

/*
 * the arenas
 *
 * Symbols, relocations and jumps are made and never individually
 * given back, and paying malloc for that is what put asz over the
 * side of a 64k machine: the allocator in libc keeps a three-byte
 * header per block and rounds the payload up to a three-byte granule,
 * so a seven-byte relocation cost twelve and a ten-byte jump cost
 * fifteen.  Here the header is paid once per chunk.
 *
 * There are two of them, and which is which is the whole point.
 *
 * JUMPS AND RELOCATIONS ARE NEVER BOTH ALIVE.  add_jump records only
 * in pass 0 - "if (pass != 0) return" - and add_reloc records only
 * after it - "if (!pass) return" - and relax_jmp, the one thing that
 * reads the jump list, runs once, at the seam between the two.  The
 * jump list is dead from the moment it returns and used to be carried
 * to the end of the assembly anyway, alongside a relocation list that
 * had not existed when it was built.  For c1/lower.s that was 818
 * jumps, 8,180 bytes, held for nothing.
 *
 * So jumps come out of their own arena and it is released whole when
 * relaxation finishes.  The peak is max(symbols+jumps, symbols+relocs)
 * rather than the sum of all three.
 *
 * This is also why the two structures are NOT unified into one record
 * with the fields of both: they do not overlap in time, so a shared
 * record would make every relocation carry a target offset, a
 * condition and a jr flag it has no use for, and would keep the whole
 * lot alive for the entire assembly.  It costs more, not less.
 *
 * PERMCHUNK is a multiple of the 129-byte granule libc's malloc grabs
 * from sbrk, so a chunk is a whole number of them.
 */
#define PERMCHUNK   (8 * 129)       /* 1032 */

extern void gripe();

/*
 * A chunk begins with the link to the one before it, so an arena can
 * be handed back without a record of what was put in it.
 */
struct arena {
    char *chunk;                /* newest chunk, or 0 */
    char *free;                 /* next free byte in it */
    unsigned short left;        /* how many are left there */
};

static struct arena perm;       /* symbols and relocations: to the end */
static struct arena jumparena;  /* jumps: dead after relax_jmp */

/*
 * Which jumps became jr, one bit each, indexed by the jump's ordinal.
 *
 * Both passes read the same source and meet the same instructions in
 * the same order, so the Nth jp of pass 0 is the Nth jp of pass 1 and
 * the ordinal is a name both passes agree on without being told.  It
 * costs nothing to compute - a counter - and nothing to store: 733
 * jumps is 92 bytes, where the table of addresses this replaces was
 * 642 and the search through it ran once per jump.
 *
 * Every jp is counted, including the ones that can never relax - an
 * absolute target, a condition jr does not have.  Their bits are
 * simply never set.  Counting them is what makes the ordinal purely
 * syntactic: if either pass decided what to count by looking at a
 * target or a condition, the two could disagree and every bit after
 * the disagreement would name the wrong jump.
 */
/*
 * The open jumps: the graph relaxation reasons over.
 *
 * A jump is only affected by conversions inside its own span, and jr
 * reaches 128 back and 127 forward from the byte after it, so nothing
 * outside a span of the location pointer can matter to it.  A span of
 * D bytes holds at most D/3 jumps at a byte saved each, so D - D/3 <=
 * 128 puts the ceiling at 192.  Once the location pointer is further
 * than that beyond a jump, no conversion can reach it and its answer
 * is final: the bit is written and the node goes.
 *
 * That is the memory argument.  The graph holds what is open, not
 * what exists - nm.c has 733 jumps and a few dozen open at once - so
 * it is bounded by the reach of the instruction, not by the program.
 */
/*
 * Two different distances, and conflating them costs relaxations.
 *
 * JR_REACH is how far a jump can ever see: a span of D bytes holds at
 * most D/3 jumps at a byte saved each, so D - D/3 <= 128 caps it at
 * 192.  A target further than that can never be brought into range
 * and is dismissed the moment it is known.
 *
 * JR_HOLD is how long a node must be kept, which is longer.  A node
 * depends on the jumps inside its reach, and the last of those does
 * not settle until its own reach has passed - so a node is only final
 * once the location pointer is two reaches beyond it.  Held for one
 * reach instead of two, three jumps in c0/tparse.c stayed jp that the
 * whole-file fixpoint relaxes.
 *
 * MAXOPEN follows from JR_HOLD: a jp is 3 bytes, so that many bytes
 * hold at most 128 of them.
 */
#define JR_REACH    192         /* a target beyond this is unreachable */
#define JR_HOLD     384         /* a node is final two reaches back */
#define MAXOPEN     128         /* 384/3 */

struct open {
    unsigned short idx;         /* the jump's ordinal, its name in jrbits */
    unsigned short addr;        /* where the jp is */
    struct symbol *sym;         /* target */
    unsigned short offset;      /* added to the target */
    unsigned char cond;         /* condition, 0 if unconditional */
};

static struct open opens[MAXOPEN];
static unsigned char nopen;

/*
 * The answers, one bit per jump, written as the nodes retire - so it
 * has to exist before pass 0 starts, when the jump count is not yet
 * known.  A fixed 1K covers 8192 jumps; nm.c, the largest object in
 * the tree, has 733.  Past that a jump simply keeps its bit clear and
 * stays a jp, which is correct and a byte longer.
 */
#define JRBYTES 1024

static unsigned char jrbits[JRBYTES];
static unsigned short jrbytes = JRBYTES;
static unsigned short jidx;     /* ordinal of the jump being handled */

void keep_relaxed();

char *
arena_alloc(a, n)
struct arena *a;
unsigned short n;
{
    char *p;

    /*
     * Even, so the shorts and pointers in what comes back are laid
     * out the way the compiler expects them.
     */
    n = (n + 1) & ~1;

    if (n > a->left) {
        unsigned short want = n + sizeof(char *);

        if (want < PERMCHUNK)
            want = PERMCHUNK;
        p = malloc(want);
        if (!p)
            gripe("out of memory");
        *(char **)p = a->chunk;
        a->chunk = p;
        a->free = p + sizeof(char *);
        a->left = want - sizeof(char *);
    }
    p = a->free;
    a->free += n;
    a->left -= n;
    return p;
}

void
arena_free(a)
struct arena *a;
{
    char *c, *n;

    for (c = a->chunk; c; c = n) {
        n = *(char **)c;
        free(c);
    }
    a->chunk = 0;
    a->free = 0;
    a->left = 0;
}

char *
permalloc(n)
unsigned short n;
{
    return arena_alloc(&perm, n);
}

/*
 * segment tops 
 */
unsigned short text_top;
unsigned short data_top;
unsigned short bss_top;

/*
 * sizes for header
 */
unsigned short text_size;
unsigned short mem_size;
unsigned short data_size;
unsigned short bss_size;
int nlocalsym;		/* local data/bss symbols written for sizing */

char pass;

char segment;

/*
 * -l listing support.  get_line() calls list_line() as each source
 * line is retired, so the record carries what the line ASSEMBLED
 * to: the address it began at, the segment, and the bytes emitted
 * while it was current (first few shown, the rest counted).  Only
 * the final pass writes; sizes shift while pass 0 is still
 * guessing, and every earlier pass would list lies.  The symbol
 * table - statics included, which the object file never shows -
 * follows at the end.
 */
extern char l_flag;
extern FILE *lstfp;
extern unsigned char linebuf[];
extern struct symbol *symbols;
static unsigned short lst_addr;
static unsigned char lst_seg;
static unsigned char lst_bytes[6];
static int lst_have;		/* bytes captured */
static int lst_count;		/* bytes emitted */
static char lst_text[256];
static char lst_live;		/* a line is being collected */

void
list_line()
{
	int i;
	char *p;

	if (!l_flag || pass != 1)
		return;
	if (lst_live && lstfp &&
	    lst_text[0] != 0 && lst_text[0] != '\n') {
		register unsigned char *bp = lst_bytes;
		fprintf(lstfp, "%c %04x  ", "?tdb"[lst_seg], lst_addr);
		for (i = lst_have; i; i--)
			fprintf(lstfp, "%02x ", *bp++);
		if (lst_count > lst_have)
			fprintf(lstfp, "+%d", lst_count - lst_have);
		for (i = 3 * lst_have + (lst_count > lst_have ? 3 : 0);
		    i < 20; i++)
			fputc(' ', lstfp);
		for (p = lst_text; *p && *p != '\n'; p++)
			fputc(*p, lstfp);
		fputc('\n', lstfp);
	}
	/* start collecting the line about to be read */
	lst_addr = cur_address;
	lst_seg = segment;
	lst_have = lst_count = 0;
	lst_live = 1;
	lst_text[0] = 0;
}

/* the token layer hands over the fresh line once it is stripped */
void
list_take()
{
	int i;

	if (!l_flag || pass != 1)
		return;
	{
		register char *d = lst_text;
		unsigned char *s = linebuf;
		for (i = 255; i && *s; i--)
			*d++ = *s++;
		*d = 0;
	}
	/* a label at the line head defines at the CURRENT address */
	lst_addr = cur_address;
	lst_seg = segment;
}

void
list_symbols()
{
	struct symbol *sym;

	if (!l_flag || !lstfp)
		return;
	list_line();		/* the final line's record */
	fprintf(lstfp, "\nsymbols:\n");
	for (sym = symbols; sym; sym = sym->next)
		fprintf(lstfp, "%c %04x  %s\n",
		    "utdbae"[sym->seg], sym->value, sym->name);
}


struct rhead textr = { "text" };
struct rhead datar = { "data" };

/*
 * jump records for jp->jr relaxation
 * only jp instructions with resolvable targets in text segment
 */
struct jump *jumps;

/*
 * local labels - synthetic name architecture
 * hash table mapping label number to pending/last symbols
 */
struct local_state *local_hash[LOCAL_HASHSZ];
int local_seq;

struct symbol *symbols;
struct symbol *symbols_tail;  /* for append order */

/*
 * The symbols, in buckets while the source is read.
 *
 * sym_fetch used to read the whole list to answer every question.
 * On tools/nm.c - 1111 labels, 7966 lookups - that was 16,035
 * instructions per lookup and 54% of the assembler, which is 19% of
 * a build of the tree.
 *
 * So there is no list until the end.  next chains a bucket, and the
 * buckets are hung back together into one list at emission by
 * sym_relist, which is where a list starts being wanted and where
 * the ordinal is handed out.  The ordinal does not have to be the
 * order the source mentioned things in - it goes into relocations
 * and only has to agree with the table it indexes - so nothing is
 * owed to the old order, and this way the index costs one table and
 * not a pointer in every symbol.
 *
 * That matters here.  nm.ps is the tightest file the assembler sees,
 * with 6,262 bytes to spare, and a pointer apiece across its symbols
 * is 3,600 of them - most of the headroom, spent on the one file
 * that has none to spare.  This table is 512 bytes and that is all
 * it is.
 */
#define NSYMHASH 256            /* a power of two: see symhash_of */

static struct symbol *symhash[NSYMHASH];

/*
 * Hash a name, over the same characters sym_fetch compares.
 *
 * It stops at SYMLEN because the comparison does: two names that
 * agree through SYMLEN characters ARE the same symbol here, and they
 * have to land in the same bucket to be found so.  Compiler output
 * shares prefixes heavily - no27_2, _C13, L1024 - so every character
 * has to move the result, which the shift does.
 */
static unsigned char
symhash_of(name)
char *name;
{
    register unsigned short h;
    register char *p;
    register unsigned char n;

    h = 0;
    p = name;
    n = SYMLEN;
    while (n-- && *p)
        h = (h << 1) + *p++;
    return h & (NSYMHASH - 1);
}

extern unsigned char skipwhite();
extern char alpha();
extern char symchar();
extern char escape();
extern struct symbol *sym_update();

/*
 * convert token to register number
 * strips T_BIAS and maps IX/IY registers to H-L range
 */
unsigned char
tok2reg(tok)
unsigned char tok;
{
	tok &= ~T_BIAS;  /* Strip 0x80 bit */

	/* Map IX registers (IXH, IXL, (IX+d)) down to H-L range */
	if (tok >= 23 && tok <= 25) {
		tok -= 19;  /* 23->4, 24->5, 25->6 */
	}
	/* Map IY registers (IYH, IYL, (IY+d)) down to H-L range */
	else if (tok >= 26 && tok <= 28) {
		tok -= 22;  /* 26->4, 27->5, 28->6 */
	}

	return tok;
}

extern char match();
extern unsigned long parsenum();
extern void gripe();
extern void gripe2();

void
save_symn()
{
	register char *d = sym_name;
	char *s = token_buf;
	unsigned char i = SYMLEN + 1;

	while (--i) {
		if (!(*d = *s))
			return;
		d++;
		s++;
	}
	*d = '\0';
}

/*
 * reset local label state at start of each pass
 * deterministic reset ensures same synthetic names in both passes
 */
void
local_reset()
{
    int i;
    struct local_state *ls, *next;

    {
    register struct local_state **bp = local_hash;
    for (i = LOCAL_HASHSZ; i; i--, bp++) {
        for (ls = *bp; ls; ls = next) {
            next = ls->next;
            free(ls);
        }
        *bp = 0;
    }
    }
    local_seq = 0;
}

/*
 * lookup or create local state for label number n
 */
struct local_state *
local_lookup(n, create)
int n;
int create;
{
    int h;
    struct local_state *ls;

    h = n % LOCAL_HASHSZ;
    for (ls = local_hash[h]; ls; ls = ls->next) {
        if (ls->num == n)
            return ls;
    }
    if (!create)
        return 0;
    /*
     * malloc and not the arena: these are the only things here that
     * really are given back - local_reset empties the table between
     * segments - and arena memory cannot be freed one node at a time.
     * Assembling lib/libu/sbrk.s is what caught it, with glibc saying
     * "free(): invalid pointer" about a block that had never been a
     * malloc block.  There are very few of them; the numbered local
     * labels are all that make one.
     */
    ls = (struct local_state *)malloc(sizeof(struct local_state));
    if (!ls)
        gripe("out of memory");
    ls->num = n;
    ls->pending = 0;
    ls->last = 0;
    ls->next = local_hash[h];
    local_hash[h] = ls;
    return ls;
}

/*
 * create a new synthetic symbol for local label n
 * name format: __LN_seq (e.g., __L1_001)
 */
struct symbol *
local_mksym(n)
int n;
{
    char name[16];

    sprintf(name, "__L%d_%04d", n, local_seq++);
    return sym_update(name, SEG_UNDEF, 0, 0);
}

/*
 * define a local label occurrence (N:)
 * if pending forward refs exist, define that symbol
 * always create a new symbol for backward refs
 */
void
local_define(n, addr)
int n;
unsigned short addr;
{
    struct local_state *ls;
    struct symbol *sym;

    ls = local_lookup(n, 1);

    if (ls->pending) {
        /* define the pending forward reference symbol */
        sym = ls->pending;
        sym_update(sym->name, segment, addr, 0);
        ls->pending = 0;
    } else {
        /* no pending forward refs, create symbol for backward refs */
        sym = local_mksym(n);
        sym_update(sym->name, segment, addr, 0);
    }
    ls->last = sym;
}

/*
 * resolve a local label reference (Nf or Nb)
 * returns symbol pointer for the reference
 */
struct symbol *
local_resolve(n, dir)
int n;
char dir;
{
    struct local_state *ls;

    ls = local_lookup(n, 1);

    if (dir == 'f' || dir == 'F') {
        /* forward reference: create or reuse pending symbol */
        if (!ls->pending) {
            ls->pending = local_mksym(n);
        }
        return ls->pending;
    } else {
        /* backward reference: use last defined symbol */
        if (!ls->last) {
            gripe("backward reference to undefined local label");
        }
        return ls->last;
    }
}

extern void get_line();
extern unsigned char peekchar();
extern unsigned char nextchar();
extern void consume();

#ifdef DEBUG
char *tokenname(t)
unsigned char t;
{
    static char tbuf[30];
    if (t < ' ') {
        switch (t) {
        case '\t':
            sprintf(tbuf, "\\t");
            break;
        case '\n':
            sprintf(tbuf, "\\n");
            break;
        default:
            sprintf(tbuf, "\\%o", t); 
            break;
        }
    } else if (t < T_BIAS) {
        sprintf(tbuf, "%c", t);
    } else {
        return tokname[t - T_BIAS];
    }
    return tbuf;
}
#endif

/*
 * the lexer. 
 *
 * really quite sloppy.  the notion of what a token is
 * is quite imprecise.  really, what this is a input
 * scanner that returns special character codes for
 * recognized strings of related characters.
 *
 * [a-zA-Z_0-9]+ -> T_NAME, token_buf filled
 * [digits]+ '\escape' 'c' -> T_NUM, token_val filled
 * "string" -> T_STR, token_buf filled
 *
 * anything else passes as the character
 * finally, return 0 if end of line
 * and -1 for end of file
 * NB: ambiguity: how is ABBAH parsed?  we call it a NAME.
 * to make it a number, prefix it with 0. 0ABBAH.
 */
void
get_token()
{
    int i = 0;
    unsigned char c;

    /* skip over whitespace (comments stripped in get_line) */
    while (1) {
		/* ensure buffer has content */
		if (!*lineptr) {
			get_line();
		}

		c = skipwhite();

        if (c == T_EOF) {
            cur_token = T_EOF;
            return;
        }
        break;
    }

    /* if it looks like a symbol, fill it */
    if (alpha(c)) {
        token_buf[i++] = nextchar();
        while (1) {
            c = peekchar();
            if (symchar(c)) {
                token_buf[i++] = nextchar();
            } else {
                break;
            }
        }
        token_buf[i++] = '\0';

        /*
         * "_cinit.3" is a compiler-generated static: the function it
         * belongs to, and which of that function's statics it is.
         * The '.' is not part of the name, it marks the join - and
         * the join is made HERE because this is the layer that knows
         * how wide the symbol table field is.
         *
         * When the two do not fit, the FUNCTION name gives way and
         * the number survives whole.  The number is what makes the
         * symbol unique; truncating it would put two of a function's
         * statics at one address, silently, which is exactly what the
         * warning below cannot prevent once it has happened.
         */
        {
            register char *h = token_buf;
            int lim = m_flag ? 9 : 15;

            /*
             * The marker is a '.', which the assembler already takes
             * inside a symbol - it is how .edata and .ebss are
             * spelled.  Only a trailing run of digits is a join, and
             * only on a name that does not START with a dot, so the
             * segment sentinels and any hand-written name keep the
             * dots they were given.
             */
            if (*token_buf != '.') {
                register char *t;

                h = 0;
                for (t = token_buf; *t; t++)
                    if (*t == '.')
                        h = t;
                if (h) {
                    for (t = h + 1; *t >= '0' && *t <= '9'; t++)
                        ;
                    if (*t || t == h + 1)
                        h = 0;          /* not a join */
                }
            } else {
                h = 0;
            }
            if (h) {
                register char *num = h + 1;     /* the digits */
                int nlen = strlen(num);
                register char *d;

                if (nlen > lim)
                    nlen = lim;
                if (h - token_buf > lim - nlen)
                    h = token_buf + (lim - nlen);
                for (d = h; *num; )
                    *d++ = *num++;
                *d = '\0';
                i = (d - token_buf) + 1;
            }
        }

        if (i > (m_flag ? 10 : 16)) {  /* >9 or >15 chars plus null terminator */
            printf("%s:%d warning: symbol '%s' longer than %d characters\n",
                   infile, lineNum, token_buf, m_flag ? 9 : 15);
        }
        c = T_NAME;
    }

    /* numbers can have radix info, so look for a delimiter */
    else if ((c >= '0') && (c <= '9')) {
        token_buf[i++] = nextchar();
        while (1) {
            c = peekchar();
            if ((c == ')') || (c == ',') || (c == ' ') ||
                (c == '\t') || (c == '\n') || (c == T_EOF) ||
                (c == '+') || (c == '-') || (c == ':') || (c == '&')) {
                break;
            }
            token_buf[i++] = nextchar();
        }
        token_buf[i++] = '\0';
        /*
         * check for local label reference: digits + f/b
         * e.g., "1f" = forward ref to label 1, "19b" = backward ref to label 19
         */
        {
            int len = i - 1;  /* length without null terminator */
            char lastch = token_buf[len - 1];
            if (len >= 2 && (lastch == 'f' || lastch == 'b' ||
                             lastch == 'F' || lastch == 'B')) {
                /* check if all chars before last are digits */
                int j, alldigits = 1;
                char *tp = token_buf;
                for (j = len - 1; j; j--, tp++) {
                    if (*tp < '0' || *tp > '9') {
                        alldigits = 0;
                        break;
                    }
                }
                if (alldigits) {
                    token_buf[len - 1] = '\0';  /* remove f/b */
                    token_val = parsenum(token_buf);
                    token_buf[0] = lastch;  /* direction f/b */
                    token_buf[1] = '\0';
                    c = T_LOCAL;
                } else {
                    token_val = parsenum(token_buf);
                    c = T_NUM;
                }
            } else {
                token_val = parsenum(token_buf);
                c = T_NUM;
            }
        }
    }

    /*
     * literal character in quotes is a number
     * but standalone ' is returned as itself (for af')
     */
    else if (c == '\'') {
        nextchar();  /* consume the ' */
        i = peekchar();
        if (i == '\n' || i == -1) {
            /* standalone ' at end of line - return as itself */
            /* c is already '\'' */
        } else {
            /* character literal 'X' */
            token_val = nextchar();
            if (token_val == '\\') {
                token_val = escape();
            }
            if (nextchar() != '\'') {
                gripe("unterminated char literal");
            }
            c = T_NUM;
        }
    }

    /*
     * literal string detected - just parse into token_buf
     */
    else if (c == '\"') {
        while (1) {
            c = nextchar();
            if (c == '\n') {
                gripe("unterminated string");
            }
            if (c == '\"') {
                break;
            }
            if (c == '\\') {
                c = escape();
            }
            token_buf[i++] = c;
        }
        token_buf[i++] = '\0';
        c = T_STR;
    }

    /*
     * Single-character token (operators, punctuation, etc.)
     * Need to consume the character that skipwhite() peeked at
     */
    else {
        c = nextchar();
    }

    cur_token = c;

#ifdef DEBUG
    if (verbose > 5) {
        printf("get_token: %d %s", c, tokenname(c));
        if (c == T_NAME) {
            printf(":%s", token_buf);
        } else if (c == T_NUM) {
            printf(":0x%lx", token_val);
        }
        printf("\n");
    }
#endif
	return;
}

/*
 * require a specific token
 */
void
need(c)
unsigned char c;
{
	get_token();

	if (cur_token != c) {
        char s[20];
        sprintf(s, " %d", c);
		gripe2("expected character", s);
	}
}

/*
 * Hang the buckets back together into one list.
 *
 * Called once, where the source has all been read and the ordinals
 * are about to be handed out.  Everything from there on wants a list
 * and none of it cares what order the list is in.
 *
 * The buckets are left pointing into it.  Nothing looks a symbol up
 * after this - the last input is long since read - and a lookup that
 * did would run off the end of its bucket into somebody else's.
 */
static void
sym_relist()
{
	unsigned short h;
	struct symbol *sym;

	symbols = 0;
	symbols_tail = 0;
	for (h = 0; h < NSYMHASH; h++) {
		if (!(sym = symhash[h]))
			continue;
		if (symbols_tail)
			symbols_tail->next = sym;
		else
			symbols = sym;
		while (sym->next)
			sym = sym->next;
		symbols_tail = sym;
	}
}

/*
 * fetches the symbol
 * returns pointer to found symbol, or null
 */
struct symbol *
sym_fetch(name)
char *name;
{
	struct symbol *sym;
	register char *sp;
	char *np;
	unsigned char i;

	for (sym = symhash[symhash_of(name)]; sym; sym = sym->next) {
		sp = sym->name;
		np = name;
		i = SYMLEN + 1;
		while (--i) {
			if (*sp != *np)
				break;
			if (!*sp)
				return sym;
			sp++;
			np++;
		}
		if (!i)
			return sym;	/* equal through all SYMLEN chars */
	}
	return NULL;
}

/*
 * defines or redefines a symbol
 */
struct symbol *
sym_update(name, seg, value, visible)
char *name;
short seg;
unsigned short value;
int visible;
{
	struct symbol *sym;
	int i;

	sym = sym_fetch(name);

	if (!sym) {
		/*
		 * sizeof carries one byte of name - see asm.h - so a name
		 * of i characters wants i more, terminator included.
		 */
		{
			register char *s = name;
			for (i = 0; i < SYMLEN && s[i]; i++)
				;
		}
		sym = (struct symbol *) permalloc(sizeof(struct symbol) + i);
		{
			/* into its bucket, at the head */
			unsigned char h = symhash_of(name);

			sym->next = symhash[h];
			symhash[h] = sym;
		}
		sym->seg = SEG_UNDEF;
		sym->index = 0xffff;
		{
			register char *d = sym->name;
			char *s = name;
			int n = i;

			while (n--)
				*d++ = *s++;
			*d = 0;
		}
	}

	if ((sym->seg != SEG_UNDEF) && (seg == SEG_UNDEF)) {
		/* Symbol already defined, just marking visible - preserve value */
		if (visible) sym->index = 0;
		return sym;
	}

	/*
	 * update the symbol
	 */
    if ((sym->seg != SEG_UNDEF) &&
        (sym->seg != seg)) {
		printf("pass: %d from: %s to: %s\n", pass, segname[sym->seg], segname[seg]);
        gripe2("segment for symbol changed", name);
    }
    /*
     * Two labels of one name.  It used to take the second quietly,
     * which put both at one address - and the compiler now generates
     * names, so the odd collision is not something a person wrote and
     * can see.  A static in f() is _f2; a global that happens to be
     * called f2 is the same symbol, and only this notices.
     *
     * On value, not merely on being defined twice: the second pass walks the
     * same labels again and assigns them the same addresses, which is
     * not a redefinition.  A second label in the same place with a
     * different value is.
     */
    if ((sym->seg != SEG_UNDEF) && (sym->seg == seg) &&
        (sym->value != value) && (pass == 0)) {
        gripe2("symbol defined twice", name);
    }
	sym->seg = seg;
	sym->value = value;
    if (visible) sym->index = 0;
	return sym;
}

void
freerelocs(rh)
struct rhead *rh;
{
    /*
     * Arena memory: the nodes go when the arena does, in asm_reset.
     * Dropping the list is all there is to do here.
     */
    rh->tail = 0;
    rh->head = 0;
    nrel = 0;
    if (relfp)
        fseek(relfp, 0L, SEEK_SET);
}

/*
 * Hand the whole jump arena back.  Not a walk-and-free: these came
 * out of an arena and are not malloc blocks.
 */
void
freejumps()
{
    arena_free(&jumparena);
    jumps = 0;
}

/*
 * resets all allocation stuff
 * this is what we run between assemblies.
 * it should clean out everything.
 */
extern void io_reset();

void
asm_reset()
{
    unsigned short h;

    /*
     * Everything below came out of an arena, so releasing the arena
     * is what frees it; the lists just get dropped.
     */
    symbols = 0;
    symbols_tail = 0;
    for (h = 0; h < NSYMHASH; h++)
        symhash[h] = 0;
    freerelocs(&textr);
    freerelocs(&datar);
    freejumps();
    for (jidx = 0; jidx < JRBYTES; jidx++)
        jrbits[jidx] = 0;
    jidx = 0;
    nopen = 0;
    arena_free(&perm);
    io_reset();
}

/*
 * adds an reference into a relocation table
 * we only do this in the second pass, since that's when
 * all symbols and segment addresses are resolved
 */
void
add_reloc(tab, addr, sym, hilo)
struct rhead *tab;
unsigned short addr;
struct symbol *sym;
unsigned char hilo;
{
	struct reloc *r;

	if (!pass)
		return;

#ifdef DEBUG
    if (verbose > 2)
        printf("add_reloc: %s %x %s %s\n",
            tab->segment, addr, sym ? sym->name : "nosym",
            hilo == RELOC_HI ? "hi" : hilo == RELOC_LO ? "lo" : "word");
#endif

    if (sym->seg == SEG_ABS)
        return;

    if (sym->seg == SEG_UNDEF)
        return;

    {
        struct relrec rr;

        rr.tab = tab;
        rr.addr = addr;
        rr.sym = sym;
        rr.hilo = hilo;
        if (fwrite((char *)&rr, sizeof(rr), 1, relfp) != 1)
            gripe("cannot write relocation scratch");
        nrel++;
        return;
    }

	if (!tab->head) {
		tab->tail = tab->head = r;
	} else {
		tab->tail->next = r;
	}
	tab->tail = r;
}

/*
 * record a jp instruction for potential conversion to jr
 * only in pass 0, only for text segment
 */
void
add_jump(addr, sym, offset, cond)
unsigned short addr;
struct symbol *sym;
unsigned short offset;
unsigned char cond;
{

    /*
     * The counter runs over every jp, text and data alike, because
     * is_relaxed() reads jidx-1 for the jump just counted and would
     * otherwise pick up the previous jump's bit.  Only a text jump is
     * recorded below, so only one can be relaxed; a data jump keeps
     * its zero bit and stays a jp.
     */
    jidx++;
    if (segment != SEG_TEXT)
        return;
    if (pass != 0)
        return;
    /*
     * No record means no relaxation, in both passes alike - which is
     * what a $-relative target needs, since shrinking the instruction
     * would move where $+n points.
     */
    if (used_dollar)
        return;
    /*
     * The ones that can never be a jr get no node: jr has no PO, PE,
     * P or M, and an absolute target's distance is not known until
     * the linker places the segment.  Their bits stay clear.
     */
    if (cond != 0 && (cond < T_NZ || cond > T_CR))
        return;
    if (!sym)
        return;
    /*
     * A target already known and already out of reach resolves here.
     * Shrinking can only bring a jump closer by the conversions
     * inside its own span - at most a third of it - so a distance
     * past a span can never come back, and holding a node for it
     * would occupy a slot for the next 192 bytes to no purpose.
     *
     * A target NOT yet known cannot be dismissed this way, and
     * neither can one merely out of range today: a backward jump can
     * have open forward jumps between it and its target, and their
     * converting is exactly what brings it in.  Those keep their
     * nodes and are retried until the location pointer leaves them.
     */
    if (sym->seg == SEG_TEXT) {
        int d = (int)(sym->value + offset) - (int)(addr + 2);

        if (d < -JR_REACH || d > JR_REACH)
            return;
    }
    /*
     * Out of room is not an error: a jump with no node stays a jp,
     * which is always correct and one byte longer.
     */
    if (nopen >= MAXOPEN)
        return;

    opens[nopen].idx = jidx - 1;
    opens[nopen].addr = addr;
    opens[nopen].sym = sym;
    opens[nopen].offset = offset;
    opens[nopen].cond = cond;
    nopen++;
}

/*
 * WHAT PASS 1 STILL NEEDS TO KNOW, and it is one bit.
 *
 * The emitter asks, at every jp it reaches, whether that jp was
 * relaxed to a jr - "j = find_jump(addr); if (j && j->is_jr)" - so the
 * jump records could not simply be released when relax_jmp finished,
 * which is what the first attempt at this did and what broke every
 * object file it produced.
 *
 * But is_jr is all it asks.  The target, the offset, the condition and
 * the list link are relaxation's own working state and mean nothing
 * after it converges.  So relaxation ends by writing down the
 * addresses that became jr, two bytes each, and gives the records
 * back: for c1/lower.s that is 818 jumps at ten bytes replaced by the
 * relaxed subset at two.
 *
 * The scan is shorter than the one it replaces, which walked every
 * jump; this walks only the ones that changed.
 */
static void
jrset(i)
unsigned short i;
{
    if ((i >> 3) < jrbytes)
        jrbits[i >> 3] |= 1 << (i & 7);
}

/*
 * A node converted: it is a byte shorter, so what follows moves down.
 * Both sets are bounded - the later open nodes, and the labels
 * defined since, which are the text symbols above it.  cur_address
 * moves too: pass 0 has emitted nothing and is free to.
 */
static void
jrshrink(at)
unsigned short at;
{
    struct symbol *s;
    int i;
    unsigned short h;

    for (i = 0; i < nopen; i++)
        if (opens[i].addr > at)
            opens[i].addr--;
    /* the buckets: there is no list yet, and this runs before there is */
    for (h = 0; h < NSYMHASH; h++)
        for (s = symhash[h]; s; s = s->next)
            if (s->seg == SEG_TEXT && s->value > at)
                s->value--;
    /*
     * cur_address and nothing else.  change_seg derives text_top from
     * it at the end of the segment, so decrementing text_top here as
     * well takes the byte off twice: the header then claims a text
     * shorter than the one that was emitted, everything after it in
     * the object is misplaced, and ar reads code where the symbol
     * table should be and indexes no symbols for the member at all.
     *
     * The loop this came from ran after the pass, when text_top was
     * final and adjusting it was the only way to reach it.
     */
    if (cur_address > at)
        cur_address--;
}

/*
 * Drop node i by moving the last one onto it.  Written out field by
 * field because ccc has no struct assignment - "opens[i] = opens[n]"
 * is a struct value, which it says so about and refuses - and this
 * file is compiled by the compiler it belongs to.
 */
static void
opendrop(i)
int i;
{
    nopen--;
    opens[i].idx = opens[nopen].idx;
    opens[i].addr = opens[nopen].addr;
    opens[i].sym = opens[nopen].sym;
    opens[i].offset = opens[nopen].offset;
    opens[i].cond = opens[nopen].cond;
}

static int
jrtry(i)
int i;
{
    struct open *o = &opens[i];
    int target, dist;

    if (o->sym->seg != SEG_TEXT)
        return 0;               /* undefined yet, extern, or not ours */
    target = o->sym->value + o->offset;
    dist = target - (o->addr + 2);
    if (dist < -128 || dist > 127)
        return 0;
    jrset(o->idx);
    jrshrink(o->addr);
    return 1;
}

/*
 * The location pointer has reached here.  Settle what can settle, and
 * retire what it has left out of reach.  Iterated because the nodes
 * are a graph - each conversion shortens the others - but it is a
 * local fixpoint over at most MAXOPEN nodes, never over the file.
 */
void
jrprune(here)
unsigned short here;
{
    int i, changed;

    do {
        changed = 0;
        for (i = 0; i < nopen; i++)
            if (jrtry(i)) {
                opendrop(i);
                changed = 1;
                i--;
            }
    } while (changed);

    for (i = 0; i < nopen; i++)
        if (here > opens[i].addr + JR_HOLD) {
            opendrop(i);
            i--;
        }
}

int
is_relaxed()
{
    unsigned short i = jidx - 1;    /* add_jump has already counted it */

    if ((i >> 3) >= jrbytes)
        return 0;
    return (jrbits[i >> 3] >> (i & 7)) & 1;
}

/*
 * End of pass 0.  Whatever is still open is within a span of the end
 * of the segment, so give the graph a last chance to settle and let
 * the rest retire unrelaxed.
 *
 * There is no iteration over the file here now, and no table to walk.
 * The answers were decided as the location pointer passed them and
 * written to jrbits on the way.  What stood here - a list of every
 * jump in the program, iterated to a fixpoint, each conversion paying
 * a walk of every symbol and every jump - is gone.
 */
void
relax_jmp()
{
    if (no_relax) {
        nopen = 0;
        return;
    }
    jrprune(0xffff);
    nopen = 0;
}

/*
 * the stand-in symbol for $ in the segment now being assembled.
 * bss has no operands to relocate, so it gets none.
 */
struct symbol *
segbase()
{
	struct symbol *sym;

	if (segment == SEG_TEXT)
		sym = &dollarsym[0].s;
	else if (segment == SEG_DATA)
		sym = &dollarsym[1].s;
	else
		return 0;

	sym->seg = segment;
	sym->index = 0xffff;
	sym->value = 0;
	sym->name[0] = '$';
	sym->name[1] = 0;
	return sym;
}

/*
 * outputs a relocation table to whitesmith's object
 *
 * tab = relocation table
 */
extern FILE *tmpfp;

/*
 * Walk the scratch file rather than a list, taking the records that
 * belong to this table.  Read twice, once per segment, which costs a
 * second pass over a file that is already written and is the whole
 * price of not holding it in core.
 */
void
reloc_out(tab, base)
struct rhead *tab;
unsigned short base;
{
	int last = base;
	int bump;
	int seg;
	int size;
	long i;
	struct relrec rr;
	struct relrec *r = &rr;

	fseek(relfp, 0L, SEEK_SET);
	for (i = 0; i < nrel; i++) {
		if (fread((char *)&rr, sizeof(rr), 1, relfp) != 1)
			gripe("cannot read relocation scratch");
		if (rr.tab != tab)
			continue;
		seg = r->sym->seg;
		size = (r->hilo == RELOC_WORD) ? 2 : 1;
#ifdef DEBUG
		if (verbose > 3) {
			printf("reloc: base: %x addr: %x seg: %s(%d) %s %s\n",
				   base, r->addr, segname[seg], seg, r->sym->name,
				   r->hilo == RELOC_HI ? "hi" : r->hilo == RELOC_LO ? "lo" : "");
		}
#endif
		bump = r->addr - last;
#ifdef DEBUG
		if (verbose > 4) {
			printf("bump: %d\n", bump);
		}
#endif
		wsEncBump(tmpfp, bump);

		if (seg == SEG_UNDEF) {
			printf("reloc for undef\n");
		} else if (seg >= SEG_TEXT && seg <= SEG_ABS &&
				   r->sym->index == 0xffff) {
			/* local symbol - segment-relative relocation */
			wsEncReloc(tmpfp, seg, 0, r->hilo);
		} else {
			/* global/extern symbol reference */
			wsEncReloc(tmpfp, -1, r->sym->index, r->hilo);
		}
		last += bump + size;
	}
	wsEndReloc(tmpfp);
}

/*
 * emits a byte into assembly output
 * no bytes emitted on first pass, only update addresses
 *
 * b = byte to emit
 */
void
emitbyte(b)
unsigned char b;
{
	if (pass == 1 && l_flag) {
		if (lst_have < 6)
			lst_bytes[lst_have++] = b;
		lst_count++;
	}
	if (pass == 1) {
		switch (segment) {
		case SEG_TEXT:
			outbyte((char) b);
			break;
		case SEG_DATA:
			outtmp((char) b);
			break;
		case SEG_BSS:
			if (b)
				gripe("data in bss");
			break;
		default:
			break;
		}
	}

	cur_address++;
}

/*
 * emits a little endian word to the binary
 *
 * w = word to emit
 */
void
emitword(w)
unsigned short w;
{
	emitbyte(w & 0xFF);
	emitbyte(w >> 8);
}

/*
 * emits a little endian long (4 bytes) to the binary
 */
void
emitlong(l)
unsigned long l;
{
	emitbyte(l & 0xFF);
	emitbyte((l >> 8) & 0xFF);
	emitbyte((l >> 16) & 0xFF);
	emitbyte((l >> 24) & 0xFF);
}

void
outword(word)
unsigned short word;
{
	outbyte(word & 0xFF);
	outbyte(word >> 8);
}

/*
 * fills a region with either zeros or undefined allocated space
 *
 * size = number of bytes to fill
 */
void
fill(size)
unsigned short size;
{

#ifdef DEBUG
	if (verbose > 3)
		printf("fill segment: %d for %d\n", segment, size);
#endif

	while (size--)
		emitbyte(0);
}

/*
 * emits up to two bytes, and handles relocation tracking
 *
 * size = number of bytes to emit
 * vp = value to push out
 */
void
emit_exp(size, vp)
unsigned short size;
struct expval *vp;
{
	unsigned short rel;
    unsigned char seg;

    if (vp->sym) {
        seg = vp->sym->seg;
    } else {
        seg = SEG_ABS;
    }
	if (seg == SEG_UNDEF) {
		/* if we are on the second pass, error out */
		if (pass == 1)
			gripe2("undefined symbol ", vp->sym->name);
	}

	if (vp->hilo != RELOC_WORD) {
		/*
		 * hi() or lo() byte extraction from symbol
		 */
		unsigned short val;
		if (vp->sym && pass) {
			/* for local symbols, emit segment-relative offset
			 * for global/extern symbols, emit just the addend */
			if (vp->sym->index == 0xffff) {
				/* local symbol - convert absolute to segment-relative */
				val = vp->num.w + vp->sym->value;
				if (vp->sym->seg == SEG_DATA)
					val -= text_size;
				else if (vp->sym->seg == SEG_BSS)
					val -= text_size + data_size;
			} else {
				val = vp->num.w;
			}
			if (vp->hilo == RELOC_HI)
				val >>= 8;
			emitbyte(val & 0xff);
			switch (segment) {
			case SEG_TEXT:
				add_reloc(&textr, cur_address - 1, vp->sym, vp->hilo);
				break;
			case SEG_DATA:
				add_reloc(&datar, cur_address - 1, vp->sym, vp->hilo);
				break;
			default:
				gripe("invalid segment");
			}
		} else {
			/* no relocation - emit full value */
			val = vp->num.w + (vp->sym ? vp->sym->value : 0);
			if (vp->hilo == RELOC_HI)
				val >>= 8;
			emitbyte(val & 0xff);
		}
	} else if (size == 1) {
		/*
		 * here we output only a byte
		 */
		if ((seg >= SEG_EXT) && (pass == 1))
			gripe("cannot extern byte");

		if (seg == SEG_TEXT) {
			/* the addend counts: jr foo+2, and jr $+2, where the
			 * whole address is the addend and the symbol is only
			 * there to say which segment it is in */
			rel = ((vp->sym->value + vp->num.w) - cur_address) - 1;
			if ((rel < 0x80) || (rel > 0xFF7F))
				emitbyte(rel);
			else
				gripe("relative out of bounds");
		} else {
			emitbyte(vp->num.b);
		}

	} else {

		if (vp->sym && pass) {
			unsigned short emit_val;
			switch (segment) {
			case SEG_TEXT:
				add_reloc(&textr, cur_address, vp->sym, RELOC_WORD);
				break;
			case SEG_DATA:
				add_reloc(&datar, cur_address, vp->sym, RELOC_WORD);
				break;
			default:
				gripe("invalid segment");
			}
			/* for local symbols, emit segment-relative offset
			 * for global/extern symbols, emit just the addend */
			if (vp->sym->index == 0xffff) {
				/* local symbol - convert absolute to segment-relative */
				emit_val = vp->num.w + vp->sym->value;
				if (vp->sym->seg == SEG_DATA)
					emit_val -= text_size;
				else if (vp->sym->seg == SEG_BSS)
					emit_val -= text_size + data_size;
				emitword(emit_val);
			} else {
				emitword(vp->num.w);
			}
		} else if (vp->sym) {
			/* absolute symbol - emit full value */
			emitword(vp->num.w + vp->sym->value);
		} else {
			/* no symbol - emit literal value */
			emitword(vp->num.w);
		}
	}
}

/*
 * helper function to emit an immediate and do type checking
 * only absolute resolutions will be allowed, unless hi/lo
 */
void
emit_imm(vp)
struct expval *vp;
{
	if (vp->hilo != RELOC_WORD) {
		/* hi/lo byte extraction - use emit_exp to handle relocation */
		emit_exp(1, vp);
		return;
	}
	if (vp->sym && vp->sym->seg != SEG_ABS && (pass == 1)) {
		printf("sym: %s seg: %s\n", vp->sym->name, segname[vp->sym->seg]);
		gripe("must be absolute");
	}

	emitbyte(vp->num.b);
}

/*
 */
void
db()
{
    unsigned char c;
    struct expval value;

	while (1) {
		c = peekchar();
        if (c == '\n')
            break;
        if (c == T_EOF)
            break;

		c = skipwhite();

		if (c == '"') {
            /* eat the double quote */
            nextchar();

            while (1) {
                c = nextchar();

                if (c == '\n') {
                    gripe("unterminated string constant");
                    break;
                }

                if (c == '\"') {
                    break;
                }

                if (c == '\\') {
                    c = escape();
                }
                emitbyte(c);
            }
		} else {
            c = operand(&value);
            if (c != T_PLAIN) {
                gripe("unexpected value");
            }
			emit_exp(1, &value);
		}
		if (peekchar() != ',')
			break;
		else
			need(',');
	}
}

void
dw()
{
    struct expval value;

	while ((peekchar() != '\n') && (peekchar() != T_EOF)) {
        if (operand(&value) != T_PLAIN) {
            gripe("unexpected value");
        }
		emit_exp(2, &value);
		if (peekchar() != ',')
			break;
		else
			need(',');
	}
}

void
dl()
{
    struct expval value;

	while (peekchar() != '\n' && peekchar() != T_EOF) {
        if (operand(&value) != T_PLAIN) {
            gripe("unexpected value");
        }
		/* No relocation support for longs - just emit the value */
		if (value.sym && pass == 1) {
			gripe("cannot use symbol in .dl");
		}
		emitlong(value.num.l);
		if (peekchar() != ',')
			break;
		else
			need(',');
	}
}

void
ds()
{
    unsigned char c;
    struct expval value;

    c = operand(&value);
    if (c != T_PLAIN && (value.sym != 0)) {
        gripe("ds requires absolute argument");
    }
    /*
     * A reservation is a count of bytes, so it cannot be negative.
     * The word is unsigned, so one that is arrives here as an enormous
     * positive number and is accepted without comment: c1 emitting
     * ".ds -6" for an array with no dimension gave the object a bss of
     * 65530, which every later size is then computed against.  Nothing
     * downstream can tell that from a real 65530.
     */
    if (value.num.w > 0x7fff) {
        gripe("ds count is negative");
        return;
    }
    fill(value.num.w);
}

/*
 * parses an operand, 
 * returns token describing the argument,
 * populate vp if it's passed in.
 * if the operand is an (ix+d), then the expval is the displacement
 */
unsigned char
operand(vp)
struct expval *vp;
{
	int i;
	char c;
	unsigned char ret;
    int indir = 0;
    int hilo_paren = 0;  /* 1 if hi()/lo() style (needs closing paren) */

    vp->num.l = 0;
    vp->sym = 0;
    vp->hilo = RELOC_WORD;

	/*
	 * check if there is anything next (skip whitespace first)
	 */
    c = skipwhite();
	if ((c == '\n') || (c == T_EOF)) {
		return 255;
    }

	/*
	 * check for HiTech C style .low. or .high. prefix
	 * can't use get_token() because symchar() includes '.'
	 */
	if (c == '.') {
		char buf[6];
		int bi = 0;
		nextchar();  /* consume the dot */
		/* read keyword manually (up to 5 chars) */
		while (bi < 5) {
			c = peekchar();
			if (c >= 'a' && c <= 'z') {
				buf[bi++] = nextchar();
			} else if (c >= 'A' && c <= 'Z') {
				buf[bi++] = nextchar() + ('a' - 'A');
			} else {
				break;
			}
		}
		buf[bi] = '\0';
		if (peekchar() != '.') {
			gripe("expected .low. or .high.");
		}
		nextchar();  /* consume trailing dot */
		if (match(buf, "low")) {
			vp->hilo = RELOC_LO;
		} else if (match(buf, "high")) {
			vp->hilo = RELOC_HI;
		} else {
			gripe("expected .low. or .high.");
		}
		get_token();
		goto have_token;
	}

	/*
	 * read the token
	 */
	get_token();
have_token:

	/*
	 * hi() or lo() byte extraction?
	 */
	if (cur_token == T_NAME && (match(token_buf, "hi") || match(token_buf, "lo"))) {
		vp->hilo = (token_buf[0] == 'h') ? RELOC_HI : RELOC_LO;
		hilo_paren = 1;  /* need closing paren */
		need('(');
		get_token();
	}

	/* after skipping whitespace/comments, may be at end of line */
	if (cur_token == '\n')
		return 255;

	/*
	 * maybe a register symbol? sometimes 'c' means carry
	 */
	if (cur_token == T_NAME) {
		/*
		 * The first character again, as in asm_instr.  This walked
		 * all forty-three register names for every named operand in
		 * the file, calling match on each to be told no.
		 */
		for (i = 0; op_table[i].token != 255; i++) {
			if (token_buf[0] != op_table[i].mnem[0])
				continue;
			if (match(token_buf, op_table[i].mnem)) {
				return op_table[i].token;
			}
		}
	}

	/*
	 * maybe in parenthesis?
	 */
	if (cur_token == '(') {
		get_token();
		if (cur_token == T_NUM) {
			/* numeric indirect like (1234h) */
			indir++;
		} else if (cur_token == T_NAME) {
            if (match(token_buf, "hl")) {
                need(')');
                return T_HL_I;
            } else if (match(token_buf, "c")) {
                need(')');
                return T_C_I;
            } else if (match(token_buf, "sp")) {
                need(')');
                return T_SP_I;
            } else if (match(token_buf, "bc")) {
                need(')');
                return T_BC_I;
            } else if (match(token_buf, "de")) {
                need(')');
                return T_DE_I;
            } else if (match(token_buf, "ix") || match(token_buf, "iy")) {
				/*
				 * (ix+d) (ix-d) (iy+d) (iy-d)
				 * handle expressions like (ix+1+-2) = (ix-1)
				 * populate displacement and eat ')'
				 */
				ret = token_buf[1] == 'x' ? T_IX_D : T_IY_D;
				vp->num.w = 0;
				c = skipwhite();
				if ((c == '+') || (c == '-')) {
					/* accumulate all +N and -N terms */
					while ((c == '+') || (c == '-')) {
						char op = c;
						int sign = 1;
						nextchar();  /* consume +/- */
						c = skipwhite();
						/* handle signed number like +-2 */
						if (c == '-') {
							sign = -1;
							nextchar();
						} else if (c == '+') {
							nextchar();
						}
						get_token();
						/* resolve absolute symbols to numbers */
						if (cur_token == T_NAME) {
							struct symbol *sym = sym_fetch(token_buf);
							if (sym && sym->seg == SEG_ABS) {
								token_val = sym->value;
								cur_token = T_NUM;
							}
						}
						if (cur_token != T_NUM) {
							gripe("index displacement must be constant");
						}
						i = sign * token_val;
						if (op == '-') {
							vp->num.w -= i;
						} else {
							vp->num.w += i;
						}
						c = skipwhite();
					}
				} else {
					/* no displacement - convert to indirect */
					ret = (ret == T_IX_D) ? T_IX_I : T_IY_I;
				}
				need(')');
            	return ret;
			} else {
				indir++;
				/* fall through */
			}
		}
	}

    if (cur_token == T_NAME) {
        vp->sym = sym_fetch(token_buf);
        if (!vp->sym) {
            if (pass == 1) {
                gripe2("undefined symbol ", vp->sym->name);
            } else {
                vp->sym = sym_update(token_buf, SEG_UNDEF, 0, 0);
            }
	    }
        /* If symbol is an absolute constant, convert to number */
        if (vp->sym && vp->sym->seg == SEG_ABS) {
            vp->num.w = vp->sym->value;
            vp->sym = 0;
            cur_token = T_NUM;
        }
    } else if (cur_token == T_NUM) {
		vp->num.w = token_val;
    } else if (cur_token == T_LOCAL) {
        /* local label ref: token_val = label number, token_buf[0] = direction */
        vp->sym = local_resolve(token_val, token_buf[0]);
    } else if (cur_token == '$') {
		vp->num.w = insn_address;
		vp->sym = segbase();
		used_dollar = 1;
    } else if (cur_token == '-') {
		get_token();
		if (cur_token == T_NUM) {
			vp->num.w = -token_val;
		} else if (cur_token == '$') {
			vp->num.w = -insn_address;
			used_dollar = 1;
		} else {
			gripe("expected number or $ after -");
		}
    } else {
        gripe("need an operand");
    }

	c = skipwhite();

	/* accumulate +N, -N, and &N terms (handle +-N syntax) */
	while ((c == '+') || (c == '-') || (c == '&')) {
		char op = c;
		int sign = 1;
		nextchar();
		c = skipwhite();
		/* handle signed number like +-2 */
		if (c == '-') {
			sign = -1;
			nextchar();
		} else if (c == '+') {
			nextchar();
		}
		get_token();
		if (cur_token == T_NUM) {
            i = sign * token_val;
		} else if (cur_token == T_NAME) {
			/* handle absolute symbol in expression */
			struct symbol *s = sym_fetch(token_buf);
			if (!s || s->seg != SEG_ABS) {
				gripe("expression operand must be absolute");
			}
			i = sign * s->value;
		} else {
			gripe("expected number after operator");
		}
		if (op == '-') {
			vp->num.w -= i;
		} else if (op == '&') {
			vp->num.w &= i;
		} else {
			vp->num.w += i;
		}
		c = skipwhite();
	}

    if (indir) {
	    need(')');
        return T_INDIR;
	}
	if (hilo_paren)
		need(')');
	return T_PLAIN;
}

/*
 * changes segments for first pass segment top tracking
 * save our place
 * next = next segment
 */
void
change_seg(next)
char next;
{
	switch (segment) {
	case SEG_TEXT:
		text_top = cur_address;
		break;
	case SEG_DATA:
		data_top = cur_address;
		break;
	case SEG_BSS:
		bss_top = cur_address;
		break;
	default:
		break;
	}

	switch (next) {
	case SEG_TEXT:
		cur_address = text_top;
		break;
	case SEG_DATA:
		cur_address = data_top;
		break;
	case SEG_BSS:
		cur_address = bss_top;
		break;
	default:
		break;
	}
	segment = next;
}

/*
 * perform assembly functions
 * two passes over the source code:
 * pass 0: locate symbols in relative segments, calculate sizes
 * pass 1: emit code and data with final addresses
 * then output symbol table and relocations
 */
void
assemble()
{
    unsigned short type;
	struct symbol *sym;
	unsigned short symb;		/* which bucket, when walking them */
    unsigned short next;
	struct expval eqval;

	asm_reset();

	pass = 0;

	segment = SEG_TEXT;
	text_top = data_top = bss_top = 0;
	cur_address = 0;

	/*
	 * run passes
	 */
	while (1) {

		change_seg(SEG_TEXT);
		cur_address = 0;
		text_top = 0;
		/* reset local labels at start of each pass for deterministic names */
		local_reset();

#ifdef DEBUG
		if (verbose) {
			printf("start of pass %d\n", pass);
			printf
				("text_top: %d data_top: %d bss_top: %d mem_size: %d\n",
				 text_top, data_top, bss_top, mem_size);
		}
#endif

		while (1) {
            /* where this statement starts, for $ */
            insn_address = cur_address;
            used_dollar = 0;
            /*
             * The location pointer has moved: settle and retire.
             * This is what keeps the graph bounded - without it the
             * nodes would accumulate for the whole segment.
             */
            if (!pass)
                jrprune(cur_address);
            get_token();

            if (cur_token == T_EOF) {
                break;
            }

#ifdef DEBUG
			if (verbose > 4)
				printf("line %d: %s", lineNum, linebuf);
#endif

			/*
			 * command read 
			 */
			if (cur_token == '.') {
				get_token();

				if (cur_token != T_NAME)
					gripe2("expected directive", token_buf);

				next = 0;
				if (match(token_buf, "text")) {
					next = 1;
				} else if (match(token_buf, "data")) {
					next = 2;
				} else if (match(token_buf, "bss")) {
					next = 3;
				}

				/*
				 * change segment 
				 */
				if (next != 0) {
					change_seg(next);
					consume();
					continue;
				}

				if (match(token_buf, "globl") ||
					match(token_buf, "global")) {
					while (1) {
						get_token();
						if (cur_token != T_NAME)
							gripe("expected symbol");
						if (pass == 0) {
							sym = sym_update(token_buf, SEG_UNDEF, 0, 1);
						}
						/* see if there is another */
						if (peekchar() == ',')
							need(',');
						else
							break;
					}
					consume();
					continue;
				}

				if (match(token_buf, "extern")) {
					while (1) {
						get_token();
						if (cur_token != T_NAME)
							gripe("expected symbol");
						if (pass == 0) {
							sym = sym_update(token_buf, SEG_EXT, 0, 1);
						}
						/* see if there is another */
						if (peekchar() == ',')
							need(',');
						else
							break;
					}
					consume();
					continue;
				}

				/*
				 * .ds <byte count> 
				 */
				if (match(token_buf, "ds")) {
					ds();
					consume();
					continue;
				}

				/*
				 * .defb <byte>|<string>[,...] 
				 */
				if (match(token_buf, "defb") ||
					match(token_buf, "db")) {
					db();
					consume();
					continue;
				}

				/*
				 * .defw <word>[,...]
				 */
				if (match(token_buf, "defw") ||
					match(token_buf, "dw")) {
					dw();
					consume();
					continue;
				}

				/*
				 * .defl <long>[,...]
				 */
				if (match(token_buf, "defl") ||
					match(token_buf, "dl")) {
					dl();
					consume();
					continue;
				}


				/*
				 * .error <text>
				 *
				 * How pass2 reports a fault it can only find
				 * once it is emitting - a switch too wide for
				 * the case table, a frame too large to restore
				 * bc from, a dozen of them.  It has no channel
				 * back to the driver except the file it is
				 * writing, so it writes the complaint here and
				 * this refuses to assemble.
				 *
				 * Until now there was no such directive, so
				 * every one of those came out as "unkown
				 * directive" with the real message visible
				 * only because the offending line is echoed
				 * after it.  The message is the point; print
				 * it as one.
				 */
				if (match(token_buf, "error")) {
					unsigned char *p = linebuf;

					while (*p && *p != '.')
						p++;
					if (*p == '.')
						p++;
					while (*p && *p != ' ' && *p != '\t')
						p++;
					while (*p == ' ' || *p == '\t')
						p++;
					printf("%s:%d %s", infile, lineNum, p);
					exit(1);
				}

				printf("%s\n", token_buf);
				gripe("unknown directive");
				continue;
			}

			/*
			 * symbol read
			 */
			else if (cur_token == T_NAME) {
				/*
				 * try to get the type of the symbol
				 */
				if (asm_instr(token_buf)) {
					/*
					 * it's an instruction
					 */
					consume();
				}
				/*
				 * HiTech C style pseudo-ops (no leading dot)
				 */
				else if (match(token_buf, "psect")) {
					get_token();
					if (cur_token != T_NAME)
						gripe("expected segment name");
					next = 0;
					if (match(token_buf, "text"))
						next = 1;
					else if (match(token_buf, "data"))
						next = 2;
					else if (match(token_buf, "bss"))
						next = 3;
					if (next)
						change_seg(next);
					consume();
				} else if (match(token_buf, "defb")) {
					db();
					consume();
				} else if (match(token_buf, "defw")) {
					dw();
					consume();
				} else if (match(token_buf, "defs")) {
					ds();
					consume();
				} else if (match(token_buf, "global")) {
					while (1) {
						get_token();
						if (cur_token != T_NAME)
							gripe("expected symbol");
						if (pass == 0) {
							sym = sym_update(token_buf, SEG_UNDEF, 0, 1);
						}
						if (peekchar() == ',')
							need(',');
						else
							break;
					}
					consume();
				} else if (peekchar() == '=') {
					/*
					 * it's a symbol definition
					 */
					save_symn();
					get_token();

					type = operand(&eqval);
					/* plain constant is absolute */
					if (type == T_PLAIN && !eqval.sym)
						type = SEG_ABS;

					sym_update(sym_name, type, eqval.num.w, 0);
					consume();
				} else if (peekchar() == ':') {
					/*
					 * set the new symbol (if it is the first pass)
					 * label:: (double colon) exports the symbol
					 */
					int visible = 0;
					nextchar();  /* consume first : */
					if (peekchar() == ':') {
						nextchar();  /* consume second : */
						visible = 1;
					}
					if (pass == 0) {
						sym_update(token_buf, segment, cur_address, visible);
					}
				} else {
					/*
					 * might be "symbol equ value" - peek ahead
					 */
					save_symn();
					get_token();
					if (cur_token == T_NAME && match(token_buf, "equ")) {
						type = operand(&eqval);
						/* plain constant is absolute */
						if (type == T_PLAIN && !eqval.sym)
							type = SEG_ABS;
						sym_update(sym_name, type, eqval.num.w, 0);
						consume();
					} else {
						gripe("unexpected symbol");
					}
				}
			}
			/*
			 * numeric label (local label) - e.g., "1:" or "19:"
			 */
			else if (cur_token == T_NUM) {
				if (peekchar() == ':') {
					nextchar();  /* consume : */
					local_define(token_val, cur_address);
				} else {
					gripe("expected : after local label");
				}
			} else if (cur_token != '\n') {
				gripe("unexpected token");
			}
		}

		change_seg(SEG_TEXT);
        
#ifdef DEBUG
		if (verbose) {
			printf("end of pass %d\n", pass);
			printf
				("text_top: %d data_top: %d bss_top: %d mem_size: %d\n\n",
				 text_top, data_top, bss_top, mem_size);
		}
#endif

		pass++;

		/*
		 * pass 1, so we know our text + data segment sizes
		 */
		if (pass == 1) {

			change_seg(SEG_TEXT);

			/* relax jp->jr before finalizing sizes */
			relax_jmp();

			/*
			 * The ordinals start again for the second pass, which
			 * hands them out at the same instructions and so reads
			 * back the bits relax_jmp has just written.
			 */
			jidx = 0;

			mem_size = text_top + data_top + bss_top;
			text_size = text_top;
			data_size = data_top;
			bss_size = bss_top;

            next = 0;

            /*
             * We've seen everything, so we can assign indexes.
             *
             * Over the buckets, not a list: pass 2 is still to come
             * and it looks symbols up, so the buckets have to stay
             * buckets until it is done with them.  The order the
             * ordinals come out in is this walk's order, which is
             * nobody's business but the relocations', and they are
             * written from these same values.
             */
	        for (symb = 0; symb < NSYMHASH; symb++)
	        for (sym = symhash[symb]; sym; sym = sym->next) {

                if (sym->seg == SEG_UNDEF) {
                    /* Treat undefined symbols as extern */
                    sym->seg = SEG_EXT;
                    sym->index = next++;
                } else if (sym->index == 0) {
                    sym->index = next++;
                }
		        if (sym->seg == SEG_DATA) {
                    sym->value += text_size;
                }
                if (sym->seg == SEG_BSS) {
                    /* sym->value += text_size + data_size; */
                    sym->value += text_size;
                    sym->value += data_size;
                }
            }

			outbyte(MAGIC);		/* magic */
			outbyte(m_flag ? CONF_9 : CONF_15);		/* config byte */
			/*
			 * The symbol table carries three kinds of entry:
			 * the indexed symbols counted in "next", the local
			 * data/bss symbols, and the two segment sentinels.
			 * Only the first are named by relocations, and the
			 * other two are appended after them so that every
			 * relocation index stays what it was.
			 */
			nlocalsym = 0;
			for (symb = 0; symb < NSYMHASH; symb++)
			for (sym = symhash[symb]; sym; sym = sym->next)
				if (sym->index == 0xffff &&
				    (sym->seg == SEG_DATA || sym->seg == SEG_BSS))
					nlocalsym++;
			outword((next + nlocalsym + 2) *
				((m_flag ? 9 : 15) + 3));
			outword(text_size);	/* text */
			outword(data_size);	/* data */
			outword(bss_size);	/* bss */
			outword(0);			/* stack+heap */
			outword(0);			/* textoff */
			outword(text_size);	/* dataoff */

#ifdef DEBUG
            if (verbose)
                printf("magic %x text:%d data:%d bss:%d heap:%d "
                       "symbols:%d textoff:%x dataoff:%x\n",
                       m_flag ? 0x9914 : 0x9917, text_size, data_size, bss_size, 0,
                       next * ((m_flag ? 9 : 15) + 3), 0, text_size);
#endif

			/*
			 * reset segment addresses to their final addresses
			 */
			text_top = 0;
			data_top = text_size;
			bss_top = data_top + data_size;
			cur_address = 0;

            lineNum = 0;

            io_reset();
            if (infp == stdin) {
                infp = inbuffp;
            }
			fseek(infp, 0L, SEEK_SET);

			continue;
		}

		if (pass == 2)
			break;
	}

	/*
	 * The passes are done and nothing looks a symbol up again, so
	 * the buckets can become the list the rest of this wants.
	 */
	sym_relist();

	/*
	 * output symbols and relocation tables
	 */
	for (sym = symbols; sym; sym = sym->next) {
		switch (sym->seg) {
		case SEG_UNDEF:
			type = 0x08;
			break;
		case SEG_TEXT:
			type = 0x05 | 0x08;
			break;
		case SEG_DATA:
			type = 0x06 | 0x08;
			break;
		case SEG_BSS:
			type = 0x07 | 0x08;
			break;
		case SEG_ABS:
			type = 0x04 | 0x08;
			break;
		case SEG_EXT:
			type = 0x08;
			break;
		default:
			break;
		}

#ifdef DEBUG
		if (verbose > 3) {
			printf("sym: %9s index: %5d seg: %s(%d) type: %x\n",
				sym->name, sym->index, segname[sym->seg], sym->seg, type);
		}
#endif

		if (sym->index == 0xffff)
			continue;
		outtmp(sym->value & 0xff);
		outtmp(sym->value >> 8);
		outtmp(type);
		/*
		 * The field in the object file is fixed width and the name
		 * is not any more, so the zeroes that used to come free out
		 * of a padded array get written here.
		 */
		{
			register char *sp = sym->name;

			for (next = 0; next < (m_flag ? 9 : 15); next++)
				outtmp(*sp ? *sp++ : 0);
		}
	}

	/*
	 * The local data and bss symbols, for sizing and nothing else.
	 *
	 * A static is allocated in bss like any other object but has no
	 * entry in the symbol table, so the bytes it occupies belong to
	 * no name.  That is invisible until the linker has to work out
	 * how big the symbol BEFORE it is: an unreferenced static
	 * silently becomes part of its neighbour, and if that neighbour
	 * is an uninitialised global shared by forty objects, the size
	 * the linker merges on is too big.  A referenced one is pinned
	 * by its own segment-relative relocation; an unreferenced one
	 * has nothing to pin it, and that is the case this covers.
	 *
	 * They are written after the indexed symbols and keep index
	 * 0xffff, so no relocation names them and their references stay
	 * segment-relative exactly as before.  Nothing about the link
	 * changes; the linker reads them to bound a size and otherwise
	 * ignores them.  LOCAL, so they never collide.
	 */
	for (sym = symbols; sym; sym = sym->next) {
		if (sym->index != 0xffff)
			continue;
		if (sym->seg != SEG_DATA && sym->seg != SEG_BSS)
			continue;
		outtmp(sym->value & 0xff);
		outtmp(sym->value >> 8);
		outtmp(sym->seg == SEG_DATA ? 0x06 : 0x07);
		{
			register char *sp = sym->name;

			for (next = 0; next < (m_flag ? 9 : 15); next++)
				outtmp(*sp ? *sp++ : 0);
		}
	}

	/*
	 * SENTINEL: where data and bss actually end, in this object.
	 *
	 * Nothing in the symbol table says how big a symbol is - the
	 * entry is a value, a type and a name - so the size of the LAST
	 * symbol in a segment cannot be worked out from the symbols
	 * alone.  Every other one is the distance to the next; the last
	 * one runs to the end of the segment, and the end of the segment
	 * is what was missing.
	 *
	 * The linker needs those sizes to merge the same uninitialised
	 * global defined by a header in forty objects, which is how C of
	 * this vintage is written - and to tell that case apart from two
	 * objects that disagree about what the thing is, which is a bug
	 * and has to be said out loud rather than silently resolved.
	 *
	 * Emitted here rather than written in the source: no .s has to
	 * know, and one that does not cooperate cannot get it wrong.
	 *
	 * LOCAL - no 0x08.  A global sentinel in every object would
	 * collide in every link, which is the exact problem it exists to
	 * let the linker solve.  The names start with a dot so that no C
	 * identifier can ever collide with them either.
	 */
	{
		static char *sentinel[2] = { ".edata", ".ebss" };
		unsigned short sval[2];
		unsigned char sseg[2];
		int i, j;

		sval[0] = text_size + data_size;
		sseg[0] = 0x06;			/* data, local */
		sval[1] = text_size + data_size + bss_size;
		sseg[1] = 0x07;			/* bss, local */

		for (i = 0; i < 2; i++) {
			char *p = sentinel[i];

			outtmp(sval[i] & 0xff);
			outtmp(sval[i] >> 8);
			outtmp(sseg[i]);
			/* name, NUL padded to the field width - stop AT the
			 * NUL rather than indexing past it */
			for (j = 0; j < (m_flag ? 9 : 15); j++)
				outtmp(*p ? *p++ : 0);
		}
	}

	reloc_out(&textr, 0);
	reloc_out(&datar, text_top);

	list_symbols();
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
