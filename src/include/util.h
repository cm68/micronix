/*
 * generally useful utility functions
 *
 * util.h
 * Changed: <2021-12-23 15:35:39 curt>
 */

typedef unsigned long long u64;
typedef unsigned short u16;
typedef unsigned char u8;

extern u64 now64();
extern char *bitdef(u8 v, char**defs);
void skipwhite(char **s);

void dumpmem(unsigned char (*readbyte)(u16 addr), u16 addr, int len);
void hexdump(void *addr, int len);

int register_trace(char *name);
extern char *tracenames[];
extern int traceflags;

extern int logfd;

void l(const char *format, ...);
void lc(const char *format, ...);
void trace(int bits, const char *format, ...);
void tracec(int bits, const char *format, ...);

extern int devnum(char *name, char *dtp, int *majorp, int *minorp);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
