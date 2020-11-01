/*
 * Copyright (c) 1985 by Morris Code Works
 *
 * maketime.c
 */
#include	"make.h"
#ifndef linux
extern int errno;
#include <stdio.h>
#include <stat.h>
#endif

#ifdef linux
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#endif

extern char debug;
struct stat statb
#ifndef linux
 = 0
#endif
;

/*
 * get last update time 
 */
unsigned long
FileTime(fname)
    char *fname;
{
    int i;
    unsigned long rv;
 
    i = stat(fname, &statb);
    if (i < 0) {
        if (debug > 1)
            printf("stat error %s %d\n", fname, errno);
        return 0L;
    }
#ifdef linux
    rv = statb.st_mtim.tv_sec;
#else
    rv = statb.modtime;
#endif
    if (debug > 1) 
        printf("stat of file %s returns %lu\n", fname, rv);
    return rv;
}

/*
 * return current system time 
 */
unsigned long
CurrTime()
{
    unsigned long tt;
    time(&tt);
    if (debug) 
        printf("curtime returns %lu\n", tt);
    return tt;
}

/*
 * convert time to external 
 */
char *
ListTime(val, dp)
    unsigned long val;          /* internal date/time */
    register char *dp;          /* formatted string ptr */
{
    sprintf(dp, "%x %x", (val >> 16) & 0xffff, val & 0xffff);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
