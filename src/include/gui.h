/*
 * we use curses for the GUI, and we have certain regions where we plant content.
 *
 * include/gui.h
 *
 * Changed: <2023-06-20 09:45:48 curt>
 */

/* window ID's */
#define W_DIS   0
#define W_DUMP  1
#define W_STACK 2
#define	W_CMD	3

#define W_BC    4
#define W_DE    5
#define W_HL    6
#define W_SP    7
#define W_BC1   8
#define W_DE1   9
#define W_HL1   10
#define W_PC    11
#define W_IX    12
#define W_IY    13
#define W_AF    14
#define W_AF1   15
#define W_F     16
#define W_F1    17
#define W_IR    18
#define W_IFF   19
#define W_IRQ   20
#define W_IM    21

extern void message(char *fmt, ...);
extern void makewins(FILE *tty);
extern void dumpcpu();
extern int monitor();
extern void add_breakpoint(unsigned short addr);
extern void dump_stops();
extern int watchpoint_hit();
extern int breakpoint_at(unsigned short addr);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
