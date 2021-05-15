#include <types.h>
#include <std.h>
#include <time.h>


main()
{
	long clock;
	struct time *tm;
	char buf[1000];
	int fd;

	time(&clock);
	tm = localtime(&clock);
	
	fd = creat("build.h", 0777);
	sprintf(buf, "#define	BUILD_DATE \"Created %2d/%2d/%2d\\n\"\n", 
		tm->month + 1, tm->day_month, tm->year + 1900);
	write(fd, buf, strlen(buf));
	close(fd);
	exit(0);
}

