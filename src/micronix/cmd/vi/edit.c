/*
 * edit commands
 */

#include "stevie.h"

edit()
{
    int c, c1, c2;
    char *p, *q;

    Prenum = 0;

    /*
     * position the display and the cursor at the top of the file. 
     */
    Topchar = Filemem;
    Topline = 1;
    Curschar = Filemem;
    Cursrow = Curscol = 0;

    if (State == INSERT)
        message("Insert");
    else
        message("");

    for (;;) {
        /*
         * Figure out where the cursor is based on Curschar. 
         */
        cursupdate();
        if (State == INSERT)
            message("Insert Mode");
        /*
         * printf("Curschar=(%d,%d) row/col=(%d,%d)",
         * Curschar,*Curschar,Cursrow,Curscol); 
         */
        status();
        windgoto(Cursrow, Curscol);
        windrefresh();
        c = vgetc();
        switch (State) {
        case NORMAL:
            /*
             * We're in the normal (non-insert) mode. 
             * Pick up any leading digits and compute 'Prenum' 
             */
            if ((Prenum > 0 && isdigit(c)) || (isdigit(c) && c != '0')) {
                Prenum = Prenum * 10 + (c - '0');
                break;
            }
            /*
             * execute the command 
             */
            normal(c);
            Prenum = 0;
            break;
        case INSERT:
            /*
             * We're in insert mode. 
             */
            switch (c) {
            case '\033':       /* an ESCape ends input mode */

                /*
                 * If we're past the end of the file, (which should 
                 * only happen when we're editing a new file or a 
                 * file that doesn't have a newline at the end of 
                 * the line), add a newline automatically. 
                 */
                if (Curschar >= Fileend) {
                    insertchar('\n');
                    Curschar--;
                }

                /*
                 * Don't end up on a '\n' if you can help it. 
                 */
                if (Curschar > Filemem && *Curschar == '\n'
                    && *(Curschar - 1) != '\n') {
                    Curschar--;
                }
                State = NORMAL;
                message("");
                Uncurschar = Insstart;
                Undelchars = Ninsert;
                /*
                 * Undobuff[0] = '\0'; 
                 * construct the Redo buffer 
                 */
                p = Redobuff;
                q = Insbuff;
                while (q < Insptr)
                    *p++ = *q++;
                *p++ = '\033';
                *p = '\0';
                updatescreen();
                break;
            case '\177': case '\b':     /* backspace and DEL */
                if (Curschar <= Insstart)
                    beep();
                else {
                    int wasnewline = 0;

                    if (*Curschar == '\n')
                        wasnewline = 1;
                    Curschar--;
                    delchar();
                    Insptr--;
                    Ninsert--;
                    if (wasnewline)
                        Curschar++;
                    cursupdate();
                    updatescreen();
                }
                break;
            case '\r':                  /* CR becomes NL */
                c = '\n';
                /* fall through */
            default:
                insertchar(c);
                break;
            }
            break;
        }
    }
}

insertchar(c)
    int c;
{
    char *p;

    if (!anyinput()) {
        inschar(c);
        *Insptr++ = c;
        Ninsert++;
    } else {
        /*
         * If there's any pending input, grab 
         * it all at once. 
         */
        p = Insptr;
        *Insptr++ = c;
        Ninsert++;
        while ((c = vpeekc()) != '\033') {
            c = vgetc();
            *Insptr++ = c;
            Ninsert++;
        }
        *Insptr = '\0';
        insstr(p);
    }
    updatescreen();
}

getout()
{
    windgoto(Rows - 1, 0);
    windrefresh();
    write(1, "\r\n", 2);
    windexit(0);
}

cursupdate()
{
    char *p;
    int inc, c, nlines;
    extern int Tabstop;

    /*
     * special case: file is completely empty 
     */
    if (Fileend == Filemem) {
        Topchar = Curschar = Filemem;
        Topline = 1;
    } else if (Curschar < Topchar) {
        nlines = cntlines(Curschar, Topchar);
        Topline -= nlines;
        /*
         * if the cursor is above the top of 
         * the screen, put it at the top of the screen.. 
         */
        Topchar = Curschar;
        /*
         * ... and, if we weren't very close to begin with, 
         * we scroll so that the line is close to the middle. 
         */
        if (nlines > Rows / 3)
            scrolldown(Rows / 3);
        else {
            /*
             * make sure we have the current line completely 
             * on the screen, by setting Topchar to the 
             * beginning of the current line (in a strange way). 
             */
            if ((p = prevline(Topchar)) != NULL && (p = nextline(p)) != NULL) {
                Topchar = p;
            }
        }
        updatescreen();
    } else if (Curschar >= Botchar && Curschar < Fileend) {
        nlines = cntlines(Botchar, Curschar);
        /*
         * If the cursor is off the bottom of the screen, 
         * put it at the top of the screen.. 
         */
        Topchar = Curschar;
        Topline += nlines;
        /*
         * ... and back up 
         */
        if (nlines > Rows / 3)
            scrolldown((2 * Rows) / 3);
        else
            scrolldown(Rows - 2);
        updatescreen();
    }

    Cursrow = Curscol = Cursvcol = 0;
    for (p = Topchar; p < Curschar; p++) {
        c = *p & 0xff;
        if (c == '\n') {
            Cursrow++;
            Curscol = Cursvcol = 0;
            continue;
        }
        /*
         * A tab gets expanded, depending on the current column 
         * the width of a character representation
         * tab gets expanded, ^A - ^Z, and \ooo for everying else odd
         */
        if (c == '\t')
            inc = (Tabstop - (Curscol) % Tabstop);
        else if (c < 27)
            inc = 2;
        else if ((c < ' ') || (c > 0x7e))
            inc = 4;
        else
            inc = 1;
        Curscol += inc;
        Cursvcol += inc;
        if (Curscol >= Columns) {
            Curscol -= Columns;
            Cursrow++;
        }
    }
}

scrolldown(nlines)
    int nlines;
{
    int n;
    char *p;

    /*
     * Scroll up 'nlines' lines. 
     */
    for (n = nlines; n > 0; n--) {
        if ((p = prevline(Topchar)) == NULL)
            break;
        Topchar = p;
        Topline--;
    }
}

/*
 * oneright
 * oneleft
 * onedown
 * oneup
 *
 * Move one char {right,left,down,up}.  Return 1 when
 * sucessful, 0 when we hit a boundary (of a line, or the file).
 */

oneright()
{
    char *p;

    p = Curschar;
    if ((*p++) == '\n' || p >= Fileend || *p == '\n')
        return (0);
    Curschar++;
    return (1);
}

oneleft()
{
    char *p;

    p = Curschar;
    if (*p == '\n' || p == Filemem || *(p - 1) == '\n')
        return (0);
    Curschar--;
    return (1);
}

beginline()
{
    while (oneleft());
}

/*
 * go up n blocks
 */
upblock(n)
{
    char *p;

    if (n == 0) n = 1;
    p = Curschar;

    while (n--) {
        while ((p = prevline(p)) != NULL) {
            if (*p == '\n') break;
        }
        if (p) {
            Curschar = p;
        } else {
            Curschar = Filemem;
            break;
        }
    }
}

oneup(n)
{
    char *p, *np;
    int savevcol, k;

    savevcol = Cursvcol;
    p = Curschar;
    for (k = 0; k < n; k++) {
        /*
         * Look for the previous line 
         */
        if ((np = prevline(p)) == NULL) {
            /*
             * If we've at least backed up a little .. 
             */
            if (k > 0)
                break;          /* to update the cursor, etc. */
            else
                return (0);
        }
        p = np;
    }
    Curschar = p;
    /*
     * This makes sure Topchar gets updated so the complete line 
     * is one the screen. 
     */
    cursupdate();
    /*
     * try to advance to the same (virtual) column 
     * that we were at before. 
     */
    Curschar = coladvance(p, savevcol);
    return (1);
}

/*
 * go down n blocks
 */
downblock(n)
{
    char *p;

    if (n == 0) n = 1;
    p = Curschar;

    while (n--) {
        while ((p = nextline(p)) != NULL) {
            if (*p == '\n') break;
        }
        if (p) {
            Curschar = p;
        } else {
            Curschar = Fileend - 1;
            break;
        }
    }
}

onedown(n)
{
    char *p, *np;
    int k;

    p = Curschar;
    for (k = 0; k < n; k++) {
        /*
         * Look for the next line 
         */
        if ((np = nextline(p)) == NULL) {
            if (k > 0)
                break;
            else
                return (0);
        }
        p = np;
    }
    /*
     * try to advance to the same (virtual) column 
     * that we were at before. 
     */
    Curschar = coladvance(p, Cursvcol);
    return (1);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */

