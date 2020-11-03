/*
 * commands.c - commands without an address
 *      ... also handles the  :w  command, which can contain an address
 *
 *
 *  Entry point:
 *
 *      int simp_cmd(n, c)
 *      int n;
 *      char c;
 *              If the count n, character c and, in some cases, characters that
 *              follow c constitute one of the commands listed below, then the
 *              command is performed and 1 is returned.  Otherwise, 0 is returned.
 *
 *      The commands recognized by simp_cmd() are:
 *
 *      a       - append characters after the cursor location
 *      A       - append characters at the end of the current line
 *      C       - change the remainder of the line (same as c$)
 *      D       - delete the remainder of the line (same as d$)
 *      i       - insert characters at the cursor location
 *      J       - join two lines together
 *      m       - mark the current position
 *      o       - insert lines below the current line
 *      O       - insert lines above the current line
 *      p       - put the contents of the yank buffer below the current line
 *      P       - put the contents of the yank buffer above the current line
 *      q       - quit; don't save the modifications
 *      r<char> - replace the current character by <char>
 *      s       - substitute for the current character (same as c<space>)
 *      u       - undo the most recent buffer-change command
 *      <n>x    - delete n characters (same as <n>d<space>)
 *      ZZ      - save the modifications and quit
 *      ?       - tell the current position in the file
 *      .       - redo the most recent buffer-change command
 *      *       - iterate the last search-change pair
 *      :e      - edit another file
 *      :r      - read a file; place its contents below the current line
 *      :R      - read a file; place its contents above the current line
 *      :w      - write a range of lines to a file
 *
 */

#include "s.h"

#include <stdio.h>

char cur_file[MAXTEXT] = 0;     /* remembers name of the current file */

int
simp_cmd(n, c)
    int n;
    char c;
{
    int cur_line, cur_pos, i;
    char *t, text1[MAXTEXT], text2[MAXTEXT];

    b_getcur(&cur_line, &cur_pos);
    switch (c) {
    case 'a':
        b_gets(cur_line, text1);
        if (text1[0] != '\0') { /* unless the line is empty */
            b_setcur(cur_line, cur_pos + 1);
            s_refresh();
        }
        do_insert();
        break;
    case 'A':
        b_gets(cur_line, text1);
        b_setcur(cur_line, strlen(text1));
        s_refresh();
        do_insert();
        break;
    case 'C':
        k_donext("c$");
        break;
    case 'D':
        k_donext("d$");
        break;
    case 'i':
        do_insert();
        break;
    case 'J':
        if (cur_line >= b_size()) {
            UNKNOWN;
            break;
        }
        b_gets(cur_line, text1);
        i = strlen(text1);
        /*
         * put a space between the two lines 
         */
        text1[i] = ' ';
        b_gets(cur_line + 1, text2);
        /*
         * strip leading white characters from second line 
         */
        for (t = text2; isspace(*t); ++t);
        if (i + 1 + strlen(t) >= MAXTEXT - 1) {
            s_savemsg("Line length exceeds %d.", MAXTEXT);
            break;
        }
        strcpy(text1 + i + 1, t);
        b_delete(cur_line, cur_line + 1);
        b_insert(cur_line, text1);
        b_setcur(cur_line, i);
        break;
    case 'm':
        b_setmark();
        break;
    case 'o':
        ++cur_line;
        /*
         * no break statement; fall into ... 
         */
    case 'O':
        b_insert(cur_line, "");
        b_setcur(cur_line, 0);
        s_refresh();
        do_insert();
        break;
    case 'p':
        do_put(1);
        break;
    case 'P':
        do_put(-1);
        break;
    case 'q':
        if (b_modified()) {
            s_putmsg("Discard? ");
            if (k_getch() != 'y') {
                UNKNOWN;
                break;
            }
        }
        if (s_ismsg())
            s_refresh();
        b_free();
        k_finish();
        s_finish();
        exit(0);
    case 'r':
        sprintf(text1, "c %c%c", k_getch(), ESCAPE);
        k_donext(text1);        /* c<space><char><esc> */
        break;
    case 's':
        k_donext("c ");         /* c<space> */
        break;
    case 'u':
        undo();
        break;
    case 'x':
        if (n == 0)             /* set default */
            n = 1;
        sprintf(text1, "%dd ", n);
        k_donext(text1);        /* <n>d<space> */
        break;
    case 'Z':
        if (k_getch() != 'Z') {
            UNKNOWN;
            break;
        }
        if (b_modified()) {
            sprintf(text1, ":w%cq", CR);
            k_donext(text1);    /* :w<return>q */
        } else
            k_donext("q");
        break;
    case '?':
        sprintf(text1, "%s: %sline %d of %d", cur_file,
            (b_modified())? "[Modified] " : "", cur_line, b_size());
        s_savemsg(text1, 0);
        break;
    case '.':
        k_redo();
        break;
    case '*':
        do_star();
        break;
    case ':':
        do_io();
        break;
    case '#':
        adjust(n);
        break;
    default:
        return (0);
    }
    return (1);
}

/*
 * Entry point:
 *
 *      do_star() - the star command
 *              From the minimum of the current and marked lines, alternate
 *              between n and redo commands until the maximum of the two lines
 *              is reached.  The marked line defaults to the last buffer line.
 */
#define STAR_MAX  50            /* limits the number of n and redo commands */

do_star()
{
    static int all_lines, doing_star = 0, iterations, start_line = 0,
        start_pos;
    int cur_line, cur_pos, done, mark_line, mark_pos, old_line, old_pos;

    if (!b_modified()) {
        UNKNOWN;
        return;
    }
    if (s_ismsg()) {            /* an error message is waiting; don't
                                 * continue */
        doing_star = 0;
        if (start_line == 0)
            UNKNOWN;
        else
            b_setcur(start_line, start_pos);
        return;
    }
    if (!doing_star) {          /* initialize for this star command */
        doing_star = 1;
        iterations = 0;
        b_getcur(&start_line, &start_pos);
        b_getmark(&mark_line, &mark_pos);
        if (mark_line > 0 && mark_line < start_line) {
            /*
             * guarantee that cursor precedes mark 
             */
            b_setmark();
            b_setcur(mark_line, mark_pos);
            b_getcur(&start_line, &start_pos);
            b_getmark(&mark_line, &mark_pos);
        }
        if (mark_line == 0 || mark_line == b_size() || start_line == b_size())
            all_lines = 1;
        else {
            all_lines = 0;
            /*
             * move mark to the following line 
             */
            b_setcur(mark_line + 1, 0);
            b_setmark();
            b_setcur(start_line, start_pos);
        }
    }
    /*
     * execute an n command 
     */
    b_getcur(&old_line, &old_pos);
    address(0, 'n', ' ');
    b_getcur(&cur_line, &cur_pos);
    b_getmark(&mark_line, &mark_pos);
    /*
     * done if past mark or if the search fails 
     */
    done = (((!all_lines && cur_line >= mark_line) ||
            (cur_line < old_line) ||
            (cur_line == old_line)) && (cur_pos <= old_pos));
    if (!done && ++iterations <= STAR_MAX)
        /*
         * execute a redo command then return to this procedure 
         */
        k_donext(".*");
    else {
        if (done)
            if (iterations == 1)
                s_savemsg("1 change", 0);
            else
                s_savemsg("%d changes", iterations);
        else
            s_savemsg("%d changes; type '*' to continue.", STAR_MAX);
        doing_star = 0;
        b_setcur(start_line, start_pos);
    }
}

/*
 *      do_io();
 *              Read the part of an edit command after ':', then execute it.
 */
do_io()
{
    int cur_line, cur_pos, size;
    char *file, *s_getmsg(), msg[80], *reply;

    /*
     * get the remainder of the command 
     */
    reply = s_getmsg(":");
    b_getcur(&cur_line, &cur_pos);

    /*
     * write commands contain an address; treat them as a special case 
     */
    if (*reply == 'w') {
        do_write(reply + 1);
        b_setcur(cur_line, cur_pos);
        return;
    }

    /*
     * find the start of the file name 
     */
    for (file = reply + 1; *file == ' '; ++file);
    if (*file == '\0')
        file = cur_file;        /* default: use the current file name */
    switch (*reply) {
    case 'e':
        if (b_modified()) {
            s_putmsg("Discard? ");
            if (k_getch() != 'y')
                break;
        }
        if (file != cur_file)
            strcpy(cur_file, file);     /* remember name */
        b_delete(1, b_size());
        b_newcmd();             /* so the :e command cannot be undone */
        if ((size = do_read(file, 1)) > 0) {
            sprintf(msg, "%s: %d lines", file, size);
            s_savemsg(msg, 0);
        } else {
            if (size == -1) {
                sprintf(msg, "%s is a new file", file);
                s_savemsg(msg, 0);
            }
            b_insert(1, "");
            b_setcur(1, 0);
        }
        /*
         * record that buffer contents match external file 
         */
        b_unmod();
        break;
    case 'r':
        ++cur_line;
        /*
         * no break statement; fall into ... 
         */
    case 'R':
        if ((size = do_read(file, cur_line)) > 5)
            s_savemsg("%d lines read", size);
        else if (size == -1) {
            sprintf(msg, "Cannot read %s.", file);
            s_savemsg(msg, 0);
        }
        break;
    default:
        UNKNOWN;
        break;
    }
}

/*
 * do_read - read a file to buffer at the indicated line;
 * a returned value >= 0 tells number of lines that were read from the file
 * a returned value -1 means that the file could not be opened for reading
 * a returned value -2 means that the file contains nonprintable characters
 */
int
do_read(file, line)
    char *file;
    int line;
{
    FILE *fp;
    FILE *fopen();
    int i, c;
    char text[MAXTEXT];

    if ((fp = fopen(file, "r")) == NULL)
        return (-1);
    /*
     * Read the first ten characters and check that they are printable.
     * Some C I/O packages read '\r' (<return>) characters.  Also,
     * nonprintable characters in files might be used for, e.g.,
     * printer control characters.
     */
    for (i = 0; (c = getc(fp)) != EOF && i < 10; ++i)
        if (!isprint(c) && c != '\n' && c != '\t') {
            sprintf(text, "%s is not a text file", file);
            s_savemsg(text, 0);
            fclose(fp);
            return (-2);
        }
    if (i == 0) {
        sprintf(text, "%s is empty", file);
        s_savemsg(text, 0);
        fclose(fp);
        return (0);
    }

    rewind(fp);
    /*
     * copy the file to the buffer 
     */
    for (i = line; fgets(text, MAXTEXT, fp) != NULL; ++i) {
        text[strlen(text) - 1] = '\0';  /* trim off the newline */
        if (b_insert(i, text) == 0)
            break;
    }
    fclose(fp);
    /*
     * move to the first nonwhite character in the first line read 
     */
    b_setline(line);
    return (i - line);
}

/*
 * do_write - write a file according to given specifications
 */
do_write(specs)
    char *specs;
{
    int cur_line, cur_pos, n, new_line, new_pos;
    char *addr, *file;

    /*
     * special case: :w<return> writes buffer to current file 
     */
    if (*specs == '\0') {
        write_lines(1, b_size(), cur_file);
        return;
    }
    /*
     * special case: :w<space>file<return> writes buffer to named file 
     */
    if (*specs == ' ') {
        for (file = specs + 1; *file == ' '; ++file);
        write_lines(1, b_size(), file);
        return;
    }

    /*
     * get the count that follows ":w" 
     */
    n = 0;
    for (addr = specs; isdigit(*addr); ++addr)
        n = 10 * n + *addr - '0';
    if (*addr == '\0') {
        UNKNOWN;
        return;
    }

    /*
     * find and mark the end of the address 
     */
    for (file = addr; *file != '\0' && *file != ' '; ++file);
    if (*file == ' ')
        *file++ = '\0';

    b_getcur(&cur_line, &cur_pos);
    /*
     * push the address, minus the first character, back on the input 
     */
    k_donext(addr + 1);
    address(n, *addr, ':');
    b_getcur(&new_line, &new_pos);
    if (new_line == cur_line && new_pos == cur_pos) {
        /*
         * the address did not make sense 
         */
        UNKNOWN;
        return;
    }

    /*
     * special case: :w<addr><return> writes to the current file 
     */
    if (*file == '\0')
        file = cur_file;

    if (cur_line <= new_line)
        write_lines(cur_line, new_line, file);
    else
        write_lines(new_line, cur_line, file);
}

/*
 * write_lines - write a range of lines to a file
 */
write_lines(from, to, file)
    int from, to;
    char *file;
{
    FILE *fopen(), *fp;
    int count = to - from + 1;
    char text[MAXTEXT - 1];

    /*
     * be cautious about overwriting files 
     */
    if (!strsame(file, cur_file)) {
        if ((fp = fopen(file, "r")) != NULL) {
            s_putmsg("Overwrite? ");
            if (k_getch() == 'y')
                fclose(fp);
            else
                return;
        }
    } else if (from > 1 || to < b_size()) {
        s_putmsg("Write partial buffer to current file? ");
        if (k_getch() != 'y')
            return;
    }

    if ((fp = fopen(file, "w")) == NULL) {
        sprintf(text, "Cannot write %s.", file);
        s_savemsg(text, 0);
        return;
    }
    /*
     * if entire buffer is saved, record that the user is free to quit 
     */
    if (strsame(file, cur_file) && from == 1 && to == b_size())
        b_unmod();
    /*
     * write the lines 
     */
    while (from <= to) {
        b_gets(from++, text);
        fprintf(fp, "%s\n", text);
    }
    fclose(fp);
    sprintf(text, "%s: %d line%s", file, count, (count == 1) ? "" : "s");
    s_savemsg(text, 0);
}
