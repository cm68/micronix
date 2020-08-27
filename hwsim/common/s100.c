/*
 * s100.c
 *
 * a 24 bit address bus, and i/o registration framework
 */

#include <stdio.h>
#include "sim.h"
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

int trace_ior;                  // i/o registration trace

/*
 * the S100 bus memory space
 */
static byte physmem[16*1024*1024];

/*
 * access memory in the 24 bit S100 space using a physical address
 */
byte
physread(paddr p)
{
    p &= 0xffffff;
    return physmem[p];
}

void
physwrite(paddr p, byte v)
{
    p &= 0xffffff;
    physmem[p] = v;
}

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
    printf("input from undefined port %x\n", p);
    return 0xff;
}

void
undef_out(portaddr p, byte v)
{
    p &= 0xff;
    printf("output to  undefined port %x -> %x\n", p, v);
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

    trace_ior = register_trace("ioregister");
    for (i = 0; i < 256; i++) {
        input_handler[i] = undef_in;
        output_handler[i] = undef_out;
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
