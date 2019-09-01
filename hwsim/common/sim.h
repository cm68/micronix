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
