#include <std.h>
#include <time.h>


main()
{
	long clock;
	struct time *tm;

	time(&clock);
	tm = localtime(&clock);
	
	printf("#define	BUILD_DATE \"Created %2d/%2d/%2d\\n\"\n", 
		tm->month + 1, tm->day_month, tm->year + 1900);
	exit(0);
}

