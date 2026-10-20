/*
 * The interrupt topology.
 *
 * include/sys/ints.h
 *
 * The board is discovered, not compiled in: pic_init() (sys/pic_init.c)
 * probes at boot and fills the resident line table (sys/pic8259.h), so a
 * driver never has to know the topology, and one kernel boots every
 * board.  N8259 is the one number still known ahead of time - how many
 * 8259s the build carries - and it is what sys/multio.c compiles its
 * clock/uart slave half against.
 */
#ifndef N8259
#define N8259       1
#endif
