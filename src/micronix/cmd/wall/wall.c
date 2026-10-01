/*
 * wall
 *
 * v7 wall (usr/src/cmd/wall.c), ported to micronix.
 *
 * cmd/wall/wall.c
 *
 * The shape is the one v7 wrote: read the message from the standard
 * input, then go down /etc/utmp and write it to the terminal of every
 * login the file names.
 *
 * What the port had to say differently, and both are the 1982 page
 * saying it rather than the 1982 source:
 *
 *	super-user	wall (1) says "Only the super-user may use wall",
 *			and v7's wall.c has no such test - any user could
 *			broadcast.  The check is here: "Not super-user" on
 *			the standard error and exit 1.  The 1.61 /bin/wall
 *			binary agrees with the page rather than with v7 -
 *			it calls getuid() before it reads the message and
 *			it carries the string "Not super-user" - though
 *			the branch was never seen to be taken, because
 *			usersim answers getuid() with 0 whatever the real
 *			user is.
 *
 *	the message	wall (1)'s example has each user seeing
 *			"Broadcast message ...".  v7's source capitalised
 *			the M.  The micronix spelling is used here, and it
 *			is the one in the 1.61 binary.
 *
 * <utmp.h> is not used.  There is one in this tree and it names the
 * same three fields tty, name and time; the record is defined here
 * with v7's field names instead, the way login.c defines it, because
 * no library here reads utmp.
 *
 * The port also bounds the read.  v7 fills mesg[3000] from the
 * standard input with no test, so a message of more than 3000 bytes
 * writes past the end of it; here the buffer stops the loop.  That is
 * a change to the original and not a translation of it.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>

#define	USERS	50
#define	MSIZE	3000

struct utmp {			/* 20 bytes, v7 layout - see login.c */
	char	ut_line[8];	/* +0  the tty name, e.g. "tty2" */
	char	ut_name[8];	/* +8  the login name */
	long	ut_time;	/* +16 */
};

char	mesg[MSIZE];
int	msize;
struct	utmp utmp[USERS];

main(argc, argv)
int argc;
char *argv[];
{
	register i;
	register struct utmp *p;
	FILE *f;

	if (getuid() != 0) {
		fprintf(stderr, "Not super-user\n");
		exit(1);
	}
	if ((f = fopen("/etc/utmp", "r")) == NULL) {
		fprintf(stderr, "Cannot open /etc/utmp\n");
		exit(1);
	}
	fread((char *)utmp, sizeof(struct utmp), USERS, f);
	fclose(f);
	f = stdin;
	if (argc >= 2) {
		if ((f = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr, "Cannot open %s\n", argv[1]);
			exit(1);
		}
	}
	while (msize < MSIZE && (i = getc(f)) != EOF)
		mesg[msize++] = i;
	fclose(f);
	for (i = 0; i < USERS; i++) {
		p = &utmp[i];
		if (p->ut_name[0] == 0)
			continue;
		sleep(1);
		sendmes(p->ut_line);
	}
	exit(0);
}

sendmes(tty)
char *tty;
{
	register i;
	char t[50], buf[BUFSIZ];
	FILE *f;

	i = fork();
	if (i == -1) {
		fprintf(stderr, "Try again\n");
		return;
	}
	if (i)
		return;
	strcpy(t, "/dev/");
	strcat(t, tty);

	if ((f = fopen(t, "w")) == NULL) {
		fprintf(stderr, "cannot open %s\n", t);
		exit(1);
	}
	setbuf(f, buf);
	fprintf(f, "Broadcast message ...\n\n");
	fwrite(mesg, msize, 1, f);
	exit(0);
}
