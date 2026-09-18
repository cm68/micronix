/*
 * mmu_test.c - unit test for the RI10 (grow) memory-fault path added to
 * mpz80.c.  Standalone: it mirrors getpte(), super() and the three lines
 * the fix added to get_byte()/put_byte() verbatim, then exercises them.
 *
 * build:  cc -o mmu_test mmu_test.c
 */

#include <stdio.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int paddr;

/* ST_ bits, copied from mpz80.c */
#define ST_VOID  0x01
#define ST_IORQ  0x02
#define ST_HALT  0x04
#define ST_INT   0x08
#define ST_STOP  0x10
#define ST_AUX   0x20
#define ST_R10   0x40
#define ST_READ  0x80
#define ST_RESET (ST_VOID|ST_IORQ|ST_HALT|ST_INT|ST_STOP|ST_AUX|ST_READ)

/* permission codes, from include/sys/proc.h */
#define NONE 0
#define FULL 3
#define GROW 7

/* the machine state the accessor reads */
static byte maps[0x200];        /* the page table, 0x600-0x7ff */
static byte taskreg;            /* task register */
static byte trapreg;            /* trap address register (0x400) */
static int  mem_pending_fault;  /* set when a RI10 reference traps */
static byte phys[0x100000 >> 12][0x1000];  /* a few pages of physical ram */

static int failures;

/* mpz80.c:288 */
static int super(void) { return (taskreg & 0xf) == 0; }

/* mpz80.c:566 - do a virtual lookup, return physical address and attr */
static void getpte(word addr, paddr *paddrp, byte *attrp)
{
    byte taskid = taskreg & 0xf;
    byte page = (addr & 0xf000) >> 12;
    byte pte = (taskid << 5) + (page << 1);

    *paddrp = ((taskreg & 0xf0) << 16) | (maps[pte] << 12) + (addr & 0xfff);
    *attrp = maps[pte + 1];
}

static void physwrite(paddr pa, byte v) { phys[(pa >> 12) & 0xff][pa & 0xfff] = v; }

/*
 * The write path, exactly as put_byte's mapped branch now reads: translate,
 * shift the trap-address register on the bus cycle, trap-on-RI10, then do
 * the write (the write still lands - the "on write" trap is deferred).
 */
static void put_byte(word addr, byte value)
{
    paddr pa;
    byte attr;

    getpte(addr, &pa, &attr);
    trapreg = ((addr >> 12) << 4) | (trapreg >> 4);
    if (!super() && (attr & 0x4))
        mem_pending_fault = 1;
    physwrite(pa, value);
}

/* map a logical page of task t to physical page seg with permission per */
static void setmap(byte task, byte page, byte seg, byte per)
{
    byte pte = (task << 5) + (page << 1);
    maps[pte] = seg;
    maps[pte + 1] = per;
}

static void check(const char *what, int ok)
{
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        failures++;
}

int main(void)
{
    /* the trap bits the fix hands to trap() must route to fault(): ST_R10
     * set (MTRAP), ST_INT clear (deferred, resume next instruction). */
    check("trap bits = 0xf7 (MTRAP, deferred)",
        ((ST_RESET & ~ST_INT) | ST_R10) == 0xf7);

    /* a grow segment (physical 0x70), mapped to task 1 pages 8 and 9 */
    setmap(1, 8, 0x70, GROW);
    setmap(1, 9, 0x70, GROW);
    /* a nailed-down data page below the break: task 1 page 7, FULL */
    setmap(1, 7, 0x6f, FULL);

    taskreg = 0x01;             /* user (task 1) */

    /* shift register tracks the last two pages accessed */
    trapreg = 0;
    put_byte(0x6fff, 0xaa);     /* page 6 -> lower=0, upper=6 */
    put_byte(0x7fff, 0xbb);     /* page 7 -> lower=6, upper=7 */
    check("shift register holds (7<<4)|6 after two accesses",
        trapreg == ((7 << 4) | 6));

    /* writing a FULL page does not fault, and the byte lands */
    mem_pending_fault = 0;
    put_byte(0x7053, 0x5c);
    check("FULL page write: no fault", mem_pending_fault == 0);
    check("FULL page write: byte stored", phys[0x6f][0x053] == 0x5c);

    /* writing a GROW page completes AND sets the pending fault */
    mem_pending_fault = 0;
    put_byte(0x8053, 0x6f);
    check("GROW page write: fault raised", mem_pending_fault == 1);
    check("GROW page write: byte stored (write completes)",
        phys[0x70][0x053] == 0x6f);
    /* the trap address register now names the faulting page (upper) and
     * the page accessed just before it (lower = 7, the FULL data page) */
    check("trap register = (8<<4)|7 after GROW write", trapreg == 0x87);

    /* the same GROW page, but from the supervisor, does not trap */
    mem_pending_fault = 0;
    taskreg = 0x00;             /* supervisor */
    put_byte(0x8053, 0x6f);
    check("GROW page write from task 0: no fault", mem_pending_fault == 0);

    printf("\n%s\n", failures ? "FAILED" : "all checks passed");
    return failures != 0;
}
