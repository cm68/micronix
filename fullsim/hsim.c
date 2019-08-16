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
    "V_IO", "V_INST", "V_IOR", "V_MAP", "V_DJDMA", "V_MIO", "V_HDCA", "V_MPZ", "V_IMD", "V_BIO", 0
};

int verbose;

jmp_buf inst_start;
MACHINE cpu;

/*
 * the S100 bus memory space
 */
byte physmem[16*1024*1024];

volatile int breakpoint;

int stops[10];

void
stop()
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

unsigned char 
get_vbyte(long addr)
{
}

static void
push(unsigned short s)
{
    cpu.registers.word[Z80_SP] -= 2;
    put_word(cpu.registers.word[Z80_SP], s);
}

static unsigned short
pop()
{
    unsigned short i;

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

int (*prearg_hook[MAXDRIVERS])();       // called just before arg processing
int (*startup_hook[MAXDRIVERS])();      // called just before emulation
void (*poll_hook[MAXDRIVERS])();        // this gets called between instructions

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
usage(char *complaint, char *p)
{
    int i;

    fprintf(stderr, "%s", complaint);
    fprintf(stderr, "usage: %s [<options>]\n",
        p);
    fprintf(stderr, "\t-d\t<drive file> (may be repeated)\n");
    fprintf(stderr, "\t-b\t<boot rom file>\n");
    fprintf(stderr, "\t-t\topen a debug terminal window\n");
    fprintf(stderr, "\t-s\t\tstop before execution\n");
    fprintf(stderr, "\t-v <verbosity>\n");
    for (i = 0; vopts[i]; i++) {
        fprintf(stderr, "\t%x %s\n", 1 << i, vopts[i]);
    }
    exit(1);
}

char **drivenames;
char *bootrom;
char *rom_image;
int rom_size;

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
            case 'd':
                if (!argc--) {
                    usage("drive file\n", progname);
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
                for (i = 0; drivenames[i]; i++)
                    printf("drivenames %d %s\n", i, drivenames[i]); 
                break;
            case 'b':
                if (!argc--) {
                    usage("boot rom name missing\n", progname);
                }
                bootrom = strdup(*argv++);
                break;
            case 'v':
                if (!argc--) {
                    usage("verbosity not specified \n", progname);
                }
                verbose = strtol(*argv++, 0, 0);
                break;
            case 's':
                breakpoint++;
                break;
            default:
                usage("unrecognized option", progname);
                break;
            }
        }
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
        fd = open(bootrom, O_RDONLY);
        if (!fd) {
            perror(bootrom);
            exit(errno);
        }
        i = read(fd, rom_image, rom_size);
        if (i < 0) {
            perror(bootrom);
            exit(errno);
        }
        close(fd);
        bootrom = strdup(bootrom);
        i = strlen(bootrom);
        // if there's a similarly named symfile, use it
        if (bootrom[i-4] == '.') {
            strcpy(&bootrom[i-3], "sym");
            load_symfile(bootrom);
        }
    }

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
            "tty > /proc/%d/fd/%d ; while test -d /proc/%d ; do sleep 1 ; done ; sleep 60",
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
    unsigned char f;
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
 * breakpoints and watchpoints are handled using the same data structure
 */
struct point
{
    vaddr addr;
    byte value;
    struct point *next;
};

struct point *breaks;
struct point *watches;

int
watchpoint_hit()
{
    struct point *p;
    byte n;

    for (p = watches; p; p = p->next) {
        if (p->value == -1) {
            p->value = get_byte(p->addr);
        }
        n = get_byte(p->addr);
        if (n != p->value) {
            printf("value %02x at %04x changed to %02x\n",
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
 * command line processor for monitor/debugger
 * this might want to have plugins, too, for board-specific regs
 */
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
                for (i = 0; i < sizeof(stops); i++) {
                    if ((i % 16) == 0)
                        printf("\n%02d: ", i);
                    printf("%03d ", stops[i]);
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
                if (i < sizeof(stops)) {
                    stops[i] = c;
                }
            }
            break;
#ifdef notdef
        case 'm':                   // dump map
            skipwhite(&s);
            if (*s) {
                i = strtol(s, &s, 16);
            } else {
                i = taskreg & 0xf;
            }
            printmap(i);
            break;
#endif
        case 'd':                   // dump memory
            skipwhite(&s);
            if (*s) {
                i = getaddress(&s);
            } else {
                if (lastaddr == -1) {
                    i = cpu.registers.word[Z80_SP];
                } else {
                    i = lastaddr;
                }
            }
            dumpmem(&get_vbyte, i, 256);
            lastaddr = (i + 256) & 0xfff;
            break;
        case 'l':                   // list
            skipwhite(&s);
            if (*s) {
                i = getaddress(&s);
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
#ifdef notdef
            printf("m [taskid] : dump mapping\n");
#endif
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
 * this is the cpu emulator
 */
static void
emulate_z80()
{
    unsigned char *ip;
    int i;

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

        if (watchpoint_hit()) {
            breakpoint = 1;
        }
        if (point_at(&breaks, cpu.pc, 0)) {
            if (point_at(&breaks, cpu.pc, 0)) {
                pid();
                printf("break at %04x\n", cpu.pc);
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
        // setjmp(&inst_start);

        Z80Emulate(&cpu, 1, &cpu);
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
