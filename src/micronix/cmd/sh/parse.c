/*
 * parse.c - NOT THE PARSER
 *
 * micronix/cmd/sh/parse.c
 *
 * The real parser has not been written.  This is a placeholder that
 * splits a line on whitespace and REFUSES anything else, so that the
 * rest of the shell can be built and exercised without this quietly
 * getting a redirection or a pipeline wrong.  Anything it cannot
 * honestly handle is an error, not a guess.
 *
 * What the real one has to do, from man1/sh.1 and from the binary:
 *
 *	< > >> |	redirection and pipes.  The binary keeps the
 *			metacharacters as a string "<>&|" at 0x17cd.
 *	&		run in the background.
 *	'  "  \		quoting.
 *	`cmd`		substitution.  The routine at 0x2aa2 is the
 *			one that does it: it pipes, forks, closes the
 *			write end in the parent, fdopens the read end
 *			and reads the child's output back a line at a
 *			time through a 512 byte buffer.
 *	aliases		expanded before anything else.
 *
 * H2c12 in the binary is the entry point to all of it, and it is the
 * bulk of the program.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include "sh.h"

char *metachars = "<>&|";       /* 0x17cd */

int
parse(line, p)
char *line;
struct pipeline *p;
{
    struct cmd *c;
    char *s;

    p->ncmd = 0;
    c = &p->cmd[0];
    c->argc = 0;
    c->in = c->out = (char *)0;
    c->append = 0;
    c->bg = 0;

    for (s = line; *s; s++) {
        if (*s == '\n') {
            *s = '\0';
            break;
        }
    }

    for (s = line; *s; s++) {
        if (strchr(metachars, *s) || *s == '\'' || *s == '"' || *s == '`') {
            warn("parser not written: cannot handle %c yet", (char *)(int)*s);
            return -1;
        }
    }

    s = line;
    while (*s) {
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s)
            break;
        if (c->argc >= MAXARG - 1) {
            warn("too many arguments", 0);
            return -1;
        }
        c->argv[c->argc++] = s;
        while (*s && *s != ' ' && *s != '\t')
            s++;
        if (*s)
            *s++ = '\0';
    }
    c->argv[c->argc] = (char *)0;

    if (c->argc == 0)
        return 0;
    p->ncmd = 1;
    return 1;
}
