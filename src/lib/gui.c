/*
 * the curses gui
 *
 * lib/gui.c
 *
 * Changed: <2023-07-27 16:33:02 curt>
 *
 */
#include <curses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "gui.h"
#include "sim.h"
#include "disz80.h"
#include "mnix.h"

extern unsigned short lookup_sym(char *);
extern unsigned char get_byte(unsigned short addr);
extern int mypid;
extern int verbose;
extern void pverbose();
extern unsigned short lastaddr;

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

#define REG_ROWS    6
#define STACK_ROWS  16
#define DUMP_ROWS   16
#define DIS_ROWS    4
#define CMD_ROWS    4

#define MIN_FOR_STACK   (REG_ROWS+STACK_ROWS+DUMP_ROWS+DIS_ROWS+CMD_ROWS)
#define MIN_LINES   (REG_ROWS + DUMP_ROWS + DIS_ROWS + CMD_ROWS)

void
makewins(FILE *tty)
{
    int i;
    int row;
    int lines;
    int cols;
    struct winsize w;

    if (ioctl(fileno(tty), TIOCGWINSZ, &w) != 0) {
        perror("can't get window size");
        exit(1);
    }
    lines = w.ws_row;
    cols = w.ws_col;
    if (lines < MIN_LINES) {
        fprintf(stderr, "need minimum of %d lines: got %d\n", MIN_LINES, lines);
        exit(1);
    }


    if (!win) {
        win = malloc(sizeof(WINDOW *) * W_COUNT);
    }

    if (!(screen = newterm("xterm", tty, tty))) {
        perror("newterm fail");
        exit(1);
    }
    set_term(screen);
    cbreak();

    /* make register windows */
    for (i = 0; i < sizeof (reglayout)/sizeof(reglayout[0]); i++) {
        win[reglayout[i].index] = labelwin(reglayout[i].label, 3,
            reglayout[i].w, reglayout[i].y, reglayout[i].x);
    }
    row = REG_ROWS;

    /* if there's room, make window for stack dump */
    if (lines > MIN_FOR_STACK) {
        win[W_STACK] = labelwin("stack", STACK_ROWS, 78, row, 0);
        row += STACK_ROWS;
    }

    win[W_DUMP] = labelwin("dump", DUMP_ROWS, 78, row, 0);
    row += DUMP_ROWS;

    disas_lines = (lines - (row + 4)) / 2;

    win[W_DIS] = labelwin("disassembly", disas_lines + 2, 78, row, 0);
    scrollok(win[W_DIS], TRUE);
    clear_dis();

    row += disas_lines + 2;
    win[W_CMD] = labelwin("command", lines - row, 78, row, 0);
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

/*
 * format flags into fbuf
 */
void
fflags(unsigned char f, char *fbuf)
{
    int i;
    char fname[] = "CNVXHYZS";

    for (i = 0; i < sizeof(fbuf); i++) {
        fbuf[i] = ((1 << i) & f) ? fname[i] : ' ';
    }
    fbuf[8] = '\0';
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
    for (i = 0; i < 7; i++) { 
        message(disas_pc[i] == -1 ? "%d " : "%04x ", disas_pc[i]); 
    }
    message("\n");
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
    char fbuf[9];

    pc = z80_get_reg16(pc_reg);

    if (!win) {
        char outbuf[40];
        char *s;
        format_instr(pc, outbuf);
        s = get_symname(pc);
        if (s) {
            message("%s:\n", s);
        }
        f = z80_get_reg8(f_reg);
        fflags(f, fbuf);
        message("bc: %04x de: %04x hl: %04x sp: %04x: af: %04x %s %04x %s\n",
            z80_get_reg16(bc_reg), z80_get_reg16(de_reg), z80_get_reg16(hl_reg), 
            z80_get_reg16(sp_reg), z80_get_reg8(a_reg) << 8 | z80_get_reg8(f_reg), fbuf, 
            pc, outbuf);
        return;
    }

    update_dis(pc);

    f = z80_get_reg8(f_reg);
    fflags(f, fbuf);
    mvwaddstr(win[W_F], 0, 0, fbuf);
    wrefresh(win[W_F]);

    f = z80_get_reg8(f1_reg);
    fflags(f, fbuf);
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
    if (win[W_STACK]) 
        memdump(win[W_STACK], z80_get_reg16(sp_reg) - 128, 256);
}

int
read_line(char *buf, int buflen)
{
    int i;
    if (win) {
        i = wgetnstr(win[W_CMD], buf, buflen);
    } else {
        if (fgets(buf, buflen, stdin) == NULL) {
            i = ERR;
        } else {
            i = strlen(buf);
            if (buf[i-1] == '\n')
                buf[i-1] = '\0';
            i = OK;
        }
    }
    return i;
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
        wrefresh(win[W_CMD]);
    }
    va_end(args);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
