/*
 * broken down time structure used by localtime and gmtime
 */

struct time
	{
	COUNT	seconds,
		minutes,
		hours,
		day_month,	/* 1 - 31 */
		month,		/* 0 - 11 */
		year,		/* year - 1900 */
		day_week,	/* Sunday = 0 */
		day_year,	/* 0 - 365 */
		daylight;	/* daylight savings time  - boolean */
	};


COUNT timezone;
