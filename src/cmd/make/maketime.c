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
#endif

/*
 * get last update time 
 */
unsigned long
FileTime(fname)
    char *fname;                /* filename to get time of */
{

    struct stat statb;
    int i;
    
    i = stat(fname, &statb);
    if (i < 0) {
        printf("stat error %s %d\n", fname, errno);
    }
    printf("stat of file %s returns %lu\n", fname, statb.modtime);
#ifdef linux
    return statb.st_mtim.tv_sec;
#else
    return statb.modtime;
#endif

#ifdef CPM
    unsigned long tolong();     /* convert 'time' to long */
    char fcb[36];               /* working file-control-block */
    void setmem();              /* set block of memory to value */
    int user;                   /* files user number */

    /*
     * initialize the fcb 
     */
    user = fcbinit(fname, fcb);
    setmem(&fcb[16], 16, 0);

    /*
     * get files update time 
     */
    if (user != 255) {
        (void) setusr(user);
        bdos(102, fcb);
        (void) rstusr();
    } else
        bdos(102, fcb);

    /*
     * convert & return time to caller 
     */
    return (tolong(&fcb[28]));
#endif
}

/*
 * return current system time 
 */
unsigned long
CurrTime()
{
    unsigned long tt;
    time(&tt);
    printf("curtime returns %lu\n", tt);
    return tt;
#ifdef CPM
    unsigned long tolong();     /* convert time */
    char time[4];               /* working area */

    /*
     * read the current time-of-day 
     */
    bdos(105, &time);

    /*
     * convert to 'long' and return to caller 
     */
    return (tolong(&time));
#endif
}

#ifdef CPM
/*
 * convert to external fmt 
 */
unsigned long
tolong(cp)
    char cp[];                  /* internal date array */
{
    unsigned long work;         /* 'long' work area */
    register char *wp;          /* ptr to the work area */

    wp = &work;

    *wp++ = cp[3];
    *wp++ = cp[2];
    *wp++ = cp[0];
    *wp = cp[1];

    return (work);
}

/*
 * convert to internal fmt 
 */
void
todate(fp, tp)
    char fp[];                  /* ptr to 'long' data */
    register char *tp;          /* ptr to internal data */
{
    *tp++ = fp[2];
    *tp++ = fp[3];
    *tp++ = fp[1];
    *tp = fp[0];
}
#endif

/*
 * convert time to external 
 */
char *
ListTime(val, dp)
    unsigned long val;          /* internal date/time */
    register char *dp;          /* formatted string ptr */
{
    sprintf(dp, "%x %x", (val >> 16) & 0xffff, val & 0xffff);
#ifdef CPM
    register char *sp;
    static struct idate
    {                           /* internal representation */
        int Days;               /* same size as a 'long' */
        char Hours;
        char Minutes;
    } InternalDate;

    static char *DayOfWeek[] = {
        "Sat ", "Sun ", "Mon ", "Tue ", "Wed ", "Thu ", "Fri "
    };
    static int MonthCount[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    static int years, months, days, ddays;
    void todate();              /* convert to internal fmt */

    /*
     * move the day of week string 
     */
    todate(&val, &InternalDate);
    for (sp = DayOfWeek[InternalDate.Days % 7]; *sp; *dp++ = *sp++);

    /*
     * compute the Date and move into ExternalDate 
     */
    /*
     * first calculate the current year 
     */
    for (years = 78, days = InternalDate.Days;
        days > (ddays = (years & 3) ? 365 : 366); ++years, days -= ddays);

    /*
     * days now contains the remaining days in the year 
     */
    MonthCount[1] = ddays - 337;        /* 28 or 29 for Feb */

    /*
     * figure out which month this is 
     */
    for (months = 0; days > MonthCount[months]; days -= MonthCount[months++]);
    ++months;                   /* convert to real world number */
    /*
     * move the date into the ExternalDate string 
     */
    *dp++ = (months / 10) + '0';
    *dp++ = (months % 10) + '0';
    *dp++ = '/';
    *dp++ = (days / 10) + '0';
    *dp++ = (days % 10) + '0';
    *dp++ = '/';
    *dp++ = (years / 10) + '0';
    *dp++ = (years % 10) + '0';
    *dp++ = ' ';

    /*
     * move the time into the ExternalDate string 
     */
    *dp++ = ((InternalDate.Hours >> 4) & 0x0f) + '0';
    *dp++ = (InternalDate.Hours & 0x0f) + '0';
    *dp++ = ':';
    *dp++ = ((InternalDate.Minutes >> 4) & 0x0f) + '0';
    *dp++ = (InternalDate.Minutes & 0x0f) + '0';
    *dp = '\0';

    return (dp - 18);
#endif
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
