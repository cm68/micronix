/*
 * STEVIE - ST Editor for VI Enthusiasts   ...Tim Thompson...twitch!tjt...
 */

#include "stevie.h"
#ifdef linux
#include <sys/stat.h>
#else
#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>
#endif

/*
 * set by calling windinit
 */
int Rows INIT;
int Columns INIT;

/*
 * What's currently on the screen, a single
 * array of size Rows*Columns. 
 */
char *Realscreen INIT;

/*
 * What's to be put on the screen. 
 */
char *Nextscreen INIT;

/*
 * Current file name 
 */
char *Filename INIT;

/*
 * The contents of the file, as a single array. 
 */
char *Filemem INIT;

/*
 * Pointer to the end of allocated space for
 * Filemem. (It points to the first byte AFTER the allocated space.) 
 */
char *Filemax INIT;

/*
 * Pointer to the end of the file in Filemem. 
 * (It points to the byte AFTER the last byte.) 
 */
char *Fileend INIT;

/*
 * Pointer to the byte in Filemem which is
 * in the upper left corner of the screen. 
 */
char *Topchar INIT;

/*
 * the line number corresponding to Topchar
 */
int Topline INIT;

/*
 * Pointer to the byte in Filemem which is
 * just off the bottom of the screen. 
 */
char *Botchar INIT;

/*
 * Pointer to byte in Filemem at which the
 * cursor is currently placed. 
 */
char *Curschar INIT;

/*
 * Current position of cursor
 */
int Cursrow INIT;
int Curscol INIT;

/*
 * Current virtual column, the column number of
 * the file's actual line, as opposed to the 
 * column number we're at on the screen.  This 
 * makes a difference on lines that span more 
 * than one screen line. 
 */
int Cursvcol INIT;

/*
 * This is the current state of the command interpreter.
 */
int State = NORMAL;

/*
 * The (optional) number before a command. 
 */
int Prenum INIT;

/*
 * This is where the latest insert/append mode started.  
 */
char *Insstart INIT;

/*
 * Set to 1 if something in the file has been 
 * changed and not written out. 
 */
int Changed INIT;

int Debug INIT;

int Tabstop INIT;

/*
 * Each command should stuff characters into this
 * buffer that will re-execute itself. 
 */
char Redobuff[1024] INIT;

/*
 * Each command should stuff characters into this buffer that will undo its
 * effects. 
 */
char Undobuff[1024] INIT;

/*
 * Each insertion gets stuffed into this buffer. 
 */
char Insbuff[1024] INIT;

/*
 * Curschar is restored to this before undoing. 
 */
char *Uncurschar = NULL;

/*
 * Number of characters in the current insertion. 
 */
int Ninsert INIT;

/*
 * Number of characters to delete, when undoing. 
 */
int Undelchars INIT;

char *Insptr = NULL;

main(argc, argv)
    int argc;
    char **argv;
{
	unlink("logfile");
    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {
        case 'd':
            Debug = 1;
            break;
        }
        argc--;
        argv++;
    }

    if (argc <= 1) {
        write(2, "usage: stevie {file}\n", 21);
        exit(1);
    }

    Filename = strdup(argv[1]);
    
    statinit();
    windinit();

    /*
     * Make sure Rows/Columns are big enough 
     */
    if (Rows < 3 || Columns < 16) {
        write(2, "terminal size error\n", 20); 
        windexit(2);
    }

    Tabstop = 8;

    screenalloc();
    filealloc();

    screenclear();

    Fileend = Filemem;
    if (readfile(Filename, Fileend, 0))
        filemess("[New File]");
    Topchar = Curschar = Filemem;

    updatescreen();

    edit();

    windexit(0);
}

isspace(c)
char c;
{
    return ((c == ' ') || (c == '\t'));
}

isdigit(c)
char c;
{
    return ((c >= '0') && (c <= '9'));
}

/*
 * filetonext()
 *
 * Based on the current value of Topchar, transfer a screenfull of
 * stuff from Filemem to Nextscreen, and update Botchar.
 */
filetonext()
{
    int row, col;
    char *screenp = Nextscreen;
    char *memp = Topchar;
    char *endscreen;
    char *nextrow;
    char extra[16];
    int nextra = 0;
    int c;
    int n;

    /*
     * The number of rows shown is Rows-1. 
     * The last line is the status/command line. 
     */
    endscreen = &screenp[(Rows - 1) * Columns];

    row = col = 0;
    while (screenp < endscreen && memp < Fileend) {

        /*
         * Get the next character to put on the screen. 
         * The 'extra' array contains the extra stuff that is 
         * inserted to represent special characters (tabs, and 
         * other non-printable stuff.  The order in the 'extra' 
         * array is reversed. 
         */

        if (nextra > 0) {
            c = extra[--nextra];
        } else {
            c = (unsigned) (0xff & (*memp++));
        }

        /*
         * when getting a character from the file, we 
         * may have to turn it into something else on 
         * the way to putting it into 'Nextscreen'. 
         */
        if (c == '\t') {
            strcpy(extra, "        ");
            /*
            * tab amount depends on current column 
            */
            nextra = ((Tabstop - 1) - col % Tabstop);
            c = ' ';
        } else if (c == '\n') {
            row++;
            /*
             * get pointer to start of next row 
             */
            nextrow = &Nextscreen[row * Columns];
            /*
             * blank out the rest of this row 
             */
            while (screenp != nextrow)
                *screenp++ = ' ';
            col = 0;
            continue;
        } else if (c < 27) {
            extra[0] = c + '@';
            c = '^';
            nextra = 1;
        } else if ((c < ' ') || (c > 0x7e)) {
            unsigned char v = c;
            c = '\\';

            for (nextra = 0; nextra < 3; nextra++) {
                extra[nextra] = (v & 7) + '0';
                v >>= 3;
            }
        }

        /*
         * store the character in Nextscreen 
         */
        if (col >= Columns) {
            row++;
            col = 0;
        }
        *screenp++ = c;
        col++;
    }

    /*
     * make sure the rest of the screen is blank 
     */
    while (screenp < endscreen)
        *screenp++ = ' ';

    /*
     * put '~'s on rows that aren't part of the file. 
     */
    if (col != 0)
        row++;
    while (row < Rows) {
        Nextscreen[row * Columns] = '~';
        row++;
    }
    Botchar = memp;
}

/*
 * nexttoscreen
 *
 * Transfer the contents of Nextscreen to the screen, using Realscreen
 * to avoid unnecessary output.
 */

nexttoscreen()
{
    char *np = Nextscreen;
    char *rp = Realscreen;
    char *endscreen;
    char nc;
    int row = 0, col = 0;
    int gorow = -1, gocol = -1;

    endscreen = &np[(Rows - 1) * Columns];

    for (; np < endscreen; np++, rp++) {
        /*
         * If desired screen (contents of Nextscreen) does not 
         * match what's really there, put it there. 
         */
        if ((nc = (*np)) != (*rp)) {
            *rp = nc;
            /*
             * if we are positioned at the right place, 
             * we don't have to use windgoto(). 
             */
            if (!(gorow == row && gocol == col))
                windgoto(gorow = row, gocol = col);
            windputc(nc);
            gocol++;
        }
        if (++col >= Columns) {
            col = 0;
            row++;
        }
    }
    windrefresh();
}

updatescreen()
{
    filetonext();
    nexttoscreen();
}

screenclear()
{
    int n;

    windclear();
    /*
     * blank out the stored screens 
     */
    for (n = Rows * Columns - 1; n >= 0; n--) {
        Realscreen[n] = ' ';
        Nextscreen[n] = ' ';
    }
}

filealloc()
{
    if ((Filemem = malloc((unsigned) FILELENG)) == NULL) {
        write(2, "file memory allocation failure\n", 32);
        windexit(1);
    }
    Filemax = Filemem + FILELENG;
}

screenalloc()
{
    Realscreen = malloc((unsigned) (Rows * Columns));
    Nextscreen = malloc((unsigned) (Rows * Columns));
}

readfile(fname, fromp, nochangename)
    char *fname;
    char *fromp;
    int nochangename;           /* if 1, don't change the Filename */
{
    int fd;
    char *p;
    int c, n;
    int unprint = 0;
    struct stat sbuf;
    int size;

    sprintf(msgbuf, "Reading %s...", fname);
    message(msgbuf);

    if (!nochangename)
        Filename = strdup(fname);

    if (stat(fname, &sbuf) != 0) {
        Fileend = Filemem;
        return (1);
    }

#ifdef linux
    size = sbuf.st_size;
#else
    size = sbuf.d.size1 + (sbuf.d.size0 << 16);
#endif
    if (size + fromp > Filemax) {
        sprintf(msgbuf, "cannot insert file (limit is %d)!\n", FILELENG);
        write(2, msgbuf, strlen(msgbuf));
        windexit(1);
    }

    if ((fd = open(fname, 0)) < 0) {
        Fileend = Filemem;
        return (1);
    }

    /*
     * let's make a hole
     */
#ifdef notdef
    sprintf(msgbuf, "space: %d used: %d offset: %d size: %d",
        Filemax - Filemem, Fileend - Filemem, fromp - Filemem, size);
    logmsg(msgbuf);
#endif

    Fileend = Fileend + size;
    p = Fileend;
    for (n = size; n; n--) {
        if (p - size == Filemem)
            break;
        *p = *(p - size);
        p--;
    }
    c = read(fd, fromp, size);

    sprintf(msgbuf, "\"%s\" %d characters", fname, size);
    message(msgbuf);
    close(fd);
    return (0);
}

/*
 * this buffer is where characters are inserted by the editor to
 * be read from by the command input loop.   so if, for example,
 * I issue the command cwfoo<esc>, this gets jammed into the redo buffer.
 * and then advance my search to the next word, and type ., the redo
 * buffer contents get shoved into getcbuff.
 * when vgetc gets called, this command is repeated as if I typed it.
 */
static char getcbuff[1024] INIT;
static char *getcnext = getcbuff;

/*
 * append s to getcbuff.  
 * if getcnext is not set, set it to the buffer start
 */
stuffin(s)
    char *s;
{
    strcat(getcbuff, s);
}

/*
 * rather lame but useful character appender.
 * sort of varargs-ish.  stop as soon as we see a null.
 */
addtobuff(s, c1, c2, c3, c4, c5, c6)
    char *s;
    char c1, c2, c3, c4, c5, c6;
{
    if ((*s++ = c1) == '\0')
        return;
    if ((*s++ = c2) == '\0')
        return;
    if ((*s++ = c3) == '\0')
        return;
    if ((*s++ = c4) == '\0')
        return;
    if ((*s++ = c5) == '\0')
        return;
    if ((*s++ = c6) == '\0')
        return;
}

/*
 * this is the input getter.  empty getcnext first, then go to
 * read the keyboard.
 */
vgetc()
{
    char nc = *getcnext;

    if (nc == '\0') {
        return (windgetc());
    }
    if (*++getcnext == '\0') {
        getcnext = getcbuff;
        getcbuff[0] = '\0';
    }
    return (nc);
}

vpeekc()
{
    char nc = *getcnext;

    if (nc)
        return (nc);

    return (-1);
}

/*
 * anyinput
 *
 * Return non-zero if input is pending.
 */

anyinput()
{
    if (*getcnext)
        return (1);
    return (0);
}

#ifdef linux
logmsg(s)
char *s;
{
        int fd;

    if ((fd = open("logfile", 1)) < 0) {
        fd = creat("logfile", 0777);
    }
    if (fd >= 0) {
        lseek(fd, 0, 2);
        write(fd, s, strlen(s));
        write(fd, "\n", 1);
        close(fd);
    }
}
#endif

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
