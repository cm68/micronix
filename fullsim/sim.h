/*
 * global definitions for simulator
 */
typedef unsigned long paddr;	// 24 bit physical address
typedef unsigned short vaddr;	// 16 bit virtual address
typedef unsigned char portaddr;
typedef unsigned char portdata;

typedef portdata (*inhandler)(portaddr port);
typedef void (*outhandler)(portaddr port, portdata val);

extern inhandler input_handler[256];
extern outhandler output_handler[256];

extern void register_startup_hook(int (*hookfunc)());
extern void register_input(portaddr portnum, inhandler func);
extern void register_output( portaddr portnum, outhandler func);

extern void ioinit();
extern void output(portaddr p, portdata v);
extern portdata input(portaddr p);

extern void ioinit();
extern void output(portaddr p, portdata v);

extern portdata input(portaddr p);

int imd_read(void *ip, int drive, int trk, int side, int sec, char *buf);
void *load_imd(char *fname);
