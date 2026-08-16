/*
 * Routines which deal with the characteristics of the terminal.
 *
 * cmd/less/screen.c
 *
 * The original said "uses termcap to be as terminal-independent as
 * possible", and micronix has no termcap to use.  What it has is a
 * terminal: the simulator's is an ANSI pty and the machine's console
 * speaks the same escapes, so the capabilities the original looked
 * up are written in below and the function surface is unchanged -
 * funcs.h neither knows nor cares where the strings came from.
 *
 * The terminal modes are the micronix tty driver's, through gtty and
 * stty on file descriptor 2 - the keyboard fd, see ttyin.c.  The
 * driver has RAW and nothing gentler, so raw mode takes CRMOD with
 * it; output.c makes every newline a return-newline while raw mode
 * is on, and in_raw_mode below is how it knows.
 */

#include "less.h"
#include <types.h>		/* sgtty.h speaks in UINT8s */
#include <sys/sgtty.h>

public char auto_wrap = 1;	/* Terminal does \r\n when write past margin */
public char ignaw = 1;		/* Terminal ignores \n immediately after wrap */
public int erase_char, kill_char; /* The user's erase and line-kill chars */
public int sc_width, sc_height;	/* Height & width of screen */
public char sc_window = -1;	/* window size for forward and backward */
public int bo_width, be_width;	/* Printing width of boldface sequences */
public int ul_width, ue_width;	/* Printing width of underline sequences */
public int so_width, se_width;	/* Printing width of standout sequences */
public int in_raw_mode;		/* For output.c's newline mapping */

extern char quiet;		/* If VERY_QUIET, use visual bell for bell */

/*
 * The capabilities, spelled ANSI.
 */
static char sc_home[] =		"\033[H";
static char sc_addline[] =	"\033[L";
static char sc_clear[] =	"\033[H\033[2J";
static char sc_eol_clear[] =	"\033[K";
static char sc_s_in[] =		"\033[7m";
static char sc_s_out[] =	"\033[0m";
static char sc_u_in[] =		"\033[4m";
static char sc_u_out[] =	"\033[0m";
static char sc_b_in[] =		"\033[1m";
static char sc_b_out[] =	"\033[0m";
static char sc_lower_left[16];	/* built by get_term: row is sc_height */

/*
 * Change terminal to "raw mode", or restore to "normal" mode.
 * Raw mode here means micronix RAW: a read completes on one
 * keystroke, nothing echoes, and nothing on output is cooked -
 * which is why in_raw_mode is public, see the header comment.
 */
	public void
raw_mode(on)
	int on;
{
	struct sgtty s;
	static struct sgtty save_term;

	if (on)
	{
		/*
		 * Get terminal modes, twice: one to keep, one to
		 * change.  The compiler has no struct assignment.
		 */
		gtty(2, &save_term);
		gtty(2, &s);

		erase_char = s.erase;
		kill_char = s.kill;

		s.mode |= RAW;
		s.mode &= ~(ECHO|CRMOD);
		stty(2, &s);
		in_raw_mode = 1;
	} else
	{
		stty(2, &save_term);
		in_raw_mode = 0;
	}
}

/*
 * The alarm that guards the size probe.  It only has to exist: its
 * firing is what breaks the read on a terminal that never answers.
 */
static
probe_alarm()
{
}

/*
 * Ask the terminal how big it is: park the cursor at the far corner
 * - it clamps to the real one - and ask where it landed with the
 * ANSI position report.  The reply, ESC [ rows ; cols R, comes back
 * on the keyboard fd.  Raw for just this exchange, the same
 * in-and-out as the keystroke reads.
 *
 * The alarm is the escape hatch for a terminal that does not speak
 * ANSI and never answers: after a second the read is broken, the
 * parse fails, and the sizes stay 24 by 80.  This works under
 * usersim too - its handlers no longer restart a blocked read, and
 * an interrupted one comes back EINTR the way the machine's would.
 */
	static
probe_size()
{
	struct sgtty osave, s;
	char buf[32];
	char c;
	register int i;
	int rows, cols;

	gtty(2, &osave);
	gtty(2, &s);
	s.mode |= RAW;
	s.mode &= ~(ECHO|CRMOD);
	stty(2, &s);

	puts("\033[9999;9999H\033[6n");
	flush();

	signal(14, probe_alarm);
	alarm(1);
	i = 0;
	while (i < sizeof buf - 1)
	{
		if (read(2, &c, 1) != 1)
			break;
		buf[i++] = c;
		if (c == 'R')
			break;
	}
	alarm(0);
	signal(14, 0);
	buf[i] = '\0';
	stty(2, &osave);

	rows = cols = 0;
	for (i = 0;  buf[i] != '\0' && buf[i] != '[';  i++)
		;
	if (buf[i] == '[')
	{
		i++;
		while (buf[i] >= '0' && buf[i] <= '9')
			rows = rows * 10 + buf[i++] - '0';
		if (buf[i] == ';')
		{
			i++;
			while (buf[i] >= '0' && buf[i] <= '9')
				cols = cols * 10 + buf[i++] - '0';
		}
	}
	/*
	 * Believe an answer that describes a terminal somebody could
	 * be sitting at; anything else keeps the default.
	 */
	if (rows >= 3 && rows < 100 && cols >= 40 && cols <= 200)
	{
		sc_height = rows;
		sc_width = cols;
	}
	puts("\033[H");
	flush();
}

/*
 * The terminal is ANSI, and its size is asked for rather than
 * assumed: 24 by 80 is what a terminal that cannot answer gets.
 * The strings above are the capabilities; the escape sequences
 * print nothing so every width is zero.
 */
	public void
get_term()
{
	char junk[6];

	sc_height = 24;
	sc_width = 80;

	if (gtty(1, junk) == 0 && gtty(2, junk) == 0)
		probe_size();

	if ((sc_window <= 0) || (sc_window >= sc_height))
		sc_window = sc_height-1;

	so_width = se_width = 0;
	ul_width = ue_width = 0;
	bo_width = be_width = 0;

	sprintf(sc_lower_left, "\033[%d;1H", sc_height);
}


/*
 * Below are the functions which perform all the
 * terminal-specific screen manipulation.
 */


/*
 * Initialize terminal
 */
	public void
init()
{
}

/*
 * Deinitialize terminal
 */
	public void
deinit()
{
}

/*
 * Home cursor (move to upper left corner of screen).
 */
	public void
home()
{
	puts(sc_home);
}

/*
 * Add a blank line (called with cursor at home).
 * Should scroll the display down.
 */
	public void
add_line()
{
	puts(sc_addline);
}

/*
 * Move cursor to lower left corner of screen.
 */
	public void
lower_left()
{
	puts(sc_lower_left);
}

/*
 * Ring the terminal bell.
 */
	public void
bell()
{
	if (quiet == VERY_QUIET)
		vbell();
	else
		putc('\7');
}

/*
 * Output the "visual bell", if there is one.  There is not.
 */
	public void
vbell()
{
}

/*
 * Clear the screen.
 */
	public void
clear()
{
	puts(sc_clear);
}

/*
 * Clear from the cursor to the end of the cursor's line.
 * {{ This must not move the cursor. }}
 */
	public void
clear_eol()
{
	puts(sc_eol_clear);
}

/*
 * Begin "standout" (bold, underline, or whatever).
 */
	public void
so_enter()
{
	puts(sc_s_in);
}

/*
 * End "standout".
 */
	public void
so_exit()
{
	puts(sc_s_out);
}

/*
 * Begin "underline" (hopefully real underlining,
 * otherwise whatever the terminal provides).
 */
	public void
ul_enter()
{
	puts(sc_u_in);
}

/*
 * End "underline".
 */
	public void
ul_exit()
{
	puts(sc_u_out);
}

/*
 * Begin "bold"
 */
	public void
bo_enter()
{
	puts(sc_b_in);
}

/*
 * End "bold".
 */
	public void
bo_exit()
{
	puts(sc_b_out);
}

/*
 * Erase the character to the left of the cursor
 * and move the cursor left.
 */
	public void
backspace()
{
	/*
	 * Try to erase the previous character by overstriking with a space.
	 */
	putc('\b');
	putc(' ');
	putc('\b');
}

/*
 * Output a plain backspace, without erasing the previous char.
 */
	public void
putbs()
{
	putc('\b');
}
