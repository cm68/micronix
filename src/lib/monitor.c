/*
 * the command line processor
 *
 * lib/monitor.c
 *
 * Changed: <2023-06-23 13:43:54 curt>
 *
 */
#include <curses.h>
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
 * breakpoints and watchpoints are handled using the same data structure
 */
struct point {
    unsigned short addr;
    int value;
    struct point *next;
};

struct point *breaks;
struct point *watches;

struct point *
point_at(struct point *head, unsigned short addr, struct point **pp)
{
    struct point *p;

    if (pp)
        *pp = 0;
    for (p = head; p; p = p->next) {
        if (p->addr == addr) {
            break;
        }
        if (pp) {
            *pp = p;
        }
    }
    return p;
}

int
breakpoint_at(unsigned short addr)
{
    if (point_at(breaks, addr, 0))
        return 1;
    return 0;
}

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
            message("value %02x at %04x changed to %02x\n",
                p->value, p->addr, n);
            p->value = n;
            return (1);
        }
    }
    return (0);
}

void
add_breakpoint(unsigned short addr)
{
    struct point *p;

    p = malloc(sizeof(*p));
    p->addr = addr;
    p->next = breaks;
    breaks = p;
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
        i = wgetnstr(win[W_CMD], cmdline, sizeof(cmdline));
        s = cmdline;

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

int
next_cmd(char **sp)
{
    message("unimplemented\n");
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
point_cmd(char **sp, struct point **head)
{
    unsigned short addr = 0;
    int delete = 0;
    struct point *p, *prev;

    if (**sp == '-') {
        (*sp)++;
        delete = 1;
    }

    while (**sp) {
        skipwhite(sp);
        addr = strtol(*sp, sp, 16);
        p = point_at(*head, addr, &prev);
        if (p && delete) {
            if (prev) {
                prev->next = p->next;
            } else {
                *head = p->next;
            }
            free(p);
        } else if ((!p) && (!delete)) {
            p = malloc(sizeof(*p));
            p->addr = addr;
            p->next = *head;
            *head = p;
        }
    }

    if (addr == 0) {
        if (delete) {
            while ((p = *head)) {
                *head = p->next;
                free(p);
            }
        } else {
            for (p = *head; p; p = p->next) {
                message("%04x\n", p->addr);
            }
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
    point_cmd(sp, &breaks);
    return 0;
}

/*
 * add or delete watchpoint
 * w [-][addr] [...]
 */
int
watch_cmd(char **sp)
{
    point_cmd(sp, &watches);
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
    register_mon_cmd('h', "\t\t\thelp", help_cmd);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */