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
 * memory map of the 0 page
 */
#define RAM         0x000       // 1k of ram
#define REGS        0x400       // registers
#define MAP         0x600       // map ram
#define EPROM       0x800       // half of the eprom
#define FPU         0xc00       // 9511/9512
#define LOCAL       0x1000      // everything below here is on board

/*
 * mpz80 cpu registers and local ram
 */
byte local_ram[0x400];      // used for register save areas and map shadow  - 0x000 - 0x3ff
byte maps[0x200];           // map registers write only                     - 0x600 - 0x7ff
byte eprom[0x400];          // half at a time                               - 0x800 - 0xbff
byte fpu_data;              //                                              - c08
byte fpu_cmd;               //                                              - c00
byte fpu_status;            //                                              - c00

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
static char *nullbits[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/* write */
byte fpseg;
#define FPSEG   0x400       // front panel segment

byte fpcol;
#define FPCOL   0x401       // front panel column

byte next_taskreg;          // taskreg after countdown
byte taskreg;
#define TASK    0x402       // task register

byte maskreg;
#define MASK    0x403       // mask register
#define     MASK_STOP   0x01
#define     MASK_AUX    0x02        
#define     MASK_TINT   0x04
#define     MASK_STEP   0x08
#define     MASK_HALT   0x10
#define     MASK_SINT   0x20
#define     MASK_IO     0x40
#define     MASK_ZIO    0x80
static char *mask_bits[] = { "stop", "aux", "tint", "step", "halt", "sint", "io", "zio" };
static byte *wregp[] = { &fpseg, &fpcol, &next_taskreg, &maskreg };
static char **wregbits[] = { 0, 0, 0, mask_bits };

/* read */
byte trapreg;
#define TRAP    0x400       // trap address register

byte keybreg;
#define KEYB    0x401       // front panel keyboard connector 12C
#define     KB_UNUSED   0x01        // P1 - 12
#define     KB_DIAG     0x02        // P1 - 13 if high, run diagnostics
static char *keyb_bits[] = { 0, "diag", 0, 0, 0, 0, 0, 0 };

// this register is negated:  if the switch is on, the value reads low
byte switchreg;
#define SWT     0x402       // switch register board location 16D
#define     SW_NOMON    0x04        // S6 - if high, no minotaur
#define     SW_JUMP     0xf8        // S1 - S5 negated jump address
#define     SW_DJDMA    0x10        // boot djdma
#define     SW_HDDMA    0x08        // boot hdcdma
#define     SW_HDCA     0x00        // boot hdca
#define     SW_RESET    0x01
#define     SW_IPEND    0x02
static char *swt_bits[] = { "reset", "ipend", "monitor", 0, 0, 0, 0, 0 };

int trace_mpz80;
int trace_map;
int trace_mem;

byte trapstat;
#define STAT    0x403       // trap status register
#define     ST_VOID     0x01
#define     ST_IORQ     0x02
#define     ST_HALT     0x04
#define     ST_INT      0x08
#define     ST_STOP     0x10
#define     ST_AUX      0x20
#define     ST_R10      0x40
#define     ST_READ     0x80
#define ST_RESET    (ST_VOID | ST_IORQ | ST_HALT | ST_INT | ST_STOP | ST_AUX | ST_READ)

static char *stat_bits[] = { "void", "iorq", "halt", "int", "stop", "aux", "r10", "read" };

static char **rregbits[] = { 0, keyb_bits, swt_bits, stat_bits };
static char *rregname[] = { "trap", "keyb", "switch", "trapstat" };
static byte *rregp[] = { &trapreg, &keybreg, &switchreg, &trapstat };

/*
 * fpu is given little more than a tickle here
 */
static char *fpustatb[] = { 0, "expover", "expunder", "diveerr", 0, "zero", "sign", "busy" };
static char *fpucmd[] = { 
    "CLR", "SADD", "SSUB", "SMUL", "SDIV", "CHSS", "PTOS", "POPS",  // 00 - 07
    "XCHS", 0, 0, 0, 0, 0, 0, 0,                                    // 08 - 0F
    0, 0, 0, 0, 0, 0, 0, 0,                                         // 10 - 17
    0, 0, 0, 0, 0, 0, 0, 0,                                         // 18 - 1F
    0, 0, 0, 0, 0, 0, 0, 0,                                         // 20 - 27
    0, "DAD", "DSUB", "DMUL", "DDIV", "CHSD", "PTOD", "POPD",       // 28 - 2F
    0, 0, 0, 0, 0, 0, 0, 0,                                         // 30 - 37
    0, 0, 0, 0, 0, 0, 0, 0                                          // 38 - 3F
};

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

char *wregname[] = { "fpseg", "fpcol", "trap", "mask" };
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
 *
 * to makes things even clearer, I'm going to call various state flags by the
 * name used in the mpz80 manual.
 *
 */

/*
 * these count down bus cycles of a certain kind to facilitate task switches
 * and implement the trap function
 */

int trapcount;

/*
 * a counter set by writing the task register and decremented by M1
 * if nonzero, also inhibits interrupt detection and traps
 */
static byte delay;

/*
 * task 0 is accessing less than 0x1000.  null bus cycles go out to the bus.
 */
static byte local;

void
trap(byte trapbits)
{
    if (trace & trace_mpz80) printf("trap %x %s\n", trapbits, bitdef(trapbits, stat_bits));
    taskreg = 0;
    trapcount = 15;
    trapstat = trapbits;
}

/*
 * do a virtual lookup of address and return the physical address and page attributes
 */
static void
getpte(word addr, paddr *paddrp, byte *attrp)
{
    byte taskid = taskreg & 0xf;
    byte page = (addr & 0xf000) >> 12;
    byte pte = (taskid << 5) + (page << 1);

    *paddrp = ((taskreg & 0xf0) << 16) | (maps[pte] << 12) + (addr & 0xfff);
    *attrp = maps[pte + 1];
}

static int prefix;      // was the last M1 a prefix instruction
inst_disabled = 0;
extern int trace_inst;

/*
 * the mpz80 inhibits reads and writes for a fixed number of memory cycles after a trap
 * it also lets some instructions fetch from task 0 when doing a task switch
 */
unsigned char
get_byte(vaddr addr)
{
    byte attr;
    byte retval;

    vaddr orig = addr;
    paddr pa = addr;
    char *seg = "";
    char *regname = "";
    char *desc = "";

    // if we are trapping, ignore the passed in address
    if (trapcount && running) {
        if (trace & trace_inst) {
            inst_disabled = 1;
            trace &= ~trace_inst;
        }
        addr = 0xbf0 + (15 - trapcount);
        if (trace & trace_mpz80) printf("mpz80: trap %x replaced by %x\n", orig, addr);
        trapcount--;
        if (inst_disabled && !trapcount) {
            trace |= trace_inst;
        }
    }

    // the task register starts a countdown for instruction fetches
    if (delay != 0) {
        if (z80_get_reg8(status_reg) & S_M1) {
            delay--;
        }
        if (delay == 0) {
            if (trace & trace_mpz80) printf("switching taskreg\n");
            taskreg = next_taskreg;
        }
    }

    local = ((taskreg & 0xf) == 0) && (addr < 0x1000);

    if (!local) {                           // if we are accessing mapped ram
        getpte(addr, &pa, &attr);
        seg = "mapped:";
        retval = physread(pa);
    } else switch (addr & 0xe00) {
    case RAM: case RAM + 0x200:             // 1k static ram
        seg = "ram:";
        pa = addr;
        retval = local_ram[addr];
        break;
    case REGS:                              // on board registers, only 2 bits decoded
        seg = "regs:";
        pa = addr & 3;
        regname = rregname[pa];
        retval = *rregp[pa];
        desc = bitdef(retval, rregbits[pa]);
        break;
    case MAP:                               // the map registers don't allow read
        printf("illegal map register read\n");
        break;
    case EPROM: case EPROM + 0x200:         // eprom
        pa = addr - EPROM;
        seg = "eprom:";
        retval = eprom[pa];
        break;
    case FPU:                               // fpu has 2 registers
        seg = "fpu:";
        pa = (addr >> 3) & 1;
        if (pa) {
            regname = "status";
            desc = bitdef(fpu_status, fpustatb);
            retval = fpu_status;
        } else {
            regname = "data";
            retval = fpu_data;
        }
        break;
    }

    /*
     * when we encounter a halt instruction with M1 and are not in task 0, we return a NOP to the
     * emulation.  the next M1 after this starts executing at 0xbf0 by disabling the address bus
     * and driving the fetch via a counter
     */
    if (((taskreg & 0xf) != 0) && 
        (z80_get_reg8(status_reg) & S_M1) && 
        (retval == 0x76) && 
        (!prefix) && (maskreg & MASK_HALT)) {
        trap(ST_RESET & ~ST_HALT);
        seg = "nop:";
        retval = 0;
    }

    if (running && (z80_get_reg8(status_reg) & S_M1) && 
        ((retval == 0xED) || (retval == 0xDD) || (retval == 0xFD) || (retval == 0xCB))) {
        prefix = 1;
    } else {
        prefix = 0;
    }

    if (running && (trace & trace_mem)) {
        printf("mem: read %04x (%s%06x) %s got %02x %s\n", orig, seg, pa, regname, retval, desc);
    } 
    return retval;
}

void
put_byte(vaddr addr, unsigned char value)
{
    paddr pa;
    byte attr;
    byte local;
    char **bitsp = 0;
    char *seg = "";
    char *regname = "";
    char *desc = "";
    char *cmd;
 
    local = ((taskreg & 0xf) == 0) && (addr < 0x1000);

    if (!local) {                           // mapped ram
        seg = "mapped:";
        getpte(addr, &pa, &attr);
        physwrite(pa, value);
    } else switch(addr & 0xe00) {
    case RAM: case RAM + 0x200:             // 1k static ram
        seg = "ram:";
        pa = addr;
        local_ram[addr] = value;
        break;
    case REGS:                              // on-board registers, 2 bits decoded
        seg = "regs:";
        pa = addr & 3;
        desc = bitdef(value, wregbits[pa]);
        regname = wregname[pa];
        *wregp[pa] = value;
        if ((addr & (REGS | 0x3)) == TASK) {
            setrom(1);
            delay = 8;
        }
        break;
    case MAP:                               // access to mapping ram
        seg = "maps:";
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
        break;
    case EPROM: case EPROM + 0x200:         // fung wha?!
        printf("write to eprom\n");
        break;
    case FPU:                               // write to fpu registers
        seg = "fpu:";
        pa = (addr >> 3) & 1;
        if (pa) {
            regname = "command";
            desc = fpucmd[value];
            if (!cmd) cmd = "";
            fpu_cmd = value;
        } else {
            regname = "data";
            fpu_data = value;
        }
        break;
    }
    if (running && (trace & trace_mem)) {
        printf("mem: write %04x (%s%06x) %s put %02x %s\n", addr, seg, pa, regname, value, desc);
    } 
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
    trap(ST_RESET);
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
    trace_mem = register_trace("mem");

    register_prearg_hook(mpz80_init);
    register_startup_hook(mpz80_startup);
    register_usage_hook(mpz80_usage);
    register_mon_cmd('m', "[task]\tdump memory map\n", map_cmd);
}


/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
