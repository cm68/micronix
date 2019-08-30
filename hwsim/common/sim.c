/* 
 * sim.c
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
#include "util.h"
#include "imd.h"
#include "dis.h"

#define S_FLAG  0x80
#define Z_FLAG  0x40
#define Y_FLAG  0x20
#define H_FLAG  0x10
#define X_FLAG  0x08
#define PV_FLAG 0x04
#define N_FLAG  0x02
#define C_FLAG  0x01

#define PC_REG      (*cpureg.pc_ptr)
#define SP_REG      (*cpureg.pc_ptr)
#define A_REG       (*cpureg.a_ptr)
#define F_REG       (*cpureg.a_ptr)
#define B_REG       (*cpureg.a_ptr)
#define C_REG       (*cpureg.a_ptr)
#define D_REG       (*cpureg.a_ptr)
#define E_REG       (*cpureg.a_ptr)
#define H_REG       (*cpureg.a_ptr)
#define L_REG       (*cpureg.a_ptr)
#define IX_REG      (*cpureg.ix_ptr)
#define IY_REG      (*cpureg.iy_ptr)
#define I_REG       (*cpureg.i_ptr)
#define R_REG       (*cpureg.i_ptr)
#define BUS_STATUS  (*cpureg.status)
#define BUS_CONTROL (*cpureg.control)

#define BC_REG  ((B_REG << 8) | C_REG)
#define DE_REG  ((D_REG << 8) | E_REG)
#define HL_REG  ((H_REG << 8) | L_REG)

#define EXIT_PORT   0   // output to here ends the simulator
#define DUMP_PORT   1   // output to here makes a memory dump with registers
#define INPUT_PORT  2   // this is for patching into pip for import
#define OUTPUT_PORT 2   // this is for patching into pip for export

#define	LISTLINES	8
#define	STACKTOP	0xffff
#define MAXIMUM_STRING_LENGTH   100

struct cpuregs cpureg;

int terminal_fd;
int debug_terminal;
int mypid;
char *mytty;

int trace;
int trace_inst;
int trace_bio;
int trace_ior;
int trace_io;

struct {
    char *name;
    int *valuep;
} def_traces[] = {
    {"inst", &trace_inst },
    {"bio", &trace_bio },
    {"ior", &trace_ior },
    {"io", &trace_io },
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
    printf("breakpoint signal\n");
    stop();
}

unsigned int
reloc(unsigned short addr)
{
    return 0;
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

void
exit_port_handler(portaddr p, byte v)
{
    printf("exit port tickled %x\n", v);
    exit(0);
}

#define ALLMEM  64*1024
void
dump_port_handler(portaddr p, byte v)
{
    int fd;
    int i;
    char *dumpbuf = malloc(ALLMEM);

    printf("dump port tickled %x\n", v);
    fd = creat("dumpfile", 0777);
    for (i = 0; i < ALLMEM; i++) {
        dumpbuf[i] = get_word(i);
    }
    write(fd, dumpbuf, ALLMEM);
    write(fd, &PC_REG, 2);
    write(fd, &SP_REG, 2);
    write(fd, &F_REG, 1);
    write(fd, &A_REG, 1);
    write(fd, &B_REG, 1);
    write(fd, &C_REG, 1);
    write(fd, &D_REG, 1);
    write(fd, &E_REG, 1);
    write(fd, &H_REG, 1);
    write(fd, &L_REG, 1);
    write(fd, &IX_REG, 2);
    write(fd, &IY_REG, 2);
    write(fd, &I_REG, 2);
    write(fd, &R_REG, 2);
    write(fd, &BUS_STATUS, 1);
    write(fd, cpureg.sbits, sizeof(cpureg.sbits));
    write(fd, &BUS_CONTROL, 1);
    write(fd, cpureg.cbits, sizeof(cpureg.cbits));
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

void (*poll_hook[MAXDRIVERS])();        // this gets called between instructions
int (*prearg_hook[MAXDRIVERS])();       // called just before arg processing
int (*startup_hook[MAXDRIVERS])();      // called just before emulation
void (*usage_hook[MAXDRIVERS])();       // called inside usage()

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

void
register_prearg_hook(int (*func)())
{
    int i;

    for (i = 0; i < MAXDRIVERS; i++) {
        if (!prearg_hook[i]) {
            prearg_hook[i] = func;
            return;
        }
    }
    exit(2);
}

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
register_usage_hook(void (*func)())
{
    int i;

    for (i = 0; i < MAXDRIVERS; i++) {
        if (!usage_hook[i]) {
            usage_hook[i] = func;
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
    fprintf(stderr, "usage: %s [<options>] <drive file> ...\n", p);
    fprintf(stderr, "\t-b\t<boot rom file>\n");
    fprintf(stderr, "\t-c\t<configuration switch value>\n");
    fprintf(stderr, "\t-x\topen a debug terminal window\n");
    fprintf(stderr, "\t-s\t\tstop before execution\n");
    fprintf(stderr, "\t-t <tracebits>\n");
    for (i = 0; tracenames[i]; i++) {
        fprintf(stderr, "\t%x %s\n", 1 << i, tracenames[i]);
    }
    for (i = 0; i < MAXDRIVERS; i++) {
        if (usage_hook[i]) {
            (*usage_hook[i])();
        }
    }
    exit(1);
}

char **drivenames;
char *rom_filename;
char *rom_image;
int rom_size;
int config_sw = 0;

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
    byte f;
    char outbuf[40];
    char fbuf[9];
    char *s;
    int i;
    word bc, de, hl;

    strcpy(fbuf, "        ");

    format_instr(PC_REG, outbuf, &get_byte, &lookup_sym, &reloc);
    s = lookup_sym(PC_REG);
    if (s) {
        printf("%s\n", s);
    }
    printf("%04x: %-20s ", PC_REG, outbuf);

    f = F_REG;

    if (f & C_FLAG)
        fbuf[0] = 'C';
    if (f & N_FLAG)
        fbuf[1] = 'N';
    if (f & PV_FLAG)
        fbuf[2] = 'V';
    if (f & X_FLAG)
        fbuf[2] = 'X';
    if (f & H_FLAG)
        fbuf[3] = 'H';
    if (f & Y_FLAG)
        fbuf[4] = 'Y';
    if (f & Z_FLAG)
        fbuf[5] = 'Z';
    if (f & S_FLAG)
        fbuf[6] = 'S';

    printf(
        " %s a:%02x bc:%04x de:%04x hl:%04x ix:%04x iy:%04x sp:%04x tos:%04x\n",
        fbuf,
        A_REG, BC_REG, DE_REG, HL_REG, IX_REG, IY_REG, SP_REG, get_word(SP_REG));
}

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
            return BC_REG;
        }
        if (strcasecmp(&wordbuf[1], "de") == 0) {
            return DE_REG;
        }
        if (strcasecmp(&wordbuf[1], "hl") == 0) {
            return HL_REG;
        }
        if (strcasecmp(&wordbuf[1], "ix") == 0) {
            return IX_REG;
        }
        if (strcasecmp(&wordbuf[1], "iy") == 0) {
            return IY_REG;
        }
        if (strcasecmp(&wordbuf[1], "pc") == 0) {
            return IY_REG;
        }
        if (strcasecmp(&wordbuf[1], "sp") == 0) {
            return SP_REG;
        }
        if (strcasecmp(&wordbuf[1], "tos") == 0) {
            return get_word(SP_REG);
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

#ifdef notdef
// system call trace for micronix
void
scall_trace(char **sp)
{
    char c;
    int i;

    c = 1;
    skipwhite(sp);
    if (!**sp) {
        for (i = 0; i < sizeof(stops); i++) {
            if ((i % 16) == 0)
                printf("\n%02d: ", i);
            printf("%03d ", stops[i]);
        }
        printf("\n");
    }
    if (**sp == '-') {
        (*sp)++;
        c = 0;
    }
    if (**sp) {
        i = getaddress(sp);
    }
    if (i) {
        if (i < 0) {
            i = -i;
            c = 0;
        }
        if (i < sizeof(stops)) {
            stops[i] = c;
        }
    }
    return 0;
}
#endif

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
            i = PC_REG;
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
    return 0;
}

/*
 * breakpoints and are done with an 8k bitmap
 */
char breaks[8192];          // if set, stop
int nbreaks = 0;            // number of breakpoints

int
breakpoint_at(vaddr a)
{
    if (nbreaks && (breaks[a / 8] & (1 << (a % 8)))) {
        return 1;
    } else {
        return 0;
    }
}

int
break_cmd(char **sp)
{
    int i;
    int b;
    vaddr addr;
    int delete = 0;
 
    if (!**sp) {
        printf("%d breakpoints\n", nbreaks);
        if (!nbreaks) {
            return 0;
        }
        for (i = 0; i < 8192; i++) {
            if (breaks[i]) {
                for (b = 0; b < 8; b++) {
                    if (breaks[i] & (1 << b)) {
                        char *s;
                        addr = i * 8 + b;
                        s = lookup_sym(addr);
                        if (s) {
                            printf("%04x %s\n", addr, s);
                        } else {
                            printf("%04x\n", addr);
                        }
                    } 
                }
            }
        }
        return 0;
    }
    if (**sp == '-') {
        (*sp)++;
        delete = 1;
    }

    i = 0;
    while (**sp) {
        skipwhite(sp);
        addr = getaddress(sp);
        if (delete) {
            if (breakpoint_at(addr)) nbreaks--;
            breaks[addr/8] &= ~(1 << (addr % 8));
        } else {
            if (!breakpoint_at(addr)) nbreaks++;
            breaks[addr/8] |= (1 << (addr % 8));
        }
        i++;
    }
    if (delete && !i) {
        for (i = 0; i < 8192; i++) {
            breaks[i] = 0;
            nbreaks = 0;
        }
    }
    return 0;
}

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
        PC_REG = strtol(*sp, sp, 16);
    }
    inst_countdown = -1;
    return 1;
}

int
trace_cmd(char **sp)
{
    int k = trace;
    char *s = "trace set to:\n";
    int i;

    if (**sp == '?') {
        s = "trace can be:\n";
        k = -1;
    } else if (**sp) {
        trace = strtol(*sp, sp, 16);
        k = trace;
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
    { 'g', " [address]\tgo", go_cmd },
    { 'r', "\tdump registers", regs_cmd },
    { 't', "trace\tset trace", trace_cmd },
    { 'q', "\tquit", exit_cmd },
    { 'x', "\texit", exit_cmd },
    { 'h', "\thelp", help_cmd },
    { '?', "\thelp", help_cmd },
    { 0, 0, 0 }
};

int
main(int argc, char **argv)
{
    char *progname = *argv++;
    char *s;
    char **argvec;
    int i;
    char *ptyname;
    int fd;

    /*
     * run the driver startup hooks before argument processing
     * to set defaults, etc before command line options get to
     * override them
     */
    for (i = 0; i < MAXDRIVERS; i++) {
        if (prearg_hook[i]) {
            if ((*prearg_hook[i])()) {
                printf("prearg hook %d failed\n", i);
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
            case 'x':
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
            case 't':
                if (!argc--) {
                    usage("trace not specified \n", progname);
                }
                trace = strtol(*argv++, 0, 0);
                break;
            case 's':
                inst_countdown = 0;
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

    /*
     * load the boot rom if there is one
     */
    if (rom_size) {
        rom_image = malloc(rom_size);
        fd = open(rom_filename, O_RDONLY);
        if (!fd) {
            perror(rom_filename);
            exit(errno);
        }
        i = read(fd, rom_image, rom_size);
        if (i < 0) {
            perror(rom_filename);
            exit(errno);
        }
        close(fd);
        rom_filename = strdup(rom_filename);
        i = strlen(rom_filename);
        // if there's a similarly named symfile, use it
        if (rom_filename[i-4] == '.') {
            strcpy(&rom_filename[i-3], "sym");
            load_symfile(rom_filename);
        }
    }

    mytty = strdup(ttyname(0));
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
        // we need to capture the tty name so we can send output to it
        sprintf(cmd,
            "bash -c 'tty > /proc/%d/fd/%d ; while test -d /proc/%d ; do sleep 1 ; done ; sleep 60'",
            mypid, pipefd[1], mypid);
        if (!fork()) {
            // try terminals in order of preference
            execlp("xfce4-terminal", "xfce4-terminal", 
                "--disable-server",
                "--command", cmd, 
                (char *) 0);

            execlp("mate-terminal", "mate-terminal", 
                "--sm-client-disable",
                "--disable-factory",
                "--command", cmd, 
                (char *) 0);

            unlink("logfile");

            execlp("xterm", "xterm", 
                "-geometry", "120x40", 
                "-fn", "8x13bold", 
                "-l", "-lf", "logfile",
                "-e", "bash", 
                "-c", cmd, 
                (char *) 0);
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
    } else {
        stdout = freopen("logfile", "w+", stdout);
        if (!stdout) {
            perror("lose");
        }
        setvbuf(stdout, 0, _IONBF, 0);
        printf("log file\n");
    }

    if (trace) {
        printf("trace %x ", trace);
        for (i = 0; tracenames[i]; i++) {
            if (trace & (1 << i)) {
                printf("%s ", tracenames[i]);
            }
        }
        printf("\n");
    }

    signal(SIGUSR1, stop_handler);

    setup_sim_ports();
    z80_init(&cpureg);

    // another driver hook
    for (i = 0; i < MAXDRIVERS; i++) {
        if (startup_hook[i]) {
            if ((*startup_hook[i])()) {
                printf("startup hook %d failed\n", i);
                exit(1);
            }
        }
    }

    /*
     * the main emulation loop
     */
    while (1) {

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
        if (breakpoint_at(PC_REG)) {
            printf("break at %04x\n", PC_REG);
            inst_countdown = 0;
        }
        if ((trace & trace_inst) || (inst_countdown == 0)) {
            dumpcpu();
        }
        if (inst_countdown == 0) {
            monitor();
        }

#ifdef notdef
        /*
         * if we know we aren't debugging, don't bother to pop up here
         * otherwise, run for 1 instruction so we get control to check for breakpoints
         */
        if ((nbreaks == 0) && (inst_countdown == -1) && !(trace & trace_inst)) {
            i = 1000000;
        } else {
            i = 1;
        }
#endif
        z80_run();
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
