/*
 * makekey
 *
 * v7 makekey (usr/src/cmd/makekey.c), ported to micronix.
 *
 * cmd/makekey/makekey.c
 *
 * Ten bytes in and thirteen out.  The ten are a key of eight
 * characters followed by a salt of two, and the thirteen are the
 * string the password hash - crypt (3) - returns for them.  It is
 * the one-rotor cipher's key generator and not a thing anyone runs
 * by hand: crypt (1) forks it, writes the ten bytes down a pipe and
 * reads the thirteen back, which is how a key word becomes the
 * thirteen characters the tables are built from.
 *
 * It is deliberately slow.  v7's comment says the transformation is
 * expensive to perform, a significant part of a second, and that is
 * the point of the fork - the cost is paid once per run, not once
 * per byte - and of the salt, which is what stops one precomputed
 * table answering for every key.
 *
 * WHERE THIS IS INSTALLED.  crypt (1) execs /usr/lib/makekey, then
 * /lib/makekey as a fallback, and neither was on the 1.6 image.  The
 * tree has /usr/lib - it holds less.help and the yacc and lex
 * command files - so that is where this goes, and the GNUmakefile
 * and makefile beside this set BINDIR and BIN to /usr/lib for it.
 * /lib is not the choice: that is LIBDIR, the library directory ccc
 * reads libc.a out of, and a program there would be a program in the
 * wrong place.
 *
 * What the port had to say differently:
 *
 *	the key		v7 declared "char key[8]" and handed it to the
 *			hash undeclared and unterminated.  The hash
 *			walks a password to its NUL, so an eight
 *			character key - the full eight, which is what
 *			the sender can produce - had it reading whatever
 *			followed on the stack until it found a zero.
 *			The buffer here is nine bytes and the ninth is
 *			the NUL, so exactly the eight key bytes are
 *			hashed.  That is what v7 meant; it is not always
 *			what v7 did.
 *
 *	crypt()		v7 declared it by hand.  It is in libc and
 *			declared in <crypt.h> - see crypt (3) - and the
 *			name is the library's, so nothing here is called
 *			crypt either.
 *
 * ccc needs the <types.h> and <unistd.h> that v7's source did not
 * name: read and write are declared there, and an undeclared read
 * returning int would still work but an undeclared declaration is
 * not something to carry on purpose.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <crypt.h>

main()
{
	char key[9];
	char salt[2];

	read(0, key, 8);
	read(0, salt, 2);
	key[8] = '\0';
	write(1, crypt(key, salt), 13);
	return (0);
}
