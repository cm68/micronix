/* Morrow decision 1
 * this emulates the MPZ80
 *
 * Copyright (c) 2019, Curt Mayer
 * do whatever you want, just don't claim you wrote it.
 * warrantee:  madness!  nope.
 *
 * This code is free, do whatever you want with it.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
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
#include "util.h"

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
#define     KB_DIAG     0x02        // P1 - 13 if high, run diagnostics

// this register is negated:  if the switch is on, the value reads low
byte switchreg;
#define SWT     0x402       // switch register board location 16D
#define     SW_NOMON    0x04        // S6 - if high, no minotaur
#define     SW_JUMP     0xf8        // S1 - S5 negated jump address
#define     SW_DJDMA    0x10        // boot djdma
#define     SW_HDDMA    0x08        // boot hdcdma
#define     SW_HDCA     0x00        // boot hdca

int trace_mpz80;
int trace_map;

byte trapstat;
#define STAT    0x403       // trap status register

/*
 * these count down bus cycles of a certain kind to facilitate task switches
 * and implement the trap function
 */
int trapcount;
int taskcount;
word trapaddr;

/*
 * copy the appropriate half of the rom into the executable address space
 * this is rare, so it's ok to make slow.
 */
void
setrom(int page)
{
    static int last = 0xff;
    if (last != page) {
        if (trace & trace_map)
            printf("enable page %d of eprom\n", page);
        memcpy(eprom, rom_image + (page * 0x400), 0x400);
        last = page;
    }
}

char *rregname[] = { "trap", "keyb", "switch", "trapstat" };
char *pattr[] = { "no access", "r/o", "execute", "full" };

// dump out memory map
int
map_cmd(char **sp)
{
    int i;
    byte task;
    byte attr;
    int page;

    skipwhite(sp);
    if (**sp) {
        i = strtol(*sp, sp, 16);
    } else {
        i = taskreg & 0xf;
    }

    task = i & 0xf;

    printf("task %d map\n", task);
    for (i = 0; i < 16; i++) {
        page = task * 32 + (i << 1);
        attr = maps[page + 1];
        printf("0x%04x 0x%06x %s%s\n",
            i << 12, 
            maps[page] << 12,
            pattr[attr & 0x3], (attr & 0x4) ? " r10" : "");
    }
    return 0;
}

/*
 * the MPZ80 actually does something tricky:  the first 15 M1 reads are
 * satisfied from 0xbf0 - 0xbff.  we don't actually modify the PC at all, 
 * it just merrily counts as usual, and we feed it opcodes from bf0-bff in rom.
 * the Z80 still thinks it is executing at it's normal PC.
 * but since the task register also got reset, all the memory writes go to
 * to supervisor space
 */
void
trap()
{
    taskreg = 0;
    trapcount = 15;
    trapaddr = z80_get_reg16(pc_reg);
}

/*
 * the mpz80 inhibits reads and writes for a fixed number of memory cycles after a trap
 * it also lets some instructions fetch from task 0 when doing a task switch
 */
unsigned char
get_byte(vaddr addr)
{
    byte taskid;
    byte page;
    byte pte;
    paddr pa;
    byte attr;
    byte retval;
    static vaddr lastfetch = 0;

    // if we are trapping, ignore the passed in address
    if (trapcount) {
        int offset = addr - trapaddr;
        if (offset > 16) {
            if (running) {
                printf("bizarre offset in trap %x %x %x\n", trapaddr, addr, offset);
            }
        } else {
            addr = 0xbf0 + offset;
        }
        if (running) {
            trapcount--;
        }
    }

    // the task register starts a countdown for instruction fetches
    if (taskctr != 0) {
        if (z80_get_reg8(status_reg) & S_M1) {
            taskctr--;
        }
        if (taskctr == 0) {
            if (trace & trace_mpz80) printf("switching taskreg\n");
            taskreg = next_taskreg;
        }
    }

    taskid = taskreg & 0xf;
    page = (addr & 0xf000) >> 12;
    pte = (taskid << 5) + (page << 1);
    pa = ((taskreg & 0xf0) << 16) | (maps[pte] << 12) + (addr & 0xfff);
    attr = maps[pte + 1];

    if ((taskid != 0) || (addr > 0x1000)) {
        retval = physread(pa);
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
            if (trace & trace_mpz80) printf("unknown local reg %x read\n", addr);
            return 0;
        }
        if (trace & trace_mpz80) printf("mpz80: read %s register %x\n", rregname[addr & 0x03], retval);
    } else if (addr < EPROM) {
        if (trace & trace_mpz80) printf("illegal map register read\n");
        return 0;
    } else if (addr < FPU) {
        retval = eprom[addr & 0x3ff];
    } else {
        retval = fpu[addr & 0x3ff];
    }
    return retval;
}

char *wregname[] = { "fpseg", "fpcol", "trap", "mask" };
void
put_byte(vaddr addr, unsigned char value)
{
    byte taskid = taskreg & 0xf;
    byte page = (addr & 0xf000) >> 12;
    byte pte = (taskid << 5) + (page << 1);
    paddr pa = ((taskreg & 0xf0) << 16) | (maps[pte] << 12) + (addr & 0xfff);
    byte attr = maps[pte + 1];

    // access to physical memory through mapping ram
    if ((taskid != 0) || (addr > 0x1000)) {
        physwrite(pa, value);
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
            if (trace & trace_mpz80) printf("unknown local reg %x read\n", addr);
            return;
        }
        if (trace & trace_mpz80) printf("mpz80: %s write %x\n", wregname[addr & 0x3], value);
        return;
    }

    // access to mapping ram
    if (addr < EPROM) {
        paddr offset = addr & 0x1ff;
        byte task = (addr >> 5) & 0xf;
        byte page = (addr >> 1) & 0xf;

        if (trace & trace_map) {
            printf("map register %x write %x task %d page %x ", addr, value, task, page);
            if (addr & 0x01) {
                printf("%s %s\n", value & 0x8 ? "r10" : "", pattr[value & 0x3]);
            } else {
                printf("physical 0x%2x000\n", value);
            }
        }
        maps[offset] = value;
        return;

    }
    // write to eprom ?!
    if (addr < FPU) {
        if (trace & trace_mpz80) printf("write to eprom\n");
        return;
    }

    // write to fpu registers
    fpu[addr & 0x3ff] = value;
}

byte
input(portaddr p)
{
    return s100_input(p);
}

void
output(portaddr p, byte v)
{
    s100_output(p, v);
}

/*
 * whenever a trap happens, this address is forced onto the address bus
 */
#define	TRAPADDR	0xBF0

void
mpz80_usage()
{
    printf("config switch values:\n");
    printf("\t0x04 - no monitor entry\n");
    printf("\t0xf8 - start address mask\n");
    printf("\t0x00 - boot hdca\n");
    printf("\t0x08 - boot hddma\n");
    printf("\t0x10 - boot djdma\n");
}

/*
 * this sets up our emulator settings before we do argument processing
 */
int
mpz80_init()
{
    rom_filename = "mon447.bin";
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
    if (config_sw & CONF_SET) {
        switchreg = config_sw & 0xff;   // could be multiple bytes of config
    }
    z80_set_reg16(pc_reg, 0);
    trap();
    return 0;
}

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_mpz80_driver()
{
    trace_mpz80 = register_trace("mpz80");
    trace_map = register_trace("map");

    register_prearg_hook(mpz80_init);
    register_startup_hook(mpz80_startup);
    register_usage_hook(mpz80_usage);
    register_mon_cmd('m', "[task]\tdump memory map\n", map_cmd);
}


/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
