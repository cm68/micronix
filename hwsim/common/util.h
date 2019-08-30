/*
 * generally useful utility functions
 */

extern char *bitdef(unsigned char v, char**defs);
void skipwhite(char **s);
void dumpmem(unsigned char (*readbyte) (unsigned short addr), unsigned short addr, int len);
int register_trace(char *name);
void hexdump(void *addr, int len);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
