/*
 * getpwent, getpwuid, getpwnam, setpwent, endpwent - /etc/passwd.
 *
 * lib/libc/getpwent.c
 *
 * The 1982 library module, moved here from cmd/login, where it had
 * been living because this tree had no such module and login was the
 * only thing that wanted one.  login's own note gives the reason it
 * belongs here instead: in the 1982 binary these are one library
 * module, and login is only their caller.
 *
 * The addresses login read the code out of are the evidence for it,
 * and they are kept: 0x0f0b (getpwent), 0x1031 (the separators),
 * 0x1035 (fld), 0x1103 (getpwnam), 0x115d (setpwent), 0x119b
 * (endpwent).
 *
 * getpwuid is not among them.  The binary had no use for it - login
 * looks names up and never numbers - but getpwent.3 documents it, so
 * it is here, and it is getpwnam with the comparison changed.
 *
 * The record is the tree's own <pwd.h>, and the twelve bytes line up
 * with the binary's: name+0, passwd+2, uid+4, gid+5, person+6, dir+8,
 * shell+10, uid and gid one byte each.  login reads those two bytes
 * together as one word, because setuid and chown here take the owner
 * packed as uid | gid << 8 - which is why the struct has that shape.
 *
 * One entry comes back at a time, from a static area that the next
 * call overwrites.  getpwent.3 says so: copy anything to be kept.
 *
 * endpwent is the one place the manual and the binary disagree.  The
 * binary's epilogue is a bare return, so login's copy is void, and
 * getpwent.3 declares int.  A library owes its callers what the
 * manual says, so it returns 0 here.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include	<types.h>
#include	<stdio.h>
#include	<pwd.h>

static char *	fld();
int		setpwent();

static FILE *		pwf;		/* 0x60c4 - the open stream */
static char		pwline[256];	/* 0x60c6 - one line of it */
static struct passwd	pwent;		/* 0x61c8 - the record handed back */

/*
 * 0x1035 - end the field at the cursor: NUL the separator and return
 * the character after it, or NULL if the line ran out first.  The
 * separators are the literal ":\n\r" the binary keeps at 0x1031.
 */
static char *
fld(p)
char *p;
{
	if (p == NULL)
		return (NULL);
	while (*p) {
		if (*p == ':' || *p == '\n' || *p == '\r')
			break;
		p++;
	}
	if (*p == 0)
		return (NULL);
	*p++ = 0;
	return (p);
}

/*
 * 0x0f0b - the next entry, or NULL at the end of the file.  A line
 * with too few fields is skipped; the three tests below are the three
 * the binary makes, not a fourth.
 */
struct passwd *
getpwent()
{
	char *cp;

	if (pwf == NULL && setpwent() == 0)
		return (NULL);
	for (;;) {
		if (fgets(pwline, sizeof pwline, pwf) == NULL)
			return (NULL);

		cp = pwline;
		pwent.name = cp;
		cp = fld(cp);
		pwent.passwd = cp;
		cp = fld(cp);
		if (cp == NULL)
			continue;
		pwent.uid = atoi(cp);
		cp = fld(cp);
		if (cp == NULL)
			continue;
		pwent.gid = atoi(cp);
		cp = fld(cp);
		pwent.person = cp;
		cp = fld(cp);
		pwent.dir = cp;
		cp = fld(cp);
		pwent.shell = cp;
		/*
		 * The shell field is NOT ended by a fld call: the strip
		 * below turns its '\n' (or '\r') into the NUL.  A fld
		 * here would run past the newline and leave cp at
		 * end-of-string, which the *cp == 0 test below reads as
		 * a malformed line and skips.
		 */
		if (cp == NULL)
			continue;
		while (*cp && *cp != '\r' && *cp != '\n')
			cp++;
		if (*cp == 0)
			continue;
		*cp = 0;
		return (&pwent);
	}
}

/*
 * 0x115d - rewind a stream already open, otherwise open the file.
 */
int
setpwent()
{
	if (pwf != NULL)
		rewind(pwf);
	else if ((pwf = fopen("/etc/passwd", "r")) == NULL)
		return (0);
	return (1);
}

/*
 * 0x1103 - the first entry whose name matches.
 */
struct passwd *
getpwnam(name)
char *name;
{
	struct passwd *p;

	if (setpwent() == 0)
		return (NULL);
	for (;;) {
		p = getpwent();
		if (p == NULL)
			return (NULL);
		if (strcmp(p->name, name) == 0)
			return (p);
	}
}

/*
 * getpwnam's other way round - the first entry whose number matches.
 */
struct passwd *
getpwuid(uid)
int uid;
{
	struct passwd *p;

	if (setpwent() == 0)
		return (NULL);
	for (;;) {
		p = getpwent();
		if (p == NULL)
			return (NULL);
		if (p->uid == uid)
			return (p);
	}
}

/*
 * 0x119b - close the stream.  The binary's epilogue is a bare return
 * and this returns 0; see the note at the head of the file.
 */
int
endpwent()
{
	if (pwf != NULL) {
		fclose(pwf);
		pwf = NULL;
	}
	return (0);
}
