/* 
 * hwsim/sim.c
 *
 * Changed: <2023-06-23 14:23:42 curt>
 *
 * this is the general emulator framework
 *
 * includes main, the debugger and i/o hooks
 * debug terminal and logging
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
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <unistd.h>

#include <setjmp.h>

#include "sim.h"
#include "z80glue.h"
#include "hwsim.h"
#include "gui.h"
#include "util.h"
#include "../micronix/include/obj.h"
#include "imd.h"
#include "disz80.h"
#include "mnix.h"
#include "../micronix/include/types.h"
#include "../micronix/include/sys/fs.h"
#include <fslib.h>
#include "../micronix/include/obj.h"

#define S_FLAG  0x80
#define Z_FLAG  0x40
#define Y_FLAG  0x20
#define H_FLAG  0x10
#define X_FLAG  0x08
#define PV_FLAG 0x04
#define N_FLAG  0x02
#define C_FLAG  0x01

#define DUMP_PORT   1   // output to here makes a memory dump with registers
#define INPUT_PORT  2   // this is for patching into pip for import
#define OUTPUT_PORT 2   // this is for patching into pip for export

#define	LISTLINES	8
#define	STACKTOP	0xffff
#define MAXIMUM_STRING_LENGTH   100

#define LOGFILE     "logfile"

#ifndef NODEBUG
int program_counter;

int debug_terminal;
int log_output;
int debug_suppress;      /* -q: gate the debug terminal output on debugger entry */
int debug_logfd = -1;    /* -D: named debug log file */
int debug_gate_fd = -1;  /* write end of the gate control pipe, set by route_debug */
#endif
int mypid;
int running;
int listing;

#ifndef NODEBUG
int traceflags;

int trace_inst;
int trace_bio;
int trace_io;
int trace_symbols;
int trace_timer;
#endif

#ifndef NODEBUG
/*
 * A trace trigger, like the one on a logic analyzer: run quietly until
 * the machine reaches a place, then start recording.  Held as the string
 * dis_space prints for that place, so arming it needs no knowledge of
 * how a space is spelled and testing it is a compare.
 */
char tracetrig[16];
#endif

/*
 * Floppies on the 5 1/4 inch port.  The positional arguments fill the
 * 8 inch port, because that is what every disk in this tree is; -5 puts
 * one on the other port.  djdma.4 numbers them the same way: minor 0-3
 * are the 8 inch drives and 4-7 the 5 1/4 inch ones.
 */
char **fivenames;

#ifndef NODEBUG
/*
 * How many instructions to record once it fires.  Zero means until the
 * machine stops, which is what you want when you do not yet know how far
 * the interesting part runs.  Capturing BEFORE the trigger would answer
 * "how did I get here" and costs a ring buffer to do; this only answers
 * "what happens next", which is a countdown.
 */
long tracelen;

struct {
    char *name;
    int *valuep;
} def_traces[] = {
    {"inst", &trace_inst },
    {"bio", &trace_bio },
    {"io", &trace_io },
    {"symbols", &trace_symbols },
    {"timer", &trace_timer },
    { 0, 0 }
} ;

volatile int inst_countdown = -1;

int stops[10];

/*
 * Stop at the next instruction boundary, the way a watchpoint does.
 * This is the version for code inside the emulation loop; a signal
 * cannot use it (see stop_handler).
 */
void
stop()
{
    inst_countdown = 0;
}

/*
 * SIGUSR1: kick the running machine into the debugger.
 *
 * The handler cannot plant the stop in inst_countdown.  The emulation
 * loop counts that down at the bottom, and a signal blocked through
 * z80_run() is delivered by mysigunblock() just above the countdown -
 * so the zero the handler wrote became -1 before the top of the loop
 * tested it, and kill -USR1 looked like it did nothing.  stop_request is
 * read at the top of the loop, where nothing decrements it.
 *
 * The handler only sets the latch.  It runs in the middle of the
 * emulation, so it does not print and does not touch the monitor; the
 * loop does both when it sees the flag.
 */
volatile sig_atomic_t stop_request;

void
stop_handler()
{
    stop_request = 1;
}
#endif

#ifndef NODEBUG
/*
 * The disassembler used to be handed its callbacks - format_instr took
 * &get_byte, &lookup_sym, &reloc and &mnix_sc - and now calls these two
 * by name instead (disz80.h).  Nothing here relocates, so this is the
 * identity answer rather than a stub with a different meaning.
 */
unsigned int
get_reloc(unsigned short addr)
{
    return 0;
}

/*
 * The monitor's v command drives this, and pverbose reports it.  hwsim
 * came with the same idea under another name: -t sets traceflags at
 * startup out of the same tracenames table.  They are kept as one value
 * so that setting either one is visible to the other, rather than the
 * command line and the monitor disagreeing about what is being traced.
 */
int verbose;

void
pverbose()
{
    int i;

    traceflags = verbose;
    message("verbose %x ", verbose);
    for (i = 0; tracenames[i]; i++) {
        if (verbose & (1 << i)) {
            message("%s ", tracenames[i]);
        }
    }
    message("\n");
}
#endif

#ifndef NODEBUG
struct sym {
    char *name;
    vaddr value;
    struct sym *next;
} *syms[2];

void
add_sym(int task, char *name, vaddr v)
{
    struct sym *s;
    s = malloc(sizeof(*s));
    s->next = syms[task & 1];
    syms[task & 1] = s;
    s->name = strdup(name);
    s->value = v;
}

int
find_symbol(char *ls)
{
    struct sym *s = syms[0];

    while (s) {
        if (strcasecmp(s->name, ls) == 0) {
            return (s->value);
        }
        s = s->next;
    }
    return -1;
}

char *
lookup_sym(unsigned int symaddr)
{
    struct sym *s = syms[super() ? 0 : 1];
    unsigned short addr = symaddr & 0xffff;

    while (s) {
        if (s->value == addr) {
            return (s->name);
        }
        s = s->next;
    }
    return 0;
}

/*
 * The shared monitor and gui ask for a symbol by address through this
 * name; usersim answers it out of the symbol table of the process it is
 * running.  Here there is no one process to ask about - the MPZ80 task
 * register selects an address space, and every task has its own idea of
 * what lives at 0x1000.
 *
 * So for now this answers only for supervisor space, which is what
 * lookup_sym already does: super() is (taskreg & 0xf) == 0, so a symbol
 * comes back only while the machine is in task 0.  Symbols in user space
 * read as unknown rather than as the kernel's symbol of the same address,
 * which is the answer that would actively mislead.
 *
 * The real fix is to key symbol tables by task register and load one per
 * address space - kernel plus whatever user program is being debugged -
 * at which point this becomes a lookup in the table taskreg selects.
 */
char *
get_symname(unsigned short addr)
{
    return lookup_sym(addr);
}

void
load_symfile(char *s)
{
    FILE *sf;
    int v;
    char namebuf[20];
    char kbuf[20];
    char linebuf[100];
    int i = 0;

    sf = fopen(s, "r");
    if (!sf) return;
    while (1) {
        if (fgets(linebuf, sizeof(linebuf), sf) == 0) {
            break;
        }
        if (linebuf[0] == '#') {
            continue;
        }
        if (sscanf(linebuf, "%s %s 0x%x", kbuf, namebuf, &v) != 3) {
            if (sscanf(linebuf, "%x %s", &v, namebuf) != 2) {
                continue;
            }
        }
        add_sym(0, namebuf, v);
        i++;
    }
    printf("added %d symbols from %s\n", i, s);
    fclose(sf);
}

/*
 * Load the kernel's symbols straight out of the object file, so -S takes
 * the freshly linked unix binary rather than a separately generated .sym
 * file.  The object header gives the text and data sizes; the symbol
 * table follows them, and each entry is value(2) + flag(1) + name.
 */
void
load_kernel_syms(char *s)
{
    FILE *kf;
    struct obj hdr;
    unsigned char *tab;
    int symlen, n, off;
    char namebuf[16];

    kf = fopen(s, "r");
    if (!kf) return;
    if (fread(&hdr, sizeof(hdr), 1, kf) != 1 || hdr.ident != OBJECT) {
        fclose(kf);
        load_symfile(s);        /* not an object: a text .sym file */
        return;
    }
    symlen = (hdr.conf & 0x07) * 2 + 1;    /* 9 or 15 characters */
    tab = malloc(hdr.table ? hdr.table : 1);
    if (!tab) {
        fclose(kf);
        return;
    }
    fseek(kf, 16 + hdr.text + hdr.data, SEEK_SET);
    n = fread(tab, 1, hdr.table, kf);
    fclose(kf);

    off = 0;
    while (off + 3 <= n) {
        vaddr value = tab[off] | (tab[off + 1] << 8);
        unsigned char flag = tab[off + 2];
        int len = symlen;
        if (off + 3 + len > n)
            len = n - off - 3;
        if ((flag & SF_GLOBAL) && (flag & SF_DEF)) {
            memcpy(namebuf, (char *)&tab[off + 3], len);
            namebuf[len] = 0;
            add_sym(0, namebuf, value);
        }
        off += 3 + symlen;
    }
    free(tab);
    printf("added kernel symbols from %s\n", s);
}
#endif

/*
 * The root filesystem image, named by the disk controller as it opens
 * its backing store.  image_for_dev() returns it for any device for
 * now - the message carries the device number, but the simulator does
 * not yet keep a device -> image table.
 */
#ifndef NODEBUG
static char *ctl_image;

void
set_ctl_image(char *path)
{
    ctl_image = strdup(path);
}

static char *
image_for_dev(int dev)
{
    return ctl_image;
}

/*
 * Read the symbol table of inode `inum` from the filesystem image for
 * `dev`, and add its symbols to `task`'s table.  A stripped object has
 * a zero table size and is skipped.
 */
void
load_task_syms(int task, int dev, int inum)
{
    struct super *fs;
    struct dsknod *dp;
    struct obj hdr;
    char *img;
    char blk[512];
    unsigned char *tab;
    int symoff, symlen, n, i, off, got;
    char namebuf[16];

    img = image_for_dev(dev);
    if (!img || openfs(img, &fs) < 0)
        return;
    dp = iget(fs, inum);
    if (!dp) {
        closefs(fs);
        return;
    }
    if (fileread(dp, 0, blk) < (int)sizeof(hdr)) {
        ifree(dp);
        closefs(fs);
        return;
    }
    memcpy(&hdr, blk, sizeof(hdr));
    if (hdr.ident != OBJECT || hdr.table == 0) {
        /* not an object, or stripped */
        ifree(dp);
        closefs(fs);
        return;
    }
    symlen = (hdr.conf & CONF_SYMASK) * 2 + 1;
    n = hdr.table / (symlen + 3);
    symoff = sizeof(hdr) + hdr.text + hdr.data;
    tab = malloc(hdr.table);
    if (!tab) {
        ifree(dp);
        closefs(fs);
        return;
    }
    off = symoff;
    got = 0;
    while (got < hdr.table) {
        int within = off & 511;
        int chunk = 512 - within;
        fileread(dp, off, blk);
        if (chunk > hdr.table - got)
            chunk = hdr.table - got;
        memcpy(tab + got, blk + within, chunk);
        got += chunk;
        off += chunk;
    }
    for (i = 0; i < n; i++) {
        unsigned char *p = tab + i * (symlen + 3);
        unsigned short value = p[0] | (p[1] << 8);
        memcpy(namebuf, p + 3, symlen);
        namebuf[symlen] = 0;
        add_sym(task, namebuf, value);
    }
    free(tab);
    ifree(dp);
    closefs(fs);
}
#endif

/*
 * The kernel's control port.  exec() writes five bytes here - task
 * byte, device major, device minor, inode number low then high - and
 * the fifth byte triggers the symbol-table load for that task.  The
 * task byte is the Z280 task register the new program runs in (1 for
 * user code); the device selects which filesystem image holds the
 * inode.
 */
#ifndef NODEBUG
static int ctrl_n;              /* bytes seen so far */
static int ctrl_task, ctrl_dev, ctrl_inum;
#endif

void
ctrl_out(portaddr port, byte val)
{
#ifndef NODEBUG
    switch (ctrl_n++) {
    case 0:
        ctrl_task = val;
        return;
    case 1:
        ctrl_dev = val << 8;    /* major */
        return;
    case 2:
        ctrl_dev |= val;        /* minor */
        return;
    case 3:
        ctrl_inum = val;        /* inode low */
        return;
    default:
        ctrl_inum |= val << 8;  /* inode high */
        ctrl_n = 0;
        load_task_syms(ctrl_task, ctrl_dev, ctrl_inum);
        return;
    }
#else
    /* the kernel writes these bytes on every exec; there is no symbol
     * table to load in a hardware build, so discard them */
    (void)port;
    (void)val;
#endif
}

#ifndef NODEBUG
/* The exec'd program's name, written by the kernel to port 0xd1 on each
 * exec so the tracer can label user-space addresses with "make" instead
 * of "tsk1".  Indexed by the full task register - task 0 is the kernel,
 * tasks 1-15 are user processes.  The message is a task byte followed
 * by the NUL-terminated name. */
char prog_name[16][32];
static int name_task;
static int name_n;             /* 0 = expecting task byte; else 1 + length */
#endif

void
name_out(portaddr port, byte val)
{
#ifndef NODEBUG
    if (name_n == 0) {
        name_task = val & 0xf;              /* first byte: the task id */
        name_n = 1;
        return;
    }
    if (val == 0) {
        if (name_n <= (int)sizeof(prog_name[name_task]))
            prog_name[name_task][name_n - 1] = 0;
        name_n = 0;
        return;
    }
    if (name_n < (int)sizeof(prog_name[name_task]))
        prog_name[name_task][name_n - 1] = val;
    name_n++;
#else
    (void)port;
    (void)val;
#endif
}

#define dumpreg8(rn) r = z80_get_reg8(rn) ; write(fd, &r, 1)
#define dumpreg16(rn) rr = z80_get_reg16(rn) ; write(fd, &rr, 2)

#define ALLMEM  64*1024
void
dump_port_handler(portaddr p, byte v)
{
    int fd;
    int i;
    byte r;
    word rr;
    char *dumpbuf = malloc(ALLMEM);

    printf("dump port tickled %x\n", v);
    fd = creat("dumpfile", 0777);
    for (i = 0; i < ALLMEM; i++) {
        dumpbuf[i] = get_word(i);
    }
    write(fd, dumpbuf, ALLMEM);
    dumpreg16(pc_reg);
    dumpreg16(sp_reg);
    dumpreg16(bc_reg);
    dumpreg16(de_reg);
    dumpreg16(hl_reg);
    dumpreg16(ix_reg);
    dumpreg16(iy_reg);
    dumpreg8(a_reg);
    dumpreg8(f_reg);
    dumpreg8(i_reg);
    dumpreg8(r_reg);
    dumpreg8(iff_reg);
    dumpreg8(control_reg);
    dumpreg8(status_reg);
    close(fd);
    free(dumpbuf);
}

/*
 * to use the following output ports with pip,  patch it using ddt
 * 0103 jmp 10a
 * 0106 jmp 110
 * 0109 nop
 * 010a in 2
 * 010c sta 109
 * 010f ret
 * 0110 mov a,c
 * 0111 out 2
 * 0113 ret
 * 
/*
 * pip from INP: calls to 0x103 to get a bype of data into 0x109
 */
static int inp_fd = -1;
static byte 
pip_input_handler(portaddr p)
{
    byte buf = 0x1a;

    if (inp_fd == -1) {
        inp_fd = open("file.inp", O_RDONLY);
    }
    if (inp_fd >= 0) {
        read(inp_fd, &buf, 1);
    }
    if (buf == 0x1a) {
        if (inp_fd >= 0) {
            close(inp_fd);
        }
        inp_fd = -1;
    }
    return buf;
}

/*
 * pip to OUT: calls to 0x106 with character in C
 */
static int out_fd = -1;

static void
pip_output_handler(portaddr p, byte v)
{
    if (out_fd == -1) {
        out_fd = creat("file.out", 0777);
    }
    if (out_fd >= 0) {
        write(out_fd, &v, 1);
        if (v == 0x1a) {
            close(out_fd);
            out_fd = -1;
        }
    }
}

void
setup_sim_ports()
{
    register_output(DUMP_PORT, dump_port_handler);
    register_input(INPUT_PORT, pip_input_handler);
    register_output(OUTPUT_PORT, pip_output_handler);
}

/*
 * these driver hooks are called at various times to abstract the emulator
 * the registration functions must be called before main() by constructor magic
 */
#define MAXDRIVERS 8

struct driver *drivers[MAXDRIVERS];
/*
void (*poll_hook[MAXDRIVERS])();        // this gets called between instructions
int (*prearg_hook[MAXDRIVERS])();       // called just before arg processing
int (*startup_hook[MAXDRIVERS])();      // called just before emulation
void (*usage_hook[MAXDRIVERS])();       // called inside usage()
*/
int ndrivers;

void
register_driver(struct driver *d)
{
    drivers[ndrivers++] = d;
}

void
usage(char *complaint, char *p)
{
    int i;

    if (complaint[0]) {
        fprintf(stderr, "%s", complaint);
        if (complaint[strlen(complaint) - 1] != '\n')
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "usage: %s [<options>] [<drive> ...]\n", p);
    fprintf(stderr, "  a drive is <controller><unit>:<file> - djdma0:boot.IMD,\n");
    fprintf(stderr, "  hdcdma0:hddma-0, hdca1:/tmp/scratch - or a bare file,\n");
    fprintf(stderr, "  which is the next floppy.  controllers are the -B names.\n");
    fprintf(stderr, "\t-h\thelp\n");
    fprintf(stderr, "\t-b\t<boot rom file>\n");
    fprintf(stderr, "\t-B\t<djdma|hdcdma|hdca> boot from this, and skip the monitor\n");
    fprintf(stderr, "\t-c\t<configuration switch value>\n");
    fprintf(stderr, "\t-d\t<directory holding the hard drive unit files>\n");
    fprintf(stderr, "\t-m\t<bytes> ram size - 768k, 0xc0000, 1m (default 16m)\n");
    fprintf(stderr, "\t-5\t<file> a floppy on the 5 1/4 inch port\n");
    fprintf(stderr, "\t-H\tdon't exit the simulation on a task-0 halt\n");
    fprintf(stderr, "\t-L\t<file> tee the console (uart0) output to this file\n");
#ifndef NODEBUG
    fprintf(stderr, "\t-F\trun the fast build (d1p) instead\n");
    fprintf(stderr, "\t-S\t<kernel binary>\n");
    fprintf(stderr, "\t-T\t<space:addr>[,count] trace from here, for count instructions\n");
    fprintf(stderr, "\t-W\t<addr>[-<addr>] report writes to this range and keep going\n");
    fprintf(stderr, "\t-P\t<physaddr>[-<physaddr>] report writes to this physical range\n");
    fprintf(stderr, "\t-x\topen a debug terminal window\n");
    fprintf(stderr, "\t-t\t<tracebits>, or names: -t syscall,trap,all\n");
    fprintf(stderr, "\t-l\tproduce logfile\n");
    fprintf(stderr, "\t-n\tno console log: trace to the logfile, not the terminal\n");
    fprintf(stderr, "\t-D\t<file> tee the debug/trace stream to this file\n");
    fprintf(stderr, "\t-q\tsuppress the debug stream until the debugger is entered\n");
    fprintf(stderr, "\t\t(kill -USR1 stops the machine in the debugger)\n");
    for (i = 0; tracenames[i]; i++) {
        fprintf(stderr, "\t%x %s\n", 1 << i, tracenames[i]);
    }
#endif
    for (i = 0; i < ndrivers; i++) {
        if (drivers[i]->usage_hook) {
            (*drivers[i]->usage_hook)();
        }
    }
    exit(1);
}

/*
 * Parse a size with an optional k/m suffix, base 0 so 0x hex works too.
 */
static paddr
parsesize(char *s)
{
    paddr n = strtol(s, &s, 0);

    if (*s == 'k' || *s == 'K')
        n <<= 10;
    else if (*s == 'm' || *s == 'M')
        n <<= 20;
    return n;
}

char **drivenames;
int anydrive;                   /* a hdcdma/hdca unit was named on the command line */
char *rom_filename;
char *sym_filename;
char *kern_filename;
char *rom_image;
int rom_size;
int config_sw = 0;
int halt_exit = 1;		/* exit the simulation on a task-0 halt */

sigset_t mysignalmask;

void
mysigblock()
{
    sigprocmask(SIG_BLOCK, &mysignalmask, 0);
}

void
mysigunblock()
{
    sigprocmask(SIG_UNBLOCK, &mysignalmask, 0);
}

/*
 * linux signal() is too flaky to even comtemplate.
 */
sighandler_t
mysignal(int signum, sighandler_t handler)
{
    struct sigaction new;
    struct sigaction old;

    sigaddset(&mysignalmask, signum);

    new.sa_handler = handler;
    new.sa_flags = SA_RESTART;
    sigemptyset(&new.sa_mask);

    sigaction(signum, &new, &old);
    return (old.sa_handler);
}

/*
 * various things in the simulator will want to have timers popping and getting
 * callouts.  linux has a create_timer facility for this, which is hugely
 * complicated and non-portable.  screw that.  setitimer/sigalarm it is.
 * the recurring timeouts are handled in exactly the same way. when
 * the old one pops, we schedule the next.
 */
/*
 * Timeouts run on simulated time - z80 cycles - and not on the host's
 * clock.
 *
 * They used to be setitimer and SIGALRM, which meant a disk completion
 * or a clock tick landed wherever the host happened to be when the
 * signal arrived.  Two runs of the same disk then did different things:
 * one wedged after stat("/dev/ttyA"), one corrupted a name it was
 * building, one got as far as /etc/passwd and dropped into the monitor.
 * All three are the same race, and none of them could be reproduced on
 * purpose, which makes them unfindable - you cannot bisect a fault that
 * moves when you look at it.
 *
 * Counting cycles gives the same machine every time.  It is also closer
 * to the hardware: a controller that says it will interrupt in thirty
 * milliseconds means thirty milliseconds of processor time, not thirty
 * milliseconds of whatever else the host was doing.
 */
struct timeout {
    char *name;
    unsigned long long when;        // in cycles
    unsigned long long interval;    // if recurring, else 0
    void (*handler)(int a);
    int arg;                        // argument to pass handler
};


#define MILLION 1000000
#define MAXTIMEOUTS 10

/*
 * What the simulated processor runs at.  The Decision 1 is a 4MHz Z80,
 * and this is the number that turns a driver's microseconds into cycles.
 */
#define CPU_HZ  4000000

extern unsigned long long sim_cycles;   /* d1/mpz80.c: one per instruction */

/* wall-clock time the simulation started, for the exit report */
static struct timeval sim_wall_start;

struct timeout timeouts[MAXTIMEOUTS];

/*
 * Simulated microseconds, from the cycle counter.
 *
 * Anything modelling how long the machine's own hardware takes - a
 * character at a baud rate, a controller's turnaround - measures it with
 * this and not with a host wall clock, or the answer depends on what the host was
 * doing.  It lives here rather than in util.c because the host tools
 * link that library without a processor, and a cycle count means nothing
 * to fsck.
 */
unsigned long long
simnow64()
{
    return sim_cycles / (CPU_HZ / 1000000);
}

/*
 * Report the simulation speed at exit: how many simulated cycles the run
 * got through per wall-clock second, and what fraction that is of a real
 * Z80 clocked at 4 MHz (sim_cycles is one per instruction, so the ratio is
 * approximate - a real Z80 averages several clocks per instruction).
 */
void
sim_report(void)
{
    struct timeval now;
    double wall, cyc_per_sec, ratio;

    if (gettimeofday(&now, NULL) != 0)
        return;
    wall = (now.tv_sec - sim_wall_start.tv_sec)
         + (now.tv_usec - sim_wall_start.tv_usec) / 1e6;
    if (wall <= 0)
        return;

    cyc_per_sec = sim_cycles / wall;
    ratio = cyc_per_sec / (double)CPU_HZ;

    printf("sim: %llu cycles in %.2f s = %.0f cyc/s (%.3fx a 4 MHz Z80)\n",
           (unsigned long long)sim_cycles, wall, cyc_per_sec, ratio);
    fflush(stdout);
}

/*
 * SIGINT/SIGTERM/SIGHUP: report the run and go, so a Ctrl-C on the sim
 * prints the same speed line a normal exit would.  _exit() skips the
 * atexit handlers, so both of them are asked for by name here: the
 * report, and hanging up the xterms, which the atexit handler would
 * otherwise have done.  An xterm is a child of the simulator, so it
 * outlives it - without this the windows stay up with nothing behind
 * them, attached to a machine that is gone.
 */
void
signal_report(int sig)
{
    sim_report();
    close_terminals();
    _exit(0);
}

/*
 * Called once per instruction from the main loop.  Everything happens
 * here, between instructions, which is what makes it repeatable.
 */
void
check_time_outs()
{
    int i;
    struct timeout *tp;

    for (i = 0; i < MAXTIMEOUTS; i++) {
        tp = &timeouts[i];

        while (tp->handler && sim_cycles >= tp->when) {
            void (*handler)(int a) = tp->handler;
            int arg = tp->arg;

            if (tp->interval) {
                tp->when += tp->interval;
            } else {
                tp->handler = 0;
                tp->arg = 0;
            }
#ifndef NODEBUG
            if (traceflags & trace_timer) {
                printf("timeout %s at %llu\n",
                    tp->name ? tp->name : "?", sim_cycles);
            }
#endif
            (*handler)(arg);
        }
    }
}

void
recurring_time_out(char *name, int hertz, void (*function)(int a), int a)
{
    int i;
    struct timeout *tp;

    for (i = 0; i < MAXTIMEOUTS; i++) {
        tp = &timeouts[i];
        if (tp->handler)
            continue;
        tp->interval = (unsigned long long) CPU_HZ / hertz;
        tp->handler = function;
        tp->name = name;
        tp->arg = a;
        tp->when = sim_cycles + tp->interval;
        return;
    }
    printf("timeout overflow");
    exit(3);
}

/*
 * call function in usec_from_now
 */
void
time_out(char *name, int usec_from_now, void (*function)(int a), int arg)
{
    int i;
    struct timeout *tp;
    
    for (i = 0; i < MAXTIMEOUTS; i++) {
        tp = &timeouts[i];
        if (tp->handler)
            continue;
        tp->interval = 0;
        tp->handler = function;
        tp->name = name;
        tp->arg = arg;
        tp->when = sim_cycles +
            ((unsigned long long) usec_from_now * CPU_HZ) / MILLION;
        return;
    }
    printf("timeout overflow");
    exit(3);
}

void
cancel_time_out(void (*handler)(), int arg)
{
    int i;
    struct timeout *tp;
    
    for (i = 0; i < MAXTIMEOUTS; i++) {
        tp = &timeouts[i];
        if (tp->handler == handler && tp->arg == arg) {
            tp->handler = 0;
            tp->arg = 0;
        }
	}
}

char fbuf[0];

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

#ifdef notdef
/*
 * utility functions for the command processors
 */
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
        if (strcasecmp(&wordbuf[1], "bc") == 0) {
            return z80_get_reg16(bc_reg);
        }
        if (strcasecmp(&wordbuf[1], "de") == 0) {
            return z80_get_reg16(de_reg);
        }
        if (strcasecmp(&wordbuf[1], "hl") == 0) {
            return z80_get_reg16(hl_reg);
        }
        if (strcasecmp(&wordbuf[1], "ix") == 0) {
            return z80_get_reg16(ix_reg);
        }
        if (strcasecmp(&wordbuf[1], "iy") == 0) {
            return z80_get_reg16(iy_reg);
        }
        if (strcasecmp(&wordbuf[1], "pc") == 0) {
            return z80_get_reg16(pc_reg);
        }
        if (strcasecmp(&wordbuf[1], "sp") == 0) {
            return z80_get_reg16(sp_reg);
        }
        if (strcasecmp(&wordbuf[1], "tos") == 0) {
            return get_word(z80_get_reg16(sp_reg));
        }
    }
    if ((i = find_symbol(wordbuf)) == -1) {
        i = strtol(wordbuf, &wp, 16);
    }
    return i;
}

/*
 * all the command processors take a pointer to the pointer to the input string
 * and we have skipped any white space.  if we're at the end of the command,
 * we're pointing at a null.
 * all command processors assume that we are going to do another command.  if
 * we want to return to the simulation, we need to return from monitor.
 * that's the convention we use.  so, if our command returns 1, then we are
 * going to simulate some more.
 */
int lastaddr = -1;
char cmdline[100];

#define MONCMDS 25

struct moncmd {
    char cmd;
    char *help;
    int (*handler)(char **cmdlinep);
};

extern struct moncmd moncmds[];

void
register_mon_cmd(char c, char *help, int (*handler)(char **p))
{
    int i;
    for (i = 0; i < MONCMDS; i++) {
        if ((moncmds[i].cmd == 'c') || (moncmds[i].cmd == 0)) {
            moncmds[i].cmd = c;
            moncmds[i].help = help;
            moncmds[i].handler = handler;
            return;
        }
    }
}

/*
 * command line processor for monitor/debugger
 * the whole thing is plugin-driven
 */
void
monitor()
{
    struct point *p, *prev, **head;
    char l;
    char c;
    int i;
    int delete;
    char *s;

    while (1) {
        printf(">>> ");
        
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
        for (i = 0; i < MONCMDS; i++) {
            if (moncmds[i].cmd == c) {
                if ((*moncmds[i].handler)(&s)) {
                    lastaddr = -1;
                    return;
                }
                break;
            }
        }
        if (i == MONCMDS) {
            printf("unknown command %c\n", c);
        }
    }
}

int
list_cmd(char **sp)
{
    int i;
    int l;
    int c;
    char *s;

    if (**sp) {
        i = getaddress(sp);
    } else {
        if (lastaddr == -1) {
            i = z80_get_reg16(pc_reg);
        } else {
            i = lastaddr;
        }
    }
    for (l = 0; l < LISTLINES; l++) {
        c = format_instr(i, cmdline, &get_byte, &lookup_sym, &reloc, &mnix_sc);
        s = lookup_sym(i);
        if (s) {
            printf("%s\n", s);
        }
        printf("%04x: %-20s\n", i, cmdline);
        i += c;
        lastaddr = i & 0xffff;
    }
    return 0;
}

#endif


#ifdef notdef
int
dump_cmd(char **p)
{
    vaddr i;

    if (**p) {
        i = getaddress(p);
    } else {
        if (lastaddr == -1) {
            i = 0;
        } else {
            i = lastaddr;
        }
    }
    dumpmem(&get_byte, i, 256);
    lastaddr = (i + 256) & 0xffff;
    return 0;
}

/*
 * we do something gnarly here:  we'll call the instruction formatter to find out how many bytes are used
 * by this instruction, and put a temporary breakpoint just after it.
 */
int
next_cmd(char **sp)
{
    char outbuf[40];
    word pc;
    int i;

    pc = z80_get_reg16(pc_reg);

    i = format_instr(pc, outbuf, &get_byte, &lookup_sym, &reloc, &mnix_sc);
    next_break = pc + i;

    return 1;
}

int
step_cmd(char **sp)
{
    int i = 1;
    if (**sp) {
        i = strtol(*sp, sp, 16);
    }
    inst_countdown = i;
    return 1;
}

int
go_cmd(char **sp)
{
    if (**sp) {
        z80_set_reg16(pc_reg, strtol(*sp, sp, 16));
    }
    inst_countdown = -1;
    return 1;
}

int
trace_cmd(char **sp)
{
    int k = traceflags;
    char *s = "trace set to:\n";
    int i;

    if (**sp == '?') {
        s = "trace can be:\n";
        k = -1;
    } else if (**sp) {
        traceflags = strtol(*sp, sp, 16);
        k = traceflags;
    } 
    puts(s);
    for (i = 0; tracenames[i]; i++) {
        if (k & (1 << i)) {
            printf("\t%x %s\n", 1 << i, tracenames[i]);
        }
    }
    return 0;
}

int
exit_cmd(char **sp)
{
    exit(1);
}

int
regs_cmd(char **sp)
{
    dumpcpu();
    return 0;
}

int
help_cmd(char **sp)
{
    int i;
    printf("commands:\n");
    for (i = 0; i < MONCMDS; i++) {
        if (moncmds[i].help) {
            putchar(moncmds[i].cmd);
            puts(" ");
            puts(moncmds[i].help);
        }
    }
    return 0;
}

struct moncmd moncmds[MONCMDS] = {
    { 'l', " [addr]\tlist instructions", list_cmd },
    { 'b', "[-][<addr>] [...]\tadd or delete breakpoint", break_cmd },
    { 'd', " [addr]\tdump memory", dump_cmd },
    { 's', " [inst count]\tstep", step_cmd },
    { 'n', "\tstep over", next_cmd },
    { 'g', " [address]\tgo", go_cmd },
    { 'r', "\tdump registers", regs_cmd },
    { 't', "trace\tset trace", trace_cmd },
    { 'q', "\tquit", exit_cmd },
    { 'x', "\texit", exit_cmd },
    { 'h', "\thelp", help_cmd },
    { '?', "\thelp", help_cmd },
    { 0, 0, 0 }
};
#endif


/*
 * The boot device, by name.
 *
 * The rom decides what to boot from the top five bits of the
 * configuration switch - mon447.s at tstsw - and the numbers are not
 * memorable:
 *
 *	0x00	boothd, which is the HDCA
 *	0x08	nuboot, which is the HDC-DMA
 *	0x10	the DJ-DMA floppy
 *
 * with 0x04 on top of any of them to skip the monitor and go straight
 * to the boot.  Everything written down about running this machine says
 * "switches 0x0c" and then explains what 0x0c is; this says hdcdma and
 * does not need explaining.  -c still takes the number for anything
 * these three do not cover, and a -c after -B wins.
 */
static struct {
    char *name;
    int sw;
} bootdevs[] = {
    { "hdca",   0x00 },
    { "hdcdma", 0x08 },
    { "djdma",  0x10 },
    { 0, 0 }
};

static int
bootdev(char *name)
{
    int i;

    for (i = 0; bootdevs[i].name; i++) {
        if (strcmp(bootdevs[i].name, name) == 0) {
            return bootdevs[i].sw | 0x04;
        }
    }
    return -1;
}

#ifndef NODEBUG
/*
 * Route the debug stream (fd 1: l()/trace()/dumpcpu()/monitor()) to a
 * debug log file and, optionally, the invoking terminal.  A child copies
 * a pipe to the two places; the parent's stdout is the pipe.
 *
 * term selects how the terminal is treated:
 *   DBG_TERM_OFF     - never write the terminal (the console owns it, or -n)
 *   DBG_TERM_ON      - write the terminal, but only once the debugger has
 *                      been entered (-q): the gate starts closed
 *   DBG_TERM_ALWAYS  - write the terminal from the start
 *
 * logfd is written unconditionally (when >= 0), so the pre-debugger trace
 * is never lost even while the terminal is gated off.
 */
#define DBG_TERM_OFF     0
#define DBG_TERM_ON      1
#define DBG_TERM_ALWAYS  2

static void
route_debug(int logfd, int term)
{
    int termfd = dup(1);        /* the invoking terminal */
    int p[2], ctl[2];
    int nfds;
    int n;
    char buf[4096];

    pipe(p);
    if (term == DBG_TERM_ON)
        pipe(ctl);              /* the gate control pipe, only for -q */
    if (fork() == 0) {
        int gate = (term == DBG_TERM_ALWAYS);
        fd_set fds;

        close(p[1]);
        if (term == DBG_TERM_ON)
            close(ctl[1]);
        nfds = p[0] + 1;
        if (term == DBG_TERM_ON && ctl[0] >= nfds)
            nfds = ctl[0] + 1;
        for (;;) {
            FD_ZERO(&fds);
            FD_SET(p[0], &fds);
            if (term == DBG_TERM_ON)
                FD_SET(ctl[0], &fds);
            select(nfds, &fds, 0, 0, 0);
            if (term == DBG_TERM_ON && FD_ISSET(ctl[0], &fds)) {
                char c;
                if (read(ctl[0], &c, 1) <= 0)
                    break;
                gate = 1;       /* debugger entered: open the gate */
            }
            if (FD_ISSET(p[0], &fds)) {
                n = read(p[0], buf, sizeof buf);
                if (n <= 0)
                    break;
                if (logfd >= 0)
                    write(logfd, buf, n);
                if (term != DBG_TERM_OFF && gate)
                    write(termfd, buf, n);
            }
        }
        close(termfd);
        if (logfd >= 0)
            close(logfd);
        sim_report();           /* _exit() skips the atexit handler */
        _exit(0);
    }

    close(p[0]);
    dup2(p[1], 1);
    close(p[1]);
    if (term == DBG_TERM_ON) {
        close(ctl[0]);
        debug_gate_fd = ctl[1]; /* the parent opens the gate on debugger entry */
    }
    close(termfd);
    setvbuf(stdout, 0, _IONBF, 0);
}

/*
 * Un-suppress the debug stream.  Idempotent; the first call to reach here
 * after -q has suppressed the terminal is what flips the gate open.
 */
static void
open_debug_gate(void)
{
    if (debug_gate_fd == -1)
        return;
    write(debug_gate_fd, "g", 1);
    close(debug_gate_fd);
    debug_gate_fd = -1;
}
#endif

/*
 * A drive named on the command line: <controller><unit>:<file>, as in
 *
 *	djdma0:boot.IMD  hdcdma0:hddma-0  hdca1:/tmp/scratch
 *
 * The controller names are the ones -B takes, so the same word means
 * the same thing in both places.  A floppy goes on the list the DJ-DMA
 * reads in order; a hard unit is recorded against the file name its
 * controller will ask drive_open for, which is hddma-<n> or hdca-<n>.
 *
 * Returns 0 if the argument was not of this shape, so that a bare file
 * name still means what it always did.
 */
static int
drivearg(char *arg)
{
    char *colon = strchr(arg, ':');
    char unit[32];
    char ctl[32];
    int n;
    int i;
    size_t len;

    if (!colon || colon == arg) {
        return 0;
    }
    len = colon - arg;
    if (len >= sizeof(ctl)) {
        return 0;
    }
    memcpy(ctl, arg, len);
    ctl[len] = 0;

    /* the trailing digits are the unit */
    i = len;
    while (i > 0 && ctl[i - 1] >= '0' && ctl[i - 1] <= '9') {
        i--;
    }
    if (i == (int)len) {            /* no unit given: djdma:file is unit 0 */
        n = 0;
    } else {
        n = atoi(&ctl[i]);
    }
    ctl[i] = 0;

    if (strcmp(ctl, "djdma") == 0) {
        int have = 0;

        if (drivenames) {
            while (drivenames[have]) have++;
        }
        if (n < have) {
            drivenames[n] = strdup(colon + 1);
            return 1;
        }
        drivenames = realloc(drivenames, sizeof(char *) * (n + 2));
        while (have < n) {
            drivenames[have++] = 0;
        }
        drivenames[n] = strdup(colon + 1);
        drivenames[n + 1] = 0;
        return 1;
    }
    if (strcmp(ctl, "hdcdma") == 0) {
        snprintf(unit, sizeof(unit), "hddma-%d", n);
        if (drive_setunit(unit, colon + 1) != 0)
            return -1;
        anydrive = 1;
        return 1;
    }
    if (strcmp(ctl, "hdca") == 0) {
        snprintf(unit, sizeof(unit), "hdca-%d", n);
        if (drive_setunit(unit, colon + 1) != 0)
            return -1;
        anydrive = 1;
        return 1;
    }
    return 0;
}

#ifndef NODEBUG
/*
 * Trace bits, by number or by name: -t 0x100000 and -t syscall are the
 * same thing, and -t syscall,trap is the pair.  The names are the ones
 * the drivers registered, which is what usage() has always printed
 * beside the numbers.
 */
static int
traceparse(char *s, char *progname)
{
    char buf[256];
    char *p;
    char *comma;
    int bits = 0;
    int i;

    if (*s >= '0' && *s <= '9') {
        return strtol(s, 0, 0);
    }
    snprintf(buf, sizeof(buf), "%s", s);
    p = buf;
    while (p) {
        if ((comma = strchr(p, ','))) {
            *comma++ = 0;
        }
        if (strcmp(p, "all") == 0) {
            bits = ~0;
        } else {
            for (i = 0; tracenames[i]; i++) {
                if (strcmp(tracenames[i], p) == 0) {
                    bits |= 1 << i;
                    break;
                }
            }
            if (!tracenames[i]) {
                fprintf(stderr, "no trace called %s\n", p);
                usage("", progname);
            }
        }
        p = comma;
    }
    return bits;
}
#endif

#ifndef NODEBUG
/*
 * -F runs the fast build.  d1p is the -DNODEBUG simulator linked beside
 * this one; exec it with the same arguments, minus the -F, so a run that
 * wants speed does not pay for a tracer that is not there.
 */
/*
 * Options only the debug build understands.  d1p (the -DNODEBUG build
 * linked beside us) has no tracer and no debugger, so those options -
 * and their values - must not be passed through -F.  Returns 2 for a
 * debug option that eats the next argument, 1 for one that does not,
 * 0 for anything else.
 */
static int
perf_option(char *arg)
{
    if (arg[0] != '-' || arg[1] == 0)
        return 0;
    switch (arg[1]) {
    case 'D': case 'S': case 'W': case 'P': case 'T': case 't':
        return 2;               /* debug option with a value */
    case 'l': case 'n': case 'x': case 'q': case 's':
        return 1;               /* debug option, no value */
    }
    return 0;
}

static void
exec_perf(char **argv, int argc, int pindex)
{
    char *prog = argv[0];
    char *slash = strrchr(prog, '/');
    char path[4096];
    char **nargv;
    int n, i;

    if (slash)
        snprintf(path, sizeof(path), "%.*sd1p", (int)(slash - prog + 1), prog);
    else
        snprintf(path, sizeof(path), "d1p");

    nargv = malloc(sizeof(char *) * argc);
    n = 0;
    for (i = 0; i < argc; i++) {
        int drop;

        if (i == pindex)            /* drop the -F */
            continue;
        drop = perf_option(argv[i]);
        if (drop == 2) {
            i++;                    /* skip the option's value too */
            continue;
        }
        if (drop == 1)
            continue;
        nargv[n++] = (i == 0) ? path : argv[i];
    }
    nargv[n] = 0;

    execv(path, nargv);
    fprintf(stderr, "%s: cannot exec %s: %s\n", prog, path, strerror(errno));
    exit(1);
}
#endif

int
main(int argc, char **argv)
{
    gettimeofday(&sim_wall_start, NULL);
    atexit(sim_report);
#ifndef NODEBUG
    {
        int pj;

        for (pj = 1; pj < argc; pj++) {
            if (strcmp(argv[pj], "-F") == 0)
                exec_perf(argv, argc, pj);
        }
    }
#endif

    char *progname = *argv++;
    char *s;
    char **argvec;
    int i;
    char *ptyname;
    int fd;
    int ret;

    /*
     * run the driver startup hooks before argument processing
     * to set defaults, etc before command line options get to
     * override them
     */
    for (i = 0; i < ndrivers; i++) {
        if (drivers[i]->prearg_hook) {
            if ((ret = (*drivers[i]->prearg_hook)())) {
                printf("prearg hook %s error %d\n", drivers[i]->name, ret);
                exit(1);
            }
        }
    }

    /*
     * Default the boot rom to one in the current directory, so the
     * simulator finds its resource files without -b; -b still overrides.
     */
    if (access("roms/mon447.bin", F_OK) == 0)
        rom_filename = "roms/mon447.bin";
    else if (access("mon447.bin", F_OK) == 0)
        rom_filename = "mon447.bin";

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
#ifndef NODEBUG
            case 'l':
                log_output = 1;
                break;
            case 'n':
                no_console_log = 1;
                break;
            case 'x':
                inst_countdown = 0;
                debug_terminal = 1;
                break;
            case 'q':
                debug_suppress = 1;
                break;
            case 'D':
                if (!argc--) {
                    usage("debug log file missing\n", progname);
                }
                {
                    char *f = *argv++;
                    debug_logfd = open(f, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (debug_logfd < 0) {
                        perror(f);
                        exit(1);
                    }
                }
                break;
#endif
            case 'L':
                if (!argc--) {
                    usage("console log file missing\n", progname);
                }
                {
                    char *f = *argv++;
                    console_logfd = open(f, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (console_logfd < 0) {
                        perror(f);
                        exit(1);
                    }
                }
                break;
            case 'c':
                if (!argc--) {
                    usage("configuration switch value missing\n", progname);
                }
                config_sw = strtol(*argv++, 0, 0) | CONF_SET;
                break;
            case 'b':
                if (!argc--) {
                    usage("boot rom name missing\n", progname);
                }
                rom_filename = strdup(*argv++);
                break;
            case 'B':
                {
                    int sw;

                    if (!argc--) {
                        usage("boot device missing: djdma, hdcdma or hdca\n",
                            progname);
                    }
                    if ((sw = bootdev(*argv++)) < 0) {
                        usage("boot device is djdma, hdcdma or hdca\n",
                            progname);
                    }
                    config_sw = sw | CONF_SET;
                }
                break;
#ifndef NODEBUG
            case 'S':
                if (!argc--) {
                    usage("kernel name missing\n", progname);
                }
                kern_filename = strdup(*argv++);
                break;
#endif
            case 'd':
                if (!argc--) {
                    usage("drive directory missing\n", progname);
                }
                drive_setdir(*argv++);
                break;
            case 'm':
                if (!argc--) {
                    usage("memory size missing\n", progname);
                }
                ram_size = parsesize(*argv++);
                break;
            case 'H':
                halt_exit = 0;
                break;
            case '5':
                if (!argc--) {
                    usage("5 1/4 inch drive file missing\n", progname);
                }
                {
                    int n = 0;

                    if (fivenames) {
                        while (fivenames[n]) n++;
                    }
                    fivenames = realloc(fivenames, sizeof(char *) * (n + 2));
                    fivenames[n] = strdup(*argv++);
                    fivenames[n + 1] = 0;
                }
                break;
#ifndef NODEBUG
            case 'W':
                if (!argc--) {
                    usage("watch address not specified\n", progname);
                }
                {
                    char *w = *argv++;
                    char *dash = strchr(w, '-');
                    unsigned lo = strtol(w, 0, 0);

                    add_write_watch(lo, dash ? strtol(dash + 1, 0, 0) : lo);
                }
                break;
            case 'P':
                if (!argc--) {
                    usage("physical watch address not specified\n", progname);
                }
                {
                    char *w = *argv++;
                    char *dash = strchr(w, '-');
                    unsigned lo = strtol(w, 0, 0);

                    add_phys_watch(lo, dash ? strtol(dash + 1, 0, 0) : lo);
                }
                break;
            case 'T':
                {
                    char spec[64];
                    char *comma;

                    if (!argc--) {
                        usage("trace trigger address missing\n", progname);
                    }
                    snprintf(spec, sizeof(spec), "%s", *argv++);
                    if ((comma = strchr(spec, ','))) {
                        *comma++ = 0;
                        tracelen = strtol(comma, 0, 0);
                    }
                    if (!dis_parse(spec, tracetrig, sizeof(tracetrig))) {
                        usage("trigger wants a space: sys:0100, tsk1:100b "
                            "or trap:05\n", progname);
                    }
                }
                break;
            case 't':
                if (!argc--) {
                    usage("trace not specified \n", progname);
                }
                traceflags = traceparse(*argv++, progname);
                break;
            case 's':
                inst_countdown = 0;
                break;
#endif
            case 'h':
                usage("", progname);
                break;
            default: {
                char msg[32];

                sprintf(msg, "unrecognized option: %c\n", s[-1]);
                usage(msg, progname);
                break;
            }
            }
        }
    }

    /*
     * The rest of the arguments are drives.  <controller><unit>:<file>
     * says which drive it is; a bare name is a floppy, appended in
     * order, which is what it has always meant.
     */
    while (*argv) {
        int r = drivearg(*argv);

        if (r < 0) {
            usage("too many named units\n", progname);
        }
        if (r) {
            argv++;
            continue;
        }
        if (!drivenames) {
            drivenames = malloc(sizeof(char *) * 2);
            i = 0;
        } else {
            for (i = 0; drivenames[i]; i++)
                ;
            drivenames = realloc(drivenames, sizeof(char *) * (i + 2));
        } 
        drivenames[i] = strdup(*argv++);
        drivenames[i+1] = 0;
    }

    /*
     * With nothing said at all, put the usual disk in the usual drive.
     * But not when a 5 1/4 inch floppy was named: that asks for a machine
     * booting off the other port, and quietly occupying the 8 inch one
     * would take that boot away - the controller tries 8 inch first.
     */
    if (!drivenames && !fivenames && !anydrive) {
        drivenames = malloc(sizeof(char *) * 2);
        drivenames[0] = "DRIVE_A.IMD";
        drivenames[1] = 0;
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
#ifndef NODEBUG
    if (debug_terminal) {
        char namebuf[100];
        int debugin;
        int debugout;

        open_terminal("debug", 0, &debugin, &debugout, 1, log_output ? LOGFILE : 0);

        sprintf(namebuf, "/proc/%d/fd/%d", getpid(), debugin);
        stdin = freopen(namebuf, "r+", stdin);
        if (!stdin) {
            fprintf(stderr, "fdopen of stdin failed %d\n", errno);
            exit(0);
        }

        sprintf(namebuf, "/proc/%d/fd/%d", getpid(), debugout);
        stdout = freopen(namebuf, "r+", stdout);
        if (!stdout) {
            fprintf(stderr, "fdopen of stdout failed %d\n", errno);
            exit(0);
        }
        setvbuf(stdout, 0, _IONBF, 0);
    } else {
        int logfd = debug_logfd;    /* -D <file>, else -1 */
        int term;

        inst_countdown = -1;
        if (logfd < 0 && log_output)
            logfd = open(LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);

        if ((config_sw >> 8) & 0x1) {
            /* uart 0 is in an xterm, so the invoking terminal is free for
             * the debug stream */
            if (no_console_log)
                term = DBG_TERM_OFF;
            else if (debug_suppress)
                term = DBG_TERM_ON;
            else
                term = DBG_TERM_ALWAYS;
        } else {
            /* the console owns the terminal: debug goes to a file only */
            term = DBG_TERM_OFF;
            if (logfd < 0)
                logfd = open(LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        }
        route_debug(logfd, term);
    }

    if (traceflags) {
        printf("trace %x ", traceflags);
        for (i = 0; tracenames[i]; i++) {
            if (traceflags & (1 << i)) {
                printf("%s ", tracenames[i]);
            }
        }
        printf("\n");
    }
#endif

    /*
     * load the boot rom if there is one
     */
    if (rom_size) {
        rom_image = malloc(rom_size);
        fd = open(rom_filename, O_RDONLY);
        if (fd < 0) {
            /*
             * open answers -1, not 0.  Testing !fd let a missing rom
             * through to the read below, which then failed with EBADF -
             * so a rom that was not there reported itself as a bad file
             * descriptor, which is a long way from the truth.
             */
            perror(rom_filename);
            exit(errno);
        }
        i = read(fd, rom_image, rom_size);
        if (i < 0) {
            perror(rom_filename);
            exit(errno);
        }
        close(fd);
#ifndef NODEBUG
        i = strlen(rom_filename);
        // if there's a similarly named symfile, use it
        if (!sym_filename && (rom_filename[i-4] == '.')) {
            sym_filename = strdup(rom_filename);
            strcpy(&sym_filename[i-3], "sym");
        }
#endif
    }

#ifndef NODEBUG
    if (sym_filename) {
        load_symfile(sym_filename);
    }
    if (kern_filename) {
        load_kernel_syms(kern_filename);
    }

    mysignal(SIGUSR1, stop_handler);
    kdump_init();

    /*
     * the monitor's command table is built at runtime by mon_init, so
     * without this call the debugger comes up with no commands at all
     */
    verbose = traceflags;
    mon_init();
#endif

    setup_sim_ports();
    z80_init();

    mysignal(SIGINT, signal_report);
    mysignal(SIGTERM, signal_report);
    /*
     * SIGHUP is how an xterm tells us its window has gone: the poller
     * behind it hangs up the simulator when the pty reads EOF
     * (lib/openx.c), which is what makes closing a window end the run
     * instead of leaving d1 behind it.  Catching it here hangs up the
     * OTHER windows on the way out too, so closing one closes them all
     * rather than leaving them attached to a machine that has stopped.
     */
    mysignal(SIGHUP, signal_report);

    // another driver hook
    for (i = 0; i < ndrivers; i++) {
        if (drivers[i]->startup_hook) {
            if ((ret = (*drivers[i]->startup_hook)())) {
                printf("startup hook %s error %d\n", drivers[i]->name, ret);
                exit(1);
            }
        }
        printf("%s loaded\n", drivers[i]->name);
    }

    /*
     * the main emulation loop
     */
    while (1) {
#ifndef NODEBUG
        program_counter = z80_get_reg16(pc_reg);
#endif

        /*
         * run the driver poll hooks
         */
        for (i = 0; i < ndrivers; i++) {
            if (drivers[i]->poll_hook) {
                (*drivers[i]->poll_hook)();
            }
        }
#ifndef NODEBUG
        /*
         * The monitor owns breakpoints now, so there is no separate
         * check for the temporary one that "step over" plants: it goes
         * into the same bitmap as any other, and breakpoint_at() takes
         * it back out again once it fires.  Watchpoints come along for
         * free with the shared monitor and are checked the way usersim
         * checks them.
         */
        /*
         * the trigger.  once it fires it stays fired - this arms the
         * trace, it does not gate it
         */
        if (tracetrig[0]) {
            char abuf[16];

            if (strcmp(dis_space(program_counter, abuf, sizeof(abuf)),
                tracetrig) == 0) {
                printf("trace trigger: %s\n", abuf);
                traceflags |= trace_inst;
                tracetrig[0] = 0;
            }
        }
        {
            int reason = 0;
            /*
             * A stop asked for from outside (SIGUSR1).  Taken here, and
             * turned into the same stop the watchpoints use, so the
             * registers are dumped and the monitor comes up.
             */
            if (stop_request) {
                stop_request = 0;
                open_debug_gate();
                printf("stopped by SIGUSR1\n");
                inst_countdown = 0;
                reason = 1;
            }
            kdump_poll();
            if (watchpoint_hit()) {
                open_debug_gate();
                printf("watchpoint\n");
                inst_countdown = 0;
                reason = 1;
            }
            if (breakpoint_at(program_counter)) {
                open_debug_gate();
                printf("breakpoint\n");
                inst_countdown = 0;
                reason = 1;
            }
            if ((traceflags & trace_inst) || (inst_countdown == 0) || ((traceflags & trace_symbols) && lookup_sym(program_counter))) {
                dumpcpu();
            }
            /*
             * capture length.  only counts while the trace is actually on, so
             * a count given with a trigger measures from the trigger
             */
            if (tracelen && (traceflags & trace_inst)) {
                if (--tracelen == 0) {
                    traceflags &= ~trace_inst;
                    printf("trace: capture complete\n");
                }
            }
            if (inst_countdown == 0) {
                if (!reason && !debug_terminal) {
                    open_debug_gate();
                    printf("single-step\n");
                }
                monitor();
            }
        }
#endif

#ifdef notdef
        /*
         * if we know we aren't debugging, don't bother to pop up here
         * otherwise, run for 1 instruction so we get control to check for breakpoints
         */
        if ((nbreaks == 0) && (inst_countdown == -1) && !(traceflags & trace_inst)) {
            i = 1000000;
        } else {
            i = 1;
        }
#endif
        running = 1;
        mysigblock();
        {
            unsigned long long before = sim_cycles;

            z80_run();

            /*
             * A halted processor still consumes time.  It sits there
             * taking bus cycles until something interrupts it, and if
             * simulated time is the cycle count then a halt that
             * advances nothing stops the clock - so the interrupt that
             * would end the halt is never delivered, and the machine
             * waits for ever on a timer that cannot fire.
             *
             * That deadlock looked like the console stopping in the
             * middle of a word, with three clock interrupts in a four
             * minute run.  Charge a nop's worth of cycles for standing
             * still.
             */
            if (sim_cycles == before) {
                sim_cycles += 4;
            }
        }
        mysigunblock();
        running = 0;
        check_time_outs();
        take_pending_trap();
#ifndef NODEBUG
        if (inst_countdown != -1) {
            inst_countdown--;
        }
#endif
    }
    exit(0);
}

/*
 * Console arbitration, discovered rather than configured.
 *
 * A boot floppy carries its answer in its bios and says nothing about it
 * anywhere a simulator could read, so there is nothing to detect up
 * front.  What there is instead is the moment of use: a bios that talks
 * to the floppy controller's serial port issues a channel command to do
 * it, and that command is unambiguous.  Ownership therefore follows the
 * most recent claim, and a claim is made by transmitting a character or
 * by arming a receive.  The serial board owns the console until told
 * otherwise, which is the normal case and stays silent.
 *
 * Ownership decides who may consume input, and only that.  Output from
 * both still reaches the same window, deliberately - that is what a
 * person watching the machine sees - and the traces say which device
 * each character came from.
 */
int console_owner = CONS_MULTIO;

static char *console_names[] = {
    "the multio uart 0",
    "the djdma bit banged serial"
};

void
console_claim(int who)
{
    if (console_owner == who) {
        return;
    }
    console_owner = who;
#ifndef NODEBUG
    fprintf(stderr, "console: input follows %s\n", console_names[who]);
#endif
}

#ifndef NODEBUG
__attribute__((constructor))
void
init_trace()
{
    int i;
    for (i = 0; def_traces[i].name; i++) {
        *def_traces[i].valuep = register_trace(def_traces[i].name);
    }
}
#endif

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
