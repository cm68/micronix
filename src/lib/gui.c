/*
 * the curses gui
 *
 * lib/gui.c
 *
 * Changed: <2023-06-20 10:50:13 curt>
 *
 */
#include <curses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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

WINDOW **win;
SCREEN *screen;

int disas_lines;
int disas_line;
int disas_pc[100];

void clear_dis();

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

struct {
    int index;
    char *label;
    int x;
    int y;
    int w;
} reglayout[] = {
    { W_BC,     "BC", 0, 0, 6 },
    { W_DE,     "DE", 6, 0, 6},
    { W_HL,     "HL", 13, 0, 6 },
    { W_SP,     "SP", 27, 0, 6 },
    { W_BC1,    "BC\'", 0, 3, 6 },
    { W_DE1,    "DE\'", 6, 3, 6 },
    { W_HL1,    "HL\'", 13, 3, 6 },
    { W_PC,     "PC", 27, 3, 6 },
    { W_IX,     "IX", 20, 0, 6 },
    { W_IY,     "IY", 20, 3, 6 },
    { W_AF,     "AF", 34, 0, 6 },
    { W_AF1,    "AF\'", 34, 3, 6 },
    { W_F,       0, 40, 0, 10 },
    { W_F1,      0, 40, 3, 10 },
    { W_IR,     "IR", 50, 0, 6 },
    { W_IM,     "IM", 57, 0, 3 },
    { W_IFF,    "IFF", 61, 0, 4 },
    { W_IRQ,    "IRQ", 66, 0, 4 }
};

void
memdump(WINDOW *w, unsigned short addr, int len)
{
    int i;
    char c;
    int pcol;
	int pline;

    wclear(w);

    pcol = 0;
    pline = 0;

    while (len) {
        if (pcol == 0)
            mvwprintw(w, pline, DUMP_ADDR, "%04x:", addr);
        c = get_byte(addr++);
        mvwprintw(w, pline, DUMP_DATA + (pcol * 3) + (pcol / 4),
            "%02x", c & 0xff);
	if ((c < ' ') || (c > 0x7e)) c = '.';
        mvwaddch(w, pline, DUMP_CHAR + (pcol) + (pcol / 4), c);
        if (pcol++ == 15) {
            pcol = 0;
            pline++;
        }
        len--;
    }
    wrefresh(w);
    if (w->_parent) wrefresh(w->_parent);
}

WINDOW *
labelwin(char *label, int high, int wide, int posy, int posx)
{
    WINDOW *frame;
    WINDOW *content;

    frame = newwin(high, wide, posy, posx);
    content = derwin(frame, high - 2, wide - 2, 1, 1);
    if (label) {
        wborder(frame, 0, 0, 0, 0, 0, 0, 0, 0);
        mvwaddstr(frame, 0, 1, label);
    }
    wrefresh(frame);
    return content;
}

void
makewins(FILE *tty)
{
    int i;

    if (!win) {
        win = malloc(sizeof(WINDOW *) * 
            ((sizeof(reglayout)/sizeof(reglayout[0])) + 4));
    }

    if (!(screen = newterm("xterm", tty, tty))) {
        perror("newterm fail");
        exit(1);
    }
    if (COLS < 80) {
        perror("not enough columns");
        exit(1);
    }

    cbreak();

    for (i = 0; i < sizeof (reglayout)/sizeof(reglayout[0]); i++) {
        win[reglayout[i].index] = labelwin(reglayout[i].label, 3,
            reglayout[i].w, reglayout[i].y, reglayout[i].x);
    }
    win[W_STACK] = labelwin("stack", 18, 78, 6, 0);
    win[W_DUMP] = labelwin("dump", 18, 78, 24, 0);
    disas_lines = LINES - 57;
    // disas_lines = 6;
    win[W_DIS] = labelwin("disassembly", disas_lines + 2, 78, 42, 0);
    scrollok(win[W_DIS], TRUE);
    clear_dis();
    win[W_CMD] = labelwin("command", 12, 78, LINES - 13, 0);
    scrollok(win[W_CMD], TRUE);
    wnoutrefresh(win[W_CMD]);
    doupdate();
}

void
regout(int regnum, unsigned short val)
{
    WINDOW *w = win[regnum];
    char buf[6];
    sprintf(buf, "%04x", val);
    mvwaddstr(w, 0, 0, buf);
    wrefresh(w);
    wrefresh(w->_parent);
}

char fbuf[9];

void
fflags(unsigned char f)
{
    int i;
    for (i = 0; i < sizeof(fbuf); i++) fbuf[i] = ' ';
    fbuf[8] = '\0';

    if (f & 1)
        fbuf[0] = 'C';
    if (f & 2)
        fbuf[1] = 'N';
    if (f & 4)
        fbuf[2] = 'V';
    if (f & 8)
        fbuf[3] = 'X';
    if (f & 16)
        fbuf[4] = 'H';
    if (f & 32)
        fbuf[5] = 'Y';
    if (f & 64)
        fbuf[6] = 'Z';
    if (f & 128)
        fbuf[7] = 'S';
}

void
clear_dis()
{
    int i;
    for (i = 0; i < disas_lines; i++) {
        disas_pc[i] = -1;
    }
}

/*
 * be smarter about disassembly window
 * keep track of where we are and update a cursor if we loop back
 */
void
update_dis(unsigned short pc)
{
    char outbuf[40];
    int i;
    char *s;

    /* message("pc: %x disas_lines: %d disas_line: %d:", 
        pc, disas_lines, disas_line);
    for (i = 0; i < 7; i++) { message(disas_pc[i] == -1 ? "%d " : "%04x ", disas_pc[i]); } ; message("\n");
    */

    mvwaddstr(win[W_DIS], disas_line, 0, "  ");
    for (i = 0; i < disas_lines; i++) {
        if (disas_pc[i] == pc) {
            // message("hit at %d\n", i);
            mvwaddstr(win[W_DIS], i, 0, "->");
            disas_line = i;
            return;
        }
    }

    if (disas_pc[disas_line] != -1) {
        disas_line++;
    }
    if (disas_line == disas_lines) {
        // message("disas scroll\n");
        for (i = 1 ; i < disas_lines; i++) {
            disas_pc[i-1] = disas_pc[i];
        }
        scroll(win[W_DIS]);
        disas_line--;
    }
    mvwaddstr(win[W_DIS], disas_line, 0, "->");
    disas_pc[disas_line] = pc;

    format_instr(pc, outbuf);
    s = get_symname(pc);
    if (s) {
        mvwprintw(win[W_DIS], disas_line, 2, "%s:", s);
    }
    mvwprintw(win[W_DIS], disas_line, 12, "%04x %s", pc, outbuf);
    wrefresh(win[W_DIS]); wrefresh(win[W_DIS]->_parent);
}

void
dumpcpu()
{
    unsigned short af;
    unsigned char f;
    unsigned short pc;

    if (!win)
        return;

    pc = z80_get_reg16(pc_reg);

    update_dis(pc);

    f = z80_get_reg8(f_reg);
    fflags(f);
    mvwaddstr(win[W_F], 0, 0, fbuf);
    wrefresh(win[W_F]);

    f = z80_get_reg8(f1_reg);
    fflags(f);
    mvwaddstr(win[W_F1], 0, 0, fbuf);
    wrefresh(win[W_F1]);

    mvwaddch(win[W_IM], 0, 0, '0' + z80_get_reg8(im_reg));
    wrefresh(win[W_IM]);

    f = z80_get_reg8(iff_reg);
    mvwaddch(win[W_IFF], 0, 0, (f & IFF1) ? '1' : ' ');
    mvwaddch(win[W_IFF], 0, 0, (f & IFF2) ? '2' : ' ');
    wrefresh(win[W_IFF]);

    mvwaddch(win[W_IRQ], 0, 0, int_pin ? '1' : ' ');
    wrefresh(win[W_IRQ]);

    regout(W_IR, (z80_get_reg8(i_reg) << 8) | z80_get_reg8(r_reg));
    regout(W_AF, z80_get_reg8(a_reg) << 8 | z80_get_reg8(f_reg));
    regout(W_AF1, z80_get_reg8(a1_reg) << 8 | z80_get_reg8(f1_reg));
    regout(W_BC, z80_get_reg16(bc_reg));
    regout(W_DE, z80_get_reg16(de_reg));
    regout(W_HL, z80_get_reg16(hl_reg));
    regout(W_BC1, z80_get_reg16(bc1_reg));
    regout(W_DE1, z80_get_reg16(de1_reg));
    regout(W_HL1, z80_get_reg16(hl1_reg));
    regout(W_IX, z80_get_reg16(ix_reg));
    regout(W_IY, z80_get_reg16(iy_reg));
    regout(W_SP, z80_get_reg16(sp_reg));
    regout(W_PC, pc);
    memdump(win[W_STACK], z80_get_reg16(sp_reg) - 128, 256);
}

void
message(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    if (!win) {
        vfprintf(stderr, fmt, args);
    } else {
        vw_printw(win[W_CMD], fmt, args);
    }
    va_end(args);
}

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
    int addr;

    while (1) {
      more:
        message("%d >>> ", mypid);
        i = wgetnstr(win[W_CMD], cmdline, sizeof(cmdline));
        s = cmdline;
        c = *s++;
        while (*s && (*s == ' '))
            s++;
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
        case 'd':
            while (*s && (*s == ' '))
                s++;
            if (*s) {
                if ((i = lookup_sym(s)) != 0) {
                    addr = i;
                } else if (!strcmp(s, "bc")) {
                    addr  = z80_get_reg16(bc_reg);
                } else if (!strcmp(s, "de")) {
                    addr  = z80_get_reg16(de_reg);
                } else if (!strcmp(s, "hl")) {
                    addr  = z80_get_reg16(hl_reg);
                } else if (!strcmp(s, "ix")) {
                    addr  = z80_get_reg16(ix_reg);
                } else if (!strcmp(s, "iy")) {
                    addr  = z80_get_reg16(iy_reg);
                } else if (!strcmp(s, "sp") || !strcmp(s, "tos")) {
                    addr  = z80_get_reg16(sp_reg);
                } else {
                    addr = strtol(s, &s, 16);
                }
            } else {
                addr = lastaddr;
            }
            addr &= 0xffff;
            memdump(win[W_DUMP], addr, 256);
            lastaddr = (addr + 256) & 0xffff;
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

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
