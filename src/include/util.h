/*
 * generally useful utility functions
 *
 * util.h
 * Changed: <2021-12-23 15:35:39 curt>
 */

typedef unsigned long long u64;
typedef unsigned short u16;
typedef unsigned char u8;

extern char *bitdef(u8 v, char**defs);
void skipwhite(char **s);

void dumpmem(unsigned char (*readbyte)(u16 addr), u16 addr, int len);
void hexdump(void *addr, int len);
void blockedit(char *buf, int len);

extern int logfd;
extern int no_console_log;

/*
 * l/lc/trace/tracec are the simulator's logging and tracing.  They are
 * performance-relevant: even when every trace bit is off, a trace() call
 * still builds its varargs (printable(), bitdef(), dis_space() ...) and
 * enters the function, and l()/lc() always format.  A hardware-facing
 * build compiles with -DNODEBUG to turn all four into no-ops so the
 * compiler drops the call and its argument evaluation together.
 *
 * register_trace() names a trace bit and tracenames[] holds the names for
 * usage()/pverbose(); traceflags is the bitmask currently being traced.
 * All three are tracing and vanish under -DNODEBUG.  register_trace is a
 * macro there that evaluates to 0, so the per-driver
 *
 *	trace_hddma = register_trace("hddma");
 *
 * still parses and every flag stays off; the trace_xxx variables those
 * lines fill are never read because the traceflags checks are compiled
 * out alongside the tracing itself.
 */
#ifdef NODEBUG
#define l(...)          ((void)0)
#define lc(...)         ((void)0)
#define trace(...)      ((void)0)
#define tracec(...)     ((void)0)
#define register_trace(name)    0
#else
int register_trace(char *name);
extern char *tracenames[];
extern int traceflags;
void l(const char *format, ...);
void lc(const char *format, ...);
void trace(int bits, const char *format, ...);
void tracec(int bits, const char *format, ...);
#endif

extern int devnum(char *name, char *dtp, int *majorp, int *minorp);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
