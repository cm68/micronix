/*
 * Copyright (c) 1985 by Morris Code Works
 *
 * cmd/Make/maketime.c
 * Changed: <2022-01-06 16:34:44 curt>
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
#include <time.h>
#include <string.h>
#include <errno.h>
#endif

struct stat statb INIT;

/*
 * get last update time 
 */
long
FileTime(fname)
char *fname;
{
    int i;
    long rv;
 
    i = stat(fname, &statb);
    if (i < 0) {
        return 0L;
    }
#ifdef linux
    rv = statb.st_mtim.tv_sec;
#else
    rv = statb.modtime;
#endif
    if (verbose > 2) 
        printf("stat of file %s returns %s\n", fname, PTime(rv));
    return rv;
}

/*
 * return current system time 
 */
long
CurrTime()
{
    long tt;
    time(&tt);
    if (verbose > 2) 
        printf("curtime returns %s\n", PTime(tt));
    return tt;
}

#ifdef linux
#define cpybuf(d,s,l)  memcpy(d,s,l)
#endif

/*
 * convert time to printable.
 * since we could call this multiple times in a printf, rotate static buffers
 */
char timebuf[100] INIT;
char *tbuf = timebuf;

char *
PTime(val)
long val;
{
    char *s;

    if (val == 0) {
        return "0";
    } 

    if (tbuf == timebuf)
        tbuf = &timebuf[sizeof(timebuf)/2];
    else
        tbuf = timebuf;

    s = ctime(&val);
    cpybuf(&tbuf[0], &s[4], 12); /* mmm dd HH:MM */
    tbuf[12] = '\0';

    return tbuf;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
