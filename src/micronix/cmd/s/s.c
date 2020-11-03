/*
 * s - a screen editor
 *
 *
 * Source files:
 *
 *              command handler:
 *      address.c       - process addresses
 *      commands.c      - commands without an address
 *      keyboard.c      - read commands
 *      lib.c           - library of C procedures
 *      operator.c      - operators c, d and y
 *      s.c             - this file; contains the main program
 *      s.h             - macro definitions
 *      yank.c          - the yank buffer
 *
 *              buffer module:
 *      Bman.c          - buffer manager
 *      buffer.c        - data structure for the buffer
 *
 *              screen module:
 *      Sman.c          - screen manager
 *      screen.c        - terminal-specific procedures
 *
 * System Dependencies:
 *
 *      To move this editor to a non-UNIX operating system, the function
 *      k_flip() in file keyboard.c must be changed.  (This functions flips the
 *      terminal driver to and from noecho-raw mode.)
 *
 *      To operate this editor on a non-ANSI standard video terminal, or one
 *      without "autowrap", requires modification of the file screen.c.
 */

#include "s.h"
#include <stdio.h>

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int count, count2, cur_line, cur_pos, new_line, new_pos;
    char c, op, cmd[MAXTEXT];

    if (argc != 2)
        fatal("usage: s file");
    s_init();
    b_init();
    k_init();
    /*
     * do the command: :e<space><file><return> 
     */
    sprintf(cmd, ":e %s%c", argv[1], CR);
    k_donext(cmd);
    for (;;) {                  /* loop over commands */
        /*
         * prepare to get a new command 
         */
        s_refresh();
        k_newcmd();
        c = k_getch();
        count = get_count(&c);
        /*
         * for simple commands, move on to the next command 
         */
        if (simp_cmd(count, c))
            continue;
        /*
         * for c, d or y operators, get the second count 
         */
        if (c == 'c' || c == 'd' || c == 'y') {
            op = c;
            c = k_getch();
            count2 = get_count(&c);
            if (count > 0 && count2 > 0)
                count *= count2;
            else
                count = max(count, count2);
        } else
            op = ' ';
        /*
         * set the buffer's idea of the cursor to the new address 
         */
        b_getcur(&cur_line, &cur_pos);
        address(count, c, op);
        /*
         * check that cursor actually moved 
         */
        b_getcur(&new_line, &new_pos);
        if (cur_line == new_line && cur_pos == new_pos)
            UNKNOWN;
        else if (op != ' ')
            operator(op, cur_line, cur_pos);
    }
    return (0);
}

/*
 * get_count - determine a count in an edit command 
 */
static int
get_count(ch_ptr)
    char *ch_ptr;
{
    int ch = *ch_ptr, count;

    if (isdigit(ch) && ch != '0') {     /* a count cannot start with zero */
        count = ch - '0';
        while (isdigit(ch = k_getch()))
            count = 10 * count + ch - '0';
    } else
        count = 0;
    *ch_ptr = ch;
    return (count);
}
