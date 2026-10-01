/*
 * getgrent, getgrgid, getgrnam, setgrent, endgrent - /etc/group.
 *
 * lib/libc/getgrent.c
 *
 * The sibling of getpwent.c, written rather than recovered: there is
 * no getgrent in the 1982 login binary to read it out of, and no
 * getgrent.3 in the 1.3 manual either.  The shape is getpwent's,
 * because /etc/group is the same colon-separated, one-record-per-line
 * format with one more field at the end.
 *
 * The field splitter and the rewind-or-open logic are that file's,
 * deliberately: a second /etc reader that split fields differently
 * would be a second set of bugs.  What is new here is the member
 * list, which getpwent has no counterpart for - gr_mem is an array
 * of pointers into the line, terminated by a NULL, and the line is
 * split on ',' after its terminator has been stripped.
 *
 * As in getpwent.c, one entry comes back at a time from a static
 * area that the next call overwrites.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include	<types.h>
#include	<stdio.h>
#include	<grp.h>

#define	NGMEM	32		/* member slots in the static vector */

static char *	fld();
int		setgrent();

static FILE *		grf;		/* the open stream */
static char		grline[256];	/* one line of it */
static struct group	grent;		/* the record handed back */
static char *		grmem[NGMEM];	/* the member vector */

/*
 * End the field at the cursor: NUL the separator and return the
 * character after it, or NULL if the line ran out first.  getpwent.c
 * has the same function and the same note; the separators are
 * ":\n\r".
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
 * The next entry, or NULL at the end of the file.  A line with too
 * few fields, or one with no terminator at all, is skipped - the
 * second is a line longer than the buffer, and fgets gives it back
 * without its newline.
 */
struct group *
getgrent()
{
	char *cp, *p;
	int i;

	if (grf == NULL && setgrent() == 0)
		return (NULL);
	for (;;) {
		if (fgets(grline, sizeof grline, grf) == NULL)
			return (NULL);

		cp = grline;
		grent.gr_name = cp;
		cp = fld(cp);
		grent.gr_passwd = cp;
		cp = fld(cp);
		if (cp == NULL)
			continue;
		grent.gr_gid = atoi(cp);
		cp = fld(cp);

		grent.gr_mem = grmem;
		grmem[0] = NULL;
		if (cp == NULL)		/* no member field at all */
			return (&grent);

		/*
		 * Strip the terminator first, as getpwent does for the
		 * shell field, and for the same reason: the fields
		 * above are ended by their separator, and this one has
		 * none.
		 */
		for (p = cp; *p && *p != '\r' && *p != '\n'; p++)
			;
		if (*p == 0)
			continue;
		*p = 0;

		i = 0;
		for (p = cp; *p; ) {
			if (i < NGMEM - 1)
				grmem[i++] = p;
			while (*p && *p != ',')
				p++;
			if (*p == 0)
				break;
			*p++ = 0;
		}
		grmem[i] = NULL;
		return (&grent);
	}
}

/*
 * Rewind a stream already open, otherwise open the file.
 */
int
setgrent()
{
	if (grf != NULL)
		rewind(grf);
	else if ((grf = fopen("/etc/group", "r")) == NULL)
		return (0);
	return (1);
}

/*
 * The first entry whose name matches.
 */
struct group *
getgrnam(name)
char *name;
{
	struct group *g;

	if (setgrent() == 0)
		return (NULL);
	for (;;) {
		g = getgrent();
		if (g == NULL)
			return (NULL);
		if (strcmp(g->gr_name, name) == 0)
			return (g);
	}
}

/*
 * getgrnam's other way round - the first entry whose number matches.
 */
struct group *
getgrgid(gid)
int gid;
{
	struct group *g;

	if (setgrent() == 0)
		return (NULL);
	for (;;) {
		g = getgrent();
		if (g == NULL)
			return (NULL);
		if (g->gr_gid == gid)
			return (g);
	}
}

/*
 * Close the stream, as endpwent does.
 */
int
endgrent()
{
	if (grf != NULL) {
		fclose(grf);
		grf = NULL;
	}
	return (0);
}
