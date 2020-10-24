/*
 * data structures known used by the disassembler hook
 */

/*
 * the number of bytes to adjust the return address on the stack by.
 * since the rst1 already advanced to point at the function code, 1 has
 * already been advanced over. we minimally need to bump again by 1, for
 * the function code.
 */

struct syscall {
	char argbytes;
	char *name;
	short flag;
};
#define	SF_NAME		1
#define	SF_NAME2	2
#define	SF_FD		4
#define	SF_ARG1		8
#define	SF_ARG2		16
#define	SF_ARG3		32
#define	SF_ARG4		64
#define	SF_BUF		128
#define	SF_SMALL	256

extern struct syscall syscalls[];
extern char *signame[];
