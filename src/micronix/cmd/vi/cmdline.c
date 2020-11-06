/*
 * STevie - ST editor for VI enthusiasts.   ...Tim Thompson...twitch!tjt...
 */

#include "stevie.h"

readcmdline(firstc)
    int firstc;                 /* either ':', '/', or '?' */
{
    int c;
    char buff[100];
    char *p, *q, *cmd, *arg;
    extern int Tabstop;

    gotocmd(1, 1, firstc);
    p = buff;
    if (firstc != ':')
        *p++ = firstc;
    /*
     * collect the command string, handling '\b' and @ 
     */
    for (;;) {
        c = vgetc();
        if (c == '\n' || c == '\r')
            break;
        if (c == '\b') {
            if (p > buff) {
                p--;
                /*
                 * I know this is gross, but it has the 
                 * advantage of relying only on 'gotocmd' 
                 */
                gotocmd(1, 0, firstc == ':' ? ':' : 0);
                for (q = buff; q < p; q++)
                    windputc(*q);
                windrefresh();
            }
            continue;
        }
        if (c == '@') {
            p = buff;
            gotocmd(1, 1, firstc);
            continue;
        }
        windputc(c);
        windrefresh();
        *p++ = c;
    }
    *p = '\0';

    /*
     * skip any initial white space 
     */
    for (cmd = buff; isspace(*cmd); cmd++);

    /*
     * search commands 
     */
    c = *cmd;
    if (c == '/' || c == '?') {
        cmd++;
        if (*cmd == c) {
            /*
             * the command was '//' or '??' 
             */
            repsearch();
            return;
        }
        /*
         * If there is a matching '/' or '?' at the end, toss it 
         */
        p = strchr(cmd, '\0');
        if (*(--p) == c)
            *p = '\0';
        dosearch(c == '/' ? FORWARD : BACKWARD, cmd);
        return;
    }

    /*
     * isolate the command and find any argument 
     */
    for (p = cmd; *p != '\0' && !isspace(*p); p++);
    if (*p == '\0')
        arg = NULL;
    else {
        *p = '\0';
        while (*(++p) != '\0' && isspace(*p));
        arg = p;
        if (*arg == '\0')
            arg = NULL;
    }
    if (strcmp(cmd, "q!") == 0)
        getout();
    if (strcmp(cmd, "q") == 0) {
        if (Changed)
            message("File not written out.  Use 'q!' to override.");
        else
            getout();
        return;
    }
    if (strcmp(cmd, "w") == 0) {
        if (arg == NULL) {
            writeit(Filename);
            Changed = 0;
        } else
            writeit(arg);
        return;
    }
    if (strcmp(cmd, "wq") == 0) {
        if (writeit(Filename))
            getout();
        return;
    }
    if (strcmp(cmd, "f") == 0 && arg == NULL) {
        fileinfo();
        return;
    }
    if (strcmp(cmd, "e") == 0 || strcmp(cmd, "e!") == 0) {
        if (cmd[1] != '!' && Changed) {
            message("File not written out.  Use 'e!' to override.");
        } else {
            if (arg != NULL)
                Filename = strdup(arg);
            /*
             * clear mem and read file 
             */
            Fileend = Topchar = Curschar = Filemem;
            Changed = 0;
            p = nextline(Curschar);
            readfile(Filename, Fileend, 0);
            updatescreen();
        }
        return;
    }
    if (strcmp(cmd, "f") == 0) {
        Filename = strdup(arg);
        filemess("");
        return;
    }
    if (strcmp(cmd, "r") == 0 || strcmp(cmd, ".r") == 0) {
        char *pp;

        if (arg == NULL) {
            badcmd();
            return;
        }
        /*
         * find the beginning of the next line and 
         */
        /*
         * read file in there 
         */
        pp = nextline(Curschar);
        readfile(arg, pp, 1);
        updatescreen();
        Changed = 1;
        return;
    }
    if (strcmp(cmd, ".=") == 0) {
        char messbuff[80];

        sprintf(messbuff, "line %d   character %d",
            cntlines(Filemem, Curschar), 1 + (int) (Curschar - Filemem));
        message(messbuff);
        return;
    }
    if (strcmp(cmd, "$=") == 0) {
        char messbuff[8];

        sprintf(messbuff, "%d", cntlines(Filemem, Fileend) - 1);
        message(messbuff);
        return;
    }
    if (strcmp(cmd, "set") == 0) {
        if (arg == NULL) {
            badcmd();
        } else if (strcmp(arg, "tab8") == 0) {
            Tabstop = 8;
            updatescreen();
        } else if (strcmp(arg, "tab4") == 0) {
            Tabstop = 4;
            updatescreen();
        } else {
            badcmd();
        }
        return;
    }
    badcmd();
}

badcmd()
{
    message("Unrecognized command");
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
    lastmess = strdup(s);
    logmsg(s);
}

writeit(fname)
    char *fname;
{
    int fd;
    char buff[128];
    char *p;
    int n;

    sprintf(buff, "Writing %s...", fname);
    message(buff);

    if ((fd = open(fname, 1)) < 0) {
        if ((fd = creat(fname, 0777)) < 0) {
            message("Unable to open file!");
            return (0);
        }
    }

    n = Fileend - Filemem;
    write(fd, Filemem, n);
    sprintf(buff, "\"%s\" %d characters", fname, n);
    message(buff);
    close(fd);

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

