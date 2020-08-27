/*
 * generally useful utility functions
 */

typedef unsigned long long u64;

extern u64 now64();
extern char *bitdef(unsigned char v, char**defs);
void skipwhite(char **s);
void dumpmem(unsigned char (*readbyte) (unsigned short addr), unsigned short addr, int len);

int register_trace(char *name);
void hexdump(void *addr, int len);
extern char *tracenames[];

void log(char *format, ...);
void logc(char *format, ...);
void trace(int bits, char *format, ...);
void tracec(int bits, char *format, ...);
extern int traceflags;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
