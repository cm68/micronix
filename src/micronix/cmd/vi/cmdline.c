/*
 * STevie - ST editor for VI enthusiasts.   ...Tim Thompson...twitch!tjt...
 */

#include "stevie.h"

char cmdbuf[100] INIT;
char msgbuf[80] INIT;

readcmdline(firstc)
    int firstc;                 /* either ':', '/', or '?' */
{
    int c;
    char *p, *cmd;

    gotocmd(1, 1, firstc);
    cmd = cmdbuf;
    if (firstc != ':') {
        *cmd++ = firstc;
        *cmd = '\0';
    }

    /*
     * collect the command string, handling editing
     */
    for (;;) {
        c = vgetc();

        /* end of command */
        if (c == '\n' || c == '\r')
            break;

        /* backspace or DEL */
        if (c == '\b' || c == 0x7f) {
            if (cmd <= cmdbuf)
                continue;

            cmd--;
            /*
             * I know this is gross, but it has the 
             * advantage of relying only on 'gotocmd' 
             */
            gotocmd(1, 0, firstc == ':' ? ':' : 0);
            for (p = cmdbuf; p < cmd; p++)
                windputc(*p);
            windrefresh();
            continue;
        }

        /* line kill */
        if (c == CONTROL('U')) {
            cmd = cmdbuf;
            gotocmd(1, 1, firstc);
            continue;
        }
        windputc(c);
        windrefresh();
        *cmd++ = c;
        *cmd = '\0';
    }
    docmdline(cmdbuf);
}

/*
 * broken out as a function so we can run commands from strings
 * and from the command line
 */
docmdline(cmd)
char *cmd;
{
    extern int Tabstop;
    char *s;
    char c;
    int i;
    char *arg;

    /*
     * skip any initial white space 
     */
    while (isspace(*cmd)) {
        cmd++;
    }

    c = *cmd;

    /*
     * search commands 
     */
    if (c == '/' || c == '?') {
        cmd++;
        /*
         * the command was '//' or '??' 
         */
        if (*cmd == c) {
            repsearch();
            return;
        }
        /*
         * If there is a matching '/' or '?' at the end, toss it 
         */
        i = strlen(cmd);
        if (i && (cmd[i - 1] == c)) {
            cmd[i - 1] = '\0';
        }
        dosearch(c == '/' ? FORWARD : BACKWARD, cmd);
        return;
    }

    /* skip over non-white space */
    for (arg = cmd; *arg != '\0' && !isspace(*arg); arg++)
        ;

    /* if there is any arg, find its start */
    if (*arg) {
        *arg++ = '\0';

        /* skip over white space */
        while (*arg != '\0' && isspace(*arg))
            arg++;
    }

    /*
     * at this point, cmd is a null-terminated string, and arg is also.
     * both do not start with white space. *arg may be null.
     */
    if (strcmp(cmd, "q!") == 0) {
        getout();
    }

    if (strcmp(cmd, "q") == 0) {
        if (Changed)
            message("File not written out.  Use 'q!' to override.");
        else
            getout();
        return;
    }

    if (strcmp(cmd, "w") == 0) {
        if (!*arg) {
            writeit(Filename);
            Changed = 0;
        } else {
            writeit(arg);
        }
        return;
    }

    if (strcmp(cmd, "wq") == 0) {
        if (writeit(Filename))
            getout();
        return;
    }

    if (strcmp(cmd, "f") == 0) {
        if (!*arg) {
            Filename = strdup(arg);
            filemess("");
        } else {
            fileinfo();
        }
        return;
    }

    if (strcmp(cmd, "e") == 0 || strcmp(cmd, "e!") == 0) {
        if (cmd[1] != '!' && Changed) {
            message("File not written out.  Use 'e!' to override.");
        } else {
            if (*arg) {
                free(Filename);
                Filename = strdup(arg);
            }
            /*
             * clear mem and read file 
             */
            Fileend = Topchar = Curschar = Filemem;
            Changed = 0;
            nextline(Curschar);
            readfile(Filename, Fileend, 0);
            updatescreen();
        }
        return;
    }

    if (strcmp(cmd, "r") == 0 || strcmp(cmd, ".r") == 0) {
        if (!*arg) {
            badcmd();
            return;
        }
        /* go to the next line and read file in there */
        readfile(arg, nextline(Curschar), 1);
        updatescreen();
        Changed = 1;
        return;
    }

    if (strcmp(cmd, ".=") == 0) {
        sprintf(msgbuf, "line %d   character %d",
            cntlines(Filemem, Curschar), 1 + (int) (Curschar - Filemem));
        message(msgbuf);
        return;
    }

    if (strcmp(cmd, "$=") == 0) {
        sprintf(msgbuf, "%d", cntlines(Filemem, Fileend) - 1);
        message(msgbuf);
        return;
    }

    if (strcmp(cmd, "set") == 0) {
        if (!*arg) {
            badcmd();
            return;
        }

        if (strncmp(arg, "tabstop", 7) == 0) {
            if (arg[7] == '=') {
                Tabstop = atoi(&arg[8]);
                if (Tabstop > 8 || Tabstop < 1) {
                    Tabstop = 8;
                }
                updatescreen();
            } else {
                sprintf(msgbuf, "tabstop=%d", Tabstop);
                message(msgbuf);
            }
            return;
        }
        badcmd();
        return;
    }
    badcmd();
}

badcmd()
{
    message("Unrecognized command");
}

/*
 * these functions are used to update the message line, which
 * i've defined as a command box, which is also used for transient
 * message text, and a status region.
 *
 * the status region:
 * L = 4 digits of line #, 
 * C = 3 digits of column
 * P = a file percentage or Top/Bot
 * S = 4 digits of mem space left
 * LLLL CCC SSSS PPP
 *
 */
struct statline {
    char lineno[4];
    char pad0;
    char colno[3];
    char pad1;
    char spacebytes[5];
    char pad2;
    char position[3];
    char end;
} statline INIT;

statinit()
{
    statline.pad0 = statline.pad1 = statline.pad2 = ' ';
    statline.end = '\0';
}

#ifdef linux
#define cpybuf memcpy
#endif

ifmt(s, scale, val)
char *s;
int scale;
int val;
{
    int d;

    do {
        d = val / scale;
        val -= d * scale;
        *s++ = d + '0';
        scale /= 10;
    } while (scale);
}

status()
{
    int i;

    int freespace = Filemax - Fileend;
    int totalspace = Filemax - Filemem;
    int usedspace = Fileend - Filemem;
    int cursoff = Curschar - Filemem;

    if (Curschar == Filemem) {
        cpybuf(statline.position, "Top", 3);
    } else if (Curschar == (Fileend - 1)) {
        cpybuf(statline.position, "Bot", 3);
    } else if (usedspace == 0) {
        cpybuf(statline.position, "Emp", 3);
    } else {
        i = ((Curschar - Filemem) * 100L) / usedspace;
        cpybuf(statline.position, "00%", 3);
        ifmt(statline.position, 10, i);
    }
    ifmt(statline.lineno, 1000, Topline);
    ifmt(statline.colno, 100, Cursvcol);
    ifmt(statline.spacebytes, 10000, freespace);
    windgoto(Rows - 1, Columns - 19);
    windstr(&statline);
}

gotocmd(clr, fresh, firstc)
{
    int n;

    windgoto(Rows - 1, 0);
    if (clr) {
        /*
         * clear the line 
         */
        for (n = 0; n < (Columns - 1); n++)
            windputc(' ');
        windgoto(Rows - 1, 0);
    }
    if (firstc)
        windputc(firstc);
    if (fresh)
        windrefresh();
}

message(s)
    char *s;
{
    static char *lastmess = NULL;
    char *p;

    if (lastmess != NULL) {
        if (strcmp(lastmess, s) == 0)
            return;
        free(lastmess);
    }
    gotocmd(1, 1, 0);
    /*
     * take off any trailing newline 
     */
    if ((p = strchr(s, '\0')) != NULL && *p == '\n')
        *p = '\0';
    windstr(s);
    status();
    lastmess = strdup(s);
#ifdef notdef
    logmsg(s);
#endif
}

writeit(fname)
    char *fname;
{
    int fd;
    char buff[128];
    char *p;
    int n;
    int w;

    sprintf(buff, "Writing %s...", fname);
    message(buff);

	if ((fd = creat(fname, 0777)) < 0) {
		message("Unable to create file!");
        return (0);
    }

    n = Fileend - Filemem;
    w = write(fd, Filemem, n);
    sprintf(buff, "\"%s\" %d characters", fname, n);
    message(buff);
    close(fd);
    if (w != n) {
		sprintf(buff, "write lose %d %d", w, n);
        logmsg(buff);
    }
    Changed = 0;
    return (1);
}

filemess(s)
    char *s;
{
    char buff[128];

    sprintf(buff, "\"%s\" %s", Filename, s);
    message(buff);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
