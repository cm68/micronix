/*
 * simple.c
 *
 * simple cpu - no translation or I/O shenanigans
 * intended to be template for more involved cpu cards
 */

#include <stdio.h>
#include "sim.h"

int trace_io;

byte
get_byte(word addr)
{
	return physread((paddr)addr);
}

void
put_byte(word addr, byte value)
{
	return physwrite((paddr)addr, value);
}

void
output(portaddr p, byte v)
{
    if (trace & trace_io) printf("io: output 0x%x to 0x%x\n", v, p);
    (*output_handler[p]) (p, v);
}

byte
input(portaddr p)
{
    byte v;

    v = (*input_handler[p])(p);

    if (trace & trace_io) printf("io: input 0x%x got 0x%x\n", p, v);
    return v;
}

__attribute__((constructor))
void
register_simple_driver()
{
    trace_io = register_trace("io");
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
