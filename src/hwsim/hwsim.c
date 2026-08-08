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
#include <signal.h>
#include <unistd.h>

#include <setjmp.h>

#include "sim.h"
#include "hwsim.h"
#include "gui.h"
#include "util.h"
#include "imd.h"
#include "disz80.h"
#include "mnix.h"

#define S_FLAG  0x80
#define Z_FLAG  0x40
#define Y_FLAG  0x20
#define H_FLAG  0x10
#define X_FLAG  0x08
#define PV_FLAG 0x04
#define N_FLAG  0x02
#define C_FLAG  0x01

#define EXIT_PORT   0   // output to here ends the simulator
#define DUMP_PORT   1   // output to here makes a memory dump with registers
#define INPUT_PORT  2   // this is for patching into pip for import
#define OUTPUT_PORT 2   // this is for patching into pip for export

#define	LISTLINES	8
#define	STACKTOP	0xffff
#define MAXIMUM_STRING_LENGTH   100

#define LOGFILE     "logfile"

int program_counter;

int debug_terminal;
int log_output;
int mypid;
int running;
int listing;

int traceflags;

int trace_inst;
int trace_bio;
int trace_io;
int trace_symbols;
int trace_timer;

/*
 * A trace trigger, like the one on a logic analyzer: run quietly until
 * the machine reaches a place, then start recording.  Held as the string
 * dis_space prints for that place, so arming it needs no knowledge of
 * how a space is spelled and testing it is a compare.
 */
char tracetrig[16];

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

void
stop()
{
    inst_countdown = 0;
}

void
stop_handler()
{
    // printf("breakpoint signal\n");
    stop();
}

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

struct sym {
    char *name;
    vaddr value;
    struct sym *next;
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
lookup_sym(unsigned int symaddr)
{
    struct sym *s = syms;
    unsigned short addr = symaddr & 0xffff;

    if (!super()) return 0;

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
        add_sym(namebuf, v);
        i++;
    }
    printf("added %d symbols from %s\n", i, s);
    fclose(sf);
}

void
exit_port_handler(portaddr p, byte v)
{
    printf("exit port tickled %x\n", v);
    exit(0);
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
    register_output(EXIT_PORT, exit_port_handler);
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

    fprintf(stderr, "%s", complaint);
    fprintf(stderr, "usage: %s [<options>] <drive file> ...\n", p);
    fprintf(stderr, "\t-h\thelp\n");
    fprintf(stderr, "\t-b\t<boot rom file>\n");
    fprintf(stderr, "\t-c\t<configuration switch value>\n");
    fprintf(stderr, "\t-S\t<symbol file file>\n");
    fprintf(stderr, "\t-d\t<directory holding the hard drive unit files>\n");
    fprintf(stderr, "\t-T\t<space:addr>[,count] trace from here, for count instructions\n");
    fprintf(stderr, "\t-x\topen a debug terminal window\n");
    fprintf(stderr, "\t-t\t<tracebits>\n");
    fprintf(stderr, "\t-l\tproduce logfile\n");
    for (i = 0; tracenames[i]; i++) {
        fprintf(stderr, "\t%x %s\n", 1 << i, tracenames[i]);
    }
    for (i = 0; i < ndrivers; i++) {
        if (drivers[i]->usage_hook) {
            (*drivers[i]->usage_hook)();
        }
    }
    exit(1);
}

char **drivenames;
char *rom_filename;
char *sym_filename;
char *rom_image;
int rom_size;
int config_sw = 0;

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
struct timeout {
    char *name;
    struct timeval when;    
    struct timeval interval;    // if recurring
    void (*handler)(int a);
    int arg;                    // argument to pass handler
};

static void timeout_handler();

struct itimerval timer;
struct timeval tv;
struct timeval now;

#define MILLION 1000000
#define MAXTIMEOUTS 10

struct timeout timeouts[MAXTIMEOUTS];

/*
 * go through the timeout list and start the timer if there is a need
 * this can be called inside the signal handler, so we need protection
 */
static void
timeout_sched()
{
    int i;
    struct timeout *tp;

    // put the elephant in cairo
    tv.tv_sec = tv.tv_usec = 0;

    // now find the how long to wait for the next pop
    for (i = 0; i < MAXTIMEOUTS; i++) {
        tp = &timeouts[i];
        if (!tp->handler) continue;
        if ((tv.tv_sec == 0) || timercmp(&tp->when, &tv, <=)) {
            tv = tp->when;
        }
    }
    timer.it_interval.tv_sec = timer.it_interval.tv_usec = 0;
    if (tv.tv_sec != 0) {
        gettimeofday(&now, 0);
        timersub(&tv, &now, &timer.it_value);
        setitimer(ITIMER_REAL, &timer, 0);
        if (traceflags & trace_timer) printf("arming itimer\n");
        mysignal(SIGALRM, timeout_handler);
    } else {
        timer.it_value.tv_sec = timer.it_value.tv_usec = 0;
        setitimer(ITIMER_REAL, &timer, 0);
        if (traceflags & trace_timer) printf("disarming itimer\n");
    }
}

/*
 * this signal handler gets called every time our timer pops.
 * it is responsible for calling any expired timers
 * and arming any new timer.
 */
static void
timeout_handler()
{
    int i;
    struct timeout *tp;

    // if (trace & trace_timer) write(1, "timeout_handler called\n", 24);
    // loop through all the timers, delivering callouts as needed
    for (i = 0; i < MAXTIMEOUTS; i++) {
        tp = &timeouts[i];

    again:
        if (!tp->handler) continue;

        gettimeofday(&tv, 0);

        // deliver any expired callouts.
        if (timercmp(&tp->when, &tv, <=)) {
            // if (trace & trace_timer) printf("timer callout %s\n", tp->name);
            (*tp->handler)(tp->arg);

            // increment any recurring timers and maybe deliver again
            if (timerisset(&tp->interval)) {
                timeradd(&tp->when, &tp->interval, &tp->when);
                goto again;
            }

            tp->handler = 0;
            tp->arg = 0;
        }
    }
    timeout_sched();
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
        tp->interval.tv_usec = MILLION/hertz;
        tp->interval.tv_sec = 0;
        tp->handler = function;
        tp->name = name;
        tp->arg = a;
        gettimeofday(&tp->when, 0);
        timeradd(&tp->when, &tp->interval, &tp->when);
        timeout_sched();
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
        tv.tv_usec = usec_from_now;
        tv.tv_sec = 0;
        tp->interval.tv_usec = tp->interval.tv_sec = 0;
        tp->handler = function;
        tp->name = name;
        tp->arg = arg;
        gettimeofday(&tp->when, 0);
        timeradd(&tp->when, &tv, &tp->when);
        timeout_sched();
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
    timeout_sched();
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

int
main(int argc, char **argv)
{
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
            case 'l':
                log_output = 1;
                break;
            case 'x':
                inst_countdown = 0;
                debug_terminal = 1;
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
            case 'S':
                if (!argc--) {
                    usage("symfile name missing\n", progname);
                }
                sym_filename = strdup(*argv++);
                break;
            case 'd':
                if (!argc--) {
                    usage("drive directory missing\n", progname);
                }
                drive_setdir(*argv++);
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
                traceflags = strtol(*argv++, 0, 0);
                break;
            case 's':
                inst_countdown = 0;
                break;
            case 'h':
                usage("", progname);
                break;
            default:
                usage("unrecognized option", progname);
                break;
            }
        }
    }

    // the rest of the arguments are drive names
    while (*argv) {
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

    if (!drivenames) {
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
        inst_countdown = -1;
        stdout = freopen(LOGFILE, "w+", stdout);
        if (!stdout) {
            perror("lose");
        }
        setvbuf(stdout, 0, _IONBF, 0);
        printf("log file\n");
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
        i = strlen(rom_filename);
        // if there's a similarly named symfile, use it
        if (!sym_filename && (rom_filename[i-4] == '.')) {
            sym_filename = strdup(rom_filename);
            strcpy(&sym_filename[i-3], "sym");
        }
    }

    if (sym_filename) {
        load_symfile(sym_filename);
    }

    mysignal(SIGUSR1, stop_handler);

    /*
     * the monitor's command table is built at runtime by mon_init, so
     * without this call the debugger comes up with no commands at all
     */
    verbose = traceflags;
    mon_init();

    setup_sim_ports();
    z80_init();

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
        program_counter = z80_get_reg16(pc_reg);

        /*
         * run the driver poll hooks
         */
        for (i = 0; i < ndrivers; i++) {
            if (drivers[i]->poll_hook) {
                (*drivers[i]->poll_hook)();
            }
        }
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
        if (watchpoint_hit()) {
            printf("watchpoint\n");
            inst_countdown = 0;
        }
        if (breakpoint_at(program_counter)) {
            printf("breakpoint\n");
            inst_countdown = 0;
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
            monitor();
        }

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
        z80_run();
        mysigunblock();
        running = 0;
        if (inst_countdown != -1) {
            inst_countdown--;
        }
    }
    exit(0);
}

__attribute__((constructor))
void
init_trace()
{
    int i;
    for (i = 0; def_traces[i].name; i++) {
        *def_traces[i].valuep = register_trace(def_traces[i].name);
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
