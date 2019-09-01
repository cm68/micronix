/*
 * s100.c
 *
 * a 24 bit address bus, and i/o registration framework
 */

#include <stdio.h>
#include "sim.h"

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
        if (trace & trace_ior)
            printf("input port %x registered\n", portnum);
    }
    input_handler[portnum] = func;
}

void
register_output(portaddr portnum, outhandler func)
{
    portnum &= 0xff;

    if (func != undef_out) {
        if (trace & trace_ior)
            printf("output port %x registered\n", portnum);
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
