/*
 * Structure of utmp and wtmp files
 *
 * include/utmp.h
 * Changed: <2021-12-23 15:16:58 curt>
 */

struct utmp {
	char tty[8];
	char name[8];
	UINT32 time;
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
