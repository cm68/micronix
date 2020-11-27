/*
 * Structure of utmp and wtmp files
 */

struct utmp {
	char tty [8];
	char name [8];
	UINT32 time;
};

