/*
 * decode the micronix system calls in the disassembler
 * and interactive simulator
 *
 * lib/mnix_sys.c
 *
 * Changed: <2023-06-23 12:44:24 curt>
 */
/*
 * the number of bytes to adjust the return address on the stack by.
 * since the rst1 already advanced to point at the function code, 1 has
 * already been advanced over. we minimally need to bump again by 1, for
 * the function code.
 */
#include "../include/disz80.h"
#include "../include/mnix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct syscall syscalls[] = {
/* 0  */	{3, "indir", SF_ARG1 },
/* 1  */	{1, "exit", SF_FD },
/* 2  */	{1, "fork", 0 },
/* 3  */	{5, "read", SF_FD|SF_ARG1|SF_ARG2|SF_BUF },
/* 4  */	{5, "write", SF_FD|SF_ARG1|SF_ARG2|SF_BUF },
/* 5  */	{5, "open", SF_NAME|SF_ARG2 },
/* 6  */	{1, "close", SF_FD },
/* 7  */	{1, "wait", 0 },
/* 8  */	{5, "creat", SF_NAME|SF_ARG2 },
/* 9  */	{5, "link", SF_NAME|SF_NAME2 },
/* 10  */	{3, "unlink", SF_NAME },
/* 11  */	{5, "exec", SF_NAME|SF_ARG2 },
/* 12  */	{3, "chdir", SF_NAME },
/* 13  */	{1, "time", 0 },
/* 14  */	{7, "mknod", SF_NAME|SF_ARG2|SF_ARG3 },
/* 15  */	{5, "chmod", SF_NAME|SF_ARG2 }, 
/* 16  */	{5, "chown", SF_NAME|SF_ARG2 },
/* 17  */	{3, "sbrk", SF_ARG1|SF_SMALL },
/* 18  */	{5, "stat", SF_NAME|SF_ARG2 },
/* 19  */	{5, "seek", SF_FD|SF_ARG1|SF_ARG2 },
/* 20  */	{1, "getpid", 0 },
/* 21  */	{7, "mount", SF_NAME|SF_NAME2|SF_ARG3 },
/* 22  */	{3, "umount", SF_NAME },
/* 23  */	{1, "setuid", 0 },
/* 24  */	{1, "getuid", 0 },
/* 25  */	{1, "stime", 0 },
/* 26  */	{7, "ptrace", SF_ARG1|SF_ARG2|SF_ARG3 },/* pid addr req */
/* 27  */	{1, "alarm", 0 },
/* 28  */	{3, "fstat", SF_FD|SF_ARG1 },
/* 29  */	{1, "pause", 0 },
/* 30  */	{1, "bad", 0 },
/* 31  */	{3, "stty", SF_FD|SF_ARG1 },
/* 32  */	{3, "gtty", SF_FD|SF_ARG1 },
/* 33  */	{5, "access", SF_NAME|SF_ARG2 },
/* 34  */	{1, "nice", 0 },
/* 35  */	{1, "sleep", SF_FD },
/* 36 */	{1, "sync", 0 },
/* 37  */	{3, "kill", SF_FD|SF_ARG1 },
/* 38  */	{1, "csw", 0 },
/* 39  */	{1, "ssw", 0 },
/* 40 */	{1, "bad", 0 },	
/* 41 */	{1, "dup", SF_FD },
/* 42 */	{1, "pipe", 0 },
/* 43 */	{3, "times", 0 },
/* 44 */	{9, "profil", SF_ARG1|SF_ARG2|SF_ARG3|SF_ARG4 },
/* 45 */	{1, "bad", 0 },
/* 46 */	{1, "bad", 0 },
/* 47 */	{1, "bad", 0 },
/* 48 */	{5, "signal", SF_ARG1|SF_ARG2 },
/* 49 */	{3, "lock", SF_FD|SF_ARG1 },
/* 50 */	{1, "unlock", SF_FD },
			{0, 0, 0}
};

/*
 * format a system call with args
 */
int
fmt_syscall(unsigned short addr, char *dest)
{
	unsigned char sc;
	int i;
	char nbuf[6];
	unsigned short iaddr;
	int ret;

	addr &= 0xffff;
	sc = get_byte(addr + 1);
	if ((sc < 0) || (sc >= sizeof(syscalls) / sizeof(syscalls[0]))) {
		sprintf(dest, "unknown %x\n", sc);
		return 0;
	}
	ret = syscalls[sc].argbytes;

	sprintf(dest, "%s ", syscalls[sc].name);
	if (sc == 0 && fmt_indir_sc) {
		iaddr = (get_byte(addr + 3) << 8) + (get_byte(addr + 2) & 0xff);
		iaddr &= 0xffff;
		if ((get_byte(iaddr) & 0xff) != 0xcf) {
			sprintf(dest, "(0x%x)\n", iaddr);
			return ret;
		}
		addr = iaddr;
		sc = get_byte(addr + 1);
		sprintf(dest, "(0x%x) %s ", iaddr, syscalls[sc].name);
	}
	for (i = 1; i < syscalls[sc].argbytes; i++) {
		sprintf(nbuf, "%02x ", get_byte(addr + i + 1) & 0xff);
		strcat(dest, nbuf);
	}
	return ret;
}

/*
 * format a system call - don't follow indirection
 */
int
mnix_scpr(unsigned short addr, char *dest)
{
	unsigned char sc;
	int i;
	char nbuf[6];
	unsigned short iaddr;
	int ret;

	addr &= 0xffff;
	sc = get_byte(addr + 1);
	if ((sc < 0) || (sc >= sizeof(syscalls) / sizeof(syscalls[0]))) {
		sprintf(dest, "unknown %x\n", sc);
		return 0;
	}
	ret = syscalls[sc].argbytes;

	sprintf(dest, "%s ", syscalls[sc].name);
	for (i = 1; i < syscalls[sc].argbytes; i++) {
		sprintf(nbuf, "%02x ", get_byte(addr + i + 1) & 0xff);
		strcat(dest, nbuf);
	}
	return ret;
}

char *signame[] = {
	"bogus",
	"hup",
	"int",
	"quit",
	"ill",
	"trace",
	"bg",
	"termio",
	"fpe",
	"kill",
	"bus",
	"segv",
	"sysarg",
	"pipeerr",
	"alarm",
	"term"
};

/*
 * get a system call id.  either by name or integer.
 */
int
get_syscall(char **sp)
{
    int i;
    char nbuf[20];
    char *s = *sp;
    char *d = nbuf;

    while (*s) {
        if (isupper(*s)) *s = tolower(*s);
        if (*s < 'a' || *s > 'z') break;
        *d++ = *s++;
    }
    *d = '\0';

    if (nbuf[0] >= 'a' && nbuf[0] <= 'z') {
        for (i = 0; syscalls[i].name; i++) {
            if (strcmp(nbuf, syscalls[i].name) == 0) {
                *sp += strlen(nbuf);
                return i;
            }
        }
        return -1;
    }
    i = strtol(*sp, sp, 0);
    if (i >= NSYS) {
        i = -1;
    }
    return i;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
