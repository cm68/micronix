/*
 * parse.c - turn one input line into a pipeline
 *
 * micronix/cmd/sh/parse.c
 *
 * The grammar is the binary's.  Its operator table sits at 0x2242,
 * immediately before the parser that uses it at H2257, and holds
 * eight operators in the order they have to be matched - longest
 * first, or ">>" would never be seen because ">" matches it:
 *
 *	|&  |  &  >>&  >>  >&  >  <
 *
 * The forms ending in & send the standard error the same way as the
 * standard output, which is what makes this a csh and not a Bourne
 * shell.  There is no here-document and no "2>".
 *
 * H17d7 in the binary is the tokeniser; H772d is the string equality
 * it tests each operator with, called once per operator - eight
 * times.
 *
 * "<>&|" is only part of what it splits on.  The tokeniser scans
 * against a longer string at 0x17c0 -
 *
 *      (  `     \t  "  <  >  &  |  ;  \n  \r
 *
 * - and 0x17cd, the four operators, is a second pointer into the
 * tail of it.  Reading the tail as the whole is what left ";",
 * backticks and "( )" out of this file.  See NOTES for what each of
 * them does, read by running the image.
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include "sh.h"

/*
 * The operator characters only - the tail of the set at 0x17c0, which
 * is what 0x17cd and 0x17d2 point at.  Not the whole delimiter set.
 */
char *metachars = "<>&|";

/*
 * Where a word ends: the operators, the semicolon that separates one
 * statement from the next, and the backtick.
 *
 * The backtick is a DELIMITER and not a splice, which is the whole
 * reason it is in this string: being in it is what ends a word, so
 * "A`echo hi`B" is the three words A, hi, B and not the one word
 * AhiB that a Bourne shell would make of it.  The image agrees
 * because the image is where that was read.
 *
 * The open paren is the one piece of the set at 0x17c0 still missing,
 * and it stays out until the code that acts on it arrives: a
 * character that ends a word and that nothing then consumes leaves
 * the parse loop turning on the spot, because getword returns nothing
 * and never advances past it.
 */
static char *wordstop = "<>&|;`";

/*
 * The operators, longest first.  The order is the binary's own and
 * it matters.
 */
struct op {
    char *text;
    int  code;
};

#define O_PIPEBOTH  1           /* |&  */
#define O_PIPE      2           /* |   */
#define O_BG        3           /* &   */
#define O_APPBOTH   4           /* >>& */
#define O_APPEND    5           /* >>  */
#define O_OUTBOTH   6           /* >&  */
#define O_OUT       7           /* >   */
#define O_IN        8           /* <   */

static struct op ops[] = {
    "|&",   O_PIPEBOTH,
    "|",    O_PIPE,
    "&",    O_BG,
    ">>&",  O_APPBOTH,
    ">>",   O_APPEND,
    ">&",   O_OUTBOTH,
    ">",    O_OUT,
    "<",    O_IN,
    0,      0
};

/*
 * Somewhere to put the words we pull out of the line.  The line
 * itself is chopped up in place where quoting allows it, and copied
 * here where it does not.
 */
static char wordbuf[MAXLINE * 2];
static int  wordused;

/*
 * getword() has nowhere to return an error - a word and no word look
 * the same from outside - so it says so here and parse() looks.
 */
static int wordbad;

static char *
saveword(s, n)
char *s;
int n;
{
    char *p;

    if (wordused + n + 1 > sizeof(wordbuf))
        return (char *)0;
    p = &wordbuf[wordused];
    while (n--)
        *p++ = *s++;
    *p = '\0';
    p = &wordbuf[wordused];
    wordused += strlen(p) + 1;
    return p;
}

/*
 * Which operator, if any, starts here?  Returns its code and sets
 * *len to how long it was.  The table is in longest-first order so
 * the first hit is the right one.
 */
static int
isop(s, len)
char *s;
int *len;
{
    struct op *o;
    int n;

    for (o = ops; o->text; o++) {
        n = strlen(o->text);
        if (strncmp(s, o->text, n) == 0) {
            *len = n;
            return o->code;
        }
    }
    return 0;
}

/*
 * Copy one word, dealing with quotes.  Returns a pointer to the saved
 * word and leaves *pp past it, or null at the end of the line.  A
 * quote does not survive into the word; a backslash protects exactly
 * the character after it.
 */
static char *
getword(pp)
char **pp;
{
    char buf[MAXLINE];
    char *s = *pp;
    char *q = buf;
    int quote;

    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '\0') {
        *pp = s;
        return (char *)0;
    }

    while (*s && *s != ' ' && *s != '\t') {
        if (strchr(wordstop, *s))
            break;
        if (*s == '\\' && s[1]) {
            s++;
            if (q < buf + sizeof(buf) - 1)
                *q++ = *s++;
            continue;
        }
        /*
         * The double quote, and only the double quote.  The single
         * one used to be here and was ours: the image's set at 0x17c0
         * has no "'" in it, there is no "Missing '." beside the other
         * four messages, and running the image settles it - "echo
         * 'x'" prints 'x' with the quotes still on.
         *
         * A quoted run is copied out whole, which is also why a
         * backtick inside one is never substituted: it never reaches
         * the parse loop that would act on it.  The image agrees -
         * "a `echo b` c" quoted comes back with the backticks still
         * in it.
         */
        if (*s == '"') {
            quote = *s++;
            while (*s && *s != quote) {
                if (q < buf + sizeof(buf) - 1)
                    *q++ = *s++;
                else
                    s++;
            }
            if (*s != quote) {
                perr("Missing \".");
                wordbad = 1;
                *pp = s;
                return (char *)0;
            }
            s++;
            continue;
        }
        if (q < buf + sizeof(buf) - 1)
            *q++ = *s++;
        else
            s++;
    }
    *pp = s;
    if (q == buf)
        return (char *)0;
    return saveword(buf, q - buf);
}

static void
clearcmd(c)
struct cmd *c;
{
    c->argc = 0;
    c->argv[0] = (char *)0;
    c->in = c->out = (char *)0;
    c->append = 0;
    c->bg = 0;
    c->both = 0;
}

/*
 * The name a redirection is given.  Errors here are the binary's
 * "Missing name for redirect.", raised from inside its parser.
 */
static char *
redirname(pp)
char **pp;
{
    char *w;

    w = getword(pp);
    if (!w && !wordbad)                 /* wordbad has said its piece */
        perr("Missing name for redirect.");
    return w;
}

/*
 * Command substitution.
 *
 * *pp is on the opening backtick.  Everything up to the closing one
 * is a command; it is run, and what it wrote comes back as words,
 * which are appended to this command as if they had been typed.  The
 * words are split on whitespace, so a command whose output has
 * several lines or several columns contributes several arguments -
 * "`ls -l /etc | grep passwd`" arrives as eight of them.
 *
 * An unterminated one is the image's "Missing `." and gives up the
 * line.  A backtick inside double quotes never reaches here: getword
 * copies a quoted run out whole, which is why the image prints
 * "a `echo b` c" for the quoted form and substitutes for the bare
 * one.
 */
static int
subst(pp, c)
char **pp;
struct cmd *c;
{
    char cmd[MAXLINE];
    char out[MAXLINE];
    char *s;
    char *q;
    char *w;
    int n;

    s = *pp + 1;                        /* past the opening backtick */
    q = cmd;
    while (*s && *s != '`') {
        if (q < cmd + sizeof(cmd) - 1)
            *q++ = *s;
        s++;
    }
    if (*s != '`') {
        perr("Missing `.");
        return -1;
    }
    *q = '\0';
    *pp = s + 1;                        /* past the closing one */

    if (backtick(cmd, out, sizeof(out)) < 0)
        return -1;

    /*
     * Split what came back.  An empty result contributes nothing at
     * all, which is what the image does with `echo` - the word simply
     * is not there afterwards.
     */
    s = out;
    for (;;) {
        while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
            s++;
        if (*s == '\0')
            break;
        q = s;
        while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r')
            s++;
        if (c->argc >= MAXARG - 1) {
            warn("too many arguments", 0);
            return -1;
        }
        if (!(w = saveword(q, s - q))) {
            warn("out of room for words", 0);
            return -1;
        }
        c->argv[c->argc++] = w;
        c->argv[c->argc] = (char *)0;
    }
    return 0;
}

/*
 * One statement, not one line.
 *
 * ";" separates statements - "echo a ; echo b" runs two, with or
 * without spaces around it - so a line is as many pipelines as it has
 * semicolons, and the caller asks for them one at a time.  *pp is
 * left where the next one starts, so this can be called again until
 * it says there is nothing more.
 *
 * Returns the number of commands, 0 when the rest of the line holds
 * no statement, or -1 on a syntax error, having already complained.
 * It always either returns 0 or leaves *pp further along, so a caller
 * that loops on the return value terminates.
 */
int
parse(pp, p)
char **pp;
struct pipeline *p;
{
    struct cmd *c;
    char *s;
    char *w;
    int code, len;

    wordused = 0;
    wordbad = 0;
    p->ncmd = 0;
    c = &p->cmd[0];
    clearcmd(c);

    for (s = *pp; *s; s++) {
        if (*s == '\n') {
            *s = '\0';
            break;
        }
    }

    /*
     * Empty statements are not errors and not worth reporting: a
     * leading or a doubled ";" is skipped here rather than parsed
     * into a pipeline with nothing in it.
     */
    s = *pp;
    while (*s == ' ' || *s == '\t' || *s == ';')
        s++;
    *pp = s;

    for (;;) {
        while (*s == ' ' || *s == '\t')
            s++;
        if (*s == '\0')
            break;

        if (*s == '#')                  /* a comment runs to the end */
            break;

        /*
         * The end of this statement.  Step over it so the next call
         * starts on what follows.
         */
        if (*s == ';') {
            s++;
            break;
        }

        if (*s == '`') {
            if (subst(&s, c) < 0)
                return -1;
            continue;
        }

        code = isop(s, &len);
        if (code) {
            s += len;
            switch (code) {

            case O_PIPEBOTH:
            case O_PIPE:
                if (c->argc == 0) {
                    perr("Syntax error.");
                    return -1;
                }
                c->both = (code == O_PIPEBOTH);
                if (p->ncmd + 1 >= MAXCMD) {
                    warn("too many commands in a pipeline", 0);
                    return -1;
                }
                p->ncmd++;
                c = &p->cmd[p->ncmd];
                clearcmd(c);
                continue;

            case O_BG:
                c->bg = 1;
                continue;

            case O_APPBOTH:
            case O_APPEND:
                if (!(w = redirname(&s)))
                    return -1;
                c->out = w;
                c->append = 1;
                c->both = (code == O_APPBOTH);
                continue;

            case O_OUTBOTH:
            case O_OUT:
                if (!(w = redirname(&s)))
                    return -1;
                c->out = w;
                c->append = 0;
                c->both = (code == O_OUTBOTH);
                continue;

            case O_IN:
                if (!(w = redirname(&s)))
                    return -1;
                c->in = w;
                continue;
            }
        }

        w = getword(&s);
        if (wordbad)
            return -1;
        if (!w)
            continue;
        if (c->argc >= MAXARG - 1) {
            warn("too many arguments", 0);
            return -1;
        }
        c->argv[c->argc++] = w;
        c->argv[c->argc] = (char *)0;
    }

    *pp = s;

    /*
     * A pipeline whose last stage has no words is "cmd |" with
     * nothing after it.  An empty line is not an error.
     */
    if (p->ncmd == 0 && p->cmd[0].argc == 0)
        return 0;
    if (p->cmd[p->ncmd].argc == 0) {
        perr("Syntax error.");
        return -1;
    }
    p->ncmd++;

    /*
     * & anywhere in the pipeline backgrounds the whole of it, which
     * is how it reads when it is written at the end.
     */
    {
        int i, bg = 0;
        for (i = 0; i < p->ncmd; i++)
            if (p->cmd[i].bg)
                bg = 1;
        if (bg)
            for (i = 0; i < p->ncmd; i++)
                p->cmd[i].bg = 1;
    }
    return p->ncmd;
}
