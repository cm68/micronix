/*
 * definitions for hardware simulator
 *
 * hwsim/hwsim.h
 *
 * Changed: <2023-06-23 17:22:35 curt>
 */

typedef byte (*inhandler)(portaddr port);
typedef void (*outhandler)(portaddr port, byte val);

extern inhandler input_handler[256];
extern outhandler output_handler[256];

// raw memory and i/o access - defined by bus
extern byte physread(paddr addr);
extern void physwrite(paddr addr, byte value);
extern void s100_output(portaddr p, byte v);
extern byte s100_input(portaddr p);

extern paddr ram_size;          // bytes of ram backed in the 24 bit space

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

// control port 0xd0: the kernel writes task, device and inode so the
// simulator can load the symbol table of the program being exec'd.  The
// kernel writes these bytes unconditionally on every exec, so the port
// handler stays registered under -DNODEBUG (as a no-op) rather than
// letting the write fall through to the "undefined port" diagnostic.
extern void ctrl_out(portaddr port, byte val);
extern void name_out(portaddr port, byte val);   // 0xd1: program name, NUL-term
#ifndef NODEBUG
extern void set_ctl_image(char *path);   // name the root filesystem image
extern char prog_name[16][32];           // exec'd program name per task
#endif

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
extern void multio_restore_terminal(void);

// generally useful timed callout facility
void time_out(char *name, int usec, void (*func)(int a), int arg);
void recurring_time_out(char *name, int hertz, void (*func)(int a), int arg);
void cancel_time_out(void (*func)(), int arg);

// linux freaking signal madness
typedef void (*sighandler_t)(int);
sighandler_t mysignal(int signum, sighandler_t handler);

// hard disk abstraction
void drive_setdir(char *dir);   // where drive_open looks for unit files
int drive_setunit(char *unit, char *path); // name one unit's file outright
void *drive_open(char *name);
char *drive_resolve(char *name); // the file a unit name opens
int drive_sectorsize(void *dhandle, int secsize);
int drive_write(void *dhandle, int cyl, int head, int sec, char *buf);
int drive_read(void *dhandle, int cyl, int head, int sec, char *buf);
void drive_format(void *dhandle, int firstsec, int seccode, int spt,
    int gap3, int fill, int cyl, int head); // record what a format laid down
// no sector: read header returns whichever one comes round next
int drive_header(void *dhandle, int cyl, int head, char *buf);
int drive_geometry(void *dhandle, int *cyls, int *heads, int *spt);

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
extern int halt_exit;		// exit the simulation on a task-0 halt

/*
 * Console arbitration.  Two devices can be the console - the serial
 * board and the floppy controller's own bit banged port - and they read
 * the same file descriptor, so exactly one of them may consume a
 * keystroke.  Which one is not configured anywhere and cannot be seen by
 * looking at a disk: it is however the bios on it was assembled.  So it
 * is discovered instead.  A device claims the console by using it, and
 * input follows the most recent claim.  See CONSOLE.
 */
#define	CONS_MULTIO	0		// the serial board, and the default
#define	CONS_DJDMA	1		// the floppy controller's serial port

extern int console_owner;
void console_claim(int who);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
/*
 * Watchpoints are checked on every physical write (and the virtual one on
 * every cpu store), so a hardware build drops them to no-ops under
 * -DNODEBUG rather than paying the range test in the hot path.
 */
#ifdef NODEBUG
#define add_write_watch(lo, hi)         ((void)0)
#define add_phys_watch(lo, hi)          ((void)0)
#define phys_watch_check(p, v)          ((void)0)
#else
extern void add_write_watch(unsigned short lo, unsigned short hi);
extern void add_phys_watch(unsigned int lo, unsigned int hi);
extern void phys_watch_check(unsigned int p, unsigned char v);
#endif
extern unsigned long long simnow64();
extern void take_pending_trap();
