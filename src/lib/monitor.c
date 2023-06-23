/*
 * the command line processor
 *
 * lib/monitor.c
 *
 * Changed: <2023-06-23 00:18:13 curt>
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

char sys_stop[NSYS];
char sys_trace[NSYS];

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

int
breakpoint_at(unsigned short addr)
{
    if (point_at(&breaks, addr, 0))
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
    message("added breakpoint at %04x\n", addr);
}

void
dump_stops()
{
    int i;

    for (i = 0 ; i < NSYS; i++) {
        if (!sys_stop[i]) continue;
        message("stopping syscall %s after %d\n", 
            syscalls[i].name, sys_stop[i]);
    }
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
 * return 1 if stopped, else 0
 */
int
monitor()
{
    struct point *p, *prev, **head;
    char l;
    char c;
    int i;
    int delete;
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
        c = format_instr(i, cmdline);
        s = get_symname(i);
        if (s) {
            message("%s\n", s);
        }
        message("%04x: %-20s\n", i, cmdline);
        i += c;
        lastaddr = i & 0xffff;
    }
    return 0;
}

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

break_cmd(char **sp)
{
}

next_cmd(char **sp)
{
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

go_cmd(char **sp)
{
    return (-1);
}

trace_cmd(char **sp)
{
}

verbose_cmd(char **sp)
{
}

#ifdef notdef

        head = &breaks;
        switch (c) {
        case 'v':
            while (*s && (*s == ' '))
                s++;
            if (*s) {
                verbose = strtol(s, &s, 16);
                pverbose();
            }
            break;
 
        case 'c':
            c = 1;
            while (*s && (*s == ' '))
                s++;
            if (!*s) {
                dump_stops();
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
                if (i < sizeof(sys_stop)) {
                    sys_stop[i] = c;
                }
            }
            break;

        case 'l':
            while (*s && (*s == ' '))
                s++;
            if (*s) {
                i = strtol(s, &s, 16);
            } else {
                if (lastaddr == -1) {
                    i = z80_get_reg16(pc_reg);
                } else {
                    i = lastaddr;
                }
            }
            for (l = 0; l < LISTLINES; l++) {
                c = format_instr(i, cmdline);
                s = get_symname(i);
                if (s) {
                    message("%s\n", s);
                }
                message("%04x: %-20s\n", i, cmdline);
                i += c;
                lastaddr = i & 0xffff;
            }
            break;
        case 's':
            while (*s && (*s == ' '))
                s++;
            if (*s) {
                i = strtol(s, &s, 16);
            } else {
                i = 1;
            }
            dumpcpu();
            return (i);
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
                        message("%04x\n", p->addr);
                    }
                }
            }
            break;
        case '?':
        case 'h':
            message("commands:\n");
            message("l <addr> :list\n");
            message("d <addr> :dump memory\n");
            message("g: continue\n");
            message("s: single step\n");
            message("q: exit\n");
            message("b [-] <nnnn> ... :breakpoint\n");
            message("w [-] <nnnn> ... :watchpoint\n");
            message("c [-] <nn> :system call trace\n");
            message("v <nn> :set verbose\n");
            break;
        default:
            message("unknown command %c\n", c);
            break;
        case 0:
            break;
        }
    }
}
#endif

/*
 * all the command processors take a pointer to the pointer to the input string
 * and we have skipped any white space.  if we're at the end of the command,
 * we're pointing at a null.
 *
 * all command processors assume that we are going to do another command.  if
 * we want to return to the simulation, we need to return from monitor.
 * that's the convention we use.  so, if our command returns 1, then we are
 * going to simulate some more.
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

    message("commands:\n");
    for (i = 0; i < nmoncmds; i++) {
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
    register_mon_cmd('l', "[addr]\tlist instructions", list_cmd);
    register_mon_cmd('b', "[-][<addr>] [...]\tadd or delete breakpoint", break_cmd);
    register_mon_cmd('d', "[addr]\tdump memory", dump_cmd);
    register_mon_cmd('s', "[inst count]\tstep", step_cmd);
    register_mon_cmd('n', "\tstep over", next_cmd);
    register_mon_cmd('g', "[address]\tgo", go_cmd);
    register_mon_cmd('r', "\tdump registers", regs_cmd);
    register_mon_cmd('t', "trace\tset trace", trace_cmd);
    register_mon_cmd('q', "\tquit", exit_cmd);
    register_mon_cmd('x', "\texit", exit_cmd);
    register_mon_cmd('v', "<verbosity>\tset verbosity", verbose_cmd);
    register_mon_cmd('h', "\thelp", help_cmd);
    register_mon_cmd('?', "\thelp", help_cmd);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
