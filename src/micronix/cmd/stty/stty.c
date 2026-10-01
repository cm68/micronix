/*
 * stty [-a] [option ...]
 *
 * v7 stty (usr/src/cmd/stty.c), ported to micronix.
 *
 * cmd/stty/stty.c
 *
 * The shape is v7's and stays that way: a table of baud rates, a
 * table of mode keywords as {string, set, reset} triples, eq() to
 * take one keyword off the front of the argument, and prmodes() to
 * report.  The contents of those tables, and the report, are
 * micronix's, and the page stty (1) is where they come from.
 *
 * THE STRUCTURE IS struct sgtty, NOT v7's struct sgttyb.  The two
 * describe the same six bytes - a speed, an erase character, a kill
 * character and a mode word - so v7's field names would have worked,
 * but micronix has a header for it: <sys/sgtty.h> names the fields
 * ispeed, ospeed, erase, kill and mode, and that is exactly what
 * libu's gtty and stty move.  sys/tty.c's ttymode() writes and reads
 * the terminal with iomove(flag, tty, 6) on the head of struct tty,
 * whose first six bytes are {UINT8 ispeed; UINT8 ospeed; UINT8 erase;
 * UINT8 kill; UINT mode;} - the header's struct to the byte.  So the
 * port takes sgtty.h's struct, its field names, and its B#### and
 * mode defines.  login.c's private struct sgttyb is those same six
 * bytes under other names (int speeds; char erase, kill; int tflags),
 * written before there was a header; there is no reason for a second
 * name for one thing, and no other command in the tree uses it.
 *
 * v7's mode bits are not micronix's, even where the names match.
 * <sys/sgtty.h> carries the v6 numbers, and <sys/tty.h> - the
 * kernel's - carries the ones the driver actually tests.  RAW, ECHO,
 * CRMOD and ULMOD are the same number in both, but the tab bit is
 * TABS in sgtty.h where tty.h calls it XTABS, and the 8-bit input bit
 * ALL8 has no name in sgtty.h at all.  The two are spelled out below,
 * as init.c spells them out, for the same reason.
 *
 * THE TAB BIT IS THE ONE PLACE THE PORT DOES NOT FOLLOW THE stty ON
 * THE DISTRIBUTION DISK, and the documents are all on one side of it.
 * stty (2) lists the bit as "0002 Expand tabs"; <sys/tty.h> gives it
 * the same gloss, "Expand tabs via spaces"; sys/tty.c emits a tab as
 * spaces under it; and stty (1) calls the state that goes with it
 * "-tabs", "Replace tabs by spaces when printing; default".  So the
 * keyword -tabs turns the bit on and tabs turns it off, which is
 * v7's sense as well.  The distributed /bin/stty has it the other way
 * round in both the keyword and the report - its -tabs clears the bit
 * and its report calls the cleared bit "-tabs" - so a user following
 * stty (1) gets the opposite of what the disk's command does.  This
 * port follows the page.
 *
 * WHAT THE PORT TOOK OUT.
 *
 *	ioctl(1, TIOCHPCL, NULL)  there is no terminal ioctl here - the
 *			only ioctl in the tree is the block-device one -
 *			and nothing replaces it.  sgtty.h has a HUP bit,
 *			"drop dtr on last close", but no file under sys/
 *			reads it, so there is no keyword for it here.
 *
 *	the delay keywords	cr0 to cr3, nl0 to nl3, tab0 to tab3,
 *			ff0, ff1, bs0, bs1, and the terminal names
 *			that set them - 33, tty33, 37, tty37, 05, vt05,
 *			tn, tn300, ti, ti700, tek - are gone, and the
 *			page says why: "Micronix presently has no
 *			built-in delays".  The bits they name are also
 *			not free here.  sgtty.h gives bs1 and shake the
 *			one number 0100000 and ff1 and ALL8 the one
 *			number 0040000, and cr1 and cr2 land on the
 *			driver's MORE and CBREAK; a keyword that sets
 *			the bit for something else is worse than no
 *			keyword.
 *
 *	even, odd	the parity bits, which the driver never tests.
 *
 *	cbreak		MORE and CBREAK are the driver's and are not
 *			on the page.
 *
 *	gspeed, ek, hup, and the ALLDELAY term	not on the page.
 *
 * WHAT IT NO LONGER HAS TO SAY DIFFERENTLY.  v7's erase and kill take
 * a control character spelled ^x.  That is kept; the page's own
 * default erase character is written ^H, so the form is wanted.  What
 * is new is that an erase or kill with no argument is an error rather
 * than a read of the argument that is not there, and that an
 * unrecognized keyword is named and then stops the command instead of
 * being complained about and then ignored.
 *
 * THE REPORT IS MICRONIX'S, NOT v7's.  v7 printed the speed, the
 * erase and kill characters, and then every flag that was SET, so a
 * terminal at its defaults still printed the defaults' names.  The
 * page here promises the other thing - "it reports the current
 * settings of the options which differ from the default settings" -
 * and the default is the one init gives a line, CRMOD|ECHO|TABS.  So
 * the report is the options that DIFFER, and -a is all of them, each
 * with its sign.  Both go to the standard error, as v7's did; -a is
 * not on the 1982 page, see stty (1).
 *
 * A keyword is applied and written back only if there was one, so the
 * query form does not rewrite the terminal with what it just read.
 *
 * What the port could NOT take from v7 is the baud rate.  v7 gave two
 * lines when input and output differed ("input speed 300 baud"); the
 * report here is one line, so the output rate follows the input rate
 * on the same line when the two disagree.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>		/* sgtty.h speaks in UINT8s */
#include <stdio.h>
#include <sys/sgtty.h>

/*
 * The two bits the driver honours that <sys/sgtty.h> does not name.
 * TABS there is XTABS in <sys/tty.h>, which is the same bit under the
 * name the kernel and the page use; ALL8 exists only in tty.h.  See
 * init.c, which defines the same two for its /etc/ttys reader.
 */
#define XTABS	TABS		/* 0000002 - expand tabs via spaces */
#define ALL8	0040000		/* keep all 8 bits of input */

struct
{
	char	*string;
	int	speed;
} speeds[] = {
	"50",	B50,
	"75",	B75,
	"110",	B110,
	"134",	B134,
	"134.5", B134,
	"150",	B150,
	"200",	B200,
	"300",	B300,
	"600",	B600,
	"1200",	B1200,
	"1800",	B1800,
	"2400",	B2400,
	"4800",	B4800,
	"9600",	B9600,
	"19200", B19200,
	0,	0,
};

/*
 * set is what the keyword turns on, reset what it turns off, and a
 * keyword that only does one of the two leaves the other 0.  nl and
 * -nl are the round trip the page describes: -nl is the default, and
 * it is what puts the carriage return to work as an end of line.
 */
struct
{
	char	*string;
	int	set;
	int	reset;
} modes[] = {
	"raw",		RAW,	0,
	"-raw",		0,	RAW,
	"cooked",	0,	RAW,

	"-nl",		CRMOD,	0,
	"nl",		0,	CRMOD,

	"echo",		ECHO,	0,
	"-echo",	0,	ECHO,

	"lcase",	ULMOD,	0,
	"-lcase",	0,	ULMOD,

	"-tabs",	XTABS,	0,
	"tabs",		0,	XTABS,

	"shake",	SHAKE,	0,
	"-shake",	0,	SHAKE,

	"data8",	ALL8,	0,
	"-data8",	0,	ALL8,
	"data7",	0,	ALL8,

	0,		0,	0,
};

/*
 * The report, in the order it prints.  set is the name of the state
 * with the bit on and clear the name of the state with it off, which
 * is the other way round from v7's - see the header.  def says which
 * of the two the driver comes up with, and so which one is not worth
 * mentioning unless -a asked for everything.
 */
struct
{
	char	*set;
	char	*clear;
	int	bit;
	int	def;
} report[] = {
	"raw",		"-raw",		RAW,	0,
	"-tabs",	"tabs",		XTABS,	1,
	"-nl",		"nl",		CRMOD,	1,
	"echo",		"-echo",	ECHO,	1,
	"lcase",	"-lcase",	ULMOD,	0,
	"shake",	"-shake",	SHAKE,	0,
	"data8",	"data7",	ALL8,	0,
	0,		0,		0,	0,
};

char	*arg;			/* the keyword being looked at now */
int	all;			/* -a: report every option, not the changes */
int	changed;		/* a keyword was taken: write the mode back */
struct sgtty mode;

main(argc, argv)
	int argc;
	char *argv[];
{

	if (gtty(1, &mode) < 0) {
		fprintf(stderr, "Not a typewriter\n");
		exit(1);
	}
	while (--argc > 0) {
		arg = *++argv;
		if (eq("-a")) {
			all = 1;
		}
		if (eq("erase")) {
			argc--;
			if (argc < 1)
				badarg("erase");
			mode.erase = chr(*++argv);
			changed = 1;
		}
		if (eq("kill")) {
			argc--;
			if (argc < 1)
				badarg("kill");
			mode.kill = chr(*++argv);
			changed = 1;
		}
		setspeed();
		setmode();
		if (arg != 0)
			fatal(arg);
	}
	if (changed)
		stty(1, &mode);
	prmodes();
	exit(0);
}

/*
 * the last keyword, as a name that the caller's variables may be set
 * from.  Clears arg, so that the scan below cannot take one argument
 * for two keywords.
 */
eq(string)
	char *string;
{
	register int i;

	if (arg == 0)
		return (0);
	i = 0;
loop:
	if (arg[i] != string[i])
		return (0);
	if (arg[i++] != '\0')
		goto loop;
	arg = 0;
	return (1);
}

/*
 * a baud rate named on the command line: both speeds, since the
 * driver has one speed and this is the only way to give it one.
 */
setspeed()
{
	register int i;

	for (i = 0; speeds[i].string; i++)
		if (eq(speeds[i].string)) {
			mode.ispeed = mode.ospeed = speeds[i].speed;
			changed = 1;
		}
}

setmode()
{
	register int i;

	for (i = 0; modes[i].string; i++)
		if (eq(modes[i].string)) {
			mode.mode &= ~modes[i].reset;
			mode.mode |= modes[i].set;
			changed = 1;
		}
}

/*
 * the character an erase or kill argument names.  ^x is the control
 * character x, as it is written on the page; anything else is taken
 * for itself.  A lone ^, or a character with the high bit set, comes
 * back as the byte it is.
 */
chr(s)
	char *s;
{

	if (s[0] == '^')
		return (s[1] & 037);
	return (s[0] & 0377);
}

/*
 * the number a speed code stands for, or "?" for a code that is not a
 * baud rate at all - which is what a line the driver has not been
 * told a speed for answers with.  The table is searched by code, so
 * the two spellings of 134 answer with the same number.
 */
char *
baud(code)
	int code;
{
	register int i;

	for (i = 0; speeds[i].string; i++)
		if (speeds[i].speed == code)
			return (speeds[i].string);
	return ("?");
}

/*
 * the erase and kill characters as the report shows them: a control
 * character as ^X, anything else as itself.
 */
char *
vis(c)
	int c;
{
	static char buf[3];

	c &= 0377;
	if (c < ' ') {
		buf[0] = '^';
		buf[1] = c + '@';
		buf[2] = '\0';
	} else {
		buf[0] = c;
		buf[1] = '\0';
	}
	return (buf);
}

prmodes()
{
	register int i;

	fprintf(stderr, "%s baud", baud(mode.ispeed));
	if (mode.ospeed != mode.ispeed)
		fprintf(stderr, ", output %s baud", baud(mode.ospeed));
	for (i = 0; report[i].set; i++) {
		if ((mode.mode & report[i].bit) != 0) {
			if (all == 0 && report[i].def)
				continue;
			fprintf(stderr, ", %s", report[i].set);
		} else {
			if (all == 0 && report[i].def == 0)
				continue;
			fprintf(stderr, ", %s", report[i].clear);
		}
	}
	fprintf(stderr, ", erase '%s'", vis(mode.erase));
	fprintf(stderr, ", kill '%s'\n", vis(mode.kill));
}

fatal(s)
	char *s;
{

	fprintf(stderr, "stty: %s: Unknown mode.\n", s);
	exit(1);
}

badarg(s)
	char *s;
{

	fprintf(stderr, "stty: %s: needs a character\n", s);
	exit(1);
}
