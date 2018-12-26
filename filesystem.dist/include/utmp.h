/*
 * Structure of utmp and wtmp files
 */

struct utmp
	{
	TEXT	tty [8],
		name [8];
		
	LONG	time;
	};

