/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * Date - print and set date
 *
 * 2.11BSD date (date.c 4.20.1, 2.11BSD 96/7/10), ported to micronix.
 *
 * cmd/date/date.c
 *
 * What the port took out, and why - each is a thing the machine does
 * not have:
 *
 *	gettimeofday	the kernel calls are time() and stime(), and
 *	settimeofday	seconds is all they carry.  There is no kernel
 *			timezone either; ctime's time_zone variable
 *			(minutes west, from libc) is the whole of the
 *			local-time story, so the -d and -t flags that
 *			set the kernel's zone went with it.
 *
 *	netsettime	the timed protocol needs sockets, and with it
 *			went the -n flag, whose only meaning was "do
 *			not do that".
 *
 *	wtmp, syslog	no /usr/adm/wtmp and no syslog on this system;
 *			the record of who set the clock is gone.
 *
 *	getopt		not in libc; two flags did not justify one, so
 *			the parsing is by hand.
 *
 * What stays: the yymmddhhmm[.ss] argument and gtime(), which turns
 * it into seconds - the tzfile.h constants it leaned on are spelled
 * out at the top - and the -u flag, which prints universal time.
 * Setting the clock is stime(), super-user only, and the argument is
 * taken as local time: time_zone minutes are added to make the GMT
 * the kernel keeps.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>
#include <ctype.h>
#include <time.h>

/*
 * what tzfile.h said, for the arithmetic gtime does
 */
#define	TM_YEAR_BASE	1900
#define	EPOCH_YEAR	1970
#define	SECS_PER_MIN	60
#define	MINS_PER_HOUR	60
#define	HOURS_PER_DAY	24
#define	DAYS_PER_NYEAR	365
#define	DAYS_PER_LYEAR	366
#define	isleap(y)	(((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

#define	ATOI2(ar)	(ar[0] - '0') * 10 + (ar[1] - '0'); ar += 2;

static long	tv;
static char	retval;		/* 0 or 1 */

/* month lengths fit a byte */
static char	dmsize[] =
	{ -1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

long	time();

main(argc, argv)
	int	argc;
	char	**argv;
{
	static char	usage[] = "usage: date [-u] [yymmddhhmm[.ss]]\n";
	char	*ap;
	int	uflag;

	uflag = 0;
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		for (ap = *argv + 1; *ap; ap++)
			switch (*ap) {
			case 'u':
				uflag = 1;
				break;
			default:
				fputs(usage, stderr);
				exit(1);
			}
		argc--, argv++;
	}

	if (argc > 1) {
		fputs(usage, stderr);
		exit(1);
	}

	time(&tv);

	if (argc) {
		if (gtime(*argv)) {
			fputs(usage, stderr);
			retval = 1;
			goto display;
		}
		/*
		 * the argument was local time; the kernel keeps GMT
		 */
		if (!uflag)
			tv += (long)time_zone * SECS_PER_MIN;
		if (stime(&tv) < 0) {
			perror("stime");
			retval = 1;
		}
	}

display:
	time(&tv);
	if (uflag)
		fputs(asctime(gmtime(&tv)), stdout);
	else
		fputs(ctime(&tv), stdout);
	exit(retval);
}

/*
 * gtime --
 *	convert user's time into number of seconds
 */
gtime(ap)
	register char	*ap;		/* user argument */
{
	register int	year, month;
	register char	*C;		/* pointer into time argument */
	struct tm	*L;
	int	day, hour, mins, secs;

	for (secs = 0, C = ap; *C; ++C) {
		if (*C == '.') {		/* seconds provided */
			if (strlen(C) != 3)
				return(1);
			*C = '\0';
			secs = (C[1] - '0') * 10 + (C[2] - '0');
			break;
		}
		if (!isdigit(*C))
			return(-1);
	}

	L = localtime(&tv);
	year = L->tm_year;			/* defaults */
	month = L->tm_mon + 1;
	day = L->tm_mday;

	switch ((int)(C - ap)) {		/* length */
		case 10:			/* yymmddhhmm */
			year = ATOI2(ap);
		case 8:				/* mmddhhmm */
			month = ATOI2(ap);
		case 6:				/* ddhhmm */
			day = ATOI2(ap);
		case 4:				/* hhmm */
			hour = ATOI2(ap);
			mins = ATOI2(ap);
			break;
		default:
			return(1);
	}

	if (*ap || month < 1 || month > 12 || day < 1 || day > 31 ||
	     mins < 0 || mins > 59 || secs < 0 || secs > 59)
		return(1);
	if (hour == 24) {
		++day;
		hour = 0;
	}
	else if (hour < 0 || hour > 23)
		return(1);

	tv = 0;
	year += TM_YEAR_BASE;
/* If year < EPOCH_YEAR, assume it's in the next century and
   the system has not yet been patched to move TM_YEAR_BASE up yet */
	if (year < EPOCH_YEAR)
		year += 100;
	if (isleap(year) && month > 2)
		++tv;
	for (--year; year >= EPOCH_YEAR; --year)
		tv += isleap(year) ? DAYS_PER_LYEAR : DAYS_PER_NYEAR;
	while (--month)
		tv += dmsize[month];
	tv += day - 1;
	tv = HOURS_PER_DAY * tv + hour;
	tv = MINS_PER_HOUR * tv + mins;
	tv = SECS_PER_MIN * tv + secs;
	return(0);
}
