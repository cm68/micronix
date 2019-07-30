
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

#include "sim.h"

typedef unsigned int ULONG;
typedef unsigned char UCHAR;
typedef unsigned short UINT;

#define	LISTLINES	8
#define	STACKTOP	0xffff
#define MAXIMUM_STRING_LENGTH   100

extern void dumpmem(unsigned char (*get) (int a), int addr, int len);
static void emulate();

int debug_terminal;
int mypid;
FILE *mytty;

#define	V_IO	(1 << 0)        /* trace I/O instructions */
#define	V_INST	(1 << 1)        /* instructions */

char *vopts[] = {
    "V_IO", "V_INST", 0
};

int verbose;

MACHINE context;

/*
 * mpz80 cpu registers and local ram
 */
char local_ram[1024];
char regs[1024];
char eprom[1024];
char fpp[1024];

/*
 * mon4.47 and mon 3.75 both were distributed on 2732A's, which are 4kb
 * devices. the rom image file contains 2 copies of this source, assembled
 * org'd at 0x800.
 * the monitor has 2 1k sections, both of which are assembled for 0x800-0xbff
 * the lowest is only enabled after a reset until the trap register is written
 * which then enables the high half.  kind of a waste of half the chip, but
 * I think the 2732A's were faster than the 2716's the board originally was
 * designed for, which is why they did the upgrade.  it may be better to
 * go back to the 2kb part, since AT28C16's are gettable in 120ns.
 */
char rom_image[4096];

#define FPSEG   0x400
#define FPCOL   0x401
#define TASK    0x402
#define MASK    0x403
#define TRAP    0x400
#define KEYB    0x401
#define SWT     0x402
#define STAT    0x403

/*
 * whenever a trap happens, this address is forced onto the address bus
 */
#define	TRAPADDR	0xBF0

volatile int breakpoint;

struct MACHINE *cp;

int stop[10];

void
stop_handler()
{
    fprintf(mytty, "breakpoint signal\n");
    breakpoint = 1;
}

void
pid()
{
    fprintf(mytty, "%d: ", mypid);
}

char *
get_symname(unsigned short addr)
{
    return 0;
}

unsigned short
reloc(unsigned short addr)
{
    return 0;
}

/*
 * task 0 has the low 4k mapped to MPZ80 control and status registers
 */
void
supervisor_read(unsigned short addr)
{
}

void
supervisor_write(unsigned short addr, unsigned char value)
{
}

/*
 * access memory through the memory mapping ram, with permissions checking
 */
void
task_read(unsigned short addr)
{
}

void
task_write(unsigned short addr, unsigned char value)
{
}

void (*memwrite) (unsigned short addr, unsigned char value);
unsigned char (*memread) (unsigned short addr);

/*
 * used by the emulator to do all translated memory operations
 * this code could break the emulation flow, depending on permissions
 * this is where all the memory mapping of onboard IO for task 0, etc
 * is abstracted out. 
 */
void
put_word(unsigned short addr, unsigned short value)
{
    (*memwrite) (addr, value);
    (*memwrite) (addr + 1, value >> 8);
}

void
put_byte(unsigned short addr, unsigned char value)
{
    (*memwrite) (addr, value);
}

unsigned short
get_word(unsigned short addr)
{
    return ((*memread) (addr) + ((*memread) (addr + 1) << 8));
}

unsigned char
get_byte(unsigned short addr)
{
    return (*memread) (addr);
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
register_input(portaddr portnum, inhandler func)
{
    input_handler[portnum] = func;
}

void
register_output(portaddr portnum, outhandler func)
{
    output_handler[portnum] = func;
}

portdata
undef_in(portaddr p)
{
    return 0xff;
}

void
undef_out(portaddr p, portdata v)
{
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
output(portaddr p, portdata v)
{
    (*output_handler) (p, v);
}

portdata
input(portaddr p)
{
    return (*input_handler) (p);
}

#define MAXDRIVERS 4
int (*startup_hook[MAXDRIVERS])();

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
usage(char *complaint, char *p)
{
    int i;

    fprintf(stderr, "%s", complaint);
    fprintf(stderr, "usage: %s [<options>] [program [<program options>]]\n",
        p);
    fprintf(stderr, "\t-t\topen a debug terminal window\n");
    fprintf(stderr, "\t-b\t\tstart with breakpoint\n");
    fprintf(stderr, "\t-v <verbosity>\n");
    for (i = 0; vopts[i]; i++) {
        fprintf(stderr, "\t%x %s\n", 1 << i, vopts[i]);
    }
    exit(1);
}

int
main(int argc, char **argv)
{
    char *progname = *argv++;
    char *s;
    char **argvec;
    int i;
    char *ttyname;
    char *bootrom;
    int fd;

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
        bootrom = "MPZ80-Mon-3.73_FB34.bin";
        bootrom = "MPZ80-Mon-3.75_0706.bin";
        bootrom = "MPZ80-Mon-4.47_F4F6.bin";
    }

    mypid = getpid();

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
        sprintf(cmd,
            "tty > /proc/%d/fd/%d ; while test -d /proc/%d ; do sleep 1 ; done",
            mypid, pipefd[1], mypid);
        if (fork()) {
            ttyname = malloc(100);
            i = read(pipefd[0], ttyname, 100);
            if (i == -1) {
                perror("pipe");
            }
            ttyname[strlen(ttyname) - 1] = 0;
        } else {
            execlp("xterm", "xterm", "-e", "bash", "-c", cmd, (char *) 0);
        }
    } else {
        ttyname = "/dev/tty";
    }
    mytty = fopen(ttyname, "r+");
    if (!mytty) {
        perror(ttyname);
        exit(errno);
    }
    dup2(fileno(mytty), TTY_FD);
    mytty = fdopen(TTY_FD, "r+");
    setvbuf(mytty, 0, _IOLBF, 0);
    signal(SIGUSR1, stop_handler);

    if (verbose) {
        fprintf(mytty, "verbose %x ", verbose);
        for (i = 0; vopts[i]; i++) {
            if (verbose & (1 << i)) {
                fprintf(mytty, "%s ", vopts[i]);
            }
        }
        fprintf(mytty, "\n");
        fprintf(mytty, "emulating %s\n", argv[0]);
    }

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

    for (i = 0; i < 256; i++) {
        if (input_handler[i]) {
            printf("input port %x registered\n", i);
        }
        if (output_handler[i]) {
            printf("output port %x registered\n", i);
        }
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
    cp = &context;
    Z80Reset(&cp->state);
    cp->state.pc = 0;

    emulate();
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

char *
lookup_sym(int i)
{
    return 0;
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
        fprintf(mytty, "%s\n", s);
    }
    fprintf(mytty, "%04x: %-20s ", cp->state.pc, outbuf);

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

    fprintf(mytty,
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
        fprintf(mytty, "%d >>> ", mypid);
        s = fgets(cmdline, sizeof(cmdline), mytty);
        if (*s) {
            s[strlen(s) - 1] = 0;
        }
        c = *s++;
        while (*s && (*s == ' '))
            s++;
        head = &breaks;
        switch (c) {
        case 'c':
            c = 1;
            while (*s && (*s == ' '))
                s++;
            if (!*s) {
                for (i = 0; i < sizeof(stop); i++) {
                    if ((i % 16) == 0)
                        fprintf(mytty, "\n%02d: ", i);
                    fprintf(mytty, "%03d ", stop[i]);
                }
                fprintf(mytty, "\n");
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
        case 'd':
            while (*s && (*s == ' '))
                s++;
            if (*s) {
                i = strtol(s, &s, 16);
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
        case 'l':
            while (*s && (*s == ' '))
                s++;
            if (*s) {
                i = strtol(s, &s, 16);
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
                    fprintf(mytty, "%s\n", s);
                }
                fprintf(mytty, "%04x: %-20s\n", i, cmdline);
                i += c;
                lastaddr = i & 0xfff;
            }
            break;
        case 'r':
            dumpcpu();
            break;
        case 's':
            dumpcpu();
            return (1);
        case 'g':
            return (0);
        case 'q':
            exit(1);
            return (0);
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
                i = strtol(s, &s, 16);
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
                        fprintf(mytty, "%04x\n", p->addr);
                    }
                }
            }
            break;
        case '?':
        case 'h':
            fprintf(mytty, "commands:\n");
            fprintf(mytty, "l <addr> :list\n");
            fprintf(mytty, "d <addr> :dump memory\n");
            fprintf(mytty, "r dump cpu state\n");
            fprintf(mytty, "g: continue\n");
            fprintf(mytty, "s: single step\n");
            fprintf(mytty, "q: exit\n");
            fprintf(mytty, "b [-] <nnnn> ... :breakpoint\n");
            fprintf(mytty, "w [-] <nnnn> ... :watchpoint\n");
            fprintf(mytty, "c [-] <nn> :system call trace\n");
            break;
        default:
            fprintf(mytty, "unknown command %c\n", c);
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
emulate()
{
    unsigned char *ip;
    int i;

    do {
        if (watchpoint_hit()) {
            breakpoint = 1;
        }
        if (point_at(&breaks, cp->state.pc, 0)) {
            if (point_at(&breaks, cp->state.pc, 0)) {
                pid();
                fprintf(mytty, "break at %04x\n", cp->state.pc);
            }
            breakpoint = 1;
        }
        if (breakpoint) {
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
        fprintf(mytty, "%c", c);
    }
    fprintf(mytty, "\n");
}

void
dumpmem(unsigned char (*readbyte) (int addr), int addr, int len)
{
    int i;

    pcol = 0;

    while (len) {
        if (pcol == 0)
            fprintf(mytty, "%04x: ", addr);
        fprintf(mytty, "%02x ", pchars[pcol] = (*readbyte) (addr++));
        len--;
        if (pcol++ == 15) {
            dp();
            pcol = 0;
        }
    }
    if (pcol != 0) {
        for (i = pcol; i < 16; i++)
            fprintf(mytty, "   ");
        dp();
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
