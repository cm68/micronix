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
extern int register_trace(char *tracename);

/* a driver registers one of these */
struct driver {
    char *name;
    void (*usage_hook)();
    int (*prearg_hook)();               // register trace points and extensions
    int (*startup_hook)();
    int (*poll_hook)();
};

extern void register_driver(struct driver *d);

// call these from the startup hook
extern void register_input(portaddr portnum, inhandler func);
extern void register_output(portaddr portnum, outhandler func);

/*
 * actual control line at the bus
 */
extern unsigned char vi_lines;      // mask: vi0 = 0x1, etc
extern int int_line;
extern int nmi_line;

/*
 * control line at the cpu pin - the cpu card could have a mask
 * this is used by the chip sim
 */
extern int int_pin;
extern int nmi_pin;

// called by driver to assert or clear vectored interrupt line
void set_vi(int signal, int card, int value);

extern void (*vi_change)(unsigned char new);
extern unsigned char (*get_intack)();

/* the cpu registers a hook that gets called when the bus int line changed */
extern void (*int_change)(int value);

// set if symbols are valid
int super();

// called by cpu simulator to get an intack byte
unsigned char int_ack();

// terminal creates an xterm that generates a signal when something is ready to read
extern void open_terminal(char *name, int signum, int *infdp, int *outfdp, int cooked, char *logfile);

// generally useful timed callout facility
void time_out(char *name, int usec, void (*func)(int a), int arg);
void recurring_time_out(char *name, int hertz, void (*func)(int a), int arg);
void cancel_time_out(void (*func)(), int arg);

// linux freaking signal madness
typedef void (*sighandler_t)(int);
sighandler_t mysignal(int signum, sighandler_t handler);

// hard disk abstraction
void *drive_open(char *name);
int drive_sectorsize(void *dhandle, int secsize);
int drive_write(void *dhandle, int cyl, int head, int sec, char *buf);
int drive_read(void *dhandle, int cyl, int head, int sec, char *buf);

/*
 * global simulator variables
 */
extern int traceflags;		// bitmask of subsystems to trace
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

enum reg8 { 
    a_reg, f_reg, b_reg, c_reg, d_reg, e_reg, h_reg, l_reg, 
    a1_reg, f1_reg, b1_reg, c1_reg, d1_reg, e1_reg, h1_reg, l1_reg, 
    i_reg, r_reg, irr_reg, im_reg, control_reg, status_reg 
};
enum reg16 { 
    pc_reg, sp_reg, 
    bc_reg, de_reg, hl_reg, 
    bc1_reg, de1_reg, hl1_reg, 
    ix_reg, iy_reg };

// control register bits
#define C_NMI   0x01
#define C_INT   0x02
#define C_RESET 0x04

// status register bits
#define S_M1    0x01
#define S_HLTA  0x02
#define S_INTA  0x04

#define IFF1    0x01
#define IFF2    0x02

byte z80_get_reg8(enum reg8 r8);
word z80_get_reg16(enum reg16 r16);

void z80_set_reg8(enum reg8 r8, byte v);
void z80_set_reg16(enum reg16, word v);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
