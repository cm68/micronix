/*
 * this file contains all the terminal specific things needed to render
 * and manage the screen
 */

#include "stevie.h"

#ifdef linux
#include <termios.h>
struct termios save_term;
#else
#ifdef CPM
#else
#include <types.h>
#include <sys/sgtty.h>
struct sgtty save_sbuf INIT;
#endif
#endif

char controlbuf[30] INIT;

extern int Columns;
extern int Rows;

windinit()
{
    int i;
    int j;

    /*
     * Initialise tty 
     * Here we want no echo, disable line buffering / erase kill and no
     * newline - curses noecho(), cbreak(), nonl() 
     */
#ifdef linux
    struct termios new_term;
    tcgetattr(0, &save_term);
    tcgetattr(0, &new_term);
    cfmakeraw(&new_term);
    tcsetattr(0, TCSANOW, &new_term);
#else
    struct sgtty sbuf;

    gtty(0, &save_sbuf);
    gtty(0, &sbuf);

    sbuf.mode |= RAW;
	sbuf.mode &= ~ECHO;

    stty(0, &sbuf);
#endif


	/*
	 * get rows and columns from the terminal:
	 * go the the right bottom by sending absurd coordinates
	 * and then asking where the cursor is
	 */
	write(1, "\033[6l", 4);             /* set absolute addressing */
    write(1, "\033[999;999H", 10);      /* send to extreme corner */
    write(1, "\033[6n", 4);             /* we're the hakawi */

    /*
     * read the current actual cursor position
	 * 	ESC [ NN ; YY R
     */
    for (i = 0;; i++) {
        read(0, &controlbuf[i], 1);
        if (controlbuf[i] == 'R') {
            break;
        }
    }
#ifdef notdef
    controlbuf[++i] = '\0';
    controlbuf[0] = 'E';
    logmsg(controlbuf);
#endif

    for (i = 2; controlbuf[i] != ';'; i++);

    Rows = atoi(&controlbuf[2]);
    Columns = atoi(&controlbuf[i+1]);

#ifdef notdef
    sprintf(controlbuf, "parsed terminal size: %d %d\n", Rows, Columns);
    logmsg(controlbuf);
#endif

    if (Columns < 60 || Columns > 200)
        Columns = 80;
    if (Rows < 16 || Rows > 80)
        Rows = 24;

    if (Rows > MAXROWS) Rows = MAXROWS;
    if (Columns > MAXCOLS) Columns = MAXCOLS;

#ifdef notdef
    sprintf(controlbuf, "set terminal size: %d %d\n", Rows, Columns);
    logmsg(controlbuf);
#endif
}

/*
 * absolute cursor position
 */
windgoto(r, c)
    int r, c;
{
    sprintf(controlbuf, "\033[%d;%dH", r+1, c+1);
	write(1, controlbuf, strlen(controlbuf));
}

windexit(r)
    int r;
{
#ifdef linux
    tcsetattr(0, TCSANOW, &save_term);
#else
    stty(0, &save_sbuf);
#endif
    exit(r);
}

/*
 * Clear the screen, also:
 *  set the origin mode
 *  go home
 *  the scrolling region
 */
windclear()
{
    sprintf(controlbuf, "\033[6l\033[2J\033[H\033[1;%dr", Rows);
    write(1, controlbuf, strlen(controlbuf));
}

windgetc()
{
    read(0, controlbuf, 1);
    return controlbuf[0];
}

windstr(s)
    char *s;
{
	write(1, s, strlen(s));
}

windputc(c)
    int c;
{
	write(1, &c, 1);
}

windrefresh()
{
    /*
     * Need a redraw here? 
     */
}

beep()
{
	write(1, "\007", 1);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */

