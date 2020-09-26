/*
 * generally useful utility functions
 */

typedef unsigned long long u64;

extern u64 now64();
extern char *bitdef(unsigned char v, char**defs);
void skipwhite(char **s);

void dumpmem(char (*readbyte)(int addr), int addr, int len);
void hexdump(void *addr, int len);

int register_trace(char *name);
extern char *tracenames[];
extern int traceflags;

extern int logfd;

void Log(const char *format, ...);
void Logc(const char *format, ...);
void trace(int bits, const char *format, ...);
void tracec(int bits, const char *format, ...);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
