/*
 * sim.h
 *
 * global definitions for simulator
 */

// the instruction set simulator
void z80_reset();
void z80_run();

// anything in the simulator has one of these types
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long paddr;	// 24 bit physical address
typedef unsigned short vaddr;	// 16 bit virtual address
typedef byte portaddr;

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

// memory access - defined by bus
extern byte physread(paddr addr);
extern void physwrite(paddr addr, byte value);

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

#define	CONF_SET	0x80000000	// config specified
extern int config_sw;

// this gets filled in by the cpu simulator
struct cpuregs {
    byte *f_ptr;
    byte *a_ptr;
    byte *b_ptr;
    byte *c_ptr;
    byte *d_ptr;
    byte *e_ptr;
    byte *h_ptr;
    byte *l_ptr;
    word *bc_ptr;
    word *de_ptr;
    word *hl_ptr;
    word *sp_ptr;
    word *ix_ptr;
    word *iy_ptr;
    word *pc_ptr;
    byte *i_ptr;
    byte *r_ptr;
    byte *bus;
    byte bitmask[4];
#define M1      0           // which bit is set in bus for M1
#define INTA    1           // which bit is set in buf for INTACK
#define INT     2           // which bit to set for INT
#define NMI     3           // which bit to set for NMI
};

extern struct cpuregs cpu;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
