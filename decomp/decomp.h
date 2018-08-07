/*
 * vim: set tabstop=4 shiftwidth=4 expandtab:
 */
typedef unsigned char bool;

/*
 * used to track lists of addresses
 */
struct addrlist {
    unsigned short addr;
    struct addrlist *next; 
};

typedef enum op { 
    noop,
    assign,             /* left = lvalue, right = rvalue */
    reg,                /* value = regnum */
    memory,             /* left = address */
    cond,               /* left = condition, right = action */
    jump, ret, call,    /* left = address */
    push,               /* left = data */ 
    pop,                /* left = data */
    extend,             /* left = data */
    rshift, lshift,     /* left = data, right = bit count */
    add, sub,           /* binary operators */
    mul, div, mod,
    and, or, xor,
    not, neg,            /* left = data */
    constant
} op_t;

/*
 * every time we encounter a jump or call instruction, we need to
 * add them to places to walk down from.
 */
struct codelabel {
    unsigned short addr;
    int visited;
    struct codelabel *next;         /* all the labels */
    struct addrlist *refs;
    unsigned short refcount;        /* how many jumps go here */
} *codelabels;

typedef enum regno { 
    breg, creg, dreg, ereg, hreg, lreg, areg, 
    bcreg, dereg, hlreg, ixreg, iyreg, spreg,
    zflag, cflag, vflag, mflag
} regno_t;

/*
 * expression struct
 * basic instructions are 2 operand, accumulator kinds of things
 * but, we express these as 3 operand trees 
 */
struct expr {
    unsigned short value;
    unsigned char flags;
#define E_UNSIGNED 0x01
#define E_BYTE     0x02
#define E_WORD     0x04
#define E_LEFT     0x08
#define E_RIGHT    0x10
#define E_TERMINAL 0x20
#define E_BIT      0x40
#define E_FLAGS    0x80
    op_t op;
    struct expr *left;
    struct expr *right;
};

/*
 * every hunk of contiguous code is a block
 * blocks start big, and then as we detect jumps into them,
 * we split them.  eventually, every jump target is a block.
 */
struct block {
    unsigned short addr;
    unsigned short len;
    struct inst *chain;
    struct block *next;
    struct codelabel *label;
};

/*
 * an instruction contains a set of machine operations
 * this could be a linked list, but as it doesn't grow
 * it's gonna be an array
 */
#define MAXOPS  6

/*
 * the instruction parser returns these
 */
struct inst {
	unsigned short addr;	/* our address */
	struct inst *prev;
	struct inst *next;

	unsigned char len;      /* bytes consumed */
    unsigned long opcode;
	char *dis;              /* the disassembly */

	int flags;
#define	I_CF	0x01		/* affects carry */
#define	I_ZF	0x02		/* affects zero */
#define	I_FL	(I_CF|I_ZF)
    struct expr *mop[MAXOPS];
};

extern int do_inst(struct inst *);
extern unsigned char getmem(unsigned short a);
extern char *const16(unsigned short a);
extern char *const8(unsigned char a);
extern char *addr(unsigned short a);
extern char *signedoff(unsigned char a);
extern char *reladdr(unsigned short pc, unsigned char a);
extern bool is_special(unsigned short addr);
extern struct expr *expr(op_t o, unsigned char f, unsigned short v, struct expr *l, struct expr *r);
extern struct codelabel *getlabel(unsigned short addr);
extern void addlabel(unsigned short addr, int vis);
unsigned short reloff(unsigned short pc, unsigned char a);

#define V_DATA  0x01
#define V_EXPR  0x02
extern int verbose;
