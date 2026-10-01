/*
 * crypt [key]
 *
 * v7 crypt (usr/src/cmd/crypt.c), ported to micronix.
 *
 * cmd/crypt/crypt.c
 *
 * The one-rotor machine is v7's, unchanged: three two hundred and
 * fifty six byte tables, a seed that starts at 123 and is folded
 * through the thirteen characters makekey returns, the same
 * "seed = 5*seed + buf[i%13]" walk that shuffles t1 and pairs up t3,
 * and the same cipher - which is its own inverse, so enciphering a
 * file and then enciphering the result again with the same word
 * gives the file back.  Nothing about the arithmetic was touched.
 *
 * THIS IS NOT THE LIBRARY'S CRYPT.  libc has a crypt - the password
 * hash of crypt (3) - and it is a different routine for a different
 * job: that one hashes a password for /etc/passwd, this one
 * enciphers a file.  The linker knows the library's as _crypt, and a
 * function of this program spelled crypt would compile to that same
 * symbol, so the key schedule below is keysetup() and nothing in
 * this file is named crypt.  See crypt (3), which points back here.
 *
 * What the port had to say differently:
 *
 *	makekey		v7 exec'd /usr/lib/makekey and, if that
 *			failed, /lib/makekey.  Both paths are kept, and
 *			cmd/makekey installs to /usr/lib - see the
 *			GNUmakefile beside it - so the first one is the
 *			one that answers.  /lib is the library
 *			directory and is not where a program goes.
 *
 *	execl		v7 called execl.  libu has one - execv.s
 *			defines both - but unistd.h declares only exec
 *			and execv, so the child builds the one element
 *			argument list and calls execv.  "-" is what v7
 *			passed, and makekey reads no arguments.
 *
 *	the key		v7 asked getpass for the key when none was
 *			named, and v7's getpass opened /dev/tty - the
 *			terminal - so that the key could be typed with
 *			the standard input redirected, which is the only
 *			way crypt is ever used.  There is no /dev/tty
 *			here and this tree's getpass reads descriptor 0,
 *			which for crypt is the file being enciphered: the
 *			key would be read out of the file.  gtty tells a
 *			terminal from a file, so a terminal still gets
 *			v7's prompt and anything else is refused rather
 *			than answered with the first line of the input.
 *
 *	the exit	v7's main falls off the end.  This one exits 0,
 *			because a filter that ran to the end of its
 *			input succeeded - /bin/crypt on the 1.6 disk
 *			exits 255 instead.
 *
 *	ECHO		v7 defined ECHO at the top and never used it; it
 *			belonged to getpass, which does its own.  Gone.
 *
 * The 1.6 disk's /bin/crypt does NOT do this cipher.  Its output
 * byte i depends on the first i+1 characters of the key, where this
 * one's whole output depends on the whole key through makekey, so
 * the two disagree on every file - see the man page.  The 1982
 * binary is what does; this port is what crypt (1) describes.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>

#define	ROTORSZ	256
#define	MASK	0377

/*
 * where makekey is looked for.  The second is v7's fallback and is
 * kept for the same reason v7 had it: the two were on different
 * systems.
 */
#define	MAKEKEY	"/usr/lib/makekey"
#define	ALTKEY	"/lib/makekey"

char	t1[ROTORSZ];
char	t2[ROTORSZ];
char	t3[ROTORSZ];

/*
 * keyprompt returns a pointer, and v7 declared getpass() by hand for
 * the same reason: an undeclared function is an int, and a pointer
 * that arrives in the same register is the same answer only by
 * accident of this machine.
 */
char	*keyprompt();
keysetup();

char	*mkargv[] = { "-", 0 };

main(argc, argv)
	int argc;
	char *argv[];
{
	register int i, n1, n2;

	if (argc != 2)
		keysetup(keyprompt());
	else
		keysetup(argv[1]);
	n1 = 0;
	n2 = 0;

	while ((i = getchar()) >= 0) {
		i = t2[(t3[(t1[(i+n1)&MASK]+n2)&MASK]-n2)&MASK] - n1;
		/*
		 * The mask is v7's putchar's, moved up here where it
		 * can be seen.  Each of the three table values is a
		 * byte and the counters run to 255, so the difference
		 * is signed; fputc writes the low byte and dropped the
		 * rest, which is the same answer, but only by
		 * agreement.
		 */
		putchar(i & MASK);
		n1++;
		if (n1 == ROTORSZ) {
			n1 = 0;
			n2++;
			if (n2 == ROTORSZ)
				n2 = 0;
		}
	}
	exit(0);
}

/*
 * the key typed at the terminal, when none was named on the command
 * line.  v7 went straight to getpass; the gtty is the port's, and
 * the comment at the top of this file says why.
 */
keyprompt()
{
	char ttyb[8];

	if (gtty(0, ttyb) < 0) {
		fprintf(stderr, "crypt: no key, and stdin is not a terminal\n");
		exit(1);
	}
	return(getpass("Enter key:"));
}

/*
 * Turn the key word into the three tables.
 *
 * v7 called this setup(); it is keysetup() here so that it is not
 * the library's _setup, which stdio.h declares even though nothing
 * defines it.  The body is v7's.
 *
 * buf is thirteen bytes: the first eight are the key, the next two
 * are the salt.  The salt is the first two characters of the key
 * again - that is v7's choice, not a random one - and those ten
 * bytes are what makekey takes.  The thirteen it returns overwrite
 * buf entirely, which is why the last three bytes of the buffer are
 * never anything but the answer.
 */
keysetup(pw)
	char *pw;
{
	int ic, i, k, temp, pf[2];
	unsigned random;
	char buf[13];
	long seed;

	strncpy(buf, pw, 8);
	while (*pw)
		*pw++ = '\0';
	buf[8] = buf[0];
	buf[9] = buf[1];
	pipe(pf);
	if (fork() == 0) {
		close(0);
		close(1);
		dup(pf[0]);
		dup(pf[1]);
		execv(MAKEKEY, mkargv);
		execv(ALTKEY, mkargv);
		exit(1);
	}
	write(pf[1], buf, 10);
	wait((int *)0);
	if (read(pf[0], buf, 13) != 13) {
		fprintf(stderr, "crypt: cannot generate key\n");
		exit(1);
	}
	seed = 123;
	for (i = 0; i < 13; i++)
		seed = seed*buf[i] + i;
	for (i = 0; i < ROTORSZ; i++)
		t1[i] = i;
	for (i = 0; i < ROTORSZ; i++) {
		seed = 5*seed + buf[i%13];
		random = seed % 65521;
		k = ROTORSZ-1 - i;
		ic = (random&MASK) % (k+1);
		random >>= 8;
		temp = t1[k];
		t1[k] = t1[ic];
		t1[ic] = temp;
		if (t3[k] != 0)
			continue;
		ic = (random&MASK) % k;
		while (t3[ic] != 0)
			ic = (ic+1) % k;
		t3[k] = ic;
		t3[ic] = k;
	}
	for (i = 0; i < ROTORSZ; i++)
		t2[t1[i]&MASK] = i;
}
