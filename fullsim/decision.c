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

#include "sim.h"

typedef unsigned int ULONG;
typedef unsigned char UCHAR;
typedef unsigned short UINT;

#define	LISTLINES	8
#define	STACKTOP	0xffff
#define MAXIMUM_STRING_LENGTH   100

extern void dumpmem(unsigned char (*get) (int a), int addr, int len);
static void emulate_z80();

int debug_terminal;
int mypid;
char *mytty;

char *vopts[] = {
    "V_IO", "V_INST", "V_IOR", "V_MAP", "V_DJDMA", "V_MIO", "V_HDCA", "V_MPZ", "V_IMD", "V_BIO", 0
};

int verbose;

MACHINE context;

/*
 * the S100 bus memory space
 */
byte physmem[16*1024*1024];

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
byte rom_image[4096];

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

#define EXIT_PORT   0
#define DUMP_PORT   1

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
        memcpy(eprom, &rom_image[page * 0x400], 0x400);
        last = page;
    }
}

char *rregname[] = { "trap", "keyb", "switch", "trapstat" };

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
        if (addr == context.state.pc) {
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

volatile int breakpoint;

struct MACHINE *cp;

int stop[10];

void
stop_cpu()
{
    breakpoint = 1;
}

void
stop_handler()
{
    printf("breakpoint signal\n");
    breakpoint = 1;
}

void
pid()
{
    printf("%d: ", mypid);
}

unsigned short
reloc(unsigned short addr)
{
    return 0;
}

struct sym {
    char *name;
    vaddr value;
    struct syms *next;
} *syms;

void
add_sym(char *name, vaddr v)
{
    struct sym *s;
    s = malloc(sizeof(*s));
    s->next = syms;
    syms = s;
    s->name = strdup(name);
    s->value = v;
}

int
find_symbol(char *ls)
{
    struct sym *s = syms;

    while (s) {
        if (strcasecmp(s->name, ls) == 0) {
            return (s->value);
        }
        s = s->next;
    }
    return -1;
}

char *
lookup_sym(unsigned short addr)
{
    struct sym *s = syms;

    while (s) {
        if (s->value == addr) {
            return (s->name);
        }
        s = s->next;
    }
    return 0;
}

void
load_symfile(char *s)
{
    FILE *sf;
    int v;
    char namebuf[20];
    char kbuf[20];
    char linebuf[100];

    sf = fopen(s, "r");
    if (!sf) return;
    while (1) {
        if (fgets(linebuf, sizeof(linebuf), sf) == 0) {
            break;
        }
        if (sscanf(linebuf, "%s %s 0x%x", kbuf, namebuf, &v) != 3) {
            if (sscanf(linebuf, "%x %s", &v, namebuf) != 2) {
                break;
            }
        }
        // printf("adding symbol %s : %x\n", namebuf, v);
        add_sym(namebuf, v);
    }
    fclose(sf);
}

/*
 * access memory in the 24 bit S100 space using a physical address
 */
byte 
physread(paddr p)
{
    return physmem[p];
}

void
physwrite(paddr p, byte v)
{
    physmem[p] = v;
}

/*
 * bulk move to physical memory
 */
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
copyin(byte *buf, paddr pa, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        buf[i] = physread(pa+i);
    }
}

/*
 * used by the emulator to do all translated memory operations
 * this code could break the emulation flow, depending on permissions
 * this is where all the memory mapping of onboard IO for task 0, etc
 * is abstracted out. 
 */
void
put_word(unsigned short addr, unsigned short value)
{
    memwrite(addr, value);
    memwrite(addr + 1, value >> 8);
}

void
put_byte(unsigned short addr, unsigned char value)
{
    memwrite(addr, value);
}

unsigned short
get_word(unsigned short addr)
{
    return memread(addr) + (memread(addr + 1) << 8);
}

unsigned char
get_byte(unsigned short addr)
{
    return memread(addr);
}

static void
push(unsigned short s)
{
    cp->state.registers.word[Z80_SP] -= 2;
    put_word(cp->state.registers.word[Z80_SP], s);
}

static unsigned short
pop()
{
    unsigned short i;

    i = get_word(cp->state.registers.word[Z80_SP]);
    cp->state.registers.word[Z80_SP] += 2;
    return (i);
}

inhandler input_handler[256];
outhandler output_handler[256];

void
exit_port_handler(portaddr p, byte v)
{
    printf("exit port tickled %x\n", v);
    exit(0);
}

#define ALLMEM  64*1024
static char dumpbuf[ALLMEM];
void
dump_port_handler(portaddr p, byte v)
{
    int fd;
    int i;

    printf("dump port tickled %x\n", v);
    fd = creat("dumpfile", 0777);
    for (i = 0; i < ALLMEM; i++) {
        dumpbuf[i] = memread(i);
    }
    write(fd, dumpbuf, ALLMEM);
    write(fd, &cp->state.pc, 2);
    write(fd, &cp->state.registers.byte[Z80_A], 1);
    write(fd, &cp->state.registers.byte[Z80_F], 1);
    write(fd, &cp->state.registers.word[Z80_BC], 2);
    write(fd, &cp->state.registers.word[Z80_DE], 2);
    write(fd, &cp->state.registers.word[Z80_HL], 2);
    write(fd, &cp->state.registers.word[Z80_SP], 2);
    write(fd, &cp->state.registers.word[Z80_IX], 2);
    write(fd, &cp->state.registers.word[Z80_IY], 2);
    close(fd);
}

byte
undef_in(portaddr p)
{
    printf("input from undefined port %x\n", p);
    stop_cpu();
    return 0xff;
}

void
undef_out(portaddr p, byte v)
{
    stop_cpu();
    printf("output to  undefined port %x -> %x\n", p, v);
}

void
register_input(portaddr portnum, inhandler func)
{
    if (func != undef_in) {
        if (verbose & V_IOR)
            printf("input port %x registered\n", portnum);
    }
    input_handler[portnum] = func;
}

void
register_output(portaddr portnum, outhandler func)
{
    if (func != undef_out) {
        if (verbose & V_IOR)
            printf("output port %x registered\n", portnum);
    }
    output_handler[portnum] = func;
}

void
ioinit()
{
    int i;

    for (i = 0; i < 256; i++) {
        input_handler[i] = undef_in;
        output_handler[i] = undef_out;
    }
}

void
output(portaddr p, byte v)
{
    (*output_handler[p]) (p, v);
}

byte
input(portaddr p)
{
    return (*input_handler[p]) (p);
}

/*
 * these driver hooks are called at reset time
 */
#define MAXDRIVERS 4
int (*startup_hook[MAXDRIVERS])();

/*
 * this gets called between instructions
 */
void (*poll_hook[MAXDRIVERS])();

#define	TTY_FD	64

void
register_startup_hook(int (*func)())
{
    int i;

    for (i = 0; i < MAXDRIVERS; i++) {
        if (!startup_hook[i]) {
            startup_hook[i] = func;
            return;
        }
    }
    exit(2);
}

void
register_poll_hook(void (*func)())
{
    int i;

    for (i = 0; i < MAXDRIVERS; i++) {
        if (!poll_hook[i]) {
            poll_hook[i] = func;
            return;
        }
    }
    exit(2);
}

char patspace[100];

char *
bitdef(byte v, char**defs)
{
    int i;

    patspace[0] = 0;

    for (i = 0; i < 8; i++) {
        if ((v & (1 << i)) && defs[i]) {
            strcat(patspace, defs[i]);
            strcat(patspace, " ");
        }
    }
    return patspace;
}

void
usage(char *complaint, char *p)
{
    int i;

    fprintf(stderr, "%s", complaint);
    fprintf(stderr, "usage: %s [<options>] [program [<program options>]]\n",
        p);
    fprintf(stderr, "\t-d\t<drive file>\n");
    fprintf(stderr, "\t-d\t<symbol file>\n");
    fprintf(stderr, "\t-t\topen a debug terminal window\n");
    fprintf(stderr, "\t-b\t\tstart with breakpoint\n");
    fprintf(stderr, "\t-v <verbosity>\n");
    for (i = 0; vopts[i]; i++) {
        fprintf(stderr, "\t%x %s\n", 1 << i, vopts[i]);
    }
    exit(1);
}

char *boot_drive = "DRIVE_A.IMD";
char *symfile;
char *b_drive;

int
main(int argc, char **argv)
{
    char *progname = *argv++;
    char *s;
    char **argvec;
    int i;
    char *ptyname;
    char *bootrom;
    int fd;
    int drives = 0;

    argc--;

    while (argc) {
        s = *argv;

        /*
         * end of flagged options 
         */
        if (*s++ != '-')
            break;

        argv++;
        argc--;

        /*
         * s is the flagged arg string 
         */
        while (*s) {
            switch (*s++) {
            case 't':
                debug_terminal = 1;
                break;
            case 'd':
                if (!argc--) {
                    usage("drive file\n", progname);
                }
                if (drives) {
                    b_drive = strdup(*argv++);
                } else {
                    boot_drive = strdup(*argv++);
                    drives++;
                }
                break;
            case 's':
                if (!argc--) {
                    usage("symfile\n", progname);
                }
                symfile = strdup(*argv++);
                break;
            case 'v':
                if (!argc--) {
                    usage("verbosity not specified \n", progname);
                }
                verbose = strtol(*argv++, 0, 0);
                break;
            case 'b':
                breakpoint++;
                break;
            default:
                usage("unrecognized option", progname);
                break;
            }
        }
    }
    /*
     * get the boot rom image file name 
     */
    if (argc) {
        bootrom = *argv;
    } else {
        bootrom = "mon447.bin";
        if (!symfile)
            symfile = "mon447.sym";
    }

    /*
     * load the boot rom
     */
    fd = open(bootrom, O_RDONLY);
    if (!fd) {
        perror(bootrom);
        exit(errno);
    }
    i = read(fd, &rom_image, sizeof(rom_image));
    if (i < 0) {
        perror(bootrom);
        exit(errno);
    }
    close(fd);

    mypid = getpid();
    mytty = strdup(ttyname(0));

    /*
     * we might be piping the simulator.  let's get an open file for our debug 
     * output and monitor functions.  finally, let's make sure the file 
     * descriptor is out of range of the file descriptors our emulation uses.
     * this is so that we can debug interactive stuff that might be 
     * writing/reading from stdin, and we want all our debug output to go to a 
     * different terminal, one that isn't running a shell.  
     * also, if we specified to open a debug window, let's connect the 
     * emulator's file descriptors to an xterm or something.
     */
    if (debug_terminal) {
        char *cmd = malloc(100);
        int pipefd[2];

        pipe(pipefd);
        // we need to capture the tty name so we can send output to it
        sprintf(cmd,
            "tty > /proc/%d/fd/%d ; while test -d /proc/%d ; do sleep 1 ; done",
            mypid, pipefd[1], mypid);
        if (!fork()) {
            execlp("xterm", "xterm", "-geometry", "120x40", "-fn", "8x13", "-e", "bash", "-c", cmd, (char *) 0);
        }
        ptyname = malloc(100);
        i = read(pipefd[0], ptyname, 100);
        if (i == -1) {
            perror("pipe");
        }
        // build a filename, null terminated at the newline
        ptyname[i] = 0;
        for (i = 0; i < strlen(ptyname); i++) {
            if (ptyname[i] == '\n') {
                ptyname[i] = 0;
                break;
            }
        }
        stdout = freopen(ptyname, "r+", stdout);
        stdin = freopen(ptyname, "r+", stdin);
        if (!stdout) {
            fprintf(stderr, "freopen of %s as stdout failed %d\n", ptyname, errno);
            exit(0);
        }
        if (!stdin) {
            fprintf(stdin, "freopen of %s as stdin failed %d\n", ptyname, errno);
            exit(0);
        }
    }

    if (verbose) {
        printf("verbose %x ", verbose);
        for (i = 0; vopts[i]; i++) {
            if (verbose & (1 << i)) {
                printf("%s ", vopts[i]);
            }
        }
        printf("\n");
        printf("emulating %s\n", argv[0]);
    }

    signal(SIGUSR1, stop_handler);
    ioinit();
    register_output(EXIT_PORT, exit_port_handler);
    register_output(DUMP_PORT, dump_port_handler);
    taskreg = 0;
    setrom(0);

    // set diagnostic, monitor or boot mode
    switchreg = SW_DJDMA | SW_NOMON;
    keybreg = 0;

    /*
     * run the driver startup hooks
     */
    for (i = 0; i < MAXDRIVERS; i++) {
        if (startup_hook[i]) {
            if ((*startup_hook[i])()) {
                printf("startup hook %d failed\n", i);
                exit(1);
            }
        }
    }

    /* presumably, this is the reset address */
    // dumpmem(&get_byte, 0, 256);

    load_symfile(symfile);
    cp = &context;
    Z80Reset(&cp->state);

    // mpz80 sort of forces this at reset by unconditionally enabling
    // the eprom and the address bits to sequence for 16 bytes
    cp->state.pc = 0xbf0;

    emulate_z80();
    exit(0);
}

struct itimerval timer;

void
set_itv_usec(int v)
{
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = v;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = v;
}

void
set_alarm()
{
    setitimer(ITIMER_REAL, &timer, 0);
}

void
dumpcpu()
{
    unsigned char f;
    char outbuf[40];
    char fbuf[9];
    char *s;
    int i;

    strcpy(fbuf, "        ");

    format_instr(cp->state.pc, outbuf, &get_byte, &lookup_sym, &reloc);
    s = lookup_sym(cp->state.pc);
    if (s) {
        printf("%s\n", s);
    }
    printf("%04x: %-20s ", cp->state.pc, outbuf);

    f = cp->state.registers.byte[Z80_F];

    if (f & Z80_C_FLAG)
        fbuf[0] = 'C';
    if (f & Z80_N_FLAG)
        fbuf[1] = 'N';
    if (f & Z80_X_FLAG)
        fbuf[2] = 'X';
    if (f & Z80_H_FLAG)
        fbuf[3] = 'H';
    if (f & Z80_Y_FLAG)
        fbuf[4] = 'Y';
    if (f & Z80_Z_FLAG)
        fbuf[5] = 'Z';
    if (f & Z80_C_FLAG)
        fbuf[6] = 'S';

    printf(
        " %s a:%02x bc:%04x de:%04x hl:%04x ix:%04x iy:%04x sp:%04x tos:%04x\n",
        fbuf,
        cp->state.registers.byte[Z80_A],
        cp->state.registers.word[Z80_BC],
        cp->state.registers.word[Z80_DE],
        cp->state.registers.word[Z80_HL],
        cp->state.registers.word[Z80_IX],
        cp->state.registers.word[Z80_IY],
        cp->state.registers.word[Z80_SP],
        get_word(cp->state.registers.word[Z80_SP]));
}

/*
 * breakpoints and watchpoints are handled using the same data structure
 */
struct point
{
    unsigned short addr;
    int value;
    struct point *next;
};

struct point *breaks;
struct point *watches;

int
watchpoint_hit()
{
    struct point *p;
    int n;

    for (p = watches; p; p = p->next) {
        if (p->value == -1) {
            p->value = get_byte(p->addr);
        }
        n = get_byte(p->addr);
        if (n != p->value) {
            fprintf("value %02x at %04x changed to %02x\n",
                p->value, p->addr, n);
            p->value = n;
            return (1);
        }
    }
    return (0);
}

struct point *
point_at(struct point **head, unsigned short addr, struct point **pp)
{
    struct point *p;

    if (pp)
        *pp = 0;
    for (p = *head; p; p = p->next) {
        if (p->addr == addr) {
            break;
        }
        if (pp) {
            *pp = p;
        }
    }
    return p;
}

int lastaddr = -1;

/*
 * read a complete command from the terminal
 * this hides the line buffering stuff that might be happening, and iterates until we get
 * a newline
 */
void
read_commandline(char *s)
{
    char c;
    int i;

    while (1) {
        i = fread(&c, 1, 1, stdin);
        *s++ = c;
        if (c == '\n') {
            *s = 0;
            return; 
        }
    }
}

void
skipwhite(char **s)
{
    while (**s && **s == ' ') {
        (*s)++;
    }
}

int
getaddress(char **s)
{
    char wordbuf[20];
    char *wp;
    int i = -1;

    wp = wordbuf;
    while (**s && **s != ' ') {
        *wp++ = *(*s)++;
        *wp = 0;
    }
    if (wordbuf[0] == '%') {
        if (strcasecmp(&wordbuf[1], "hl") == 0) {
            return cp->state.registers.word[Z80_HL];
        }
        if (strcasecmp(&wordbuf[1], "bc") == 0) {
            return cp->state.registers.word[Z80_BC];
        }
        if (strcasecmp(&wordbuf[1], "de") == 0) {
            return cp->state.registers.word[Z80_DE];
        }
        if (strcasecmp(&wordbuf[1], "sp") == 0) {
            return cp->state.registers.word[Z80_SP];
        }
        if (strcasecmp(&wordbuf[1], "tos") == 0) {
            return get_word(cp->state.registers.word[Z80_SP]);
        }
    }
    if ((i = find_symbol(wordbuf)) == -1) {
        i = strtol(wordbuf, &wp, 16);
    }
    return i;
}

int
monitor()
{
    struct point *p, *prev, **head;
    char cmdline[100];
    char l;
    char c;
    int i;
    int delete;
    char *s;

    while (1) {
      more:
        printf("%d >>> ", mypid);
        
        read_commandline(cmdline);
        s = cmdline;
        // if there is anything there, null terminate it
        if (*s) {
            s[strlen(s) - 1] = 0;
        }
        skipwhite(&s);
        // get the command character
        c = *s++;
        // skip whitespace
        skipwhite(&s);
        head = &breaks;
        switch (c) {
        case 'c':                   // system call trace
            c = 1;
            skipwhite(&s);
            if (!*s) {
                for (i = 0; i < sizeof(stop); i++) {
                    if ((i % 16) == 0)
                        printf("\n%02d: ", i);
                    printf("%03d ", stop[i]);
                }
                printf("\n");
            }
            if (*s == '-') {
                s++;
                c = 0;
            }
            if (*s) {
                i = strtol(s, &s, 16);
            }
            if (i) {
                if (i < 0) {
                    i = -i;
                    c = 0;
                }
                if (i < sizeof(stop)) {
                    stop[i] = c;
                }
            }
            break;
        case 'm':                   // dump map
            skipwhite(&s);
            if (*s) {
                i = strtol(s, &s, 16);
            } else {
                i = taskreg & 0xf;
            }
            printmap(i);
            break;
        case 'd':                   // dump memory
            skipwhite(&s);
            if (*s) {
                i = getaddress(&s);
            } else {
                if (lastaddr == -1) {
                    i = cp->state.registers.word[Z80_SP];
                } else {
                    i = lastaddr;
                }
            }
            dumpmem(&get_byte, i, 256);
            lastaddr = (i + 256) & 0xfff;
            break;
        case 'l':                   // list
            skipwhite(&s);
            if (*s) {
                i = getaddress(&s);
            } else {
                if (lastaddr == -1) {
                    i = cp->state.pc;
                } else {
                    i = lastaddr;
                }
            }
            for (l = 0; l < LISTLINES; l++) {
                c = format_instr(i, cmdline, &get_byte, &lookup_sym, &reloc);
                s = lookup_sym(i);
                if (s) {
                    printf("%s\n", s);
                }
                printf("%04x: %-20s\n", i, cmdline);
                i += c;
                lastaddr = i & 0xffff;
            }
            break;
        case 'r':               // dump registers
            dumpcpu();
            break;
        case 's':
            return (1);         // single step
        case 'g':
            return (0);
        case 'x':               // exit
        case 'q':
            exit(1);
            return (0);
        // watchpoint and breakpoint
        case 'w':              /* w [-] <addr> <addr> ... */
            head = &watches;
        case 'b':              /* b [-] <addr> <addr> ... */
            delete = 0;
            i = -1;
            if (*s == '-') {
                s++;
                delete = 1;
            }
            while (*s) {
                skipwhite(&s);
                i = getaddress(&s);
                p = point_at(head, i, &prev);
                if (p && delete) {
                    if (prev) {
                        prev->next = p->next;
                    } else {
                        *head = p->next;
                    }
                    free(p);
                } else if ((!p) && (!delete)) {
                    p = malloc(sizeof(*p));
                    p->addr = i;
                    p->next = *head;
                    *head = p;
                }
                while (*s && (*s == ' '))
                    s++;
            }
            if (i == -1) {
                if (delete) {
                    while ((p = *head)) {
                        *head = p->next;
                        free(p);
                    }
                } else {
                    for (p = *head; p; p = p->next) {
                        s = lookup_sym(p->addr);
                        if (s) {
                            printf("%04x %s\n", p->addr, s);
                        } else {
                            printf("%04x\n", p->addr);
                        }
                    }
                }
            }
            break;
        case '?':
        case 'h':
            printf("commands:\n");
            printf("l <addr> :list\n");
            printf("d <addr> :dump memory\n");
            printf("r dump cpu state\n");
            printf("m [taskid] : dump mapping\n");
            printf("g: continue\n");
            printf("s: single step\n");
            printf("x: exit\n");
            printf("q: exit\n");
            printf("b [-] <nnnn> ... :breakpoint\n");
            printf("w [-] <nnnn> ... :watchpoint\n");
            printf("c [-] <nn> :system call trace\n");
            break;
        default:
            printf("unknown command %c\n", c);
            break;
        case 0:
            break;
        }
    }
}

/*
 * this is the cpu emulator for the MPZ80
 */
static void
emulate_z80()
{
    unsigned char *ip;
    int i;

    do {

        /*
         * run the driver poll hooks
         */
        for (i = 0; i < MAXDRIVERS; i++) {
            if (poll_hook[i]) {
                (*poll_hook[i])();
            } else {
                break;
            }
        }

        if (watchpoint_hit()) {
            breakpoint = 1;
        }
        if (point_at(&breaks, cp->state.pc, 0)) {
            if (point_at(&breaks, cp->state.pc, 0)) {
                pid();
                printf("break at %04x\n", cp->state.pc);
            }
            breakpoint = 1;
        }
        if (breakpoint) {
            dumpcpu();
            breakpoint = monitor();
        }
        if (verbose & V_INST) {
            pid();
            dumpcpu();
        }
        /*
         * the second arg is the number of cycles we are allowing 
         * the emulator to run
         */
        Z80Emulate(&cp->state, 1, &context);
    } while (!cp->is_done);
}

/*
 * formatted memory dumper subroutines
 */
unsigned char pchars[16];
int pcol;

dp()
{
    int i;
    char c;

    for (i = 0; i < pcol; i++) {
        c = pchars[i];
        if ((c <= 0x20) || (c >= 0x7f))
            c = '.';
        printf("%c", c);
        if ((i % 4) == 3) { printf(" "); }
    }
    printf("\n");
}

void
dumpmem(unsigned char (*readbyte) (int addr), int addr, int len)
{
    int i;

    pcol = 0;

    while (len) {
        if (pcol == 0)
            printf("%04x: ", addr);
        printf("%02x ", pchars[pcol] = (*readbyte) (addr++));
        if ((pcol % 4) == 3) { printf(" "); }
        len--;
        if (pcol++ == 15) {
            dp();
            pcol = 0;
        }
    }
    if (pcol != 0) {
        for (i = pcol; i < 16; i++)
            printf("   ");
        dp();
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
