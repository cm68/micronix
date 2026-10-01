/*
 * passwd [name]
 *
 * v7 passwd (usr/src/cmd/passwd.c), ported to micronix.
 *
 * cmd/passwd/passwd.c
 *
 * The whole of the original is here: the same lookup, the same two
 * permission tests, the same old-password question, the same
 * insist-twice-then-accept password rule, the same clock-derived salt,
 * the same temporary file and the same line-at-a-time rewrite of the
 * password file.  What follows is what could not be carried over as it
 * stood.
 *
 * getlogin is not in this tree and there is no way to add it here: it
 * reads back the /etc/utmp record login writes, and nothing in libc
 * opens that file.  v7 asked it which user to change the password for
 * when no name was given; this asks getpwuid for the name that goes
 * with the uid the process is already running as, which is the same
 * question answered from the other end.  What is missing is v7's
 * refusal to guess when getlogin answered nothing - there is no such
 * gap to detect, because getpwuid either finds the uid or says so, and
 * says so with a different message.
 *
 * crypt and getpass are libc here - lib/libc/crypt.c and
 * lib/libc/getpass.c, declared by <crypt.h> - and are the hash the
 * password file carries and the no-echo reader login uses.  They were
 * login's own code until this tree gave them a home; the header says
 * why they belong where they are.  The call shapes are unchanged.
 *
 * pwbuf was ten bytes in the original and getpass hands back up to 128
 * through a strcpy.  Ten is the overflow, not a buffer; it is 128 here.
 * The salt array is three bytes with the third kept at NUL for the
 * same class of reason - crypt reads salt[0] and salt[1] and stops,
 * but a two-byte array with no terminator is one edit away from a
 * read past its end.
 *
 * The rewrite of /etc/passwd is by link and unlink rather than by
 * creat and copy.  There is no rename system call on this machine;
 * libu's rename is these same two calls, and they are written out here
 * rather than called so that the failure path can be handled - link
 * will not replace a name, so /etc/passwd is taken away before the new
 * one is made, and if the link then fails the file has to be put back.
 * v7's own way of writing the file, a truncating creat and a copy, is
 * that put-it-back, and it is not the method any more because creat
 * empties /etc/passwd the moment it opens it and leaves it that way
 * for as long as the copy takes.  The link is not atomic either - the
 * name is gone from the unlink to the link - but that window is two
 * system calls wide and holds nothing but the directory entry, and a
 * rename call is what would close it.  The mode
 * is set afterwards on both paths: a second name does not carry a mode
 * of its own, so the file would keep the 0600 the temporary was made
 * with, and the mode creat is handed is narrowed by the umask, which
 * is not the file's business.
 *
 * The exit status is 0 when the password was changed, where v7 fell
 * through to its single exit(1) on every path, success included.  The
 * messages and the temporary file's name are the original's; the
 * "recreat" of the original's message is spelled out.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <pwd.h>
#include <crypt.h>
#include <sys/signal.h>

char	passwd[] = "/etc/passwd";
char	temp[]	 = "/etc/ptmp";
struct	passwd *pwd;
char	*pw;
char	pwbuf[128];
char	name[512];
char	buf[512];

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *p;
	int i;
	char saltc[3];
	long salt;
	int u, fi, fo;
	int insist;
	int ok, flags;
	int c;
	int pwlen;
	FILE *tf;
	char *uname;

	insist = 0;
	if (argc < 2) {
		if ((pwd = getpwuid(getuid())) == NULL) {
			printf("Unknown user %d.\n", getuid());
			goto bex;
		}
		strcpy(name, pwd->name);
		uname = name;
		printf("Changing password for %s\n", uname);
	} else
		uname = argv[1];

	/*
	 * The getpwuid above left the stream part way into the file,
	 * and the scan below has to see every line; endpwent closes
	 * it, and the first getpwent reopens it at the top.
	 */
	endpwent();
	while (((pwd = getpwent()) != NULL) && (strcmp(pwd->name, uname) != 0))
		;
	u = getuid();
	if ((pwd == NULL) || (u != 0 && u != pwd->uid)) {
		printf("Permission denied.\n");
		goto bex;
	}
	endpwent();
	if (pwd->passwd[0] && u != 0) {
		strcpy(pwbuf, getpass("Old password:"));
		pw = crypt(pwbuf, pwd->passwd);
		if (strcmp(pw, pwd->passwd) != 0) {
			printf("Sorry.\n");
			goto bex;
		}
	}
tryagn:
	strcpy(pwbuf, getpass("New password:"));
	pwlen = strlen(pwbuf);
	if (pwlen == 0) {
		printf("Password unchanged.\n");
		goto bex;
	}
	/*
	 * What kind of characters are in it, one bit each: 1 digit,
	 * 2 lower case, 4 upper case, 8 anything else.  The three tests
	 * below then say how long it has to be to be let through.
	 */
	ok = 0;
	flags = 0;
	p = pwbuf;
	while (c = *p++) {
		if (c >= 'a' && c <= 'z')
			flags |= 2;
		else if (c >= 'A' && c <= 'Z')
			flags |= 4;
		else if (c >= '0' && c <= '9')
			flags |= 1;
		else
			flags |= 8;
	}
	if (flags >= 7 && pwlen >= 4)
		ok = 1;
	if (((flags == 2) || (flags == 4)) && pwlen >= 6)
		ok = 1;
	if (((flags == 3) || (flags == 5) || (flags == 6)) && pwlen >= 5)
		ok = 1;

	if ((ok == 0) && (insist < 2)) {
		if (flags == 1)
			printf("Please use at least one non-numeric character.\n");
		else
			printf("Please use a longer password.\n");
		insist++;
		goto tryagn;
	}

	if (strcmp(pwbuf, getpass("Retype new password:")) != 0) {
		printf("Mismatch - password unchanged.\n");
		goto bex;
	}

	/*
	 * The salt: two characters out of the clock, which is what the
	 * original used and what crypt was always fed.  Every salt
	 * alphabet in use here is sixty-four characters wide, and the
	 * two adds below walk from '.' over the digits, the capitals
	 * and the small letters.
	 */
	time(&salt);
	salt += getpid();

	saltc[0] = salt & 077;
	saltc[1] = (salt >> 6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		saltc[i] = c;
	}
	saltc[2] = 0;
	pw = crypt(pwbuf, saltc);

	/*
	 * A signal arriving now would leave a half-written password
	 * file behind, so they are ignored until it is written.
	 */
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	if (access(temp, 0) >= 0) {
		printf("Temporary file busy -- try again\n");
		goto bex;
	}
	close(creat(temp, 0600));
	if ((tf = fopen(temp, "w")) == NULL) {
		printf("Cannot create temporary file\n");
		goto bex;
	}

	/*
	 * Copy the password file to the temporary, replacing the one
	 * line whose name matches.  getpwent is read from the top
	 * again: the scans above closed it.
	 */
	while ((pwd = getpwent()) != NULL) {
		if (strcmp(pwd->name, uname) == 0) {
			u = getuid();
			if (u != 0 && u != pwd->uid) {
				printf("Permission denied.\n");
				goto out;
			}
			pwd->passwd = pw;
		}
		fprintf(tf, "%s:%s:%d:%d:%s:%s:%s\n",
			pwd->name,
			pwd->passwd,
			pwd->uid,
			pwd->gid,
			pwd->person,
			pwd->dir,
			pwd->shell);
	}
	endpwent();
	fclose(tf);

	/*
	 * Put the temporary in place.  link will not replace a name,
	 * so the old file goes first; if the link then fails - no link
	 * on this filesystem, no room for the directory entry - v7's
	 * copy-and-truncate puts the file back so that the password
	 * file is never left missing.
	 *
	 * The mode is set afterwards on both paths.  A second name does
	 * not carry a mode of its own, so the file would keep the 0600
	 * the temporary was made with; and the mode creat is given is
	 * narrowed by the umask, which is not the file's business.
	 */
	unlink(passwd);
	if (link(temp, passwd) < 0) {
		if ((fi = open(temp, 0)) < 0) {
			printf("Temp file disappeared!\n");
			goto out;
		}
		if ((fo = creat(passwd, 0644)) < 0) {
			printf("Cannot recreate passwd file.\n");
			goto out;
		}
		while ((u = read(fi, buf, sizeof(buf))) > 0)
			write(fo, buf, u);
		close(fi);
		close(fo);
	}
	chmod(passwd, 0644);

	unlink(temp);
	exit(0);

out:
	unlink(temp);

bex:
	exit(1);
}
