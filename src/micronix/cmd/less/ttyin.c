/*
 * Routines dealing with getting input from the keyboard (i.e. from the user).
 */

#include "less.h"

/*
 * The boolean "reading" is set true or false according to whether
 * we are currently reading from the keyboard.
 * This information is used by the signal handling stuff in signal.c.
 * {{ There are probably some race conditions here
 *    involving the variable "reading". }}
 */
public char reading;

static char tty;

/*
 * Open keyboard for input.
 * (Just use file descriptor 2.)
 */
	public void
open_getc()
{
	tty = 2;
}

/*
 * Get a character from the keyboard.
 */
	public int
getc()
{
	char c;
	int result;
	int failures;

	/*
	 * The original looped until read answered, which is right for
	 * a read interrupted by a signal and forever for a fd 2 that
	 * is not a terminal at all.  A few failures in a row means
	 * nobody is out there; leave with the terminal modes sane.
	 */
	failures = 0;
	reading = 1;
	do
	{
		flush();
		result = read(tty, &c, 1);
		if (result != 1 && ++failures > 5)
			quit();
	} while (result != 1);
	reading = 0;
	return (c & 0177);
}
