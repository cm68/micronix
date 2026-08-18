/*
 * The libPW functions sh.c calls, given micronix answers.
 *
 * logname, logdir and logtty fed the PWB login accounting; here they
 * are fixed strings until there is a real utmp to read.  tell is lseek
 * to the current position - ftell by its v6 name.  setexit/reset are
 * the recoverable-error longjmp pair the parser uses, and times is a
 * no-op because there is no accounting file to write.
 */

#include <setjmp.h>

extern long lseek();

long
tell(fd)
int fd;
{
	return lseek(fd, 0L, 1);
}

char *
logtty()
{
	return "/dev/tty";
}

char *
logname()
{
	return "root";
}

char *
logdir()
{
	return "/";
}

static jmp_buf setexit_env;

setexit()
{
	return setjmp(setexit_env);
}

reset()
{
	longjmp(setexit_env, 1);
}

times()
{
	return 0;
}
