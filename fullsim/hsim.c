/* 
 * this is the general emulator framework
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

typedef unsigned int ULONG;
typedef unsigned char UCHAR;
typedef unsigned short UINT;

#define EXIT_PORT   0
#define DUMP_PORT   1

#define	LISTLINES	8
#define	STACKTOP	0xffff
#define MAXIMUM_STRING_LENGTH   100

static void emulate_z80();

int debug_terminal;
int mypid;
char *mytty;

// needs to be sync'd with sim.h
char *vopts[] = {
    "V_IO", "V_INST", "V_IOR", "V_MAP", 
    "V_DJDMA", "V_MIO", "V_HDCA", "V_MPZ", 
    "V_IMD", "V_BIO", "V_HDDMA", 0
};

int verbose;

jmp_buf inst_start;
MACHINE cpu;

/*
 * the S100 bus memory space
 */
byte physmem[16*1024*1024];

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

unsigned short
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

/*
 * access memory in the 24 bit S100 space using a physical address
 */
byte 
physread(paddr p)
{
    p &= 0xffffff;
    return physmem[p];
}

void
physwrite(paddr p, byte v)
{
    p &= 0xffffff;
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
put_word(vaddr addr, word value)
{
    memwrite(addr, value);
    memwrite(addr + 1, value >> 8);
}

void
put_byte(vaddr addr, byte value)
{
    memwrite(addr, value);
}

word
get_word(vaddr addr)
{
    return memread(addr) + (memread(addr + 1) << 8);
}

byte
get_byte(vaddr addr)
{
    return memread(addr);
}

static void
push(word s)
{
    cpu.registers.word[Z80_SP] -= 2;
    put_word(cpu.registers.word[Z80_SP], s);
}

static word
pop()
{
    word i;

    i = get_word(cpu.registers.word[Z80_SP]);
    cpu.registers.word[Z80_SP] += 2;
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
void
dump_port_handler(portaddr p, byte v)
{
    int fd;
    int i;
    char *dumpbuf = malloc(ALLMEM);

    printf("dump port tickled %x\n", v);
    fd = creat("dumpfile", 0777);
    for (i = 0; i < ALLMEM; i++) {
        dumpbuf[i] = memread(i);
    }
    write(fd, dumpbuf, ALLMEM);
    write(fd, &cpu.pc, 2);
    write(fd, &cpu.registers.byte[Z80_A], 1);
    write(fd, &cpu.registers.byte[Z80_F], 1);
    write(fd, &cpu.registers.word[Z80_BC], 2);
    write(fd, &cpu.registers.word[Z80_DE], 2);
    write(fd, &cpu.registers.word[Z80_HL], 2);
    write(fd, &cpu.registers.word[Z80_SP], 2);
    write(fd, &cpu.registers.word[Z80_IX], 2);
    write(fd, &cpu.registers.word[Z80_IY], 2);
    close(fd);
    free(dumpbuf);
}

#define INPUT_PORT 2

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

byte
undef_in(portaddr p)
{
    printf("input from undefined port %x\n", p);
    stop();
    return 0xff;
}

void
undef_out(portaddr p, byte v)
{
    stop();
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
    fprintf(stderr, "\t-t\topen a debug terminal window\n");
    fprintf(stderr, "\t-s\t\tstop before execution\n");
    fprintf(stderr, "\t-v <verbosity>\n");
    for (i = 0; vopts[i]; i++) {
        fprintf(stderr, "\t%x %s\n", 1 << i, vopts[i]);
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
            case 't':
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
            case 'v':
                if (!argc--) {
                    usage("verbosity not specified \n", progname);
                }
                verbose = strtol(*argv++, 0, 0);
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
            "tty > /proc/%d/fd/%d ; while test -d /proc/%d ; do sleep 1 ; done ; sleep 60",
            mypid, pipefd[1], mypid);
        if (!fork()) {
            execlp("xterm", "xterm", 
                "-geometry", "120x40", 
                "-fn", "8x13bold", 
                "-e", "bash", 
                "-c", cmd, (char *) 0);
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

    if (verbose) {
        printf("verbose %x ", verbose);
        for (i = 0; vopts[i]; i++) {
            if (verbose & (1 << i)) {
                printf("%s ", vopts[i]);
            }
        }
        printf("\n");
    }

    signal(SIGUSR1, stop_handler);
    ioinit();
    register_output(EXIT_PORT, exit_port_handler);
    register_output(DUMP_PORT, dump_port_handler);
    register_input(INPUT_PORT, pip_input_handler);

    Z80Reset(&cpu);

    // another driver hook
    for (i = 0; i < MAXDRIVERS; i++) {
        if (startup_hook[i]) {
            if ((*startup_hook[i])()) {
                printf("startup hook %d failed\n", i);
                exit(1);
            }
        }
    }

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
    byte f;
    char outbuf[40];
    char fbuf[9];
    char *s;
    int i;

    strcpy(fbuf, "        ");

    format_instr(cpu.pc, outbuf, &get_byte, &lookup_sym, &reloc);
    s = lookup_sym(cpu.pc);
    if (s) {
        printf("%s\n", s);
    }
    printf("%04x: %-20s ", cpu.pc, outbuf);

    f = cpu.registers.byte[Z80_F];

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
        cpu.registers.byte[Z80_A],
        cpu.registers.word[Z80_BC],
        cpu.registers.word[Z80_DE],
        cpu.registers.word[Z80_HL],
        cpu.registers.word[Z80_IX],
        cpu.registers.word[Z80_IY],
        cpu.registers.word[Z80_SP],
        get_word(cpu.registers.word[Z80_SP]));
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
        if (strcasecmp(&wordbuf[1], "hl") == 0) {
            return cpu.registers.word[Z80_HL];
        }
        if (strcasecmp(&wordbuf[1], "bc") == 0) {
            return cpu.registers.word[Z80_BC];
        }
        if (strcasecmp(&wordbuf[1], "de") == 0) {
            return cpu.registers.word[Z80_DE];
        }
        if (strcasecmp(&wordbuf[1], "sp") == 0) {
            return cpu.registers.word[Z80_SP];
        }
        if (strcasecmp(&wordbuf[1], "tos") == 0) {
            return get_word(cpu.registers.word[Z80_SP]);
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
            i = cpu.pc;
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
            return;
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
        return;
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
            i = cpu.registers.word[Z80_SP];
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
        cpu.pc = strtol(*sp, sp, 16);
    }
    inst_countdown = -1;
    return 1;
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
    { 'q', "\tquit", exit_cmd },
    { 'x', "\texit", exit_cmd },
    { 'h', "\thelp", help_cmd },
    { '?', "\thelp", help_cmd },
    { 0, 0, 0 }
};

/*
 * this is the cpu emulator main loop
 */
static void
emulate_z80()
{
    unsigned char *ip;
    int i;

    setjmp(&inst_start);

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
        if (breakpoint_at(cpu.pc)) {
            printf("break at %04x\n", cpu.pc);
            inst_countdown = 0;
        }
        if ((verbose & V_INST) || (inst_countdown == 0)) {
            dumpcpu();
        }
        if (inst_countdown == 0) {
            monitor();
            continue;
        }

        /*
         * if we know we aren't debugging, don't bother to pop up here
         * otherwise, run for 1 instruction so we get control to check for breakpoints
         */
#ifdef notdef
        if ((nbreaks == 0) && (inst_countdown == -1) && !(verbose & V_INST)) {
            i = 1000000;
        } else {
            i = 1;
        }
#endif
        i = 1;
        Z80Emulate(&cpu, i, &cpu);
        if (inst_countdown != -1) {
            inst_countdown--;
        }
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
