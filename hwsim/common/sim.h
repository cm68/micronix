/*
 * sim.h
 *
 * global definitions for simulator
 */

// anything in the simulator has one of these types
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int paddr;	// 24 bit physical address
typedef unsigned short vaddr;	// 16 bit virtual address
typedef word portaddr;

typedef byte (*inhandler)(portaddr port);
typedef void (*outhandler)(portaddr port, byte val);

extern inhandler input_handler[256];
extern outhandler output_handler[256];

// memory and port access functions - all memory access by instruction set is here
extern void copyin(byte *buf, paddr pa, int len);	// copy from simulation
extern void copyout(byte *buf, paddr pa, int len);	// copy to simulation
extern void put_word(word addr, word value);
extern word get_word(word addr);

// memory access - defined by cpu card - address translation, etc happens here
extern void put_byte(word addr, byte value);
extern byte get_byte(word addr);
extern void output(portaddr p, byte v);
extern byte input(portaddr p);

// raw memory and i/o access - defined by bus
extern byte physread(paddr addr);
extern void physwrite(paddr addr, byte value);
extern void s100_output(portaddr p, byte v);
extern byte s100_input(portaddr p);

// register callbacks for plugging drivers - these can be in constructors
extern void register_mon_cmd(char c, char *help, int (*handler)(char **p));
extern void register_usage_hook(void (*hookfunc)());
extern void register_prearg_hook(int (*hookfunc)());
extern void register_startup_hook(int (*hookfunc)());
extern void register_poll_hook(void (*hookfunc)());
extern int register_trace(char *tracename);

// call these from the startup hook
extern void register_input(portaddr portnum, inhandler func);
extern void register_output(portaddr portnum, outhandler func);

/*
 * drivers call set_interrupt on vectored lines
 * interrupt controller registers for handlers on vectored line and intvec.
 * cpu registers for intack and registers for int and nmi
 * chip simulator calls intack and looks at nmi and int line state
 * vectors can be up to 3 bytes long, filled from low end first.
 * so, a call to 1234 will be encoded as 0x031234cd
 */
#define INTLEN_SHIFT    24
#define INTA_LEN        (3 << INTLEN_SHIFT)
typedef unsigned int intvec;

typedef enum { vi_0, vi_1, vi_2, vi_3, vi_4, vi_5, vi_6, vi_7, interrupt, nmi, errorint, pwrfail } int_line;
typedef enum { int_clear, int_set } int_level;

// called by interrupt controller
void register_interrupt(int_line signal, void (*handler)(int_line signal, int_level level));

// called by cpu card
void register_intvec(intvec (*handler)());
void register_intack(intvec (*handler)());

intvec get_intvec();

// called by cpu simulator
intvec get_intack();

// called by driver
void set_interrupt(int_line signal, int_level level);

// maintained by interrupt handlers
extern int_level int_pin;
extern int_level nmi_pin;

// terminal creates an xterm that generates a signal when something is ready to read
extern void open_terminal(char *name, int signum, int *infdp, int *outfdp, int cooked, char *logfile);

/*
 * global simulator variables
 */
extern int trace;		// bitmask of subsystems to trace
extern int rom_size;		// set this nonzero to read
extern char *rom_image;		// binary from
extern char *rom_filename;	// here

extern int trace_bio;		// multiple drivers trace block i/o
extern int running;

#define	CONF_SET	0x80000000	// config specified
extern int config_sw;

// the instruction set simulator interface
void z80_init();
void z80_run();

enum reg8 { a_reg, f_reg, b_reg, c_reg, d_reg, e_reg, h_reg, l_reg, i_reg, r_reg, irr_reg, control_reg, status_reg };
enum reg16 { pc_reg, sp_reg, bc_reg, de_reg, hl_reg, ix_reg, iy_reg };

// control register bits
#define C_NMI   0x01
#define C_INT   0x02
#define C_RESET 0x04

// status register bits
#define S_M1    0x01
#define S_HLTA  0x02
#define S_INTA  0x04

byte z80_get_reg8(enum reg8 r8);
word z80_get_reg16(enum reg16 r16);

void z80_set_reg8(enum reg8 r8, byte v);
void z80_set_reg16(enum reg16, word v);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
