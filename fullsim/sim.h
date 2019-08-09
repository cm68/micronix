/*
 * global definitions for simulator
 */

/*
 * trace flags
 */
#define V_IO    (1 << 0)        // trace I/O instructions
#define V_INST  (1 << 1)        // instructions
#define V_IOR   (1 << 2)        // trace I/O registration
#define V_MAP   (1 << 2)        // address mapping

typedef unsigned char byte;

typedef unsigned long paddr;	// 24 bit physical address
typedef unsigned short vaddr;	// 16 bit virtual address
typedef byte portaddr;

typedef byte (*inhandler)(portaddr port);
typedef void (*outhandler)(portaddr port, byte val);

extern inhandler input_handler[256];
extern outhandler output_handler[256];

extern void register_startup_hook(int (*hookfunc)());
extern void register_input(portaddr portnum, inhandler func);
extern void register_output( portaddr portnum, outhandler func);

extern void ioinit();
extern void output(portaddr p, byte v);
extern byte input(portaddr p);

extern void ioinit();
extern void output(portaddr p, byte v);

extern byte input(portaddr p);

int imd_read(void *ip, int drive, int trk, int side, int sec, char *buf);
void *load_imd(char *fname);
