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
 * The open paren is here and the close paren is NOT, which looks
 * lopsided and is what the image does: 0x17c0 has "(" in it and no
 * ")", and "echo a)" prints a) with the paren still on.  A close
 * paren only means anything to the scan that is already looking for
 * one.
 *
 * The double quote is here because it DELIMITS.  It does not quote a
 * run inside a word the way a modern shell would: the image reads
 * "echo /etc/"pass"*" as the three words /etc/, pass and *, and globs
 * the last of them.  So a quote ends the word before it, the run
 * inside is a word of its own, and what follows the closing quote
 * starts another.
 */
static char *wordstop = "<>&|;`(\"";

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
static char wordbad;

char *
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
 * Copy one word.  Returns a pointer to the saved word and leaves *pp
 * past it, or null at the end of the line.  A backslash protects
 * exactly the character after it and does not survive into the word.
 *
 * Quoting is not done here.  The double quote is a delimiter, so a
 * quoted run is its own word and the parse loop reads it - which is
 * also why a backtick or a pattern inside quotes is left alone: the
 * word never comes through here to be looked at.
 */
static char *
getword(pp)
char **pp;
{
    char buf[MAXLINE];
    char *s = *pp;
    char *q = buf;
    char *start;
    char *t;
    char *p;
    int i;
    int escaped = 0;

    while (*s == ' ' || *s == '\t')
        s++;
    start = s;
    if (*s == '\0') {
        *pp = s;
        return (char *)0;
    }

    while (*s && *s != ' ' && *s != '\t') {
        if (strchr(wordstop, *s))
            break;
        if (*s == '\\' && s[1]) {
            escaped = 1;
            s++;
            if (q < buf + sizeof(buf) - 1)
                *q++ = *s++;
            continue;
        }

        /*
         * A positional parameter.  ONE digit: the image answers "$10"
         * with the first argument and then a literal 0, so there is
         * no way to reach a tenth.  "$*" is all of them from $1 with
         * a space between.
         *
         * A "$" in front of anything else is just a "$" - there are
         * no named variables here, and "$HOME" comes out as it was
         * written.  It is done in the middle of copying a word rather
         * than to a finished one, because "$" does not delimit: the
         * word "A$1B" is one word and becomes AalphaB.  That is also
         * why a backslash in front of it works without anything more
         * being written - the escape above has already taken the "$"
         * out of the way.
         */
        if (*s == '$' && s[1] >= '0' && s[1] <= '9') {
            i = s[1] - '0';
            if (i < shargc)
                for (p = shargv[i]; *p; p++)
                    if (q < buf + sizeof(buf) - 1)
                        *q++ = *p;
            s += 2;
            continue;
        }
        if (*s == '$' && s[1] == '*') {
            for (i = 1; i < shargc; i++) {
                if (i > 1 && q < buf + sizeof(buf) - 1)
                    *q++ = ' ';
                for (p = shargv[i]; *p; p++)
                    if (q < buf + sizeof(buf) - 1)
                        *q++ = *p;
            }
            s += 2;
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

    /*
     * A backslash does not protect a pattern character.
     *
     * The image answers "echo \*" with "No match." - not with a
     * literal * and not with the directory - so what it looks for is
     * the text as written, backslash and all, and the backslash only
     * comes out of a word that turns out not to be a pattern.  "echo
     * a\b" is ab, and "echo \*" asks the disk for \* and does not
     * find it.
     *
     * So when the word came out a pattern and an escape was taken out
     * of it, put the raw text back.  No second buffer for a case this
     * rare: the span is still there to be copied again.
     */
    if (escaped && ispattern(buf)) {
        q = buf;
        for (t = start; t < s && q < buf + sizeof(buf) - 1; t++)
            *q++ = *t;
    }
    return saveword(buf, q - buf);
}

static void
clearcmd(c)
struct cmd *c;
{
    c->argc = 0;
    c->argv[0] = (char *)0;
    c->sub = (char *)0;
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
 * Add one word to a command, as it stands.  glob.c calls this for
 * each name a pattern came to.
 */
int
addmatch(name, c)
char *name;
struct cmd *c;
{
    char *w;

    if (c->argc >= MAXARG - 1) {
        warn("too many arguments", 0);
        return -1;
    }
    if (!(w = saveword(name, strlen(name)))) {
        warn("out of room for words", 0);
        return -1;
    }
    c->argv[c->argc++] = w;
    c->argv[c->argc] = (char *)0;
    return 1;
}

/*
 * A quoted word.
 *
 * *pp is on the opening quote.  What is inside is one word however
 * much whitespace is in it, and it is taken as it stands: no pattern
 * in it is expanded - the image answers "/etc/*" with /etc/* - and no
 * backtick in it is run.
 *
 * The quote delimits rather than quoting a run inside a word, so this
 * word ends at the closing quote and whatever follows begins another.
 * "echo /etc/"pass"*" is /etc/, pass, and * expanded.
 */
static int
quoted(pp, c)
char **pp;
struct cmd *c;
{
    char buf[MAXLINE];
    char *s;
    char *q;

    s = *pp + 1;                        /* past the opening quote */
    q = buf;
    while (*s && *s != '"') {
        if (q < buf + sizeof(buf) - 1)
            *q++ = *s;
        s++;
    }
    if (*s != '"') {
        perr("Missing \".");
        return -1;
    }
    *q = '\0';
    *pp = s + 1;                        /* past the closing one */

    if (c->sub) {
        perr("Syntax error.");
        return -1;
    }
    return addmatch(buf, c) < 0 ? -1 : 0;
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
 * line.  A backtick inside double quotes never reaches here, because
 * a quoted run is taken as its own word without being scanned, which
 * is why the image prints "a `echo b` c" for the quoted form and
 * substitutes for the bare one.
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
 * A group: "( ... )", run in a child of its own.
 *
 * *pp is on the open paren.  Everything up to the matching close
 * paren is kept as text and handed to runline() in that child, which
 * is why a group holds anything a line holds - several statements,
 * pipes, redirections, and other groups.  Nesting costs nothing for
 * the same reason: "((echo nested))" is a group whose text is a
 * group, and the inner one is parsed by the child.
 *
 * The child is the whole point.  "(cd /etc ; pwd) ; pwd" prints /etc
 * and then /, and "(exit)" does not take the shell with it.
 *
 * A group and words are exclusive: the image answers "Syntax error."
 * to both "echo (echo a)" and "(echo a) b".  What may follow is
 * redirection, a pipe, or "&", and those attach to this command the
 * way they would to any other.
 */
static int
group(pp, c)
char **pp;
struct cmd *c;
{
    char buf[MAXLINE];
    char *s;
    char *q;
    int depth;
    int quote;

    s = *pp + 1;                        /* past the open paren */
    q = buf;
    depth = 1;
    while (*s) {
        /*
         * A paren inside a quoted run or a pair of backticks is not
         * ours to count.  Copy the run out whole and leave what is in
         * it to whoever parses this text later.
         */
        if (*s == '"' || *s == '`') {
            quote = *s;
            if (q < buf + sizeof(buf) - 1)
                *q++ = *s;
            s++;
            while (*s && *s != quote) {
                if (q < buf + sizeof(buf) - 1)
                    *q++ = *s;
                s++;
            }
            if (!*s)
                break;                  /* the inner parse will say so */
            if (q < buf + sizeof(buf) - 1)
                *q++ = *s;
            s++;
            continue;
        }
        if (*s == '(')
            depth++;
        if (*s == ')' && --depth == 0)
            break;
        if (q < buf + sizeof(buf) - 1)
            *q++ = *s;
        s++;
    }
    if (*s != ')') {
        perr("Missing ).");
        return -1;
    }
    *q = '\0';
    *pp = s + 1;                        /* past the close paren */

    /*
     * The unmatched paren is looked for BEFORE this, because that is
     * the order the image answers in: "echo (" is "Missing )." and
     * "echo (echo a)" is "Syntax error.".  It finds the end of the
     * group first and complains about where the group is second.
     */
    if (c->argc > 0 || c->sub) {
        perr("Syntax error.");
        return -1;
    }

    /*
     * Not saveword().  The child parses this text, and parse() begins
     * by emptying the word buffer - so text held there would be
     * overwritten by the very words being read out of it.  It lives
     * on the heap instead, where nothing else is going to reach.
     */
    c->sub = strsave(buf);
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
    int npat;                           /* patterns seen in this statement */
    int nmatch;                         /* and names they came to */

    wordused = 0;
    wordbad = 0;
    npat = nmatch = 0;
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

        if (*s == '#') {                /* a comment runs to the end */
            while (*s)                  /* and takes the line with it */
                s++;
            break;
        }

        /*
         * The end of this statement.  Step over it so the next call
         * starts on what follows.
         */
        if (*s == ';') {
            s++;
            break;
        }

        if (*s == '"') {
            if (quoted(&s, c) < 0)
                return -1;
            continue;
        }

        if (*s == '`') {
            if (subst(&s, c) < 0)
                return -1;
            continue;
        }

        if (*s == '(') {
            if (group(&s, c) < 0)
                return -1;
            continue;
        }

        code = isop(s, &len);
        if (code) {
            s += len;
            switch (code) {

            case O_PIPEBOTH:
            case O_PIPE:
                if (c->argc == 0 && !c->sub) {
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
        if (c->sub) {                   /* "(echo a) b" */
            perr("Syntax error.");
            return -1;
        }

        /*
         * A word with a pattern in it is asked of the disk; one
         * without is the word itself, and is not looked for.  A
         * pattern that finds nothing adds nothing and is not an error
         * on its own - see the end of this function.
         */
        if (ispattern(w)) {
            npat++;
            if ((len = globword(w, c)) < 0)
                return -1;
            nmatch += len;
            continue;
        }
        if (addmatch(w, c) < 0)
            return -1;
    }

    *pp = s;

    /*
     * Nothing the statement asked for was there.
     *
     * Only when NOTHING matched: the image prints /etc/passwd and
     * says not a word about the first half of
     * "echo /nosuch/* /etc/pass*", and complains only when every
     * pattern in the statement came to nothing.  A word with no
     * pattern in it is not asked and does not count either way.
     *
     * This gives up the statement and not the line - "echo *.nope ;
     * echo after" says No match. and then after - so it is a nothing
     * to run rather than a parse that failed, and runline() carries
     * on to what follows the semicolon.
     */
    if (npat && !nmatch) {
        perr("No match.");
        return 0;
    }

    /*
     * A pipeline whose last stage has no words is "cmd |" with
     * nothing after it.  An empty line is not an error.
     */
    if (p->ncmd == 0 && p->cmd[0].argc == 0 && !p->cmd[0].sub)
        return 0;
    if (p->cmd[p->ncmd].argc == 0 && !p->cmd[p->ncmd].sub) {
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
