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
void drive_setdir(char *dir);   // where drive_open looks for unit files
void *drive_open(char *name);
int drive_sectorsize(void *dhandle, int secsize);
int drive_write(void *dhandle, int cyl, int head, int sec, char *buf);
int drive_read(void *dhandle, int cyl, int head, int sec, char *buf);
void drive_format(void *dhandle, int firstsec, int seccode, int spt,
    int gap3, int fill);            // record what a format laid down
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
extern void add_write_watch(unsigned short lo, unsigned short hi);
extern unsigned long long simnow64();
extern void take_pending_trap();
