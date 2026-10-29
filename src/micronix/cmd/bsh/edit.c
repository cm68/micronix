#
/*
 * UNIX shell - a small line editor
 *
 * cmd/bsh/edit.c
 *
 * The line typed at a terminal prompt is edited here, in raw mode,
 * with the emacs keys a person expects and the arrow keys a terminal
 * sends:
 *
 *	^A ^E		start and end of line
 *	^B ^F		left and right one character
 *	^D		delete the character under the cursor, or EOF when
 *			the line is empty
 *	^H DEL		rub out the character before the cursor
 *	^K		kill to the end of the line
 *	^U		kill to the start
 *	^W		kill the word before the cursor
 *	^L		clear the screen and redraw
 *	^P ^N		previous and next history line
 *	^C		abandon the line
 *	up down left right, home end, delete
 *
 * The terminal is put in RAW mode for the duration, which on this
 * system means the driver stops cooking: no erase, no kill, no signal
 * characters, one byte per read.  Echo is off too; the editor writes
 * what it wants, with CR and LF spelled out, since output processing
 * is gone as well.
 *
 * History is the ring history.c keeps; Up is the same thing as ^P.
 * The editor is only entered when the input really is a terminal;
 * gtty decides, and a -i shell reading a pipe still gets the plain
 * line reader.
 */

#include	"defs.h"
#include	<types.h>
#include	<sys/sgtty.h>

#define	EDMAX	128

#define	CTLA	1
#define	CTLB	2
#define	CTLC	3
#define	CTLD	4
#define	CTLE	5
#define	CTLF	6
#define	CTLH	8
#define	EDTAB	9
#define	EDNL	10
#define	CTLK	11
#define	CTLL	12
#define	EDCR	13
#define	CTLN	14
#define	CTLP	16
#define	CTLU	21
#define	CTLW	23
#define	EDESC	27
#define	EDDEL	127

static char	eddraft[EDMAX];		/* the line before history took over */
static int	eddlen;
static int	edold;			/* columns the line last filled */
static int	edoutfd;

static void
edout(s, n)
	char *s;
	int n;
{
	write(edoutfd, s, n);
}

static void
edbeep()
{
	edout("\a", 1);
}

/*
 * Redraw is carriage return, the line, blanks over what was there
 * before, then backspaces to the cursor.  No cursor addressing, so it
 * works on a terminal that knows nothing but CR, BS and space.
 */
static void
edredraw(buf, len, pos)
	char *buf;
	int len, pos;
{
	int i, wide;

	wide = len > edold ? len : edold;
	edout("\r", 1);
	if (len > 0)
		edout(buf, len);
	for (i = len; i < wide; i++)
		edout(" ", 1);
	for (i = wide; i > pos; i--)
		edout("\b", 1);
	edold = wide;
}

static void
edinsert(buf, lp, pp, max, c)
	char *buf;
	int *lp;
	int *pp;
	int max, c;
{
	int i;
	char ch;

	if (*lp >= max - 1) {
		edbeep();
		return;
	}
	if (*pp == *lp) {
		i = *lp;
		buf[i] = c;
		*lp = i + 1;
		(*pp)++;
		ch = c;
		edout(&ch, 1);
		if (*lp > edold)
			edold = *lp;
		return;
	}
	for (i = *lp; i > *pp; i--)
		buf[i] = buf[i - 1];
	i = *pp;
	buf[i] = c;
	*pp = i + 1;
	(*lp)++;
	edredraw(buf, *lp, *pp);
}

static void
edrub(buf, lp, pp)
	char *buf;
	int *lp;
	int *pp;
{
	int i;

	if (*pp == 0) {
		edbeep();
		return;
	}
	if (*pp == *lp) {
		(*pp)--;
		(*lp)--;
		edout("\b \b", 3);
		edold = *lp;
		return;
	}
	for (i = *pp - 1; i < *lp - 1; i++)
		buf[i] = buf[i + 1];
	(*pp)--;
	(*lp)--;
	edredraw(buf, *lp, *pp);
}

static void
eddel(buf, lp, pp)
	char *buf;
	int *lp;
	int *pp;
{
	int i;

	if (*pp >= *lp) {
		edbeep();
		return;
	}
	for (i = *pp; i < *lp - 1; i++)
		buf[i] = buf[i + 1];
	(*lp)--;
	edredraw(buf, *lp, *pp);
}

static int
edspace(c)
	int c;
{
	return (c == ' ' || c == '\t');
}

static void
edkillend(buf, lp, pp)
	char *buf;
	int *lp;
	int *pp;
{
	*lp = *pp;
	edredraw(buf, *lp, *pp);
}

static void
edkillstart(buf, lp, pp)
	char *buf;
	int *lp;
	int *pp;
{
	int i;

	for (i = *pp; i < *lp; i++)
		buf[i - *pp] = buf[i];
	*lp -= *pp;
	*pp = 0;
	edredraw(buf, *lp, *pp);
}

static void
edkillword(buf, lp, pp)
	char *buf;
	int *lp;
	int *pp;
{
	int i, j, n;

	i = *pp;
	while (i > 0 && edspace(buf[i - 1]))
		i--;
	while (i > 0 && !edspace(buf[i - 1]))
		i--;
	n = *pp - i;
	for (j = i; j < *lp - n; j++)
		buf[j] = buf[j + n];
	*lp -= n;
	*pp = i;
	edredraw(buf, *lp, *pp);
}

static void
edload(buf, lp, pp, n)
	char *buf;
	int *lp;
	int *pp;
	int n;
{
	char *l;
	int i;

	l = histevent(n);
	i = 0;
	if (l)
		while (l[i] && i < EDMAX - 1) {
			buf[i] = l[i];
			i++;
		}
	*lp = i;
	*pp = i;
	edredraw(buf, *lp, *pp);
}

/*
 * One byte, with the tty restored around a fatal signal: a shell that
 * dies in raw mode leaves the terminal that way.
 */
static int
edread(f, save, raw)
	FILE f;
	struct sgtty *save;
	struct sgtty *raw;
{
	CHAR c;
	int rc;

	for (;;) {
		rc = read(f->fdes, &c, 1);
		if (rc == 1)
			return (c & 0377);
		if (rc < 0 && (trapnote & SIGSET)) {
			stty(f->fdes, &save);
			edout("\r\n", 2);
			sigchk();
			stty(f->fdes, raw);
			continue;
		}
		return (-1);
	}
}

static void
edprev(buf, lp, pp, hn, hposp, hfirst)
	char *buf;
	int *lp;
	int *pp;
	int hn, *hposp, hfirst;
{
	int i;

	if (*hposp > hfirst) {
		if (*hposp > hn) {
			for (i = 0; i < *lp; i++)
				eddraft[i] = buf[i];
			eddraft[*lp] = 0;
			eddlen = *lp;
		}
		(*hposp)--;
		edload(buf, lp, pp, *hposp);
	} else
		edbeep();
}

static void
ednext(buf, lp, pp, hn, hposp)
	char *buf;
	int *lp;
	int *pp;
	int hn, *hposp;
{
	int i;

	if (*hposp <= hn) {
		(*hposp)++;
		if (*hposp > hn) {
			for (i = 0; i < eddlen; i++)
				buf[i] = eddraft[i];
			*lp = eddlen;
			*pp = eddlen;
			edredraw(buf, *lp, *pp);
		} else
			edload(buf, lp, pp, *hposp);
	} else
		edbeep();
}

/*
 * Edit one line.  Returns its length, -1 at end of file, or -2 if the
 * input is not a terminal after all - the caller then reads it itself.
 */
INT
histedit(f, buf, max)
	FILE f;
	char *buf;
	int max;
{
	struct sgtty save, raw;
	int len, pos, c, c2, c3, param, hn, hpos, hfirst;

	if (max > EDMAX)
		max = EDMAX;
	edoutfd = output;
	if (gtty(f->fdes, &save) != 0)
		return (-2);

	raw.ispeed = save.ispeed;
	raw.ospeed = save.ospeed;
	raw.erase = save.erase;
	raw.kill = save.kill;
	raw.mode = save.mode;
	raw.mode |= RAW;
	raw.mode &= ~ECHO;
	if (stty(f->fdes, &raw) != 0)
		return (-2);

	len = 0;
	pos = 0;
	edold = 0;
	hn = histcount();
	hfirst = histfirst();
	hpos = hn + 1;
	buf[0] = 0;

	for (;;) {
		c = edread(f, &save, &raw);
		if (c < 0) {
			stty(f->fdes, &save);
			return (-1);
		}
		switch (c) {
		case EDCR:
		case EDNL:
			edout("\r\n", 2);
			stty(f->fdes, &save);
			buf[len] = 0;
			return (len);
		case CTLC:
			edout("^C\r\n", 4);
			stty(f->fdes, &save);
			buf[0] = 0;
			return (0);
		case CTLD:
			if (len == 0) {
				stty(f->fdes, &save);
				return (-1);
			}
			if (pos < len)
				eddel(buf, &len, &pos);
			else
				edbeep();
			break;
		case CTLA:
			pos = 0;
			edredraw(buf, len, pos);
			break;
		case CTLE:
			pos = len;
			edredraw(buf, len, pos);
			break;
		case CTLB:
			if (pos > 0) {
				pos--;
				edredraw(buf, len, pos);
			} else
				edbeep();
			break;
		case CTLF:
			if (pos < len) {
				pos++;
				edredraw(buf, len, pos);
			} else
				edbeep();
			break;
		case CTLH:
		case EDDEL:
			edrub(buf, &len, &pos);
			break;
		case CTLK:
			edkillend(buf, &len, &pos);
			break;
		case CTLU:
			edkillstart(buf, &len, &pos);
			break;
		case CTLW:
			edkillword(buf, &len, &pos);
			break;
		case CTLL:
			edout("\033[H\033[2J", 7);
			edold = 0;
			edredraw(buf, len, pos);
			break;
		case CTLP:
			edprev(buf, &len, &pos, hn, &hpos, hfirst);
			break;
		case CTLN:
			ednext(buf, &len, &pos, hn, &hpos);
			break;
		case EDESC:
			c2 = edread(f, &save, &raw);
			if (c2 != '[' && c2 != 'O')
				break;		/* the other alt keys */
			param = 0;
			for (;;) {
				c3 = edread(f, &save, &raw);
				if (c3 >= '0' && c3 <= '9') {
					param = param * 10 + c3 - '0';
					continue;
				}
				if (c3 == ';') {
					param = 0;
					continue;
				}
				break;
			}
			if (c3 < 0) {
				stty(f->fdes, &save);
				return (-1);
			}
			if (c3 == 'A')
				edprev(buf, &len, &pos, hn, &hpos, hfirst);
			else if (c3 == 'B')
				ednext(buf, &len, &pos, hn, &hpos);
			else if (c3 == 'C') {
				if (pos < len) {
					pos++;
					edredraw(buf, len, pos);
				} else
					edbeep();
			} else if (c3 == 'D') {
				if (pos > 0) {
					pos--;
					edredraw(buf, len, pos);
				} else
					edbeep();
			} else if (c3 == 'H') {
				pos = 0;
				edredraw(buf, len, pos);
			} else if (c3 == 'F') {
				pos = len;
				edredraw(buf, len, pos);
			} else if (c3 == '~') {
				if (param == 1 || param == 7) {
					pos = 0;
					edredraw(buf, len, pos);
				} else if (param == 4 || param == 8) {
					pos = len;
					edredraw(buf, len, pos);
				} else if (param == 3)
					eddel(buf, &len, &pos);
			}
			break;
		case EDTAB:
			edbeep();
			break;
		default:
			if (c >= 0x20 && c < EDDEL)
				edinsert(buf, &len, &pos, max, c);
			else
				edbeep();
			break;
		}
	}
}

/* vim: tabstop=4 shiftwidth=4 noexpandtab: */
