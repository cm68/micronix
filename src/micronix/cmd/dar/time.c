/*
 * Having to do with time and date.
 */

# include "dar.h"

/*
 * produce a dos format date field
 * from the current time according to unix.
 */

dosdate ()
	{
	short y, m, d;
	long s;
	struct tm *t;

	time (&s);
	t = localtime (&s);

	y = t->tm_year - 80;
	
	m = t->tm_mon + 1;

	d = t->tm_mday;

	return (y << 9) | (m << 5) | d;
	}

/*
 * produce a dos format date field
 * from the current time according to unix.
 */

dostime ()
	{
	short h, m, s;
	long now;
	struct tm *t;

	time (&now);
	t = localtime (&now);

	h = t->tm_hour;
	
	m = t->tm_min;

	s = t->tm_sec / 2;

	return (h << 11) | (m << 5) | s;
	}

char *monthname[]
	=
	{
	"Jan", 
	"Feb", 
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
	};

prtime (a)
	struct entry *a;
	{
	unsigned year, month, date, hour, minute;

	year = (a->date >> 9) + 1980;
	month = ((a->date >> 5) & 15) - 1;	/* 0 - 11 */
	date = a->date & 31;

	hour = a->time >> 11;
	minute = (a->time >> 5) & 63;

	printf ("%s %2d %2d:%02d ", 
		monthname [month],
		date,
		hour,
		minute);
	}
