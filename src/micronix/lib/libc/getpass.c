/*
 * getpass - read a password without echoing it
 *
 * lib/libc/getpass.c
 *
 * The seventh edition's getpass, rewritten against this kernel rather
 * than translated, because the v7 original drives the terminal with
 * ioctl(TIOCGETP/TIOCSETP) and there is no terminal ioctl here.  What
 * this tree has is gtty(2) and stty(2) and a six-byte terminal block,
 * which is what cmd/login/login.c uses to do the same job - echo_off
 * at 0x085f, echo_on at 0x0803 - and this follows it.
 *
 * The block is written out here rather than taken from
 * <sys/sgtty.h>, which names the same six bytes differently (ispeed,
 * ospeed, erase, kill, mode).  login defines its own for the same
 * reason and this is login's shape: the two speeds read as one word
 * and the mode word last, which is what the kernel moves.
 *
 * What v7 did that this does not:
 *
 *	/dev/tty	the original opened it and read the password
 *			from there, falling back to stdin, so that a
 *			password could be typed even with stdin
 *			redirected.  login reads fd 0 and so does this;
 *			the console is fd 0 on this machine.
 *
 *	stderr		the prompt goes to stderr, as the original's
 *			did, so that a redirected stdout does not eat
 *			it.
 *
 *	A FAILING gtty IS NOT FATAL.  login exits when it cannot turn
 *	echo off, because its caller has nothing to fall back on.  A
 *	library routine should not exit its caller, so the echo is left
 *	alone and the password is read with echo on.  On a terminal
 *	that cannot answer gtty there is nothing else to do, and the
 *	caller still gets the password.
 *
 * The answer is in a static buffer that the next call overwrites, and
 * it is truncated rather than overrun if the line is longer than the
 * buffer.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include	<types.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<string.h>

#define PBSIZE	128		/* v7's password buffer size */
#define ECHO	010		/* the bit echo_off clears */

/* six bytes: the two speeds as one word, then erase, kill, mode */
struct sgttyb {
	int	speeds;
	char	erase;
	char	kill;
	int	tflags;
};

static char		passwd[PBSIZE];
static struct sgttyb	ttyb;

char *
getpass(prompt)
char *prompt;
{
	register char *p;
	register int c;
	int echoed;

	write(2, prompt, strlen(prompt));

	echoed = 0;
	if (gtty(0, &ttyb) >= 0) {
		ttyb.tflags &= ~ECHO;
		if (stty(0, &ttyb) >= 0)
			echoed = 1;
	}

	p = passwd;
	while ((c = getc(stdin)) != '\n' && c != EOF && c != '\0')
		if (p < &passwd[PBSIZE - 1])
			*p++ = c;
	*p = 0;

	if (echoed && gtty(0, &ttyb) >= 0) {
		ttyb.tflags |= ECHO;
		stty(0, &ttyb);
	}
	write(2, "\n", 1);	/* the RETURN was not echoed; supply it */

	return (passwd);
}
