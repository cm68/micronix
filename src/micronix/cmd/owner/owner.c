/*
 * owner [-newuser] file ...
 *
 * v7 chown (usr/src/cmd/chown.c), ported to micronix.
 *
 * cmd/owner/owner.c
 *
 * The interface is the one micronix gave the command and not the one
 * the original had: owner reports an owner as well as setting one,
 * and takes the new owner behind a dash - "owner -root a.out" where
 * v7's chown took "chown root a.out".  Everything above the loop is
 * the man page; what is below it is the port.
 *
 * What the port took out:
 *
 *	getpwnam and getpwuid	no password routines in libc, so
 *			/etc/passwd is read here, the way ls reads
 *			it: a name is looked up by hand and a number
 *			is taken as itself.  The report wants the
 *			same scan read the other way, so the two
 *			lookups share one reader.
 *
 *	chown(name, uid, gid)	chown here takes the owner packed as
 *			uid | gid << 8 and sets both at once, and has
 *			no -1 to leave a field alone.  So every change
 *			stats first and carries the group through.
 *
 * The report prints the name where /etc/passwd has one and the number
 * where it does not, a line per file in the order the files were
 * named.  The man page fixes that much and no more.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>

struct	stat stbuf;
int	status;

main(argc, argv)
	int argc;
	char *argv[];
{
	register c;
	int uid;
	char *newuser;
	char name[16];

	newuser = NULL;
	if (argc > 1 && argv[1][0] == '-') {
		newuser = &argv[1][1];
		argc--;
		argv++;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: owner [-newuser] file ...\n");
		exit(4);
	}

	if (newuser == NULL) {
		for (c = 1; c < argc; c++) {
			if (stat(argv[c], &stbuf)) {
				Perror(argv[c]);
				continue;
			}
			if (getname(stbuf.st_uid, name) == 0)
				printf("%s\n", name);
			else
				printf("%d\n", stbuf.st_uid);
		}
		exit(status);
	}

	if (isnumber(newuser))
		uid = atoi(newuser);
	else {
		uid = getuser(newuser);
		if (uid < 0)
			fatal("%s: No such user", newuser);
	}
	for (c = 1; c < argc; c++) {
		if (stat(argv[c], &stbuf)) {
			Perror(argv[c]);
			continue;
		}
		if (chown(argv[c], uid | (stbuf.st_gid << 8))) {
			Perror(argv[c]);
			continue;
		}
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register int c;

	while (c = *s++)
		if (c < '0' || c > '9')
			return (0);
	return (1);
}

/*
 * One pass over /etc/passwd - name:passwd:uid:gid:... - for both
 * directions.  look is a name and the answer is the uid, or look is
 * NULL and the answer is the name belonging to number.  -1 or 0 when
 * there is no such entry.  Character at a time, the way ls reads it:
 * there is no fgets in this library either.
 */
getpw(look, number, name)
	char *look;
	int number;
	char *name;
{
	FILE *pf;
	register int c;
	int i, j, n;

	if ((pf = fopen("/etc/passwd", "r")) == NULL)
		return (-1);
	for (;;) {
		i = 0;
		j = 0;
		n = 0;
		while ((c = fgetc(pf)) != '\n') {
			if (c == EOF) {
				fclose(pf);
				return (-1);
			}
			if (c == ':') {
				j++;
				continue;
			}
			if (j == 0 && i < 15)
				name[i++] = c;
			if (j == 2)
				n = n * 10 + c - '0';
		}
		name[i] = '\0';
		if (look != NULL) {
			if (strcmp(name, look) == 0) {
				fclose(pf);
				return (n);
			}
		} else if (n == number) {
			fclose(pf);
			return (0);
		}
	}
}

getuser(name)
	char *name;
{
	char buf[16];

	return (getpw(name, 0, buf));
}

getname(uid, buf)
	int uid;
	char *buf;
{
	return (getpw(NULL, uid, buf));
}

fatal(fmt, a)
	char *fmt, *a;
{

	fprintf(stderr, "owner: ");
	fprintf(stderr, fmt, a);
	putc('\n', stderr);
	exit(255);
}

Perror(s)
	char *s;
{

	fprintf(stderr, "owner: ");
	perror(s);
	status = 1;
	return (1);
}
