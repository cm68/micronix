/*
 * broken down time structure used by localtime and gmtime
 *
 * include/time.h
 *
 * Changed: <2023-07-04 11:13:05 curt>
 */

struct time {
	UINT seconds;
	UINT minutes;
	UINT hours;
	UINT day_month;				/* 1 - 31 */
	UINT month;					/* 0 - 11 */
	UINT year;					/* year - 1900 */
	UINT day_week;				/* Sunday = 0 */
	UINT day_year;				/* 0 - 365 */
	UINT daylight;				/* daylight savings time - boolean */
};

UINT timezone;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
