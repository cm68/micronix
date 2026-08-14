/*
 * sh - the Micronix shell
 *
 * micronix/cmd/sh/sh.c
 *
 * Reconstructed from /bin/sh, which we have no source for.  NOTES
 * records how the binary was read and which facts here were traced
 * out of it rather than taken from man1/sh.1.
 *
 * Everything except the parser is here.  parse() is declared in sh.h
 * and not yet written; see NOTES for what it has to do.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <signal.h>
#include "sh.h"

int  login;                 /* argv[0] began with '-'   (0x9004) */
int  verbose;               /* -v: echo lines as they are run */
int  status;            /* what exit is given       (0x9008) */
int  interactive = 1;       /* the flag tested at the top of the loop */

char *prompt = "# ";        /* 0x1c6a */
char *homedir = "/";
/*
 * The search path.  The image keeps three char * in an array at
 * 0x9741, laid out backwards the way its builtin table is, pointing
 * at "." (0x1f08), "/bin" (0x1f03) and "/usr/bin" (0x1efa).  Three
 * entries, and one of them is the current directory - an earlier
 * version of this file had a colon separated string with only two,
 * which was wrong twice over.
 */
char *pathv[MAXPATHV] = { ".", "/bin", "/usr/bin", 0 };

char *progname = "sh";

char *strsave();

/*
 * The line and the pipeline built from it live out here rather than
 * in main's frame.  Between them they are more than two kilobytes,
 * and a Z80 frame will not carry that - ccc says "locals too large"
 * and it is right to.
 */
char line[MAXLINE];
struct pipeline pipe1;

/*
 * The positional parameters, $0 upwards.
 *
 * They are a WINDOW ON OUR OWN ARGV rather than a copy of anything,
 * which is what the image does and is visible in the two cases it
 * answers differently:
 *
 *	sh /tmp/args alpha beta	$0 is /tmp/args and $1 is alpha
 *	sh -c 'echo $1'		$1 is -c
 *
 * The first has moved the window along to the script, so the script
 * is $0 and its own arguments follow.  The second has not moved it at
 * all, so $1 is still the flag - which is not useful, and is what it
 * does.
 */
char **shargv;
int  shargc;

/*
 * The stack of input sources.  The binary keeps it at 0x9648 with the
 * pointer in 0x9646, pushes stdin onto it at startup, and pops one
 * level every time a source runs dry - which is what makes the source
 * builtin and script arguments work.
 */
FILE *srcstack[MAXSRC];
int  nsrc;

/*
 * Aliases.  alias with no argument lists, with one argument shows,
 * with more sets; unalias removes.
 */
struct alias {
    char *name;
    char *value;
} aliases[MAXALIAS];
int naliases;

/*
 * The builtin names, in the binary's own order, each with the number
 * its dispatch uses.  cd and chdir share a number.
 */
struct builtin builtins[] = {
    "cd",       B_CD,
    "chdir",    B_CD,
    "wait",     B_WAIT,
    "exit",     B_EXIT,
    "echo",     B_ECHO,
    "prompt",   B_PROMPT,
    "pid",      B_PID,
    "type",     B_TYPE,
    "source",   B_SOURCE,
    "path",     B_PATH,
    "home",     B_HOME,
    "kill",     B_KILL,
    "alias",    B_ALIAS,
    "unalias",  B_UNALIAS,
    "nice",     B_NICE,
    0,          0
};

void
warn(s, a)
char *s;
char *a;
{
    fprintf(stderr, "%s: ", progname);
    fprintf(stderr, s, a);
    fprintf(stderr, "\n");
}

/*
 * A message the image prints with nothing in front of it.
 *
 * Its parse errors are its own strings - "Syntax error." at 0x0111,
 * "Missing name for redirect." at 0x2227, and the four "Missing X."
 * at 0x1790 - and it writes them bare, where warn() would put "sh: "
 * on the front.  Our own complaints, the ones with no string in the
 * image behind them, keep the name: they are the shell talking about
 * itself, not the language failing to parse.
 */
void
perr(s)
char *s;
{
    fprintf(stderr, "%s\n", s);
}

void
fatal(s, a)
char *s;
char *a;
{
    warn(s, a);
    exit(1);
}

/*
 * H48dc.  Is this word a builtin?  Returns its number, or 0.
 */
int
isbuiltin(w)
char *w;
{
    struct builtin *b;

    for (b = builtins; b->name; b++) {
        if (strcmp(b->name, w) == 0)
            return b->code;
    }
    return 0;
}

char *
getalias(name)
char *name;
{
    int i;

    for (i = 0; i < naliases; i++) {
        if (strcmp(aliases[i].name, name) == 0)
            return aliases[i].value;
    }
    return (char *)0;
}

setalias(name, value)
char *name;
char *value;
{
    int i;

    for (i = 0; i < naliases; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            free(aliases[i].value);
            aliases[i].value = strsave(value);
            return;
        }
    }
    if (naliases >= MAXALIAS) {
        warn("too many aliases", 0);
        return;
    }
    aliases[naliases].name = strsave(name);
    aliases[naliases].value = strsave(value);
    naliases++;
}

char *
strsave(s)
char *s;
{
    char *p;

    p = malloc(strlen(s) + 1);
    if (!p)
        fatal("out of memory", 0);
    strcpy(p, s);
    return p;
}

static void
joinpath(buf, dir, name)
char *buf;
char *dir;
char *name;
{
    strcpy(buf, dir);
    if (buf[0] && buf[strlen(buf) - 1] != '/')
        strcat(buf, "/");
    strcat(buf, name);
}

/*
 * The honest search: ask the file system about every directory in
 * turn.  What findcmd did before there were any tables, and what it
 * falls back to when they turn out not to be worth believing.
 */
static char *
searchpath(buf, name)
char *buf;
char *name;
{
    int i;

    for (i = 0; i < MAXPATHV && pathv[i]; i++) {
        joinpath(buf, pathv[i], name);
        if (access(buf, 1) == 0)
            return buf;
    }
    return (char *)0;
}

/*
 * Where would this command be run from?
 *
 * The path is remembered - see hash.c and "Directory Hashing" in
 * man1/sh.1 - so a directory which certainly does not hold the name
 * is skipped without asking the file system, and one which claims it
 * is BELIEVED WITHOUT ASKING.  That is what the win is made of:
 * traced against the image, running "ls" is one syscall,
 *
 *	exec("/bin/ls")
 *
 * where searching costs three - access("./ls") failing,
 * access("/bin/ls") succeeding, and then the exec.
 *
 * Which means this can be WRONG, and the caller has to be ready for
 * that.  A file removed since the tables were built is still claimed,
 * and the path handed back will not exec.  spawn() answers that by
 * rebuilding and searching properly before it gives up, so a stale
 * table costs one failed exec and never a command.  The original had
 * no such thing - its page says "it may be necessary to enter the
 * command's name twice", which is exactly the cache being believed
 * when it is wrong, with the user left to work it out.
 *
 * A name no directory claims is searched for here and now, since
 * being absent from the tables is the other way they go stale.
 */
char *
findcmd(name)
char *name;
{
    static char buf[MAXPATH];
    int i;

    if (strchr(name, '/'))
        return access(name, 1) == 0 ? name : (char *)0;

    for (i = 0; i < MAXPATHV && pathv[i]; i++) {
        switch (inhash(i, name)) {
        case 0:
            continue;                   /* certainly not here */
        case 1:
            joinpath(buf, pathv[i], name);
            return buf;                 /* believed */
        }
        joinpath(buf, pathv[i], name);  /* no tables for this one */
        if (access(buf, 1) == 0)
            return buf;
    }
    return searchpath(buf, name);
}

/*
 * H4287.  Two calls to signal() with 1, for SIGINT and SIGQUIT.
 *
 * 1 is SIG_IGN.  signal() hands the kernel the address of a
 * trampoline out of _jtab rather than of the handler, and _stab holds
 * the handler the trampoline reaches - but 0 and 1 are the two values
 * that mean themselves, default and ignore, and go to the kernel as
 * they are.
 *
 * This was empty, and said so: libu could not link signal() because
 * _signal.s declared none of its symbols, so __signal and _jtab
 * resolved to nobody.  That is fixed, and so is the argument order in
 * the stub - it handed the kernel the handler where the signal number
 * belonged, and every call came back "out of range".
 */
ignoresigs()
{
    signal(2, 1);                       /* interrupt */
    signal(3, 1);                       /* quit */
}

/*
 * isatty is not in libu.a either, but gtty is, and asking a
 * descriptor for its terminal modes is how this was always done: if
 * it answers, it is a terminal.
 */
int
isatty(fd)
int fd;
{
    char buf[6];

    return gtty(fd, buf) == 0;
}

/*
 * H7e5f.  Its whole body is pushing two arguments and calling the
 * exec stub, so that is all this is.
 */
doexec(path, argv)
char *path;
char **argv;
{
    execv(path, argv);
}

/*
 * Put one command's redirections in place.  This runs in the child,
 * after the fork, so the shell's own descriptors are untouched.  The
 * dance is always the same: open the new file, close the descriptor
 * being replaced, dup the new one into its place, close the spare.
 */
redirect(c, pin, pout)
struct cmd *c;
int pin;
int pout;
{
    int fd;

    if (pin >= 0) {
        close(0);
        dup(pin);
        close(pin);
    }
    if (pout >= 0) {
        close(1);
        dup(pout);
        close(pout);
    }
    if (c->in) {
        if ((fd = open(c->in, 0)) < 0) {
            warn("cannot open %s", c->in);
            exit(1);
        }
        close(0);
        dup(fd);
        close(fd);
    }
    if (c->out) {
        if (c->append) {
            if ((fd = open(c->out, 1)) < 0)
                fd = creat(c->out, 0666);
            else
                lseek(fd, 0L, 2);
        } else {
            fd = creat(c->out, 0666);
        }
        if (fd < 0) {
            warn("cannot create %s", c->out);
            exit(1);
        }
        close(1);
        dup(fd);
        close(fd);
    }
    /*
     * The & forms send the standard error wherever the standard
     * output now goes.  Done last, so it picks up a redirection or a
     * pipe indifferently.
     */
    if (c->both) {
        close(2);
        dup(1);
    }
}

/*
 * Run one command as a child.  pin and pout are the pipe descriptors
 * it should read from and write to, or -1.  Returns the pid.
 */
int
spawn(c, pin, pout)
struct cmd *c;
int pin;
int pout;
{
    int pid;
    char *path;
    char pathbuf[MAXPATH];

    /*
     * Flush before forking.  The builtins write through stdio and the
     * children write straight to the descriptor, so without this the
     * two come out in the wrong order - and worse, a child inherits a
     * copy of whatever is still sitting in the buffer and writes it
     * out again itself.
     */
    fflush(stdout);
    fflush(stderr);

    pid = fork();
    if (pid < 0) {
        warn("cannot fork", 0);
        return -1;
    }
    if (pid == 0) {
        /*
         * A background child stops listening to the keyboard's
         * interrupt and quit, so that a ^C meant for whatever is in
         * front does not take it with it.  A foreground child keeps
         * them, because a ^C is how you stop it.
         *
         * The call was here unconditionally while ignoresigs() was
         * empty, where it cost nothing.  Now that it does something,
         * doing it for every child would mean nothing could be
         * interrupted.  Which of the two the original does is not
         * read - NOTES has the two signal() calls at H4287 and not
         * what guards them - so this is the v6 arrangement rather
         * than the binary's, and is worth confirming against it.
         */
        if (c->bg)
            ignoresigs();
        redirect(c, pin, pout);

        /*
         * A group runs here rather than being exec'd, and running
         * here in a child of the shell is exactly what makes it a
         * subshell: a cd or an exit inside it goes when the child
         * does.  Its redirection is already in place, so it applies
         * to everything in the group at once - "(echo a ; echo b) >f"
         * puts both lines in f.
         */
        if (c->sub) {
            runline(c->sub);
            exit(status);
        }

        /*
         * The tables can claim a name that is no longer there, so a
         * path that will not exec is not the end of it: rebuild them,
         * search properly, and try what that finds.  A stale table
         * costs one failed exec here and never a command.
         */
        if ((path = findcmd(c->argv[0])))
            doexec(path, c->argv);

        hashflush();
        if ((path = searchpath(pathbuf, c->argv[0])))
            doexec(path, c->argv);

        /* 0x1212 in the image, printed after the name */
        fprintf(stderr, "%s: Command not found.\n", c->argv[0]);
        exit(1);
    }
    return pid;
}

/*
 * A whole pipeline.  Each stage but the last gets a pipe to the next;
 * the parent closes both ends as it goes, or the reader never sees an
 * end of file.  Only the last command's status is kept, which is what
 * the shell reports.
 */
int
runpipeline(p)
struct pipeline *p;
{
    int i;
    int fd[2];
    int pin, pout;
    int pid[MAXCMD];
    int w, st;

    pin = -1;
    for (i = 0; i < p->ncmd; i++) {
        pout = -1;
        if (i + 1 < p->ncmd) {
            if (pipe(fd) < 0) {
                warn("cannot make a pipe", 0);
                return 1;
            }
            pout = fd[1];
        }
        pid[i] = spawn(&p->cmd[i], pin, pout);
        if (pin >= 0)
            close(pin);
        if (pout >= 0)
            close(pout);
        pin = (i + 1 < p->ncmd) ? fd[0] : -1;
    }

    if (p->cmd[p->ncmd - 1].bg) {
        /*
         * Remembered under the name of the stage whose pid is the one
         * announced, so that "kill name" and the number printed here
         * mean the same process.  A group has no name to give.
         */
        i = p->ncmd - 1;
        if (p->cmd[i].argc > 0)
            jobadd(pid[i], p->cmd[i].argv[0]);
        printf("%d\n", pid[i]);
        return 0;
    }

    st = 0;
    for (i = 0; i < p->ncmd; i++) {
        while ((w = wait(&st)) != -1) {
            /*
             * Anything reaped here is finished, background jobs of
             * the user's included - a slot left behind would name a
             * pid the system is free to hand to somebody else.
             */
            jobdone(w);
            if (w == pid[p->ncmd - 1])
                status = (st >> 8) & 0xff;
            if (w == pid[i])
                break;
        }
    }
    return status;
}

/*
 * The builtins.  Numbered as the binary numbers them so that the two
 * can be compared; see sh.h.
 */
int
dobuiltin(code, c)
int code;
struct cmd *c;
{
    int i, j;
    char *p;
    FILE *f;

    switch (code) {

    case B_CD:                          /* cd, chdir */
        p = c->argc > 1 ? c->argv[1] : homedir;
        if (chdir(p) < 0)
            warn("cannot change to %s", p);
        else
            hashflush();                /* "." is a different place now */
        return 0;

    case B_WAIT:
        while ((i = wait((int *)0)) != -1)
            jobdone(i);
        return 0;

    case B_EXIT:
        exit(c->argc > 1 ? atoi(c->argv[1]) : status);

    case B_ECHO:
        for (i = 1; i < c->argc; i++) {
            fputs(c->argv[i], stdout);
            if (i + 1 < c->argc)
                putchar(' ');
        }
        putchar('\n');
        return 0;

    case B_PROMPT:
        if (c->argc > 1)
            prompt = strsave(c->argv[1]);
        else
            printf("%s\n", prompt);
        return 0;

    case B_PID:
        printf("%d\n", getpid());
        return 0;

    case B_TYPE:                        /* print the named files */
        /*
         * type is CAT.  man1/sh.1: "The contents of each of the named
         * files are printed verbatim on the standard output."
         *
         * What was here was the meaning the word has had in every
         * shell since - where would this command be run from - and
         * both the page and the image say otherwise: "type /etc/motd"
         * prints the file.  It was written from the name rather than
         * from either source, and neither had been read for it.
         */
        for (i = 1; i < c->argc; i++) {
            if ((f = fopen(c->argv[i], "r")) == NULL) {
                fprintf(stderr, "%s: No such file or directory\n",
                    c->argv[i]);
                continue;               /* and on to the next one */
            }
            while ((j = getc(f)) != EOF)
                putc(j, stdout);
            fclose(f);
        }
        return 0;

    case B_SOURCE:                      /* read a file as input */
        if (c->argc < 2) {
            warn("source needs a file", 0);
            return 1;
        }
        return sourcefile(c->argv[1], 0) < 0 ? 1 : 0;

    case B_PATH:
        if (c->argc > 1) {
            for (i = 1; i < c->argc && i <= MAXPATHV; i++)
                pathv[i - 1] = strsave(c->argv[i]);
            pathv[i - 1] = (char *)0;
            /*
             * The old tables describe directories nobody looks in
             * now, so they go.  Nothing is read back until something
             * is looked for, and the search until then is the one
             * that asks the file system.
             */
            hashflush();
        } else {
            for (i = 0; i < MAXPATHV && pathv[i]; i++)
                printf("%s%s", i ? " " : "", pathv[i]);
            printf("\n");
        }
        return 0;

    case B_HOME:
        if (c->argc > 1)
            homedir = strsave(c->argv[1]);
        else
            printf("%s\n", homedir);
        return 0;

    case B_KILL:
        /*
         * "kill N" and "kill name".  A word beginning with a digit is
         * the number; anything else is looked up among the background
         * jobs we started - see job.c for why that is the whole of
         * the mechanism and the process table is not involved.
         *
         * The image says nothing at all to a bare "kill", so neither
         * do we.
         */
        for (i = 1; i < c->argc; i++) {
            p = c->argv[i];
            if (*p >= '0' && *p <= '9')
                j = atoi(p);
            else
                j = jobpid(p);
            if (j <= 0 || kill(j, 9) < 0) {
                fprintf(stderr, "%s: No such process\n", p);
                continue;
            }
            jobdone(j);
        }
        return 0;

    case B_ALIAS:
        if (c->argc == 1) {
            for (i = 0; i < naliases; i++)
                printf("%s\t%s\n", aliases[i].name, aliases[i].value);
        } else if (c->argc == 2) {
            if ((p = getalias(c->argv[1])))
                printf("%s\n", p);
        } else {
            /* everything after the name is the value, not just the
             * next word - "alias ll ls -l" has to keep the -l */
            char buf[MAXLINE];
            buf[0] = '\0';
            for (i = 2; i < c->argc; i++) {
                if (i > 2)
                    strcat(buf, " ");
                strcat(buf, c->argv[i]);
            }
            setalias(c->argv[1], buf);
        }
        return 0;

    case B_UNALIAS:
        for (i = 1; i < c->argc; i++) {
            int j, k;
            for (j = 0; j < naliases; j++) {
                if (strcmp(aliases[j].name, c->argv[i]) == 0) {
                    free(aliases[j].name);
                    free(aliases[j].value);
                    /* field by field: this compiler has no
                     * structure assignment */
                    for (k = j; k + 1 < naliases; k++) {
                        aliases[k].name = aliases[k + 1].name;
                        aliases[k].value = aliases[k + 1].value;
                    }
                    naliases--;
                    break;
                }
            }
        }
        return 0;

    case B_NICE:                        /* run the rest more politely */
        /*
         * "the rest" is the point of it, and the rest was thrown
         * away: the priority was set and nothing ran.  A number first
         * is how much, otherwise it is the default and everything
         * after the word is the command.
         */
        i = 1;
        if (c->argc > 1 && c->argv[1][0] >= '0' && c->argv[1][0] <= '9') {
            nice(atoi(c->argv[1]));
            i = 2;
        } else {
            nice(10);
        }
        if (i >= c->argc)
            return 0;                   /* nothing named: just the ask */
        for (j = 0; i + j < c->argc; j++)
            c->argv[j] = c->argv[i + j];
        c->argc -= i;
        c->argv[c->argc] = (char *)0;
        return B_ASCOMMAND;
    }
    return 1;
}

/*
 * A word that stands for something else.  The value is a line, so it
 * can be several words - "alias ll ls -l" - and they go in front of
 * whatever arguments were given.
 *
 * The table was set and shown and never read: "alias e echo" and then
 * "e hello" answered "e not found".
 *
 * Expansion happens once, here, and what comes out is not looked at
 * again, so "alias ls ls -F" ends rather than going round.  The value
 * is copied because the words point into it after the split.
 */
void
aliasexpand(c)
struct cmd *c;
{
    char *v, *w;
    char *nv[MAXARG];
    int n, i;

    if (c->argc == 0)
        return;
    if ((v = getalias(c->argv[0])) == (char *)0)
        return;
    v = strsave(v);
    n = 0;
    w = v;
    while (*w && n < MAXARG - 1) {
        while (*w == ' ' || *w == '\t')
            w++;
        if (!*w)
            break;
        nv[n++] = w;
        while (*w && *w != ' ' && *w != '\t')
            w++;
        if (*w)
            *w++ = '\0';
    }
    if (n == 0)
        return;
    for (i = 1; i < c->argc && n < MAXARG - 1; i++)
        nv[n++] = c->argv[i];
    for (i = 0; i < n; i++)
        c->argv[i] = nv[i];
    c->argc = n;
    c->argv[n] = (char *)0;
}

/*
 * Put a builtin's redirection in place, and take it away again.
 *
 * redirect() cannot serve here.  It exits when it cannot open the
 * file, which is right in the child it was written for and is the
 * shell going away when the builtin runs in the shell itself.  The
 * descriptors it replaces are duped out of the way and duped back.
 */
int
pushredir(c, sv)
struct cmd *c;
int *sv;
{
    int fd;

    sv[0] = sv[1] = sv[2] = -1;
    if (c->in) {
        if ((fd = open(c->in, 0)) < 0) {
            warn("cannot open %s", c->in);
            return -1;
        }
        sv[0] = dup(0);
        close(0);
        dup(fd);
        close(fd);
    }
    if (c->out) {
        if (c->append) {
            if ((fd = open(c->out, 1)) < 0)
                fd = creat(c->out, 0666);
            else
                lseek(fd, 0L, 2);
        } else {
            fd = creat(c->out, 0666);
        }
        if (fd < 0) {
            warn("cannot create %s", c->out);
            popredir(sv);
            return -1;
        }
        fflush(stdout);
        sv[1] = dup(1);
        close(1);
        dup(fd);
        close(fd);
        if (c->both) {
            sv[2] = dup(2);
            close(2);
            dup(1);
        }
    }
    return 0;
}

void
popredir(sv)
int *sv;
{
    fflush(stdout);
    fflush(stderr);
    if (sv[2] >= 0) {
        close(2);
        dup(sv[2]);
        close(sv[2]);
    }
    if (sv[1] >= 0) {
        close(1);
        dup(sv[1]);
        close(sv[1]);
    }
    if (sv[0] >= 0) {
        close(0);
        dup(sv[0]);
        close(sv[0]);
    }
}

/*
 * One pipeline: a builtin if it is a single command whose first word
 * names one, otherwise a pipeline of children.  The binary decides
 * this the same way - it looks the word up first and only forks when
 * the lookup fails.
 */
int
execute(p)
struct pipeline *p;
{
    int code, r, i;
    int sv[3];

    if (p->ncmd == 0)
        return 0;
    for (i = 0; i < p->ncmd; i++)
        aliasexpand(&p->cmd[i]);
    if (p->ncmd == 1 && p->cmd[0].argc > 0) {
        code = isbuiltin(p->cmd[0].argv[0]);
        if (code) {
            /*
             * A builtin runs in the shell, so its redirection has to
             * be put in place and taken away again around it - the
             * child that redirect() serves never happens.  Without
             * this "echo x > f" wrote x to the terminal and left no f.
             */
            if (pushredir(&p->cmd[0], sv) < 0)
                return 1;
            r = dobuiltin(code, &p->cmd[0]);
            popredir(sv);
            if (r != B_ASCOMMAND)
                return r;
        }
    }
    return runpipeline(p);
}

/*
 * Run one input line, which is as many statements as it has
 * semicolons in it.
 *
 * parse() hands them back one at a time and leaves the pointer where
 * the next one starts, so this asks until there is nothing left.  A
 * syntax error abandons the rest of the line - it has already said
 * what was wrong, and carrying on into the remains of a line nobody
 * managed to read would only say it again.
 *
 * pipe1 is reused for each statement, and can be, because execute()
 * has finished with one before the next is parsed.  It is a global
 * rather than a local here for the reason it always was: a struct
 * pipeline is the better part of two kilobytes, which is not
 * something to put on this machine's stack.
 */
void
runline(line)
char *line;
{
    char *s;
    int n;

    /*
     * On what is left of the line rather than on the answer, because
     * a statement can come to nothing without the line being over: a
     * pattern that matched nothing says "No match." and hands back
     * none, and what follows the semicolon still has to run.  A parse
     * that actually failed gives up the whole line.
     */
    s = line;
    while (*s) {
        n = parse(&s, &pipe1);
        if (n < 0)
            break;
        if (n > 0)
            execute(&pipe1);
    }
}

/*
 * Run a command and hand back what it wrote.
 *
 * This is the inside of a pair of backticks.  The child runs it
 * through runline(), so everything the shell can do the substitution
 * can do too - aliases, builtins, redirection and pipes; the image
 * takes "`ls -l /etc | grep passwd`" and so do we.  It is its own
 * process, so scribbling on pipe1 in there costs us nothing.
 *
 * Fills buf with up to n-1 bytes and terminates it.  Returns how much
 * it got, or -1 if the command could not be run at all.
 */
int
backtick(cmd, buf, n)
char *cmd;
char *buf;
int n;
{
    int fds[2];
    int pid;
    int used;
    int r;
    int st;

    if (pipe(fds) < 0) {
        warn("cannot pipe", 0);
        return -1;
    }
    if ((pid = fork()) == 0) {
        close(fds[0]);
        close(1);
        dup(fds[1]);
        close(fds[1]);
        runline(cmd);
        exit(status);
    }
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        warn("cannot fork", 0);
        return -1;
    }

    /*
     * The write end has to go before the read, or the read never sees
     * the end of the file: we would be holding one ourselves.
     */
    close(fds[1]);
    used = 0;
    while (used < n - 1) {
        if ((r = read(fds[0], &buf[used], n - 1 - used)) <= 0)
            break;
        used += r;
    }
    buf[used] = '\0';
    close(fds[0]);

    /*
     * Reap ours and nobody else's.  A background job of the user's
     * may finish while we are waiting here, and its status is not
     * this one's to swallow.
     */
    while ((r = wait(&st)) >= 0 && r != pid)
        ;
    return used;
}

/*
 * Read one line from the innermost input source, popping back out of
 * each as it runs dry.  Returns 0 at the end of everything.
 */
int
nextline(buf, n)
char *buf;
int n;
{
    while (nsrc > 0) {
        if (interactive && nsrc == 1) {
            fputs(prompt, stdout);
            fflush(stdout);
        }
        if (fgets(buf, n, srcstack[nsrc - 1]))
            return 1;
        if (nsrc > 1)
            fclose(srcstack[nsrc - 1]);
        nsrc--;
    }
    return 0;
}

/*
 * Read a startup file if it is there, by pushing it as an input
 * source.  The binary reads ~/.login and ~/.sh, named at 0x1bea and
 * 0x1bf2 and joined to the home directory.
 */
startup(name)
char *name;
{
    char path[MAXPATH];

    /*
     * The names carry their own leading slash, so a home directory
     * that is already the root would spell it twice - "//.sh".  It
     * opens either way; it reads badly in a trace.
     */
    strcpy(path, homedir);
    if (path[0] && path[strlen(path) - 1] == '/')
        path[strlen(path) - 1] = '\0';
    strcat(path, name);
    sourcefile(path, 1);
}

/*
 * Push a file as the input source.  nextline() reads from the
 * innermost one and pops back out of it when it runs dry, so this is
 * the whole of what source does and the whole of what a startup file
 * is: the shell reads ~/.sh, then ~/.login if it is a login shell,
 * then whatever it was reading before.
 *
 * The two were the same lines written twice.  What differs is the
 * file not being there: no ~/.sh is the ordinary case and says
 * nothing, and a source naming a file that is not there is a mistake
 * worth hearing about.
 */
int
sourcefile(path, quiet)
char *path;
int quiet;
{
    FILE *f;

    if (nsrc >= MAXSRC) {
        if (!quiet)
            warn("source nested too deep", 0);
        return -1;
    }
    if ((f = fopen(path, "r")) == NULL) {
        if (!quiet)
            warn("cannot open %s", path);
        return -1;
    }
    srcstack[nsrc++] = f;
    return 0;
}

main(argc, argv)
int argc;
char **argv;
{
    int i;
    char *cmdstring = 0;

    /*
     * A leading '-' on the name is what makes this a login shell,
     * which is the one thing the binary works out before anything
     * else (0x9004).
     */
    if (argv[0] && argv[0][0] == '-')
        login = 1;

    /*
     * Until a script moves it along, the window sits where it starts:
     * $0 is the shell's own name and $1 is its first argument, flag
     * or not.
     */
    shargv = argv;
    shargc = argc;

    /*
     * dir and era are cp/m spellings, kept so that fingers trained on
     * that system reach something.  They are not shell work, so they
     * are seeded here rather than built in: "alias dir" says what dir
     * is, and "alias dir ls -l" changes it, neither of which a builtin
     * could offer.  Seeded before ~/.login and ~/.sh are read, so a
     * user who wants them to mean something else says so there.
     */
    setalias("dir", "ls");
    setalias("era", "rm");

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            if (++i >= argc)
                fatal("-c needs a command", 0);
            cmdstring = argv[i];
        } else {
            break;
        }
    }

    /*
     * -c runs one string and stops.  A file argument is read as
     * input.  Otherwise it is the standard input, and whether that
     * is a terminal decides if there is a prompt.
     */
    if (cmdstring) {
        interactive = 0;
        runline(cmdstring);
        exit(status);
    }

    if (i < argc) {
        FILE *f;
        if ((f = fopen(argv[i], "r")) == NULL)
            fatal("cannot open %s", argv[i]);
        srcstack[nsrc++] = f;
        interactive = 0;
        /*
         * The script is $0 and what follows it is $1 upwards.
         */
        shargv = &argv[i];
        shargc = argc - i;
    } else {
        srcstack[nsrc++] = stdin;
        interactive = isatty(0);
    }

    /*
     * ~/.sh is every shell and ~/.login is only the one you log in
     * to, which is the csh arrangement these names come from.  Both
     * were inside the login test, so ~/.sh - the one that sets the
     * aliases and the path, and the whole reason to have a file at
     * all - never ran for any shell started from another.
     *
     * Pushed in this order because nextline() reads the innermost
     * source first: ~/.login goes on before ~/.sh so that ~/.sh is
     * read before it, and the terminal after both.
     */
    if (login)
        startup("/.login");
    startup("/.sh");

    while (nextline(line, sizeof(line))) {
        if (verbose)
            fputs(line, stderr);

        /*
         * References are expanded only for what is typed.  Fed a
         * file the image runs "!e" as a command and answers "!e:
         * Command not found.", so a script is never rewritten under
         * itself - and nsrc is what tells the two apart, the same
         * test that decides whether to prompt.
         */
        if (interactive && nsrc == 1) {
            i = histexpand(line, sizeof(line));
            if (i < 0)
                continue;               /* it has said what was wrong */
            if (i > 0)
                fputs(line, stdout);    /* show what it came to */
            histadd(line);
        }
        runline(line);
    }

    exit(status);
}
