/*
 * vim: set tabstop=4 shiftwidth=4 expandtab:
 */
#include <fcntl.h>
#include "decomp.h"

struct block *blocks;

char *vopts[] = {"V_DATA", "V_EXPR", 0 };
int verbose;

char instbuf[100];

unsigned char mem[64 * 1024];

unsigned char pchars[16];
int pcol;
char *progname;

unsigned char
getmem(unsigned short addr)
{
    return mem[addr];
}

void
usage(char *complaint, char *p)
{
    int i;

    printf("%s", complaint);
    printf("usage: %s [<options>] [program [<program options>]]\n", p);
    printf("\t-v <verbosity>\n");
    printf("\t-c call[=bytes]\n");
    printf("\t-l label\n");
    for (i = 0; vopts[i]; i++) {
        printf("\t%x %s\n", 1 << i, vopts[i]);
    }
    exit(1);
}

struct special {
    unsigned short addr;
    unsigned char extra;
    struct special *next;
} *specials;

bool
is_special(unsigned short dest)
{
    struct special *sp;

    for (sp = specials; sp; sp = sp->next) {
        if (sp->addr == dest) {
            return sp->extra;
        }
    }
    return 0;
}

struct codelabel *
getlabel(unsigned short addr)
{
    struct codelabel *c;

    for (c = codelabels; c; c = c->next) {
        if (c->addr == addr) {
            return c;
        }
        /* we found a symbol bigger */
        if (c->addr > addr) {
            return 0;
        }
    }
    return 0;
}

/*
 * add the address to the code label list - keep this sorted
 */
void
addlabel(unsigned short addr, int vis)
{
    struct codelabel *c;
    struct codelabel *p;

    p = 0;

    for (c = codelabels; c; c = c->next) {
        /* already know this one */
        if (c->addr == addr) {
            return;
        }
        /* we found a symbol bigger */
        if (c->addr > addr) {
            break;
        }
        p = c;
    }
    c = malloc(sizeof(*c));
    c->addr = addr;
    c->visited = vis;
    c->refs = 0;
    if (p) {
        c->next = p->next;
        p->next = c;
    } else {
        c->next = codelabels;
        codelabels = c;
    }
}

void
xref(unsigned short addr, unsigned short refinst)
{
    struct codelabel *c;
    struct addrlist *r;

    c = getlabel(addr);
    if (!c) {
        printf("xref lose %x\n", addr);
        return;
    }
    r = malloc(sizeof(*r));
    r->addr = refinst;
    r->next = c->refs;
    c->refs = r;
}

extern char *regname[];

char *opname[] = {
    "noop", "assign", "reg", "memory",
    "cond", "jump", "ret", "call",
    "push", "pop", "extend", "rshift",
    "lshift", "add", "sub", "mul",
    "div", "mod", "and", "or",
    "xor", "not", "neg", "constant"
};

char indent[] = "                                   ";

void
dump_expr(struct expr *e, int level)
{
    char *in = &indent[strlen(indent) - (level * 4)];

    if (!(verbose & V_EXPR))
        return;

    printf("%sexpr %s ", in, opname[e->op]); 
    switch (e->op) {
    case reg:
        printf("value: %s ", regname[e->value]);
        break;
    case constant:
        printf("value: 0x%x ", e->value);
        break;
    default:
        break;
    } 
    if (e->flags & E_UNSIGNED) printf("UNSIGNED ");
    if (e->flags & E_BYTE) printf("BYTE ");
    if (e->flags & E_WORD) printf("WORD ");
    if (e->flags & E_BIT) printf("BIT ");
    if (e->flags & E_TERMINAL) printf("TERMINAL ");
    if (e->flags & E_FLAGS) printf("FLAGS ");
    printf("\n");
    if (e->flags & E_LEFT) {
        dump_expr(e->left, level + 1);
    }
    if (e->flags & E_RIGHT) {
        dump_expr(e->right, level + 1);
    }
    
}

struct expr *
expr(op_t op, unsigned char f, unsigned short v, struct expr *left, struct expr *right)
{
    struct expr *e;

    e = malloc(sizeof(struct expr));

    e->op = op;
    e->flags = f;
    e->value = v;

    if (left) { 
        e->flags |= E_LEFT;
        e->left = left;
    }
    if (right) {
        e->flags |= E_RIGHT;
        e->right = right;
    }
    if (!left && !right) {
        e->flags | E_TERMINAL;
    }
    return (e);
}

/*
 * we run down entire flow, building the instruction list
 */
void
add_block(struct codelabel *c)
{
    struct block *b, *bl, *pb;
    struct inst *i, *p;
    int end = 0;
    unsigned short addr;
    int j;

    if (c->visited)
        return;

    addr = c->addr;

    b = malloc(sizeof(*b));
    b->addr = addr;
    b->label = c;
    c->visited = 1; 
    p = 0;

    /* insert into blocks list */
    pb = 0;
    for (bl = blocks; bl; bl = bl->next) {
        /* if we find a higher block, insert before */
        if (b->addr < bl->addr) {
            break;
        }
        pb = bl;
    } 
    /* if no previous block */
    if (pb) {
        b->next = pb->next;
        pb->next = b;
    } else {
        b->next = blocks;
        blocks = b;
    } 

    /* disassemble the block */
    while (!end) {
        i = malloc(sizeof(*i));
        i->addr = addr;
        i->dis = &instbuf;
        i->next = 0;
        i->len = 1;
        for (j = 0; j < MAXOPS; j++) {
            i->mop[j] = 0;
        }
        end = do_instr(i);

        i->dis = strdup(i->dis);
        if (p) {
            i->prev = p;
            p->next = i;
        } else {
            b->chain = i;
            i->prev = 0;
        }
        b->len += i->len;
        addr += i->len;
        p = i;

        /* if we ran into another block, terminate this one */
        if (getlabel(addr)) {
            break;
        }
    }

}

void
dump_block(struct block *b)
{
    struct inst *i;
    struct expr **ep;

    printf("%04x:\n", b->addr);
    for (i = b->chain; i; i = i->next) {
        printf("\n\t%s\n", i->dis);
        for (ep = i->mop; *ep; ep++) {
            dump_expr(*ep, 0);
        }
    }
}

/*
 * make sure that we don't have a label in the middle of this block
 * if we do, break the block
 */
void
break_block(struct block *b)
{
    struct inst *i, *p;
    struct block *nb;
    struct addrlist *r;

    p = 0;

    for (i = b->chain; i; i = i->next) {
        if (p && (r = getlabel(i->addr))) {
            break;
        }
        p = i;
    }
    if (!i)
        return;

    nb = malloc(sizeof(*nb));
    nb->label = r;
    nb->addr = i->addr;
    nb->next = b->next;
    nb->len = b->len - (i->addr - b->addr);
    nb->chain = i;

    if (!p) {
        printf("can't happen %x\n", b->addr);
        return;
    }
    p->next = 0;
    b->len = i->addr - b->addr;
    b->next = nb;
    printf("broke block %x %d ", b->addr, b->len);
    printf("at     : %x %d\n", nb->addr, nb->len);
}

main(int argc, char **argv)
{
    int fd;
    int ret;
    int pc;
    int bytes;
    int i;
    struct special *sp;
    struct codelabel *c;
    struct block *b;
    char *s;
    int pass;

    progname = *argv++;
    argc--;

    while (argc) {
        s = *argv;

        /* end of flagged options */
        if (*s++ != '-')
            break;

        argv++;
        argc--;

        /* s is the flagged arg string */
        while (*s) {
            switch (*s++) {
            case 'h':
                usage("", progname);
                break;
            case 'v':
                if (!argc--) {
                    usage("verbosity not specified \n", progname);
                }
                verbose = strtol(*argv++, 0, 0);
                break;
            case 'c':
                if (!argc--) {
                    usage("special call not specified \n",
                        progname);
                }
                s = *argv++;
                sp = malloc(sizeof(*sp));
                sp->addr = strtol(s, &s, 0);
                if (*s == '=') {
                    s++;
                    sp->extra = strtol(s, &s, 0);
                } else {
                    sp->extra = 2;
                }
                sp->next = specials;
                specials = sp;
                break;
            case 'l':
                if (!argc--) {
                    usage("label not specified\n", progname);
                }
                s = *argv++;
                addlabel(strtol(s, &s, 0), 0);
                break;
            default:
                printf("bad flag %c\n", (*s));
                break;
            }
        }
    }

    if (argc) {
        s = *argv;
    } else {
        s = "testvec";
    }

    fd = open(s, O_RDONLY);
    if (fd < 0) {
        perror(s);
        exit(1);
    }
    ret = read(fd, &mem[0x100], sizeof(mem) - 0x100);
    if (ret < 0) {
        perror("read binary");
        exit(1);
    }
    printf("read %d from %s\n", ret, s);

    addlabel(0x0, 1);
    addlabel(0x5, 1);
    addlabel(0x100, 0);

    for (sp = specials; sp; sp = sp->next) {
        printf("special %x %d\n", sp->addr, sp->extra);
    }
    for (c = codelabels; c; c = c->next) {
        printf("label %x\n", c->addr);
    }

    /* walk all the code paths, building the label list */
    do {
        i = 0;
        for (c = codelabels; c; c = c->next) {
            if (!c->visited) {
                add_block(c);
                i++;
            }
        }
    } while (i);

    /* make sure no instructions inside a block have codelabels */
    for (b = blocks; b; b = b->next) {
        break_block(b);
    }
 
    /* dump the blocks */
    for (b = blocks; b; b = b->next) {
        dump_block(b);
    }
}

char cbuf[20];
char cbufi;

char *
ncbuf()
{
    cbufi ^= 1;
    if (cbufi) {
        return &cbuf[10];
    } else {
        return &cbuf[0];
    }
}

char *
const16(unsigned short a)
{
    char *s = ncbuf();

    sprintf(s, "0x%04x", a);
    return s;
}

char *
const8(unsigned char a)
{
    char *s = ncbuf();

    sprintf(s, "0x%02x", a);
    return s;
}

char *
addr(unsigned short a)
{
    return const16(a);
}

char ibuf[20];

char *
signedoff(unsigned char i)
{
    char off;

    off = i;
    if (off < 0) {
        sprintf(ibuf, "-%d", -off);
    } else {
        sprintf(ibuf, "+%d", off);
    }
    return ibuf;
}

unsigned short
reloff(unsigned short pc, unsigned char a)
{
    char off;

    off = a;

    pc = pc + 2 + off;
    return (pc);
}

char *
reladdr(unsigned short pc, unsigned char a)
{
    sprintf(ibuf, "%x", reloff(pc, a));

    return ibuf;
}
