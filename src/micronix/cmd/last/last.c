/*
 * last - login-accounting
 *
 * cmd/last/last.c
 *
 * Reads /usr/adm/wtmp, the 20-byte records login(1) and init(8) write,
 * and prints each user session in reverse chronological order - start,
 * end, and duration.  A record with an empty name is a logout, and one
 * whose name is "~" is a reboot: sessions left open at a reboot are
 * reported as ending at "boot".  last -s prints only a total per user
 * and per tty.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>

/*
 * The wtmp record, 20 bytes: line at +0, name at +8, time at +16.
 * <utmp.h> spells this tty/name/UINT32 time; the time is a long here
 * so it feeds ctime directly.  Both are the same 20 bytes.
 */
struct utmp {
	char	tty[8];
	char	name[8];
	long	time;
};

struct sess {
	char	name[8];
	char	tty[8];
	long	start;
	long	end;		/* 0 = still logged in, -1 = ended at boot */
};

#define	MAXSESS	256
#define	MAXOPEN	64
#define	MAXF	64

static struct sess sess[MAXSESS];
static int	nsess;

static struct {
	char	tty[8];
	char	name[8];
	long	start;
} open_[MAXOPEN];
static int	nopen;

static int	sflag;		/* -s: summary only */
static char	*users[MAXF], *ttys[MAXF];
static int	nusers, nttys;

char	*ctime();

static void copy8();

/* two 8-byte fields, byte for byte */
static int
eq8(a, b)
	char *a, *b;
{
	int i;

	for (i = 0; i < 8; i++)
		if (a[i] != b[i])
			return 0;
	return 1;
}

/* a zero-padded 8-byte field against a null-terminated string */
static int
eqname(field, str)
	char *field, *str;
{
	int i;

	for (i = 0; i < 8; i++) {
		if (str[i] == '\0') {
			for (; i < 8; i++)
				if (field[i] != '\0')
					return 0;
			return 1;
		}
		if (field[i] != str[i])
			return 0;
	}
	return 1;
}

static void
add_sess(name, tty, start, end)
	char *name, *tty;
	long start, end;
{
	int i;

	if (nsess >= MAXSESS)
		return;
	for (i = 0; i < 8; i++) {
		sess[nsess].name[i] = name[i];
		sess[nsess].tty[i] = tty[i];
	}
	sess[nsess].start = start;
	sess[nsess].end = end;
	nsess++;
}

/* close the session open on a tty, if any, and take it off the list */
static void
close_tty(tty, end)
	char *tty;
	long end;
{
	int i, j;

	for (i = 0; i < nopen; i++) {
		if (eq8(open_[i].tty, tty)) {
			add_sess(open_[i].name, open_[i].tty,
				 open_[i].start, end);
			for (j = i; j < nopen - 1; j++) {
				copy8(open_[j].tty, open_[j + 1].tty);
				copy8(open_[j].name, open_[j + 1].name);
				open_[j].start = open_[j + 1].start;
			}
			nopen--;
			return;
		}
	}
}

static void
open_tty(name, tty, start)
	char *name, *tty;
	long start;
{
	int i;

	if (nopen >= MAXOPEN)
		return;
	for (i = 0; i < 8; i++) {
		open_[nopen].name[i] = name[i];
		open_[nopen].tty[i] = tty[i];
	}
	open_[nopen].start = start;
	nopen++;
}

/* one record, in file order */
static void
account(u)
	struct utmp *u;
{
	int i;

	if (u->name[0] == '~') {
		/* a reboot closes every open session at "boot" */
		for (i = nopen - 1; i >= 0; i--)
			close_tty(open_[i].tty, -1);
	} else if (u->name[0] == '\0') {
		close_tty(u->tty, u->time);
	} else {
		/* a login; a session already open on this tty was never
		   logged out, so close it at this login first */
		close_tty(u->tty, u->time);
		open_tty(u->name, u->tty, u->time);
	}
}

/* "Sun Jan  1 00:00" - the first sixteen bytes of ctime */
static void
prdate(t)
	long t;
{
	char *s;
	int i;

	s = ctime(&t);
	for (i = 0; i < 16; i++)
		putchar(s[i]);
}

static void
prdur(t)
	long t;
{
	int m;

	m = t / 60;
	printf("(%d:%02d)", m / 60, m % 60);
}

/* does this session pass the name/tty filters? */
static int
wanted(name, tty)
	char *name, *tty;
{
	int i;

	if (nusers == 0 && nttys == 0)
		return 1;
	for (i = 0; i < nusers; i++)
		if (eqname(name, users[i]))
			return 1;
	for (i = 0; i < nttys; i++)
		if (eqname(tty, ttys[i]))
			return 1;
	return 0;
}

static void
printall()
{
	int i;

	for (i = nsess - 1; i >= 0; i--) {
		if (!wanted(sess[i].name, sess[i].tty))
			continue;
		printf("%.8s  %.8s  ", sess[i].name, sess[i].tty);
		prdate(sess[i].start);
		printf(" - ");
		if (sess[i].end == -1)
			printf("boot       ");
		else if (sess[i].end == 0)
			printf("still on   ");
		else
			prdate(sess[i].end);
		if (sess[i].end > 0) {
			printf("  ");
			prdur(sess[i].end - sess[i].start);
		}
		printf("\n");
	}
}

/*
 * summary: total logged-in time per user and per tty.  A session that
 * ended at boot or is still on has no end to measure against and
 * contributes nothing.
 */
static void
summary()
{
	static char unames[MAXF][8], tnames[MAXF][8];
	static long utime[MAXF], ttime[MAXF];
	int i, j, nun, ntn;
	long d;

	nun = ntn = 0;
	for (i = 0; i < MAXF; i++)
		utime[i] = ttime[i] = 0;
	for (i = 0; i < nsess; i++) {
		if (sess[i].end <= 0)
			continue;
		if (!wanted(sess[i].name, sess[i].tty))
			continue;
		d = sess[i].end - sess[i].start;
		for (j = 0; j < nun; j++)
			if (eq8(unames[j], sess[i].name))
				break;
		if (j == nun) {
			if (nun >= MAXF)
				continue;
			copy8(unames[nun], sess[i].name);
			nun++;
		}
		utime[j] += d;
		for (j = 0; j < ntn; j++)
			if (eq8(tnames[j], sess[i].tty))
				break;
		if (j == ntn) {
			if (ntn >= MAXF)
				continue;
			copy8(tnames[ntn], sess[i].tty);
			ntn++;
		}
		ttime[j] += d;
	}
	for (i = 0; i < nun; i++) {
		printf("%.8s  ", unames[i]);
		prdur(utime[i]);
		printf("\n");
	}
	for (i = 0; i < ntn; i++) {
		printf("%.8s  ", tnames[i]);
		prdur(ttime[i]);
		printf("\n");
	}
}

static void
copy8(d, s)
	char *d, *s;
{
	int i;

	for (i = 0; i < 8; i++)
		d[i] = s[i];
}

main(argc, argv)
	int argc;
	char **argv;
{
	int fd, i, n;
	struct utmp u;

	sflag = 0;
	nusers = nttys = 0;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0) {
			sflag = 1;
		} else if (strncmp(argv[i], "tty", 3) == 0) {
			if (nttys < MAXF)
				ttys[nttys++] = argv[i];
		} else {
			if (nusers < MAXF)
				users[nusers++] = argv[i];
		}
	}

	fd = open("/usr/adm/wtmp", 0);
	if (fd < 0) {
		fprintf(stderr, "last: no /usr/adm/wtmp\n");
		exit(1);
	}
	while ((n = read(fd, &u, sizeof u)) == sizeof u)
		account(&u);
	close(fd);
	for (i = 0; i < nopen; i++)
		close_tty(open_[i].tty, 0);
	nopen = 0;

	if (sflag)
		summary();
	else
		printall();
	exit(0);
}
