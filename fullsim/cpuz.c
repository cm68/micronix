/* Morrow decision 1
 * this emulates the MPZ80
 *
 * Copyright (c) 2019, Curt Mayer
 * do whatever you want, just don't claim you wrote it.
 * warrantee:  madness!  nope.
 *
 * plugs into the z80emu code from:
 * Copyright (c) 2012, 2016 Lin Ke-Fong
 * Copyright (c) 2012 Chris Pressey
 *
 * This code is free, do whatever you want with it.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "z80emu.h"
#include "z80user.h"
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#include <setjmp.h>

#include "sim.h"

extern MACHINE cpu;
extern byte physmem[];

/*
 * memory map
 */
#define LOCAL   0x400
#define MAP     0x600
#define EPROM   0x800
#define FPU     0xc00

/*
 * mpz80 cpu registers and local ram
 */
byte local_ram[0x400];      // used for register save areas and map shadow  - 0x000 - 0x3ff
byte maps[0x200];           // map registers write only                     - 0x600 - 0x7ff
byte eprom[0x400];          // half at a time                               - 0x800 - 0xbff
byte fpu[0x400];            // floating point processor                     - 0xc00 - 0xfff

/*
 * mon4.47 and mon 3.75 both were distributed on 2732A's, which are 4kb
 * devices. the rom image file contains 2 copies, org'd at 0x800.
 * the monitor has 2 1k sections, both of which are assembled for 0x800-0xbff
 * the lowest is only enabled after a reset until the trap register is written
 * which then enables the high half.  kind of a waste of half the chip, but
 * I think the 2732A's were faster than the 2716's the board originally was
 * designed for, which is why they did the upgrade.  it may be better to
 * go back to the 2kb part, since AT28C16's are gettable in 120ns.
 */

/*
 * board registers
 */
/* write */
byte fpseg;
#define FPSEG   0x400       // front panel segment

byte fpcol;
#define FPCOL   0x401       // front panel column

byte taskctr;               // countdown for context switch
byte next_taskreg;          // taskreg after countdown
byte taskreg;
#define TASK    0x402       // task register

byte maskreg;
#define MASK    0x403       // mask register


/* read */
byte trapreg;
#define TRAP    0x400       // trap address register

byte keybreg;
#define KEYB    0x401       // front panel keyboard connector 12C
#define     KB_UNUSED   0x01        // P1 - 12
#define     KB_MON      0x02        // P1 - 13 if low, jump to monitor

// this register is negated:  if the switch is on, the value reads low
byte switchreg;
#define SWT     0x402       // switch register board location 16D
#define     SW_NOMON    0x04        // S6 - if high, no minotaur
#define     SW_JUMP     0xf8        // S1 - S5 negated jump address
#define     SW_DJDMA    0x10        // boot djdma
#define     SW_HDDMA    0x08        // boot hdcdma
#define     SW_HDCA     0x00        // boot hdca

byte trapstat;
#define STAT    0x403       // trap status register

/*
 * copy the appropriate half of the rom into the executable address space
 * this is rare, so it's ok to make slow.
 */
void
setrom(int page)
{
    static int last = 0xff;
    if (last != page) {
        if (verbose & V_MAP)
            printf("enable page %d of eprom\n", page);
        memcpy(eprom, rom_image + (page * 0x400), 0x400);
        last = page;
    }
}

char *rregname[] = { "trap", "keyb", "switch", "trapstat" };

#ifdef notdef
/*
 * this function does executes all the machinery needed to do the MPZ80's
 * trap function.  this can be called from any of the emulation and ends in
 * a longjmp to our main instruction loop with the registers set up to run
 * the rom code that saves what needs to be saved, etc.
 *
 * the MPZ80 actually does something tricky:  the first 15 memory reads are
 * satisfied from 0xbf0 - 0xbff.  we don't actually modify the PC at all.
 * that is for the code at 0xbf0 to do.  the Z80 still thinks it is executing
 * at it's normal PC.
 * so, setting the trap really just modifies the location where instructions
 * are really being fetched from, not the PC itself.
 * but since the task register also got reset, all the memory writes go to
 * to supervisor space
 */
void
trap()
{
    longjmp(trap_load, 1);
}
#endif

void
printmap(int task)
{
    int i;
    
    task &= 0xf;
    printf("task %d map\n", task);
    for (i = 0; i < 16; i++) {
        printf("0x%04x 0x%06x\n", i << 12, 
            maps[(task * 32) + (i << 1)] << 12);
    }
}

unsigned char
memread(vaddr addr)
{
    byte taskid;
    byte page;
    byte pte;
    paddr pa;
    byte attr;
    byte retval;
    static vaddr lastfetch = 0;

    // the task register starts a countdown for instruction fetches
    if (taskctr != 0) {
        // only count m1 cycles
        if (addr == cpu.pc) {
            // the emulator reads the instruction twice sometimes!
            if (lastfetch != addr) {
                if (verbose & V_MPZ) printf("taskctr decrement %x\n", addr);
                taskctr--;
                lastfetch = addr;
            }
        }
        if (taskctr == 0) {
            if (verbose & V_MPZ) printf("switching taskreg\n");
            taskreg = next_taskreg;
        }
    }

    taskid = taskreg & 0xf;
    page = (addr & 0xf000) >> 12;
    pte = (taskid << 5) + (page << 1);
    pa = ((taskreg & 0xf0) << 16) | (maps[pte] << 12) + (addr & 0xfff);
    attr = maps[pte + 1];

    if ((taskid != 0) || (addr > 0x1000)) {
        retval = physmem[pa];
    } else if (addr < LOCAL) {
        retval = local_ram[addr];
    } else if (addr < MAP) {
        switch (addr) {
        case TRAP:
            retval = trapreg;
            break;
        case KEYB:
            retval = keybreg;
            break;
        case SWT:
            retval = switchreg;
            break;
        case STAT:
            retval = trapstat;
            break;
        default:
            if (verbose & V_MPZ) printf("unknown local reg %x read\n", addr);
            return 0;
        }
        if (verbose & V_MPZ) printf("mpz80: read %s register %x\n", rregname[addr & 0x03], retval);
    } else if (addr < EPROM) {
        if (verbose & V_MPZ) printf("illegal map register read\n");
        return 0;
    } else if (addr < FPU) {
        retval = eprom[addr & 0x3ff];
    } else {
        retval = fpu[addr & 0x3ff];
    }
    return retval;
}

char *pattr[] = { "no access", "r/o", "execute", "full" };
char *wregname[] = { "fpseg", "fpcol", "trap", "mask" };
void
memwrite(vaddr addr, unsigned char value)
{
    byte taskid = taskreg & 0xf;
    byte page = (addr & 0xf000) >> 12;
    byte pte = (taskid << 5) + (page << 1);
    paddr pa = ((taskreg & 0xf0) << 16) | (maps[pte] << 12) + (addr & 0xfff);
    byte attr = maps[pte + 1];

    // access to physical memory through mapping ram
    if ((taskid != 0) || (addr > 0x1000)) {
        physmem[pa] = value;
        return;
    }

    // access to 1k on-board ram
    if (addr < LOCAL) {
        local_ram[addr] = value;
        return;
    }

    // access to local I/O registers
    if (addr < MAP) {
        switch(addr) {
        case FPSEG:
            break;
        case FPCOL:
            break;
        case TASK:
            setrom(1);
            taskctr = 8;
            next_taskreg = value;
            break;
        case MASK:
            break;
        default:
            if (verbose & V_MPZ) printf("unknown local reg %x read\n", addr);
            return;
        }
        if (verbose & V_MPZ) printf("mpz80: %s write %x\n", wregname[addr & 0x3], value);
        return;
    }

    // access to mapping ram
    if (addr < EPROM) {
        paddr offset = addr & 0x1ff;
        byte task = (addr >> 5) & 0xf;
        byte page = (addr >> 1) & 0xf;

        if (verbose & V_MAP) {
            printf("map register %x write %x task %d page %x ", addr, value, task, page);
            if (addr & 0x01) {
                printf("%s %s\n", value & 0x8 ? "r10" : "", pattr[value & 0x7]);
            } else {
                printf("physical 0x%2x000\n", value);
            }
        }
        maps[offset] = value;
        return;

    }
    // write to eprom ?!
    if (addr < FPU) {
        if (verbose & V_MPZ) printf("write to eprom\n");
        return;
    }

    // write to fpu registers
    fpu[addr & 0x3ff] = value;
}

/*
 * whenever a trap happens, this address is forced onto the address bus
 */
#define	TRAPADDR	0xBF0

/*
 * this sets up our emulator settings before we do argument processing
 */
int
mpz80_init()
{
    bootrom = "mon447.bin";
    rom_size = 4096;
    switchreg = SW_HDDMA | SW_NOMON;    // set diagnostic, monitor or boot mode
    switchreg = SW_DJDMA | SW_NOMON;    // set diagnostic, monitor or boot mode
    keybreg = 0;
    taskreg = 0;
    return 0;
}

int
mpz80_startup()
{
    setrom(0);
    cpu.pc = TRAPADDR;
}

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_mpz80_driver()
{
    register_prearg_hook(mpz80_init);
    register_startup_hook(mpz80_startup);
}


/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
