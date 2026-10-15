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

    // only have work to do if a change
    if (vi_lines != new) {
        (*vi_change)(new);     // the handler actually changes the line
    }
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
        trace(trace_ior, "input port %x registered\n", portnum);
    }
    input_handler[portnum] = func;
}

void
register_output(portaddr portnum, outhandler func)
{
    portnum &= 0xff;

    if (func != undef_out) {
        trace(trace_ior, "output port %x registered\n", portnum);
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
 * with wiring_vi_line().  The -W option picks a standard profile and then
 * applies name=value overrides, so a bare -W (or none) is today's machine.
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
};

struct wiring wiring;

/*
 * The standard profiles.  A card that shares a line is a second entry on
 * the same vi number; set_vi's card id keeps the open-collector masks
 * apart, so the one-PIC d1 machine and the two-PIC one differ only in
 * whether anything names a second controller's pins.
 */
static struct wiring profile_d1 = {
    { { "hddma", 0 }, { "hdca", 0 }, { "djdma", 1 }, { "disk1", 1 },
      { "ncr", 2 }, { "ide", 2 } },
    6,
    {
        [0] = { IRQ_VI, 0 },
        [1] = { IRQ_VI, 1 },
        [2] = { IRQ_VI, 2 },
        [3] = { IRQ_ONBOARD, 0, "uart0" },
        [4] = { IRQ_ONBOARD, 0, "uart1" },
        [5] = { IRQ_ONBOARD, 0, "uart2" },
        [7] = { IRQ_ONBOARD, 0, "clock" },
    },
    "multio",
};

static struct wiring profile_multio = {
    { { "hddma", 0 }, { "djdma", 1 }, { "hdca", 2 }, { "disk1", 1 },
      { "ncr", 3 }, { "ide", 4 }, { "nic", 5 }, { "disk3", 5 },
      { "disk2", 5 } },
    9,
    {
        [0] = { IRQ_VI, 0 },
        [1] = { IRQ_VI, 1 },
        [2] = { IRQ_VI, 2 },
        [3] = { IRQ_ONBOARD, 0, "uart0" },
        [4] = { IRQ_ONBOARD, 0, "uart1" },
        [5] = { IRQ_ONBOARD, 0, "uart2" },
        [6] = { IRQ_ONBOARD, 0, "slave" },
        [7] = { IRQ_ONBOARD, 0, "clock" },
        [8]  = { IRQ_VI, 3 },
        [9]  = { IRQ_VI, 4 },
        [10] = { IRQ_VI, 5 },
    },
    "multio",
};

static struct wiring profile_ss1 = {
    { { "hddma", 0 }, { "djdma", 1 }, { "hdca", 2 }, { "ncr", 3 },
      { "ide", 4 }, { "nic", 5 }, { "disk3", 6 }, { "disk1", 1 },
      { "disk2", 6 } },
    9,
    {
        [0] = { IRQ_VI, 0 }, [1] = { IRQ_VI, 1 }, [2] = { IRQ_VI, 2 },
        [3] = { IRQ_VI, 3 }, [4] = { IRQ_VI, 4 }, [5] = { IRQ_VI, 5 },
        [6] = { IRQ_VI, 6 },
        [7] = { IRQ_ONBOARD, 0, "slave" },
        [8] = { IRQ_VI, 7 },
        [9]  = { IRQ_ONBOARD, 0, "timer0" },
        [10] = { IRQ_ONBOARD, 0, "timer1" },
        [11] = { IRQ_ONBOARD, 0, "timer2" },
        [12] = { IRQ_ONBOARD, 0, "fpu0" },
        [13] = { IRQ_ONBOARD, 0, "fpu1" },
        [14] = { IRQ_ONBOARD, 0, "uart_tx" },
        [15] = { IRQ_ONBOARD, 0, "uart_rx" },
    },
    "ss1",
};

/*
 * The default machine is what the cards' hardcoded lines describe today:
 * the two-PIC Wunderbus + MultIO.  -w re-selects a profile and applies
 * overrides; this runs before main so the resolved table is ready when
 * the cards and the -w parser look at it.
 */
__attribute__((constructor))
static void
wiring_init(void)
{
    wiring = profile_multio;
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
    return -1;
}

static int
select_profile(struct wiring *w, char *name)
{
    if (strcmp(name, "d1") == 0)
        *w = profile_d1;
    else if (strcmp(name, "multio") == 0)
        *w = profile_multio;
    else if (strcmp(name, "ss1") == 0)
        *w = profile_ss1;
    else
        return 0;
    return 1;
}

/*
 * Parse a -W specification: "profile[,key=value]...".  The first token
 * names a profile, the rest override single entries.  Returns 0 on a
 * clean parse, -1 on a bad profile or override.  The spec string is
 * split in place (it is the argv copy), so names point into it.
 */
int
parse_wiring(char *spec)
{
    struct wiring *w = &wiring;
    char *p, *comma, *eq;

    comma = strchr(spec, ',');
    if (comma)
        *comma = '\0';
    if (!select_profile(w, spec))
        return -1;
    if (!comma)
        return 0;

    p = comma + 1;
    for (;;) {
        comma = strchr(p, ',');
        if (comma)
            *comma = '\0';
        eq = strchr(p, '=');
        if (!eq)
            return -1;
        *eq = '\0';
        if (apply_override(w, p, eq + 1) < 0)
            return -1;
        if (!comma)
            break;
        p = comma + 1;
    }
    return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
