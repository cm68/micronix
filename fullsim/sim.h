/*
 * global definitions for simulator
 */

/*
 * trace flags
 */
#define V_IO    (1 << 0)        // trace I/O instructions
#define V_INST  (1 << 1)        // instructions
#define V_IOR   (1 << 2)        // trace I/O registration
#define V_MAP   (1 << 3)        // address mapping
#define V_DJDMA (1 << 4)        // djdma
#define V_MIO   (1 << 5)        // multio
#define V_HDCA  (1 << 6)        // HDCA
#define V_MPZ   (1 << 7)        // MPZ80
#define V_IMD   (1 << 8)        // IMD processing
#define V_BIO   (1 << 9)        // dump disk blocks
#define V_HDDMA (1 << 10)       // block io

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

// tracing level
extern int register_trace(char *description);

// callbacks
extern void register_mon_cmd(char c, char *help, int (*handler)(char **p));
extern void register_usage_hook(void (*hookfunc)());
extern void register_prearg_hook(int (*hookfunc)());
extern void register_startup_hook(int (*hookfunc)());
extern void register_poll_hook(void (*hookfunc)());
extern void register_input(portaddr portnum, inhandler func);
extern void register_output(portaddr portnum, outhandler func);

extern void ioinit();
extern void output(portaddr p, byte v);
extern byte input(portaddr p);

extern void ioinit();
extern void output(portaddr p, byte v);

extern byte input(portaddr p);

// virtual floppy load/read/write/inquire functions
void *imd_load(char *fname);
int imd_read(void *ip, int drive, int cyl, int head, int sec, char *buf);
int imd_write(void *ip, int drive, int cyl, int head, int sec, char *buf);
void imd_trkinfo(void *vp, int cyl, int head, int *secs, int *secsize);

// useful utility functions
char *bitdef(byte v, char**desc);
void dumpmem(unsigned char (*readbyte) (vaddr addr), vaddr addr, int len);
void hexdump(void *addr, int len);
void skipwhite(char **sp);

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
