/*
 * micronix. this emulates the micronix user mode 
 *
 * usersim/usersim.c
 *
 * Changed: <2023-07-29 09:57:12 curt>
 *
 * Copyright (c) 2018, Curt Mayer 
 * do whatever you want, just don't claim you wrote it. 
 * warrantee: madness! nope. 
 *
 * plugs into the z80emu code from: 
 *  Copyright (c) 2012, 2016 * Lin Ke-Fong 
 *  Copyright (c) 2012 Chris Pressey This code is free, do
 * whatever you want with it. 
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#define trace xxtrace
#include <curses.h>
#undef trace
#include <pthread.h>

#include "../micronix/include/types.h"
#include "../micronix/include/sys/fs.h"

#include "../include/sim.h"
#include "../include/disz80.h"
#include "../include/util.h"
#include "../include/mnix.h"
#include "../include/fslib.h"
#include "../include/gui.h"

#ifdef __APPLE__
    typedef void (*sighandler_t)(int);
#endif

/*
 * include files from micronix source
 */
#include "../micronix/include/obj.h"

#define	LISTLINES	8
#define	STACKTOP	0xffff
#define MAXIMUM_STRING_LENGTH   100

static int do_exec(char *name, char **argv);
static void emulate();
void SystemCall();

#define	DEFROOT	"filesystem"

extern WINDOW **win;

int traceflags;
int debug_terminal;
int am_root = 1;
int mypid;
FILE *tty;
extern int logfd;

char curdir[1000] = "";
char *rootdir;
char *execprog;
ino_t rootinode;

/* i/o buffer used for real system calls */
char iobuf[65536];

struct stat sbuf;

char *initfile[] = {
    "/bin/sh",
    0
};

#define	V_SYS	(1 << 0)        /* trace system calls */
#define	V_DATA	(1 << 1)        /* dump bulk data from system calls */
#define	V_EXEC	(1 << 2)        /* exec args */
#define	V_INST	(1 << 3)        /* instructions */
#define	V_ASYS	(1 << 4)        /* even small syscalls */
#define	V_SYS0	(1 << 5)        /* dump raw syscall args */
#define	V_ERROR	(1 << 6)        /* perror on system calls */
#define V_SFAIL (1 << 7)        /* breakpoint on syscall fail */
#define V_FUNC  (1 << 8)        /* trace functions */
#define V_FUNC0 (1 << 9)        /* skip tracing c helpers */

/*
 * memory driver for trivial 64k used in usersim
 */
unsigned char memory[65536];

void
put_byte(unsigned short addr, unsigned char value)
{
    if (watchpoint_at(addr)) {
        message("watchpoint %04x\n", addr);
        watchpoint_touched = 1;
    }
    memory[addr] = value;
}

void
put_word(unsigned short addr, unsigned short value)
{

    put_byte(addr, value & 0xff);
    put_byte(addr+1, (value >> 8) & 0xff);
}

unsigned short
get_word(unsigned short addr)
{
    return memory[addr] + (memory[addr + 1] << 8);
}

unsigned char
get_byte(unsigned short addr)
{
    addr &= 0xffff;
    return memory[addr];
}

unsigned char
input(unsigned short p)
{
    return 0;
}

void
output(unsigned short p, unsigned char v)
{
}

unsigned char
int_ack()
{
    return 0;
}

void
copyout(byte *buf, paddr pa, int len)
{
    while (len--) {
        put_byte(pa++, *buf++);
    }
}

void
copyin(byte *buf, paddr pa, int len)
{
    while (len--) {
        *buf++ = get_byte(pa++);
    }
}

static void
push(unsigned short s)
{
    unsigned short sp;

    sp = z80_get_reg16(sp_reg);
    sp -= 2;
    z80_set_reg16(sp_reg, sp);

    put_word(sp, s);
}

static unsigned short
pop()
{
    unsigned short i;
    unsigned short sp;

    sp = z80_get_reg16(sp_reg);
    i = get_word(sp);
    sp += 2;
    z80_set_reg16(sp_reg, sp);

    return (i);
}

char *progname;

char *vopts[] = {
    "V_SYS", "V_DATA", "V_EXEC", "V_INST", 
    "V_ASYS", "V_SYS0", "V_ERROR", "V_SFAIL", 
    "V_FUNC", "V_FUNC0", 0
};

char *seekoff[] = { "SET", "CUR", "END" };

int verbose;

unsigned short brake;
volatile int breakpoint;

char namebuf[PATH_MAX];
char workbuf[PATH_MAX];

#define MAXFILE 63
/*
 * colossal hack for special files
 * the file offset to do alternate sector skew
 */
struct openfile {
    int major;
    int minor;
    char dt;
    int special;        // is a special file needing sector hackery
    long offset;         // notional file offset
    int filesize;
} files[MAXFILE + 1];

#define SPT 15

int
seekfile(int fd)
{
    int trk;
    int sec;
    int blkno;
    int blkoff;
    long new;

    if ((fd < 0) || (fd > MAXFILE)) {
        errno = EBADF;
        return -1;
    }

    if ((files[fd].dt == 'b') && (files[fd].offset > files[fd].filesize)) {
        errno = ENXIO;
        return -1;
    }

//message("seekfile fd: %d %d\n", fd, files[fd].offset);
    if ((files[fd].dt != 'b') || 
        (files[fd].major != 2) || 
        ((files[fd].minor & 0x8) == 0)) {
        lseek(fd, files[fd].offset, SEEK_SET);
        return 0;
    }
    
    blkno = files[fd].offset / 512;
    blkoff = files[fd].offset % 512;

    trk = blkno / SPT;
    sec = blkno % SPT;
    sec *= 2;
    if (!(SPT & 1) && sec >= SPT) sec++;    // always false

    sec %= SPT;

    new = (((trk * SPT) + sec) * 512) + blkoff;
    lseek(fd, new, SEEK_SET);
    return 0;
}

/*
 * translate our sim filename into a native filename
 * by prepending the root - we need to be a little smart
 * about this because relative paths need to first have
 * the current working directory prepended
 * this needs to be hella complicated because chroot is privileged - wtf
 */
char *
fname(char *orig)
{
    char *slash;

    /*
     * empty path is . 
     */
    if (strlen(orig) == 0) {
        sprintf(workbuf, "%s/%s", rootdir, curdir);
    } else if (*orig == '/') {
        sprintf(workbuf, "%s/%s", rootdir, orig);
    } else {
        sprintf(workbuf, "%s/%s/%s", rootdir, curdir, orig);
    }
    /*
     * now we get really stupid. we use symlinks on the last component
     * to fake out cdev and bdevs, so only resolve up to that point
     */
    slash = strrchr(workbuf, '/');
    *slash = '\0'; 
    realpath(workbuf, namebuf);
    *slash = '/';
    strcat(namebuf, slash);
    return (namebuf);
}

void
stop_handler()
{
    message("breakpoint signal\n");
    breakpoint = 1;
}

void
pid()
{
#ifdef notdef
    message("%x: ", mypid);
#endif
}

unsigned int
get_reloc(unsigned short addr)
{
    return 0;
}

#define	TTY_FD	64

void
usage(char *complaint, char *arg)
{
    int i;

    fprintf(stderr, "%s%s%s", 
        complaint ? complaint : "", 
        arg ? arg : "",
        (arg || complaint) ? "\n" : "");

    fprintf(stderr, "usage: %s [<options>] [program [<program options>]]\n",
        progname);
    fprintf(stderr, "\t-r\trun as root\n");
    fprintf(stderr, "\t-T\topen a debug terminal window\n");
    fprintf(stderr, "\t-d <root dir>\n");
    fprintf(stderr, "\t-b\t\tstart with breakpoint\n");
    fprintf(stderr, "\t-v <verbosity>\n");
    for (i = 0; vopts[i]; i++) {
        fprintf(stderr, "\t%2x %-8s", 1 << i, vopts[i]);
        if ((i % 4) == 3) fprintf(stderr, "\n");
    }
    if ((i % 4) != 0) fprintf(stderr, "\n");
    fprintf(stderr, "\t-s [<syscall>[=<count>]\n");
    fprintf(stderr, "\t-t <syscall>\n");
    for (i = 0; syscalls[i].name; i++) {
        fprintf(stderr, "\t%2d %-8s", i, syscalls[i].name);
        if ((i % 4) == 3) fprintf(stderr, "\n");
    }
    if ((i % 4) != 0) fprintf(stderr, "\n");
    exit(1);
}

/*
 * trace and stops are similar: when a system call happens that we registered for,
 * we dump and possibly stop.  we stop if the counter goes to zero
 */
extern char sys_stop[NSYS];
extern char sys_trace[NSYS];

void
pverbose()
{
    int i;

    message("verbose %x ", verbose);
    for (i = 0; vopts[i]; i++) {
        if (verbose & (1 << i)) {
            message("%s ", vopts[i]);
        }
    }
    message("\n");
}

int
main(int argc, char **argv)
{
    char *s;
    char **argvec;
    int i;
    int j;
    char *ttyname;
    struct point *p;
    char rpath[100];

    progname = *argv++;
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
            case 'h':
                usage(0, 0);
                break;
            case 'T':
                debug_terminal = 1;
                break;
            case 'r':
                am_root = 1;
                break;
            case 'd':
                if (!argc--) {
                    usage("directory not specified\n", 0);
                }
                rootdir = *argv++;
                break;
            case 'v':
                if (!argc--) {
                    usage("verbosity not specified\n", 0);
                }
                verbose = strtol(*argv++, 0, 0);
                break;
            case 's':
                if (!argc--) {
                    usage("stop syscall not specified\n", 0);
                    break;
                }
                s = *argv++;
                while (*s) {
                    i = get_syscall(&s);
                    if (i == -1) {
                        usage("unrecognized system call\n", s);
                        break;
                    }
                    j = 1;
                    if (*s == '=') {
                        s++;
                        j = strtol(s, &s, 10);
                    }
                    sys_stop[i] = j;
                    if (*s != ',') break;
                    s++;
                }
                break;
            case 't':
                if (!argc--) {
                    usage("trace syscall not specified\n", 0);
                    break;
                }
                s = *argv++;
                while (*s) {
                    i = get_syscall(&s);
                    if (i == -1) {
                        usage("unrecognized system call\n", s);
                        break;
                    }
                    sys_trace[i] = 1;
                    if (*s != ',') break;
                    s++;
                }
                break;
            case 'b':
                breakpoint++;
                break;
	    case 'p':
		argc--;
		execprog = *argv++;
		break;
            default:
                usage("unrecognized option :", --s);
                break;
            }
        }
    }
    if (!argc) {
        argc = 1;
        argv = initfile;
    }

    if (!rootdir) {
        rootdir = getenv("MICRONIX_ROOT");
    }

    if (!rootdir) {
        rootdir = DEFROOT;
    }

    /*
     * something of a hack: let's look for our rootdir if there isnt one
     * right here
     */
    for (i = 0; i < 4; i++) {
        rpath[0] = '\0';
        for (j = 0; j < i; j++) {
            strcat(rpath, "../");
        }
        strcat(rpath, rootdir);
        if ((stat(rpath, &sbuf) == 0) && 
            ((sbuf.st_mode & S_IFMT) == S_IFDIR)) {
            rootdir = strdup(rpath);
            break;
        }
    }

    /*
     * make our shared memory segments
     */
    initpids();
    initinums();

    /*
     * make our rootdir absolute
     */
    chdir(rootdir);
    getcwd(workbuf, sizeof(workbuf));
    realpath(workbuf, namebuf);
    rootdir = strdup(namebuf);
    lstat(rootdir, &sbuf);
    rootinode = sbuf.st_ino;
    allocinum(rootinode);

    if (verbose) {
        pverbose();
        message("emulating %s with root %s\n", argv[0], rootdir);
    }

    mypid = getpid();

    /*
     * we might be piping the simulator.  let's get an open file for our 
     * debug output and monitor functions.  finally, let's make sure the 
     * file descriptor is out of range of the file descriptors our emulation
     * uses.
     * this is so that we can debug interactive stuff that might be 
     * writing/reading from stdin, and we want all our debug output to go to 
     * a different terminal, one that isn't running a shell.  
     * also, if we specified to open a debug window, let's connect the 
     * emulator's file descriptors to an xterm or something.
     */
    if (debug_terminal) {
        int pipefd[2];
        char *cmd = malloc(100);
        char *args[30];
        char *s;

        char **argp = args;
        *argp++ = "xterm";
        *argp++ = "-T";
        *argp++ = "debug";
        s = getenv("DEBUG_GEOMETRY");
        if (!s) s = "80x80";
        *argp++ = "-geometry";
        *argp++ = strdup(s);
        *argp++ = "-fa";
        *argp++ = "Monospace";
        *argp++ = "-fs";
        s = getenv("DEBUG_FONTSIZE");
        if (!s) s = "12";
        *argp++ = strdup(s);
        *argp++ = "-e";
        *argp++ = "bash";
        *argp++ = "-c";
        *argp++ = cmd;
        *argp++ = (char *) 0;

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
            execvp("xterm", args);
        }
    } else {
        ttyname = "/dev/stdin";
        win = 0;
    }
    tty = fopen(ttyname, "r+");
    if (!tty) {
        perror(ttyname);
        exit(errno);
    }
    dup2(fileno(tty), TTY_FD);
    tty = fdopen(TTY_FD, "r+");
    logfd = fileno(tty);
    setvbuf(tty, 0, _IOLBF, 0);
    signal(SIGUSR1, stop_handler);

    if (debug_terminal) {
        makewins(tty);
    }

    mon_init();

	z80_init();

    /*
     * build argvec 
     */
    argvec = malloc((argc + 1) * sizeof(char *));
    for (i = 0; i < argc; i++) {
        argvec[i] = argv[i];
    }
    argvec[i] = 0;

    if (do_exec(fname(argvec[0]), argvec)) {
        return (EXIT_FAILURE);
    }
    free(argvec);

    z80_set_reg16(pc_reg, pop());
    // dumpcpu();
    emulate();
    return EXIT_SUCCESS;
}

/*
 * signal stuff
 */
unsigned short signalled;
unsigned short signal_handler[16];

void
schedule_signal(unsigned short a)
{
    signalled |= (1 << a);
}

/*
 * tty io signals are done in a fairly lame way - we poll on an alarm
 */
void
alarm_handler(int i)
{
    fd_set rfd;
    struct timeval tv;

    // message("sigalarm!\n");

    tv.tv_usec = 0;
    tv.tv_sec = 0;
    FD_ZERO(&rfd);
    FD_SET(0, &rfd);

    if (select(1, &rfd, 0, 0, &tv)) {
        schedule_signal(7);
    }
}

void
int_handler()
{
    schedule_signal(2);
}

void
quit_handler()
{
    schedule_signal(3);
}

void
bg_handler()
{
    schedule_signal(6);
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

struct symbol
{
    char *name;
    unsigned short value;
    char type;
    struct symbol *next;
};
struct symbol *syms;

char *
get_symname(unsigned short value)
{
    struct symbol *s;

    for (s = syms; s; s = s->next) {
        if (s->value == value)
            return s->name;
    }
    return 0;
}

void
add_sym(char *name, int type, unsigned short value)
{
    struct symbol *s;

    // printf("add_sym %x %x %s\n", type, value, name);
    s = malloc(sizeof(*s));
    s->name = strdup(name);
    s->value = value;
    s->type = type;
    s->next = syms;
    syms = s;
}

unsigned short
lookup_sym(char *name)
{
    struct symbol *s;

    for (s = syms; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            return s->value;
        }
    }
    return 0;
}

/*
 * this is the exec function - slightly different from the standard unix
 */
static int
do_exec(char *name, char **argv)
{
    FILE *file;
    struct obj header;
    int i;
    int ai;
    unsigned short *ao;
    int argc;
    struct fsym {
        unsigned short v;
        unsigned char t;
        char name[9];
    } fsym;

    if (verbose & V_EXEC) {
        pid();
        message("exec %s\n", name);
    }
    if (execprog && (strcmp(execprog,argv[0]) == 0)) {
	breakpoint = 1;
    }
    /*
     * count our args from the null-terminated list 
     */
    for (argc = 0; argv[argc]; argc++) {
        if (verbose & V_EXEC) {
            message("arg %d = %s\n", argc, argv[argc]);
        }
    }

    if ((file = fopen(name, "rb")) == NULL) {
        message("Can't open file %s!\n", name);
        return (errno);
    }
    fseek(file, 0, SEEK_SET);
    fread(&header, 1, sizeof(header), file);
    if (header.ident != OBJECT) {
        fseek(file, 0, SEEK_END);
        header.text = ftell(file);
        header.textoff = 0x100;
        header.dataoff = header.textoff + header.text;
        header.data = 0;
        header.bss = 0;
        header.heap = 0;
        header.table = 0;
        fseek(file, 0, SEEK_SET);
    }

    if (verbose & V_EXEC) {
        message(
            "exec header: magic: %x conf: %x symsize: %d text: %d data: %d bss: %d heap: %d textoff: %x dataoff: %x\n",
            header.ident, header.conf, header.table, header.text, header.data,
            header.bss, header.heap, header.textoff, header.dataoff);
    }

    /* reset all signal handlers */
    set_itv_usec(0);
    set_alarm();

    for (i = 0; i < 16; i++) {
        signal_handler[i] = 0;
        signal(i, SIG_DFL);
    }

    for (i = 0; i < 65536; i++) {
        put_byte(i, 0);
    }

    fread(iobuf, 1, header.text, file);
    copyout(iobuf, header.textoff, header.text);
    fread(iobuf, 1, header.data, file);
    copyout(iobuf, header.dataoff, header.data);

    if (header.table) {
        if (verbose & V_EXEC) {
            printf("got %d symbols\n", (int)(header.table / sizeof(fsym)));
        }
        for (i = 0; i < header.table / sizeof(fsym); i++) {
            fread(&fsym, 1, sizeof(fsym), file);
            add_sym(fsym.name, fsym.t, fsym.v);
        }
    }
    fclose(file);

    z80_set_reg16(sp_reg, STACKTOP);
    put_byte(8, 0x76);

    z80_set_reg16(pc_reg, header.textoff);

    brake = header.dataoff + header.data + header.bss + header.heap;

    ao = malloc(argc * sizeof(*ao));

    /*
     * now, copy the args to argv and the stack 
     */
    for (i = 0; i < argc; i++) {
        int j; 
        char *s;

        ai = argc - (i + 1);
        ao[ai] = z80_get_reg16(sp_reg) - (strlen(argv[ai]) + 1);
        z80_set_reg16(sp_reg, ao[ai]);
        // printf("copyout %s to %04x\n", argv[ai], ao[ai]);
        s = argv[ai];
        j = ao[ai]; 
        while (1) {
            put_byte(j, *s);
            if (!*s) break;
            s++;
            j++;
        }
    }
    push(0xffff);
    for (i = 0; i < argc; i++) {
        push(ao[argc - (i + 1)]);
    }
    push(argc);
    push(header.textoff);
    free(ao);
    /*
     * if (verbose & V_DATA) dumpmem(&get_byte,
     * cp->state.registers.word[Z80_SP], 256); 
     */
    return 0;
}

char
getsim(long addr)
{
    return *(unsigned char *) addr;
}

void
carry_set()
{
    unsigned char f;

    f = z80_get_reg8(f_reg);
    f |= 1;
    z80_set_reg8(f_reg, f);
}

void
carry_clear()
{
    unsigned char f;

    f = z80_get_reg8(f_reg);
    f &= 0xfe;
    z80_set_reg8(f_reg, f);
}

struct cpm_syscall {
    int type; 
    char *name;
} cpmcalls[] = {
    0, "RESET",    // 0
    0, "CONIN",    // 1
    0, "CONOUT",   // 2
    0, "RDR",      // 3
    0, "PUNCH",    // 4
    0, "LIST",     // 5
    0, "CONIO",    // 6
    0, "GETIOBYTE",// 7
    0, "SETIOBYTE",// 8
    0, "OUTSTR",   // 9
    0, "CONREAD",  // 10
    0, "CONSTAT",  // 11
    0, "BDOSVER",  // 12
    0, "DSKRESET", // 13
    0, "SELECT",   // 14
    1, "OPENF",    // 15
    0, "CLOSEF",   // 16
    0, "SFIRST",   // 17
    0, "SNEXT",    // 18
    1, "DELETE",   // 19
    1, "FREAD",    // 20
    1, "FWRITE",   // 21
    1, "FMAKE",    // 22
    0, "RENAME",   // 23
    0, "LOGVEC",   // 24
    0, "GETDRV",   // 25
    0, "SETDMA",   // 26
    0, "ALLVEC",   // 27
    0, "SETRO",    // 28
    0, "GETRO",    // 29
    0, "SETATTR",  // 30
    0, "GETDPB",   // 31
    0, "USER",     // 32
    0, "RREAD",    // 33
    1, "RWRITE",   // 34
    0, "FSIZE",    // 35
    1, "RSEEK",    // 36
    0, "DRESET",   // 37
    0, "DLOCK",    // 38
    0, "DUNLOCK",  // 39
    1, "RWRITEZ",  // 40
    0
};

void
cpmsys()
{
    unsigned char C_reg;
    unsigned short DE_reg;
    char vbuf[200];
    unsigned short from;
    struct cpm_syscall *cs = 0;
    int i;
    char *s;
    char c;

    from = get_word(z80_get_reg16(sp_reg)) - 3;
    C_reg = z80_get_reg8(c_reg);
    DE_reg = z80_get_reg16(de_reg);

    message("cp/m system call from %x - ", from);

    vbuf[0] = '\0';

    if (C_reg <= (sizeof(cpmcalls) / sizeof(cpmcalls[0]))) {
        cs = &cpmcalls[C_reg];
    }

    s = vbuf;
    if (cs) {
        s += sprintf(vbuf, "call: %s arg: %x ", cs->name, DE_reg);
        if (cs->type == 1) {
            s += sprintf(s, "fcb: %c:", get_byte(DE_reg) + '@');
            for (i = 1; i < 9; i++) {
                c = get_byte(DE_reg + i);
                if (c != ' ') *s++ = c;
            }
            *s++='.';
            for (i = 9; i < 12; i++) {
                c = get_byte(DE_reg + i);
                if (c != ' ') *s++ = c;
            }
        }
        strcpy(s, "\n");
    } else {
        sprintf(vbuf, "call: %d arg: %x\n", C_reg, DE_reg);
    }
    message(vbuf);
    dumpcpu();
    z80_set_reg16(pc_reg, pop());
}

strdump(int p)
{
#define XBUF 10
    char xbuf[XBUF];
    unsigned char c;
    int i;

    for (i = 0; i < XBUF - 1; i++) {
        xbuf[i + 1] = '\0';
        c = get_byte(p + i);
        xbuf[i] = c;
        if ((c < ' ') || (c > 0x7e)) xbuf[i] = '.';
        if (!c) break;
    }
    message("%s ", xbuf);
}

static void
funcdump(char *s)
{
#define ADUMP   4

    unsigned short a[ADUMP];
    unsigned short sp;
    int i;

    if ((verbose & V_FUNC0) && (s[0] == 'c' && s[1] == '.')) 
        return;

    sp = z80_get_reg16(sp_reg);
    for (i = 0; i < ADUMP; i++) {
        a[i] = get_word(sp + (i * 2));
    }
    message("%s(", s);
    for (i = 1; i < ADUMP; i++) {
        message("%04x", a[i]);
        if (i < (ADUMP - 1)) message(",");
    }
    message(") ");
    for (i = 1; i < ADUMP; i++) {
        strdump(a[i]);
        if (i < (ADUMP - 1)) message(" ");
    }
    message("\n");
}

/*
 * this is the actual micronix emulator that emulates all the system calls
 * of micronix. initially, it starts with exec of the named file, and then 
 * jumps to the emulation
 */
static void
emulate()
{
    int i;
    unsigned short pc;
    char *s;

    do {
        pc = z80_get_reg16(pc_reg);

        if (watchpoint_hit()) {
            breakpoint = 1;
        }
        if (breakpoint_at(pc)) {
            pid();
            message("break at %04x\n", pc);
            dumpcpu();
            breakpoint = 1;
        }
        /* cp/m system call */
        if (pc == 0x0005) {
            pid();
            cpmsys();
            breakpoint = 1;
        }
        if (breakpoint == 1) {
            breakpoint = monitor();
        }
        if ((verbose & V_INST) || (breakpoint > 1)) {
            if (breakpoint) breakpoint--;
            pid();
            dumpcpu();
        }

        if (verbose & V_FUNC) {
            s = get_symname(pc);
            if (s) {
                funcdump(s);
            }
        }

        z80_run();
        /*
         * if we have a signal to deliver, do it now
         */
        if (signalled) {
            for (i = 0; i < 16; i++) {
                if ((1 << i) & signalled) {
                    break;
                }
            }
            if (signal_handler[i]) {
                if (verbose & V_SYS) {
                    message("invoking signal %d %x\n", i, signal_handler[i]);
                }
                push(pc);
                z80_set_reg16(pc_reg, signal_handler[i]);
            } else {
                if (verbose & V_SYS) {
                    message("ignoring signal %d\n", i);
                }
            }
            signalled &= ~(1 << i);
        }
        if (z80_get_reg8(status_reg) & S_HLTA) {
            SystemCall();
        }
    } while (1);
}

/*
 * we have to be very shady about directories, since the v6 directory is directly read, and
 * the abstract read directory stuff is far in the future.  so what we do when we open a
 * directory, we translate the entire directory into V6 form and keep the buffer around
 * until the directory is closed.
 */
struct dirfd
{
    DIR *dp;
    int fd;
    unsigned char *buffer;
    int bufsize;
    int offset;
    struct dirfd *next;
    int end;
} *opendirs;

/*
 * pids are longer in linux, so we need to manage them too in shared memory.
 */
struct pids {
    pthread_mutex_t mutex;
    int linuxpid[65536];
    int hiwat;
} *pids;

void
initpids()
{
    pids = (struct pids *)mmap((void *)NULL, sizeof(struct pids), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (pids == (struct pids *)-1) {
        perror("initpids");
        exit(2);
    }
    pthread_mutex_init(&pids->mutex, NULL);
    pids->hiwat = 2;
}

/*
 * allocate a pid and register it.
 * that means we need to make sure there are no duplicates.
 */
allocpid(int lpid)
{
    int p;

    pthread_mutex_lock(&pids->mutex);
    for (p = 1; p < pids->hiwat; p++) {
        if (pids->linuxpid[p] == lpid) {
            pthread_mutex_unlock(&pids->mutex);
            return p;
        }
    }
    p = pids->hiwat++;
    pids->linuxpid[p] = lpid;
    pthread_mutex_unlock(&pids->mutex);
    return p;
}
 
/*
 * inumbers are different size, and we need to map between them
 */
struct inums {
    pthread_mutex_t mutex;
    int inumber[65536];
    int hiwat;
} *inums;

void
initinums()
{
    inums = (struct inums *)mmap((void *)NULL, sizeof(struct inums), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (inums == (struct inum *)-1) {
        perror("initinums");
        exit(2);
    }
    pthread_mutex_init(&inums->mutex, NULL);
    inums->hiwat = 2;
}

/*
 * allocate an inumber and register it.
 * that means we need to make sure there are no duplicates.
 */
allocinum(int ui)
{
    int i;

    pthread_mutex_lock(&inums->mutex);
    for (i = 0; i < inums->hiwat; i++) {
        if (inums->inumber[i] == ui) {
            pthread_mutex_unlock(&inums->mutex);
            return i;
        }
    }
    i = inums->hiwat++;
    inums->inumber[i] = ui;
    pthread_mutex_unlock(&inums->mutex);
//    printf("allocinum %d -> %d\n", ui, i);
    return i;
}

/*
 * a version 6 directory entry 
 */
struct v6dir
{
    UINT inum;
    char name[14];
};

#define	DIRINC	16

/*
 * open a directory and return the file descriptor
 */
unsigned short
dirsnarf(char *name)
{
    DIR *dp;
    struct dirent *de;
    struct dirfd *df;
    int i;
    struct v6dir *v;
    int is_root_dir = 0;

    for (i = 0; rootdir[i] == name[i]; i++)
        ;
    if ((strcmp(&name[i], "/.") == 0) ||
        (strcmp(&name[i], "/..") == 0) ||
        (strcmp(&name[i], "/") == 0)) {
        is_root_dir = 1;
    }

    dp = opendir(name);
    if (!dp)
        return -1;

    df = malloc(sizeof(struct dirfd));
    df->fd = dirfd(dp);
    df->dp = dp;
    df->bufsize = 0;
    df->buffer = 0;
    df->offset = 0;
    df->end = 0;
    v = (struct v6dir *) df->buffer;
    df->next = opendirs;
    opendirs = df;

    while (1) {
        de = readdir(dp);
        if (!de)
            break;
        if (df->end + sizeof(*v) > df->bufsize) {
            df->bufsize += DIRINC;
            df->buffer = realloc(df->buffer, df->bufsize);
            bzero(&df->buffer[df->end], DIRINC);
        }
        v = (struct v6dir *) &df->buffer[df->end];
        v->inum = allocinum(de->d_ino);
        if (v->inum == 2) v->inum = 1;
        if ((de->d_ino == rootinode) || 
            (is_root_dir && (strcmp(de->d_name, "..") == 0))) {
            v->inum = 2;
        }
        strncpy(v->name, de->d_name, 14);
        v++;
        df->end += sizeof(*v);
    }
    if (verbose & V_DATA)
        hexdump(df->buffer, df->bufsize);
    return (df->fd);
}

struct dirfd *
dirget(int fd)
{
    struct dirfd *n;

    for (n = opendirs; n; n = n->next) {
        if (n->fd == fd) {
            return (n);
        }
    }
    return (0);
}

void
dirclose(int fd)
{
    struct dirfd *n, *p;

    p = 0;

    for (n = opendirs; n; n = n->next) {
        if (n->fd == fd)
            break;
        p = n;
    }
    if (!n) {
        return;
    }
    if (p) {
        p->next = n->next;
    } else {
        opendirs = n->next;
    }
    closedir(n->dp);
    free(n->buffer);
    free(n);
}

/*
 * data structures to control terminal interface
 */
struct tcmods
{
    tcflag_t iflag;
    tcflag_t oflag;
    tcflag_t cflag;
    tcflag_t lflag;
};

struct termios ti;

/*
 * this table contains tcsetattr data for each of the tty driver mode
 * bits.  when setting a bit using stty, we have the ability to clear
 * and set bits in each of the 4 mode words in the termios structure,
 * and when clearing one, the same.  this is fully general, and avoids
 * jiggery-pokery in the actual stty call
 */

struct ttybits
{
    char *name;
    unsigned short bitmask;
    struct tcmods setclr;
    struct tcmods setset;
    struct tcmods clrclr;
    struct tcmods clrset;
} ttybits[] = {

#ifdef notdef
    {"cts", 0100000,            // use cts for hardware handshake
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        {0, 0, 0, 0}},
    {"8bit", 0040000,           // use 8 bits on input
            {ISTRIP, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        {ISTRIP, 0, 0, 0}},
    // { "cbreak", 0020000 },
    // { "more", 0010000 },
#endif

    {"raw", 0000040,            // raw input mode - no controls
            {INLCR | IGNCR | ICRNL | IXOFF | IXON, OPOST,
                0, ECHO | ECHONL | ICANON | ISIG},      // raw
            {0, 0, 0, 0},
            {0, 0, 0, 0},       // cooked
            {INLCR | ICRNL, OPOST | ONLCR, 0,
            ECHO | ECHONL | ICANON | ISIG | IEXTEN}},

#ifdef notdef
    {"crlf", 0000020,
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        {0, 0, 0, 0}},
#endif

    {"echo", 0000010,
            {0, 0, 0, 0},
            {0, 0, 0, ECHO | ECHOE},
            {0, 0, 0, ECHO | ECHOE},
        {0, 0, 0, 0}},

#ifdef notdef
    {"lcase", 0000004,
            {IUCLC, OLCUC, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        {ISTRIP, OLCUC, 0, 0}},
    {"tabs", 0000002,
            {IUCLC, OLCUC, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        {ISTRIP, OLCUC, 0, 0}},
#endif

    {0, 0,
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        {0, 0, 0, 0}}
};

void
bitdump(unsigned short v)
{
    struct ttybits *tb;

    for (tb = ttybits; tb->name; tb++) {
        if (tb->bitmask & v) {
            message("%s ", tb->name);
        }
    }
}

char *baud[] = {
    "1200", "50", "75", "110",
    "134.5", "150", "200", "300",
    "600", "1200", "1800", "2400",
    "4800", "9600", "19200", "1200"
};

void
cchar(char *s, char c)
{
    if (c == 0x7f) {
        message("%s: DEL ", s);
    } else if (c < ' ') {
        message("%s: ^%c ", s, c + '@');
    } else {
        message("%s: %c ", s, c);
    }
}

/*
 * dump out the micronix tty vector
 */
void
tty_dump(char *s, unsigned short a)
{
    char c;

    message("%s in: %s out: %s ",
        s, baud[get_byte(a)], baud[get_byte(a + 1)]);
    cchar("erase", get_byte(a + 2));
    cchar("kill", get_byte(a + 3));
    bitdump(get_word(a + 4));
    message("\n");
}

/*
 * the termios struct names
 */
struct tidbits
{
    char *name;
    tcflag_t bit;
};

struct tidbits iflags[] = {
    "IGNBRK", IGNBRK,
    "BRKINT", BRKINT,
    "IGNPAR", IGNPAR,
    "PARMRK", PARMRK,
    "ISTRIP", ISTRIP,
    "INLCR", INLCR,
    "IGNCR", IGNCR,
    "ICRNL", ICRNL,
#ifndef __APPLE__
    "IUCLC", IUCLC,
#endif
    "IXON", IXON,
    "IXANY", IXANY,
    "IXOFF", IXOFF,
    "IMAXBEL", IMAXBEL,
    "IUTF8", IUTF8,
    0, 0
};

struct tidbits oflags[] = {
    "OPOST", OPOST,
#ifndef __APPLE__
    "OLCUC", OLCUC,
#endif
    "ONLCR", ONLCR,
    "OCRNL", OCRNL,
    "ONOCR", ONOCR,
    "ONLRET", ONLRET,
#ifndef __APPLE__
    "XTABS", XTABS,
#endif
    0, 0
};

struct tidbits cflags[] = {
    0, 0
};

struct tidbits lflags[] = {
    "ISIG", ISIG,
    "ICANON", ICANON,
#ifndef __APPLE__
    "XCASE", XCASE,
#endif
    "ECHO", ECHO,
    "ECHOE", ECHOE,
    "ECHOK", ECHOK,
    "ECHONL", ECHONL,
    "ECHOCTL", ECHOCTL,
    "ECHOPRT", ECHOPRT,
    "ECHOKE", ECHOKE,
    0, 0
};

void
tbdump(char *s, struct tidbits *tb, tcflag_t v)
{
    message("%s: 0%06o ", s, v);
    while (tb->name) {
        if (tb->bit & v) {
            message("%s ", tb->name);
        }
        tb++;
    }
    message("\n");
}

/*
 * dump out the termios struct
 */
void
ti_dump()
{
    tbdump("ibits", iflags, ti.c_iflag);
    tbdump("obits", oflags, ti.c_oflag);
    tbdump("cbits", cflags, ti.c_cflag);
    tbdump("lbits", lflags, ti.c_lflag);
}

void
settimode(unsigned short mode)
{
    int i;
    struct ttybits *tb;

#ifdef notdef
    ti_dump();
    message("settimode: %06o\n", mode);
#endif

    for (tb = ttybits; tb->name; tb++) {
        if (tb->bitmask & mode) {
            ti.c_iflag &= ~tb->setclr.iflag;
            ti.c_oflag &= ~tb->setclr.oflag;
            ti.c_cflag &= ~tb->setclr.cflag;
            ti.c_lflag &= ~tb->setclr.lflag;
            ti.c_iflag |= tb->setset.iflag;
            ti.c_oflag |= tb->setset.oflag;
            ti.c_cflag |= tb->setset.cflag;
            ti.c_lflag |= tb->setset.lflag;
        } else {
            ti.c_iflag &= ~tb->clrclr.iflag;
            ti.c_oflag &= ~tb->clrclr.oflag;
            ti.c_cflag &= ~tb->clrclr.cflag;
            ti.c_lflag &= ~tb->clrclr.lflag;
            ti.c_iflag |= tb->clrset.iflag;
            ti.c_oflag |= tb->clrset.oflag;
            ti.c_cflag |= tb->clrset.cflag;
            ti.c_lflag |= tb->clrset.lflag;
        }
    }

#ifdef notdef
    ti_dump();
#endif
}

char *filename;
long
swizzle(long in)
{
    return ((in >> 16) & 0xffff) | ((in & 0xffff) << 16);
}

char hexdig[] = "01234567890ABCDEF";

char *
degrime(char *s)
{
    static char dbuf[100];
    char *d = dbuf;
    char c;

    while (*s) {
        c = *s++;
        if ((c > ' ') && (c <= 0x7e)) {
            *d++ = c;
            continue;
        }
        *d++ = '\\';
        *d++ = 'x';
        *d++ = hexdig[(c >> 4) & 0xf];
        *d++ = hexdig[c & 0xf];
    }
    *d++ = '\0';
    return dbuf;
}

/*
 * micronix system calls are done using the RST8 instruction, which
 * is a one-byte call instruction to location 8, which has a halt instruction
 * placed there by the exec call.
 *
 * what we do is look on the stack for the instruction following the RST8,
 * and get the code there, which is the system call number
 *
 * if the code is 0, the next 2 bytes are the address of another syscall
 * descriptor, which starts with a rst8 byte
 *
 * in any event, we need to adjust the return address on the stack to
 * to skip over the system call args. and return to after the syscall.
 */
void
SystemCall()
{
    unsigned char code;
    unsigned short sc;
    char indirect = 0;

    unsigned short fd;          /* from hl */
    unsigned short arg1;        /* first arg */
    unsigned short arg2;        /* second arg */
    unsigned short arg3;        /* third arg */
    unsigned short arg4;        /* fourth arg */

    char *fn;
    char *fn2;
    unsigned short ret;

    int i;
    sighandler_t handler;
    int p[2];

    struct statb *ip;
    struct dirfd *df;
    char **argvec;
    int savemode;
    struct syscall *sp;
    char name1[256];
    char name2[256];

    savemode = verbose;

    if (verbose & V_SYS0) {
        pid();
        dumpcpu();
    }

    /*
     * pop the return address from the stack 
     */
    sc = pop();
    push(sc);

    /*
     * upm can't use the rst1, since cp/m uses that memory.
     * so, it does a call (0xcd) to the halt instruction.
     * all our other code uses rst1.
     * so, either: sc[0] == 0xcf or sc[-2] == 0xcd
     * the hack is that in the second case, sc[0] is part of
     * the high part of the address of the halt.
     */
    sc -= 1;

    /*
     * make sure that we came here from a rst1 
     */
    if (((get_byte(sc) & 0xff) != 0xcf)
        && ((get_byte(sc - 2) & 0xff) != 0xcd)) {
        pid();
        dumpcpu();
        message("halt no syscall %x!\n", sc);
        exit(1);
    }

    /*
     * get the function code 
     */
    code = get_byte(sc + 1);

    /*
     * this is an indirect call - the argument points at a syscall 
     */
    if (code == 0) {
        indirect++;
        sc = get_word(sc + 2);
        if ((code = get_byte(sc)) != 0xcf) {
            message("indir no syscall %d %x!\n", code, sc);
            exit(4);
        }
        code = get_byte(sc + 1);
    }

    if (code >= NSYS) {
        fprintf(stderr, "bad system call number %d\n", code);
        exit(4);
    }

    savemode = verbose;
    /*
     * if this is a system call we are interested in, deal with it 
     */
    if (sys_stop[code] || sys_trace[code]) {
        verbose |= V_SYS;
    }
    if (sys_stop[code] && !(--sys_stop[code])) {
        breakpoint = 1;
    }

    sp = &syscalls[code];

    if (verbose & V_SYS0) {
        message("%10s %3d ", sp->name, z80_get_reg16(hl_reg));
        // dumpmem(&get_byte, sc, sp->argbytes + 1);
    }

    if (sp->flag & (SF_ARG1 | SF_NAME))
        arg1 = get_word(sc + 2);
    if (sp->flag & (SF_ARG2 | SF_NAME2))
        arg2 = get_word(sc + 4);
    if (sp->flag & SF_ARG3)
        arg3 = get_word(sc + 6);
    if (sp->flag & SF_ARG4)
        arg3 = get_word(sc + 8);
    if (sp->flag & SF_FD)
        fd = z80_get_reg16(hl_reg);
    if (sp->flag & SF_NAME) {
        copyin(name1, arg1, sizeof(name1));
        fn = name1;
    }
    if (sp->flag & SF_NAME2) {
        copyin(name2, arg2, sizeof(name2));
        fn2 = name2;
    }

    /*
     * what's with these zero length writes? 
     */
    if ((code == 4) && (arg2 == 0)) {
        goto nolog;
        verbose = -1;
        breakpoint = 1;
    }

    if ((verbose & V_SYS) && (!(sp->flag & SF_SMALL) || (verbose & V_ASYS))) {
        pid();
        message("%s(", sp->name);
        i = sp->flag & (SF_FD | SF_ARG1 | SF_NAME | SF_ARG2 | SF_NAME2 |
            SF_ARG2 | SF_ARG3 | SF_ARG4);
#define F(b, f, a) \
	if (i & (b)) { i ^= (b) ; message(f,a,i?",":""); }
        F(SF_FD, "%d%s", fd);
        F(SF_ARG1, "%04x%s", arg1);
        F(SF_NAME, "\"%s\"%s", degrime(fn));
        F(SF_ARG2, "%04x%s", arg2);
        F(SF_NAME2, "\"%s\"%s", fn2);
        F(SF_ARG3, "%04x%s", arg3);
        F(SF_ARG4, "%04x%s", arg4);
        message(") ");
    }
  nolog:

    /*
     * let's fixup the return address from the table 
     */
    push(pop() + syscalls[indirect ? 0 : code].argbytes);

    /*
     * let's make the assumption that all calls fail 
     */
    carry_set();

    // carry_clear();

    switch (code) {
    case 0:                    /* double indirect is a no-op */
        pid();
        message("double indirect syscall!\n");
        break;

    case 1:                    /* exit (hl) */
        exit(fd);
        break;

    case 2:                    /* fork */
        ret = fork();
        if (ret) {
            ret = allocpid(ret);
            push(pop() + 3);
        } else {
            mypid = getpid();
            set_alarm();
        }
        carry_clear();
        break;

    case 3:                    /* read (hl), buffer, len */
        if ((df = dirget(fd))) {
            ret = df->bufsize - df->offset;
            if (arg2 < ret) {
                ret = arg2;
            }
            copyout(&df->buffer[df->offset], arg1, ret);
            df->offset += ret;
        } else {
            if ((ret = seekfile(fd)) == 0) {
                ret = read(fd, iobuf, arg2);
                copyout(iobuf, arg1, ret);
            }
        }
        if (ret == 0xffff) {
            ret = errno;
            carry_set();
        } else {
            files[fd].offset += ret;
            carry_clear();
        }
        break;

    case 4:                    /* write (hl), buffer, len */
        if ((ret = seekfile(fd)) == 0) {
            copyin(iobuf, arg1, arg2);
            ret = write(fd, iobuf, arg2);
        }
        if (ret == 0xffff) {
            ret = errno;
            carry_set();
        } else {
            files[fd].offset += ret;
            carry_clear();
        }
        break;

    case 5:                    /* open */
        /* the unix modes and the micronix modes are compatible */
        /*
         * magic filenames 
         */
        filename = fname(fn);
        if (strcmp(fn, "/dev/console") == 0) {
            filename = "/dev/tty";
        }
        if (!stat(filename, &sbuf)) {
            if (S_ISDIR(sbuf.st_mode)) {
                ret = dirsnarf(filename);
                if (ret == 0xffff) {
                    ret = errno;
                    message("dirsnarf lose\n");
                    goto lose;
                }
            } else {
                ret = open(filename, arg2);
                if (ret == 0xffff) {
                    if (verbose & V_ERROR)
                        message("%s: %s\n", strerror(errno), filename);
                    goto lose;
                }
                files[ret].offset = 0;
                files[ret].dt = 'r';
                devnum(filename, &files[ret].dt,
                    &files[ret].major, &files[ret].minor);
                if (files[ret].dt == 'b') {
                    files[ret].filesize = sbuf.st_size;
                }
            }
            carry_clear();
        } else {
            if (verbose & V_ERROR)
                message("%s: %s\n", strerror(errno), filename);
          lose:
            ret = errno;
            carry_set();
        }
        break;

    case 6:                    /* close */
        if (dirget(fd)) {
            dirclose(fd);
        } else {
            close(fd);
        }
        if (fd <= MAXFILE) {
            files[fd].special = 0;
            files[fd].offset = 0;
            carry_clear();
        } else {
            carry_set();
        }
        break;

    case 7:                    /* wait */
        if (verbose & V_SYS) {
            pid();
            message("wait\n");
        }
        if ((ret = wait(&i)) == 0xffff) {
            if (verbose & V_SYS) {
                pid();
                message("no children\n");
            }
            ret = ECHILD;
            carry_set();
            break;
        }
        if (verbose & V_SYS) {
            pid();
            message("wait ret %x %x\n", ret, i);
        }
        if (WIFEXITED(i)) {
            z80_set_reg8(d_reg, WEXITSTATUS(i));
            z80_set_reg8(e_reg, 0);
        } else if (WIFSIGNALED(i)) {
            z80_set_reg8(d_reg, 1);
            z80_set_reg8(e_reg, WTERMSIG(i));
        } else {
            message("waitfuck %x\n", i);
        }
        ret = allocpid(ret);
        carry_clear();
        break;

    case 8:                    /* creat <name> <mode> */
        ret = creat(filename = fname(fn), arg2);
        if (ret == -1) {
            if (verbose & V_ERROR)
                message("%s: %s\n", strerror(errno), filename);
            ret = errno;
            carry_set();
        } else {
            carry_clear();
        }
        break;

    case 9:                    /* link <old> <new> */
        /*
         * special case code when doing a mkdir:  
         * our applications put links to . and .. in the
         * directory, and we need to ignore these system calls
         */
        i = strlen(fn2);
        if ((strcmp(&fn2[i - 3], "/..") == 0) ||
            (strcmp(&fn2[i - 2], "/.") == 0)) {
            carry_clear();
            ret = 0;
            break;
        }
        filename = strdup(fname(fn));
        ret = link(filename, fname(fn2));
        if (ret != 0) {
            if (verbose & V_ERROR)
                message("%s: %s\n", strerror(errno), filename);
            ret = -1;
        } else {
            carry_clear();
        }
        free(filename);
        break;

    case 10:                   /* unlink <file> */
        /*
         * special case code when doing a rmdir
         */
        i = strlen(fn);
        if ((strcmp(&fn[i - 3], "/..") == 0) ||
            (strcmp(&fn[i - 2], "/.") == 0)) {
            carry_clear();
            ret = 0;
            break;
        }
        ret = unlink(filename = fname(fn));
        if (ret != 0) {
            if (errno == EISDIR) {
                ret = rmdir(filename);
                if (ret == 0) {
                    carry_clear();
                    break;
                }
            }
            if (verbose & V_ERROR)
                message("%s: %s\n", strerror(errno), filename);
            ret = errno;
            carry_set();
        } else {
            carry_clear();
        }
        break;

    case 11:                   /* exec */
        /*
         * let's count our args 
         */
        i = 0;
        while (get_word(arg2 + (i * 2)))
            i++;
        argvec = malloc((i + 1) * sizeof(char *));
        i = 0;
        for (i = 0; (arg1 = get_word(arg2 + (i * 2))); i++) {
            copyin(name2, arg1, sizeof(name2));
            argvec[i] = strdup(name2);
        }
        argvec[i] = 0;
        ret = do_exec(fname(fn), argvec);
        if (ret) {
            ret = errno;
            carry_set();
        } else {
            carry_clear();
        }
        for (i = 0; argvec[i]; i++) {
            free(argvec[i]);
        }
        free(argvec);
        break;

    case 12:                   /* chdir <ptr to name> */
        /*
         * again, because of chroot being privileged, we need to do some
         * pretty sleazy stuff 
         */
        if (*fn == '/') {           /* if absolute path */
            strcpy(namebuf, fn);
        } else {                    /* if relative path */
            sprintf(namebuf, "%s/%s", curdir, fn);
        }
        sprintf(workbuf, "%s/%s", rootdir, namebuf);
        realpath(workbuf, namebuf);
        ret = stat(filename = namebuf, &sbuf);
        if (ret || !(S_ISDIR(sbuf.st_mode))) {
            ret = 20;
            carry_set();
        } else {
            /*
             * we are good to go - strip out root again 
             */
            if (strncmp(namebuf, rootdir, strlen(rootdir)) == 0) {
                strcpy(curdir, &namebuf[strlen(rootdir)]);
            } else {
                strcpy(curdir, "");
            }
            carry_clear();
        }
        break;
    case 13:                   /* time */
        i = time(0);
        z80_set_reg16(de_reg, i);
        ret = (i >> 16) & 0xffff;
        carry_clear();
        break;
    case 14:                   /* mknod <name> mode dev (dev == 0) for dir */
        filename = fname(fn);
        switch (arg2 & IFMT) {
        case IFDIR:
            ret = mkdir(filename, arg2 & 0777);
            break;
        case IFBLK:
        case IFCHR:
            sprintf(workbuf, "%cdev(%d,%d)", 
                ((arg2 & IFMT) == IFBLK) ? 'b' : 'c',
                (arg3 >> 8) & 0xff, arg3 & 0xff);
            ret = symlink(workbuf, filename);
            message("make dev %s %s %o %x = %d\n", filename, workbuf, arg2, arg3, ret);
            break;
        default:
            ret = -1;
            errno = EIO;
        }
        if (ret == -1) {
            if (verbose & V_ERROR)
                message("%s: %s\n", strerror(errno), filename);
            ret = errno;
            carry_set();
        } else {
            carry_clear();
        }
        break;

    case 15:                   /* chmod <name> <mode> */
        filename = fname(fn);
        if (chmod(filename, arg2 & 07777) != 0) {
            ret = errno;
            carry_set();
        }
        carry_clear();
        break;

    case 16:                   /* chown <name> <mode> */
        carry_set();
        if (am_root) {
            carry_clear();
        }
        break;

    case 17:                   /* sbrk <addr> */
        ret = brake;
        brake = arg1;
        carry_clear();
        break;

    case 18:                   /* stat fn buf */
    case 28:                   /* fstat fd buf */
        ip = (struct statb *) iobuf;

        if (code == 28) {
            ret = fstat(fd, &sbuf);
            if ((df = dirget(fd))) {
                sbuf.st_size = df->end;
            }
            arg2 = arg1;
            if ((files[fd].dt == 'c') || (files[fd].dt == 'b')) {
                files[fd].filesize = sbuf.st_size;
                sbuf.st_mode &= ~S_IFMT;
                sbuf.st_mode |=
                    (files[fd].dt == 'c') ? S_IFCHR : S_IFBLK;
                ip->d.d_addr[0] = ((files[fd].major & 0xff) << 8) | 
                    (files[fd].minor & 0xff);
            }
        } else {
            // hexdump(fn, strlen(fn));
            filename = fname(fn);
            // ret = stat(filename, &sbuf);
            for (i = 0; rootdir[i] == filename[i]; i++)
                ;
            if (strcmp(&filename[i], "/..") == 0) {
                filename[i+2] = '\0';
            }
            ret = lstat(filename, &sbuf);
            if (ret) {
                if (verbose & V_ERROR)
                    message("%s: %s\n", strerror(errno), filename);
            }
        }
        if (ret) {
            ret = errno;
            carry_set();
            break;
        }

        if (sbuf.st_ino == rootinode) {
            ip->inum = 2;
        } else {
            ip->inum = allocinum(sbuf.st_ino);
            if (ip->inum == 2) ip->inum = 1;
        }
        ip->dev = sbuf.st_dev;
        ip->d.d_nlink = sbuf.st_nlink;
        ip->d.d_uid = sbuf.st_uid;
        ip->d.d_gid = sbuf.st_gid;
        ip->d.d_atime = swizzle(sbuf.st_atime);
        ip->d.d_mtime = swizzle(sbuf.st_mtime);
        ip->d.d_mode = (sbuf.st_mode & 07777) | /* IALLOC | */
            ((sbuf.st_size > (8 * 512)) ? ILARG : 0);
        switch (sbuf.st_mode & S_IFMT) {
        case S_IFDIR:
            /*
             * windows subsystem for linux returns a busted directory size
             * we need to actually count the freaking directory entries and
             * multiply by 16.  what a lose
             */
            ip->d.d_mode |= IFDIR;
            if (code != 28) {
                DIR *dp;
                sbuf.st_size = 0;
                dp = opendir(filename);
                while (readdir(dp)) {
                    sbuf.st_size += 16;
                }
                closedir(dp);
            }
            break;
        case S_IFREG:
            break;
        case S_IFLNK:
            {
            char dt; int Maj, Min;
            if ((ret = devnum(filename, &dt, &Maj, &Min)) != 0) {
                carry_set();
                break;
            }
            ip->d.d_addr[0] = ((Maj & 0xff) << 8) | (Min & 0xff);
            ip->d.d_mode |= ((dt == 'c') ? IFCHR : IFBLK);
            }
            break;
        case S_IFCHR:
            ip->d.d_mode |= IFCHR;
            break;
        case S_IFBLK:
            ip->d.d_mode |= IFBLK;
            break;
        default:
            break;
        }
        ip->d.d_size0 = sbuf.st_size >> 16;
        ip->d.d_size1 = sbuf.st_size & 0xffff;
        // ip->mode = sbuf.st_mode;
        copyout(iobuf, arg2, 36);
        if (verbose & V_DATA) {
            dumpmem(&get_byte, arg2, 36);
        }
        carry_clear();
        break;

    case 19:                   /* seek fd where mode */
        if ((fd < 0) || (fd > MAXFILE)) {
            ret = EBADF;
            carry_set();
            break;
        }
        if (arg2 % 3) {
            i = (short) arg1;
        } else {
            i = (unsigned short) arg1;
        }
        if (arg2 > 2) {
            i *= 512;
            arg2 -= 3;
        }
        if ((df = dirget(fd))) {
            switch (arg2) {
            case 0:
                break;
            case 1:
                i += df->offset;
                break;
            case 2:
                i += df->end;
                break;
            }
            df->offset = i;
        } else {
            switch (arg2) {
            case 0:
                break;
            case 1:
                i += files[fd].offset;
                break;
            case 2:
                fstat(fd, &sbuf);
                i += sbuf.st_size;
                break;
            }
            files[fd].offset = i;
        }
//message("seek fd %d to %d\n", fd, i);
        ret = (i >> 16) & 0xffff;
        ret = 0;
        carry_clear();
        break;

    case 20:                   /* getpid */
        ret = allocpid(getpid());
        carry_clear();
        break;

    case 21:                   /* mount */
        carry_set();
        break;

    case 22:                   /* umount */
        carry_set();
        break;

    case 23:                   /* setuid */
        carry_set();
        break;

    case 24:                   /* getuid */
        ret = getuid();
        if (am_root)
            ret = 0;
        carry_clear();
        break;

    case 25:                   /* stime */
        carry_set();
        break;

    case 31:                   /* stty */
        if (verbose & V_SYS) {
            tty_dump("stty", arg1);
        }
        tcgetattr(fd, &ti);
        settimode(get_word(arg1 + 4));
        ti.c_cc[VERASE] = get_byte(arg1 + 2);
        ti.c_cc[VKILL] = get_byte(arg1 + 3);
        if (verbose & V_SYS) {
            ti_dump();
        }
        tcsetattr(fd, TCSANOW, &ti);
        carry_set();
        carry_clear();
        break;

    case 32:                   /* gtty */
        if (tcgetattr(fd, &ti)) {
            // perror("gtty"); 
            ret = ENOTTY;
            carry_set();
            break;
        }
        i = 0;
        if (!(ti.c_iflag & ISTRIP))
            i |= 040000;
        if (ti.c_iflag & ICRNL)
            i |= 020;
        if (ti.c_lflag & ECHO)
            i |= 010;
        if (!(ti.c_lflag & ICANON))
            i |= 040;
        if (ti.c_oflag & TABDLY)
            i |= 002;

        put_byte(arg1, ti.c_ispeed);
        put_byte(arg1 + 1, ti.c_ospeed);
        put_byte(arg1 + 2, ti.c_cc[VERASE]);
        put_byte(arg1 + 3, ti.c_cc[VKILL]);
        put_word(arg1 + 4, i);
        if (verbose & V_SYS) {
            tty_dump("gtty", arg1);
            // ti_dump();
        }
        ret = 0;
        carry_clear();
        break;

    case 33:                   /* access <name> <mode> */
        i = 0;
        if (arg2 & 4)
            i |= R_OK;
        if (arg2 & 2)
            i |= W_OK;
        if (arg2 & 1)
            i |= X_OK;
        ret = access(fname(fn), i);
        if (ret == -1) {
            carry_set();
        } else {
            carry_clear();
        }
        break;

    case 34:                   /* nice */
        carry_clear();
        break;

    case 35:                   /* sleep */
        sleep(fd);
        carry_clear();
        break;

    case 36:                   /* sync */
        carry_clear();
        break;

    case 37:                   /* kill <pid in hl> signal */
        carry_set();
        break;

    case 41:
        ret = dup(fd);
        if (ret == -1) {
            ret = 0;
            carry_set();
        } else {
            files[ret] = files[fd];
            carry_clear();
        }
        break;

    case 42:
        if ((i = pipe(p))) {
            ret = errno;
            carry_set();
        } else {
            ret = p[0];
            z80_set_reg16(de_reg, p[1]);
            carry_clear();
        }
        break;

    case 48:                   /* set signal handler */
        if (arg1 > 15) {
            message("signal %d out of range\n", arg1);
            carry_set();
            ret = -1;
            arg1 = 0;
            break;
        }
        ret = signal_handler[arg1];

        i = 0;
        switch (arg1) {
        case 2:                // interrupt
            signal_handler[2] = arg2;
            handler = int_handler;
            i = SIGINT;
            break;
        case 3:                // quit
            i = SIGQUIT;
            signal_handler[3] = arg2;
            handler = quit_handler;
            break;
        case 6:                // bg
            signal_handler[6] = arg2;
            i = SIGTSTP;
            handler = bg_handler;
            break;
        case 7:                // termio
            set_itv_usec(0);
            set_alarm();
            signal_handler[7] = arg2;
            i = SIGALRM;
            handler = alarm_handler;
            if (arg2 > 1) {
                set_itv_usec(200 * 1000);
                set_alarm();
            }
            break;
        default:

#ifdef notdef
            if ((arg2 != 0) && (arg2 != 1)) {
                message("unimplemented signal %s %x\n",
                    signame[arg1], arg2);
            }
#endif

            break;
        }
        if ((arg2 == 0) || (arg2 == 1)) {
            handler = arg2 ? SIG_IGN : SIG_DFL;

#ifdef notdef
            if (verbose & V_SYS) {
                pid();
                message("signal %s %s\n", signame[arg1],
                    arg2 ? "ignore" : "default");
            }
        } else if (verbose & V_SYS) {
            pid();
            message("signal %s %x\n",
                signame[arg1], signal_handler[arg1]);
#endif
        }
        if (i) {
            signal(i, handler);
        }
        carry_clear();
        break;
    default:
        pid();
        message("unrecognized syscall %d %x\n", code, code);
        carry_set();
        break;
    }

    /* if the system call failed, and we want a breakpoint */
    if ((z80_get_reg8(f_reg) & 1) && (verbose & V_SFAIL)) {
        breakpoint = 1;
    }

    z80_set_reg16(hl_reg, ret);

    /* write for 0 bytes */
    if ((code == 4) && (arg2 == 0)) {
        goto nolog2;
    }
    if ((verbose & V_SYS) && (!(sp->flag & SF_SMALL) || (verbose & V_ASYS))) {
        if ((code == 2) && (ret == 0)) {
            message("\n%d: fork() ", mypid);
        }
        if (z80_get_reg8(f_reg) & 1) {
            message(" = %04x %d %s\n", ret, errno, strerror(errno));
        } else {
            message(" = %04x\n", ret);
        }
    }
  nolog2:
    if ((verbose & V_DATA) && (sp->flag & SF_BUF)) {
        dumpmem(&get_byte, arg1, ret);
        fflush(stdout);
    }
    z80_set_reg16(pc_reg, pop());
    z80_set_reg8(status_reg, 0);
    verbose = savemode;
    if ((verbose & V_SYS) && (z80_get_reg16(hl_reg) == 0xffff)) {
        message("code = %d\n", code);
        dumpcpu();
    }
}
