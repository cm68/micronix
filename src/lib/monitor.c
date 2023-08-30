/*
 * the command line processor
 *
 * lib/monitor.c
 *
 * Changed: <2023-07-29 09:12:49 curt>
 *
 */
#define trace xxtrace
#include <curses.h>
#undef trace

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "gui.h"
#include "sim.h"
#include "disz80.h"
#include "mnix.h"

extern unsigned short lookup_sym(char *);
extern unsigned char get_byte(unsigned short addr);
extern int mypid;
extern int verbose;
extern void pverbose();

/* dump line format */
#define DUMP_ADDR   0
#define DUMP_DATA   6
#define DUMP_CHAR   57

#define LISTLINES   5

extern WINDOW **win;

char cmdline[100];

struct moncmd {
    char cmd;
    char *help;
    int (*handler)(char **cmdlinep);
};

struct moncmd *moncmds;
int nmoncmds;

char sys_stop[NSYS];    // number of syscalls to skip before stopping
char sys_trace[NSYS];   // trace out

unsigned short lastaddr;
int fmt_indir_sc = 1;

/*
 * breakpoints and watchpoints are handled using an 8k bitmap
 */
int watchpoint_touched;

unsigned short next_was;
unsigned short next_break;

char breaks[8192];
char watches[8192];
int nwatches;
int nbreaks;

int
breakpoint_at(unsigned short addr)
{
    if (!nbreaks) return 0;
    if (breaks[addr / 8] & (1 << (addr % 8))) {
        if (addr == next_break) {
            if (!next_was) {
                breaks[addr / 8] &= ~(1 << (addr % 8));
            }
        }
        return 1;
    }
    return 0;
}

int
watchpoint_at(unsigned short addr)
{
    if (!nwatches) return 0;
    if (watches[addr / 8] & (1 << (addr % 8)))
        return 1;
    return 0;
}

int
watchpoint_hit()
{
    if (watchpoint_touched) {
        watchpoint_touched = 0;
        return 1;
    }
    return 0;
}

void
add_breakpoint(unsigned short addr)
{
    if (breakpoint_at(addr))
        return;
    breaks[addr / 8] |= (1 << (addr % 8));
    nbreaks++;
    printf("added breakpoint at %04x\n", addr);
}

void
add_watchpoint(unsigned short addr)
{
    if (watchpoint_at(addr))
        return;
    watches[addr / 8] |= (1 << (addr % 8));
    nwatches++;
}

/*
 * get an address for a command
 * consume the token and advance
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
    /* special symbols for register names */

    if (strcasecmp(wordbuf, "bc") == 0) {
        return z80_get_reg16(bc_reg);
    }
    if (strcasecmp(wordbuf, "de") == 0) {
        return z80_get_reg16(de_reg);
    }
    if (strcasecmp(wordbuf, "hl") == 0) {
        return z80_get_reg16(hl_reg);
    }
    if (strcasecmp(wordbuf, "ix") == 0) {
        return z80_get_reg16(ix_reg);
    }
    if (strcasecmp(wordbuf, "iy") == 0) {
        return z80_get_reg16(iy_reg);
    }
    if (strcasecmp(wordbuf, "pc") == 0) {
        return z80_get_reg16(pc_reg);
    }
    if (strcasecmp(wordbuf, "sp") == 0) {
        return z80_get_reg16(sp_reg);
    }
    if (strcasecmp(wordbuf, "tos") == 0) {
        return get_word(z80_get_reg16(sp_reg));
    }
    i = lookup_sym(wordbuf);
    if (i != 0) {
        return i;
    }
    i = strtol(wordbuf, &wp, 16);
    return i;
}

/*
 * the return value is encoded as follows:
 * 0 - 
 * return 1 if stopped, else 0
 */
int
monitor()
{
    char c;
    int i;
    char *s;
    int addr;

    while (1) {
        dumpcpu();
        message("%d >>> ", mypid);
        i = read_line(cmdline, sizeof(cmdline));
        s = cmdline;

        if (i < 1) {
            printf("read_line returned %d\n", i);
            exit(4);
        }
        skipwhite(&s);
        c = *s++;
        skipwhite(&s);

        for (i = 0; i < nmoncmds; i++) {
            if (moncmds[i].cmd == c) {
                i = (*moncmds[i].handler)(&s);
                if (i != 0) return i;
                break;
            }
        }
        if (i == nmoncmds) {
            message("unknown command character %c\n", c);
        }
    }
}

/*
 * all the command processors take a pointer to the pointer to the input string
 * and we have skipped any white space.  if we're at the end of the command,
 * we're pointing at a null.
 *
 * commands return 0 if we want to stay interactive
 * return <n> if thats how many instructions we want to step
 * return -1 if we want to run 
 */

void
register_mon_cmd(char c, char *help, int (*handler)(char **p))
{
    moncmds = realloc(moncmds, sizeof(moncmds[0]) * (nmoncmds + 1));
    moncmds[nmoncmds].cmd = c;
    moncmds[nmoncmds].help = help;
    moncmds[nmoncmds].handler = handler;
    nmoncmds++;
}

/*
 * we do something gnarly here:  
 * we'll call the instruction formatter to find out how many bytes are used
 * by this instruction, and put a temporary breakpoint just after it.
 */

int
next_cmd(char **sp)
{
    char outbuf[60];
    unsigned short pc;
    int i;

    pc = z80_get_reg16(pc_reg);
    i = format_instr(pc, outbuf);    
    next_was = breakpoint_at(pc + i);
    next_break = pc + i;
    add_breakpoint(pc + i);
    return 0;
}

int
step_cmd(char **sp)
{
    int i;

    if (**sp) {
        i = strtol(*sp, sp, 16);
    } else {
        i = 1;
    }
    return (i);
}

int
go_cmd(char **sp)
{
    return (-1);
}

/*
 * list, add or delete syscall trace
 * [-]<syscall> ...
 */
int
trace_cmd(char **sp)
{
    int i;
    int delete = 0;
    int k = 0;

    if (!**sp) {
        message("tracing:\n");
        for (i = 0 ; i < NSYS; i++) {
            if (!sys_trace[i]) continue;
            k++;
            message("%s\t", syscalls[i].name);
            if ((k % 9) == 0) message("\n");
        }
        if ((k % 9) != 0) message("\n");
    }

    if (**sp == '-') {
        (*sp)++;
        delete = 1;
    }

    while (**sp) {
        skipwhite(sp);
        i = get_syscall(sp);
        if (i == -1) {
            message("unrecognized syscall\n");
            return 0;
        } 
        sys_trace[i] = delete ? 0 : 1;
    }
    return 0;
}

/*
 * list, add or delete syscall stops
 * <syscall>[=<skip count>] ...
 */
int
stop_cmd(char **sp)
{
    int i;
    int k = 0;

    if (!**sp) {
        message("syscall stops:\n");
        for (i = 0 ; i < NSYS; i++) {
            if (!sys_stop[i]) continue;
            k++;
            message("%s=%d\t", syscalls[i].name, sys_stop[i]);
            if ((k % 7) == 0) message("\n");
        }
        if ((k % 7) != 0) message("\n");
    }

    while (**sp) {
        skipwhite(sp);
        i = get_syscall(sp);
        if (i == -1) {
            message("unrecognized syscall\n");
            return 0;
        } 
        skipwhite(sp);
        if (**sp == '=') {
            (*sp)++;
            k = strtol(*sp, sp, 10);
        } else {
            k = 1;
        }
        sys_stop[i] = k;
    }
    return 0;
}

int
verbose_cmd(char **sp)
{
    if (**sp) {
        verbose = strtol(*sp, sp, 16);
    }
    pverbose();
    return 0;
}

/*
 * list code
 * l [addr]
 */
int
list_cmd(char **sp)
{
    unsigned short addr;
    int l;
    int c;
    char *s;

    if (**sp) {
        addr = getaddress(sp);
    } else {
        if (lastaddr == -1) {
            addr = z80_get_reg16(pc_reg);
        } else {
            addr = lastaddr;
        }
    }
    for (l = 0; l < LISTLINES; l++) {
        c = format_instr(addr, cmdline);
        if ((s = get_symname(addr))) {
            message("%s:\n", s);
        }
        message("%04x: %-20s\n", addr, cmdline);
        addr += c;
        lastaddr = addr & 0xffff;
    }
    return 0;
}

/*
 * hexdump at address
 * d [addr]
 */
int
dump_cmd(char **sp)
{
    unsigned short addr;

    if (**sp) {
        addr = getaddress(sp);
    } else {
        if (lastaddr == -1) {
            addr = 0;
        } else {
            addr = lastaddr;
        }
    }
    addr &= 0xffff;
    memdump(win[W_DUMP], addr, 256);
    lastaddr = (addr + 256) & 0xffff;
    return 0;
}

void
point_cmd(char **sp, char *map, int *mapcount)
{
    unsigned short addr = 0;
    int delete = 0;
    int index;
    int bit;

    if (**sp == '-') {
        (*sp)++;
        delete = 1;
    }

    while (**sp) {
        skipwhite(sp);
        addr = getaddress(sp);
        if (addr == 0) break;

        index = addr / 8;
        bit = (1 << (addr % 8));

        if (map[index] & bit) {
            if (delete) {
                (*mapcount)--;
                map[index] &= ~bit;
            }
        } else if (!delete) {
            (*mapcount)++;
            map[index] |= bit;
        }
    }

    if (addr == 0) {
        if (delete) {
            *mapcount = 0;
            for (index = 0; index < 8192; index++) {
                map[index] = 0;
            }
        } else {
            int k = 0;
            for (index = 0; index < 8192; index++) {
                for (bit = 0; bit < 8; bit++) {
                    if (map[index] & (1 << bit)) {
                        message("%04x\t", (index * 8) + bit);
                        k++;
                        if ((k % 9) == 0) message("\n");
                    }
                }
            }
            if ((k % 9) != 0) message("\n");
        }
    }
}

/*
 * add or delete breakpoint
 * [-][<addr>] [...]
 */
int
break_cmd(char **sp)
{
    point_cmd(sp, breaks, &nbreaks);
    return 0;
}

/*
 * add or delete watchpoint
 * w [-][addr] [...]
 */
int
watch_cmd(char **sp)
{
    point_cmd(sp, watches, &nwatches);
    return 0;
}

int
exit_cmd(char **sp)
{
    exit(1);
}

int
help_cmd(char **sp)
{
    int i;

    for (i = 0; i < nmoncmds; i++) {
        if (moncmds[i].help)
            message("%c %s\n", moncmds[i].cmd, moncmds[i].help);
    }
    return 0;
}

/*
 * set up the monitor command processor
 */
void
mon_init()
{
    register_mon_cmd('b', "[-][<addr>] ...\tadd or delete breakpoint", break_cmd);
    register_mon_cmd('c', "<syscall>[=<skip count>] ...\tadd or delete syscall stop", 
        stop_cmd);
    register_mon_cmd('d', "[addr]\t\tdump memory", dump_cmd);
    register_mon_cmd('g', "[address]\t\tgo", go_cmd);
    register_mon_cmd('l', "[addr]\t\tlist instructions", list_cmd);
    register_mon_cmd('n', "\t\t\tstep over", next_cmd);
    register_mon_cmd('q', "\t\t\tquit", exit_cmd);
    register_mon_cmd('s', "[inst count]\t\tstep", step_cmd);
    register_mon_cmd('t', "[-]<syscall> ...\tadd or delete syscall trace", trace_cmd);
    register_mon_cmd('v', "<verbosity>\t\tset verbosity", verbose_cmd);
    register_mon_cmd('w', "[-][<addr>] ...\tadd or delete watchpoint", watch_cmd);
    register_mon_cmd('h', "\t\t\thelp", help_cmd);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
