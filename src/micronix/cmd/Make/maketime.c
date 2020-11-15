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
#define INIT = 0
#endif

#ifdef linux
#define INIT
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#endif

#ifdef DEBUG
extern char debug;
#endif

struct stat statb INIT;

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
        return 0L;
    }
#ifdef linux
    rv = statb.st_mtim.tv_sec;
#else
    rv = statb.modtime;
#endif
#ifdef DEBUG
    if (debug > 1) 
        printf("stat of file %s returns %lu\n", fname, rv);
#endif
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
#ifdef DEBUG
    if (debug > 1) 
        printf("curtime returns %lu\n", tt);
#endif
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
