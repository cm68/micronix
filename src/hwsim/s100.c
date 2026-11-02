/*
 * s100.c
 *
 * a 24 bit address bus, and i/o registration framework
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "sim.h"
#include "hwsim.h"
#include "util.h"

/*
 * bus lines of interest - reflects the current actual state of the lines
 */
unsigned char vi_lines;
int int_line;
int nmi_line;

void 
default_vi_change(unsigned char new)
{
    vi_lines = new;
}

unsigned char
default_intack()
{
    return 0xff;
}

/*
 * set by the interrupt controller
 */
void (*vi_change)(unsigned char bits) = &default_vi_change;
unsigned char (*get_intack)() = &default_intack;

void
default_int_change(int new)
{
    int_line = new;
}

/*
 * set by the cpu
 */
void (*int_change)(int value) = &default_int_change;

/*
 * return a byte of interrupt ack
 */
unsigned char
int_ack()
{
    return (*get_intack)();
}

static unsigned char vi_masks[8];

/*
 * assert or deassert a vectored interrupt line.  since these are open collector,
 * we need to know which card is setting or clearing the line.  the cards have
 * a unique number from 0 to 7.  this is a hack.
 */
void
set_vi(int line, int card, int value)
{
    unsigned char new = vi_lines;

    // a card that is not wired asserts no line
    if (line < 0 || line >= 8)
        return;

    // do the open collector thing
    vi_masks[line] &= 0xff ^ (1 << card);
    vi_masks[line] |= value << card;
    value = (vi_masks[line] == 0) ? 0 : 1;

    new &= (0xff ^ (1 << line));
    new |= value << line;

    /*
     * Always tell the handler, change or not.  The lines are
     * level-triggered, and a card may hold one high across an
     * acknowledge: the 8259's IRR has to be re-latched from the level
     * when that happens, which a change-only callback cannot see.
     */
    (*vi_change)(new);
}

/*
 * input port function pointers - we only decode the low 8 bits of the address
 */
outhandler output_handler[256];
inhandler input_handler[256];

#ifndef NODEBUG
int trace_ior;                  // i/o registration trace
#endif

/*
 * the S100 bus memory space
 */
byte physmem[16*1024*1024];
byte boot_rom[BOOTROM_SIZE];    /* the 4k level 1 boot rom at 0xff0000 */

/*
 * How much of the 24 bit space is backed by ram.  Reading above this
 * returns the open-bus value 0xff and writing is lost, so a machine
 * with a smaller memory board presents exactly the absent pages that
 * meminit() probes for.  Default is the full 16 meg.
 */
paddr ram_size = 16*1024*1024;

/*
 * access memory in the 24 bit S100 space using a physical address
 * physread() and physwrite() are static inline in hwsim.h.
 */

/*
 * bulk move to physical memory
 */
void
copyout(byte *buf, paddr pa, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        physwrite(pa+i, buf[i]);
    }
}

/*
 * bulk move from physical memory
 */
void
copyin(byte *buf, paddr pa, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        buf[i] = physread(pa+i);
    }
}

/*
 * word access
 */
void
put_word(word addr, word value)
{
    put_byte(addr, value);
    put_byte(addr + 1, value >> 8);
}

word
get_word(word addr)
{
    return get_byte(addr) + (get_byte(addr + 1) << 8);
}

byte
undef_in(portaddr p)
{
    p &= 0xff;
#ifndef NODEBUG
    printf("input from undefined port %x\n", p);
#endif
    return 0xff;
}

void
undef_out(portaddr p, byte v)
{
    p &= 0xff;
#ifndef NODEBUG
    printf("output to  undefined port %x -> %x\n", p, v);
#endif
}

/*
 * Who owns the ports.
 *
 * input_handler/output_handler are the dispatch table, and they are also
 * the owner table: a port that is nobody's answers undef_in/undef_out.
 * What they cannot say is *which* card put a handler there, and that is
 * the whole of the check below - a machine where two cards answer the
 * same port does not exist, and the way that has gone wrong here before
 * was silent, with the second card's registration simply winning and the
 * first card quietly ceasing to work.
 *
 * The claimant comes from the startup loop in hwsim.c, which knows each
 * driver's name as it calls its startup hook.  A card that registers the
 * same port twice - the MultIO's banked registers - is the same owner and
 * is ordinary; only a second card is a fault.
 */
extern char *progname;

static char *in_owner[256];
static char *out_owner[256];
static char *claimant;

void
io_claimant(char *name)
{
    claimant = name;
}

static void
io_conflict(char *dir, int port, char *first, char *second)
{
    fprintf(stderr, "%s: port %x %s: %s and %s\n", progname, port, dir,
        first ? first : "(unknown)", second ? second : "(unknown)");
    exit(1);
}

byte
s100_input(portaddr p)
{
    p &= 0xff;
    return (*input_handler[p])(p);
}

void
s100_output(portaddr p, byte v)
{
    p &= 0xff;
    (*output_handler[p])(p, v);
}

void
register_input(portaddr portnum, inhandler func)
{
    portnum &= 0xff;

    if (func != undef_in) {
        if (input_handler[portnum] != undef_in && in_owner[portnum] != claimant)
            io_conflict("input", portnum, in_owner[portnum], claimant);
        trace(trace_ior, "input port %x registered\n", portnum);
        in_owner[portnum] = claimant;
    }
    input_handler[portnum] = func;
}

void
register_output(portaddr portnum, outhandler func)
{
    portnum &= 0xff;

    if (func != undef_out) {
        if (output_handler[portnum] != undef_out && out_owner[portnum] != claimant)
            io_conflict("output", portnum, out_owner[portnum], claimant);
        trace(trace_ior, "output port %x registered\n", portnum);
        out_owner[portnum] = claimant;
    }
    output_handler[portnum] = func;
}

__attribute__((constructor))
void
io_init()
{
    int i;

#ifndef NODEBUG
    trace_ior = register_trace("ioregister");
#endif
    for (i = 0; i < 256; i++) {
        input_handler[i] = undef_in;
        output_handler[i] = undef_out;
    }
}

/*
 * The interrupt wiring: two namespaces wired together by name.
 *
 * The S-100 has eight vectored-interrupt lines (VI0..VI7), driven open
 * collector by the cards.  Each 8259 has eight interrupt-request pins,
 * and a cascaded pair is one global 16-pin space - master IR0..IR7, then
 * slave IR8..IR15 - numbered the way the PC numbers a master/slave.
 *
 * A machine is a wiring table: which card asserts each VI line, and what
 * each IR pin is wired to (a VI line, or an on-board source).  Cards only
 * ever name their source; the line numbers live here and are looked up
 * with wiring_vi_line().  The -w option names the machine by its boards
 * (`ss1`, `wb`, `mio`, `wb,mio`, `mio,mio`) and then assigns the cards
 * with viN=card; a bare -w is the default Wunderbus+MultIO machine.
 */

#define NVI     8
#define NIRQ   16

enum { IRQ_NONE = 0, IRQ_VI = 1, IRQ_ONBOARD = 2 };

struct irqwire {
    int  kind;         /* IRQ_* */
    int  vi;           /* the VI line, when IRQ_VI */
    char *name;        /* the on-board source, when IRQ_ONBOARD */
};

struct cardwire {
    char *name;        /* the card's interrupt source name */
    int   line;        /* the VI line it asserts */
};

#define NCARDWIRE 12            /* room for the cards one machine can hold */

struct wiring {
    struct cardwire card[NCARDWIRE];    /* name -> vi line */
    int   ncard;
    struct irqwire irq[NIRQ];   /* pin -> source */
    char *int_driver;           /* the board asserting int_line */
    int   slave;                /* the 0x40 slave 8259 is present */
    int   cascade;              /* the master has cascade hardware */
    int   wb_master;            /* the master is a Wunderbus I/O (DIP at 0x49) */
};

struct wiring wiring;

/*
 * The wiring vocabulary is the boards, not a profile name.  A machine is
 * named by the boards it carries between the interrupt controllers and
 * the bus, followed by the cards' vector assignments:
 *
 *      ss1
 *      wb
 *      mio
 *      wb,mio
 *      mio,mio
 *
 * A single board is the master.  "wb,mio" is a Wunderbus I/O master with
 * one MultIO attached - the Wunderbus has no cascade hardware, so that
 * slave is polled.  "mio,mio" is two MultIOs cascaded, master at the
 * Wunderbus address with cascade hardware.  A lone "mio" is a MultIO
 * used alone.  Cards are placed only by viN=card, so the spec states the
 * whole wiring and nothing is implied by a profile.
 */

static int build_machine(struct wiring *w, char **board, int nboard);
static int set_card_line(struct wiring *w, char *name, int line);

static void
set_irq(struct wiring *w, int pin, int kind, int vi, char *name)
{
    w->irq[pin].kind = kind;
    w->irq[pin].vi = vi;
    w->irq[pin].name = name;
}

/* The MultIO-family pin map: with a slave, VI2 is its cascade and
 * VI3..VI5 feed its IR0..IR2. */
static void
multio_pins(struct wiring *w, int slave)
{
    set_irq(w, 0, IRQ_VI, 0, 0);
    set_irq(w, 1, IRQ_VI, 1, 0);
    if (slave)
        set_irq(w, 2, IRQ_ONBOARD, 0, "slave");
    else
        set_irq(w, 2, IRQ_VI, 2, 0);
    set_irq(w, 3, IRQ_ONBOARD, 0, "uart0");
    set_irq(w, 4, IRQ_ONBOARD, 0, "uart1");
    set_irq(w, 5, IRQ_ONBOARD, 0, "uart2");
    set_irq(w, 7, IRQ_ONBOARD, 0, "clock");
    if (slave) {
        set_irq(w, 8, IRQ_VI, 3, 0);
        set_irq(w, 9, IRQ_VI, 4, 0);
        set_irq(w, 10, IRQ_VI, 5, 0);
    }
}

/* The CompuPro SS1's two CAS 8259s. */
static void
ss1_pins(struct wiring *w)
{
    int i;

    for (i = 0; i < 7; i++)
        set_irq(w, i, IRQ_VI, i, 0);
    set_irq(w, 7, IRQ_ONBOARD, 0, "slave");
    set_irq(w, 8, IRQ_VI, 7, 0);
    set_irq(w, 9, IRQ_ONBOARD, 0, "timer0");
    set_irq(w, 10, IRQ_ONBOARD, 0, "timer1");
    set_irq(w, 11, IRQ_ONBOARD, 0, "timer2");
    set_irq(w, 12, IRQ_ONBOARD, 0, "fpu0");
    set_irq(w, 13, IRQ_ONBOARD, 0, "fpu1");
    set_irq(w, 14, IRQ_ONBOARD, 0, "uart_tx");
    set_irq(w, 15, IRQ_ONBOARD, 0, "uart_rx");
}

/*
 * Build the machine from its board names.  The caller has already split
 * the leading board tokens out of the spec.  Returns 0, or -1 for a
 * board set that is not a machine (two masters, an SS1 with a slave, ...).
 */
static int
build_machine(struct wiring *w, char **board, int nboard)
{
    int i;

    w->ncard = 0;
    for (i = 0; i < NIRQ; i++)
        set_irq(w, i, IRQ_NONE, 0, 0);
    w->int_driver = 0;
    w->slave = 0;
    w->cascade = 0;
    w->wb_master = 0;

    if (nboard == 1 && strcmp(board[0], "ss1") == 0) {
        w->int_driver = "ss1";
        ss1_pins(w);
        return 0;
    }
    if (nboard == 1 && (strcmp(board[0], "wb") == 0 ||
                        strcmp(board[0], "mio") == 0)) {
        w->int_driver = "multio";
        w->wb_master = strcmp(board[0], "wb") == 0;
        multio_pins(w, 0);
        return 0;
    }
    if (nboard == 2 && strcmp(board[1], "mio") == 0 &&
        (strcmp(board[0], "wb") == 0 || strcmp(board[0], "mio") == 0)) {
        w->int_driver = "multio";
        w->wb_master = strcmp(board[0], "wb") == 0;
        w->slave = 1;
        w->cascade = !w->wb_master;
        multio_pins(w, 1);
        return 0;
    }
    return -1;
}

/*
 * The default machine, for a run with no -w: a Wunderbus + MultIO with
 * the five standard controllers.  -w replaces it entirely.
 */
__attribute__((constructor))
static void
wiring_init(void)
{
    static char *board[2] = { "wb", "mio" };

    build_machine(&wiring, board, 2);
    set_card_line(&wiring, "djdma", 0);
    set_card_line(&wiring, "hddma", 1);
    set_card_line(&wiring, "ncr", 3);
    set_card_line(&wiring, "nic", 4);
    set_card_line(&wiring, "ide", 5);
}

/*
 * Resolve a card's source name to the VI line it asserts, -1 if it is
 * not wired.  The cards call this instead of carrying a line number.
 */
int
wiring_vi_line(char *name)
{
    int i;

    for (i = 0; i < wiring.ncard; i++)
        if (strcmp(wiring.card[i].name, name) == 0)
            return wiring.card[i].line;
    return -1;
}

/*
 * Resolve an IR pin to what is on it.  Returns 0 if the pin is wired to
 * nothing, fills *vi with the VI line when it is a VI line, and *name
 * with the on-board source when it is one.
 */
int
wiring_irq(int pin, int *vi, char **name)
{
    if (pin < 0 || pin >= NIRQ)
        return 0;
    if (wiring.irq[pin].kind == IRQ_VI) {
        if (vi)
            *vi = wiring.irq[pin].vi;
        return 1;
    }
    if (wiring.irq[pin].kind == IRQ_ONBOARD) {
        if (name)
            *name = wiring.irq[pin].name;
        return 2;
    }
    return 0;
}

/*
 * Is this board the one driving int_line?  A machine has exactly one
 * interrupt controller per wiring; the others are compiled in but dormant.
 * Each controller claims vi_change/get_intack and folds its own line only
 * when this is true for its own name.
 */
int
wiring_active(char *name)
{
    return wiring.int_driver && strcmp(wiring.int_driver, name) == 0;
}

/*
 * The hardware facts the MultIO model needs: is the 0x40 slave present,
 * does the master have cascade hardware, and is the master a Wunderbus
 * I/O board (whose DIP switch at 0x49 the kernel probes to tell the
 * master boards apart).
 */
int
wiring_slave(void)
{
    return wiring.slave;
}

int
wiring_cascade(void)
{
    return wiring.cascade;
}

int
wiring_wb_master(void)
{
    return wiring.wb_master;
}

/* set one card's line, adding the card if it is not already wired */
static int
set_card_line(struct wiring *w, char *name, int line)
{
    int i;

    for (i = 0; i < w->ncard; i++)
        if (strcmp(w->card[i].name, name) == 0) {
            w->card[i].line = line;
            return 0;
        }
    if (w->ncard >= NCARDWIRE)
        return -1;
    w->card[w->ncard].name = name;
    w->card[w->ncard].line = line;
    w->ncard++;
    return 0;
}

/* apply one key=value override; -1 on a bad key or value */
static int
apply_override(struct wiring *w, char *key, char *val)
{
    if (strncmp(key, "vi", 2) == 0 && key[2] >= '0' && key[2] <= '7' &&
        key[3] == '\0')
        return set_card_line(w, val, key[2] - '0');
    if (strncmp(key, "irq", 3) == 0 && isdigit((unsigned char)key[3])) {
        int pin = atoi(key + 3);
        if (pin < 0 || pin >= NIRQ)
            return -1;
        if (val[0] == 'v' && val[1] == 'i' && isdigit((unsigned char)val[2])) {
            w->irq[pin].kind = IRQ_VI;
            w->irq[pin].vi = atoi(val + 2);
            w->irq[pin].name = 0;
        } else {
            w->irq[pin].kind = IRQ_ONBOARD;
            w->irq[pin].vi = 0;
            w->irq[pin].name = val;
        }
        return 0;
    }
    if (strcmp(key, "int") == 0) {
        w->int_driver = val;
        return 0;
    }
    if (strcmp(key, "slave") == 0) {
        w->slave = atoi(val) != 0;
        return 0;
    }
    if (strcmp(key, "cas") == 0 || strcmp(key, "cascade") == 0) {
        w->cascade = atoi(val) != 0;
        return 0;
    }
    if (strcmp(key, "wb") == 0) {
        w->wb_master = atoi(val) != 0;
        return 0;
    }
    return -1;
}

/*
 * Parse a -w specification: "boards[,viN=card][,irqN=viM|source][,int=board]".
 * The leading comma-separated words name the boards (ss1, wb, mio,
 * wb,mio, mio,mio); the rest are assignments.  Returns 0 on a clean
 * parse, -1 on a bad board set or assignment.  The spec string is split
 * in place (it is the argv copy), so names point into it.
 */
int
parse_wiring(char *spec)
{
    struct wiring *w = &wiring;
    char *p, *comma, *eq;
    char *board[2];
    int nboard = 0;

    p = spec;
    for (;;) {
        comma = strchr(p, ',');
        if (comma)
            *comma = '\0';
        if (strchr(p, '=') != 0)
            break;                      /* the assignments start here */
        if (nboard >= 2)
            return -1;
        board[nboard++] = p;
        if (!comma)
            return (nboard && build_machine(w, board, nboard) == 0) ? 0 : -1;
        p = comma + 1;
    }

    if (nboard == 0 || build_machine(w, board, nboard) < 0)
        return -1;

    for (;;) {
        eq = strchr(p, '=');
        if (!eq)
            return -1;
        *eq = '\0';
        if (apply_override(w, p, eq + 1) < 0)
            return -1;
        if (!comma)
            return 0;
        p = comma + 1;
        comma = strchr(p, ',');
        if (comma)
            *comma = '\0';
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
