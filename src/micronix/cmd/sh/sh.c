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
#include "sh.h"

int  login;                 /* argv[0] began with '-'   (0x9004) */
int  verbose;               /* -v: echo lines as they are run */
int  status = 0;            /* what exit is given       (0x9008) */
int  interactive = 1;       /* the flag tested at the top of the loop */

char *prompt = "# ";        /* 0x1c6a */
char *homedir = "/";
char pathlist[MAXPATH] = "/bin:/usr/bin";   /* 0x1efa, 0x1f03 */

char *progname = "sh";

char *strsave();

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
    "dir",      B_DIR,
    "era",      B_ERA,
    "wait",     B_WAIT,
    "exit",     B_EXIT,
    "echo",     B_ECHO,
    "sync",     B_SYNC,
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

void
fatal(s, a)
char *s;
char *a;
{
    warn(s, a);
    exit(1);
}

/*
 * Is this word a builtin?  Returns its number, or 0.
 */
int
lookup(w)
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

/*
 * Find a command on the path.  A name with a slash in it is used as
 * given; anything else is looked for in each path element.  The
 * binary's default is /bin then /usr/bin.
 */
char *
findcmd(name)
char *name;
{
    static char buf[MAXPATH];
    char *p, *q;

    if (strchr(name, '/'))
        return access(name, 1) == 0 ? name : (char *)0;

    p = pathlist;
    while (*p) {
        q = buf;
        while (*p && *p != ':')
            *q++ = *p++;
        if (q != buf && q[-1] != '/')
            *q++ = '/';
        strcpy(q, name);
        if (access(buf, 1) == 0)
            return buf;
        if (*p == ':')
            p++;
    }
    return (char *)0;
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
        redirect(c, pin, pout);
        path = findcmd(c->argv[0]);
        if (!path) {
            warn("%s not found", c->argv[0]);
            exit(1);
        }
        execv(path, c->argv);
        warn("cannot execute %s", path);
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
        printf("%d\n", pid[p->ncmd - 1]);
        return 0;
    }

    st = 0;
    for (i = 0; i < p->ncmd; i++) {
        while ((w = wait(&st)) != -1) {
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
    int i;
    char *p;
    FILE *f;

    switch (code) {

    case B_CD:                          /* cd, chdir */
        p = c->argc > 1 ? c->argv[1] : homedir;
        if (chdir(p) < 0)
            warn("cannot change to %s", p);
        return 0;

    case B_DIR:                         /* dir - list a directory */
        return 1;                       /* handed to the external ls */

    case B_ERA:                         /* era - remove files */
        for (i = 1; i < c->argc; i++) {
            if (unlink(c->argv[i]) < 0)
                warn("cannot remove %s", c->argv[i]);
        }
        return 0;

    case B_WAIT:
        while (wait((int *)0) != -1)
            ;
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

    case B_SYNC:
        sync();
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

    case B_TYPE:                        /* where would this be run from */
        for (i = 1; i < c->argc; i++) {
            if (lookup(c->argv[i]))
                printf("%s is a builtin\n", c->argv[i]);
            else if ((p = findcmd(c->argv[i])))
                printf("%s is %s\n", c->argv[i], p);
            else
                printf("%s not found\n", c->argv[i]);
        }
        return 0;

    case B_SOURCE:                      /* read a file as input */
        if (c->argc < 2) {
            warn("source needs a file", 0);
            return 1;
        }
        if (nsrc >= MAXSRC) {
            warn("source nested too deep", 0);
            return 1;
        }
        if ((f = fopen(c->argv[1], "r")) == NULL) {
            warn("cannot open %s", c->argv[1]);
            return 1;
        }
        srcstack[nsrc++] = f;
        return 0;

    case B_PATH:
        if (c->argc > 1) {
            strncpy(pathlist, c->argv[1], MAXPATH - 1);
            pathlist[MAXPATH - 1] = '\0';
        } else {
            printf("%s\n", pathlist);
        }
        return 0;

    case B_HOME:
        if (c->argc > 1)
            homedir = strsave(c->argv[1]);
        else
            printf("%s\n", homedir);
        return 0;

    case B_KILL:
        if (c->argc < 2) {
            warn("kill needs a process", 0);
            return 1;
        }
        for (i = 1; i < c->argc; i++) {
            if (kill(atoi(c->argv[i]), 9) < 0)
                warn("cannot kill %s", c->argv[i]);
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
                    for (k = j; k + 1 < naliases; k++)
                        aliases[k] = aliases[k + 1];
                    naliases--;
                    break;
                }
            }
        }
        return 0;

    case B_NICE:                        /* run the rest more politely */
        nice(c->argc > 1 ? atoi(c->argv[1]) : 10);
        return 0;
    }
    return 1;
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
    int code;

    if (p->ncmd == 0)
        return 0;
    if (p->ncmd == 1 && p->cmd[0].argc > 0) {
        code = lookup(p->cmd[0].argv[0]);
        if (code)
            return dobuiltin(code, &p->cmd[0]);
    }
    return runpipeline(p);
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
    FILE *f;

    strcpy(path, homedir);
    strcat(path, name);
    if ((f = fopen(path, "r")) == NULL)
        return;
    if (nsrc < MAXSRC)
        srcstack[nsrc++] = f;
    else
        fclose(f);
}

main(argc, argv)
int argc;
char **argv;
{
    char line[MAXLINE];
    struct pipeline pipe1;
    int i;
    char *cmdstring = 0;

    /*
     * A leading '-' on the name is what makes this a login shell,
     * which is the one thing the binary works out before anything
     * else (0x9004).
     */
    if (argv[0] && argv[0][0] == '-')
        login = 1;

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
        if (parse(cmdstring, &pipe1) > 0)
            execute(&pipe1);
        exit(status);
    }

    if (i < argc) {
        FILE *f;
        if ((f = fopen(argv[i], "r")) == NULL)
            fatal("cannot open %s", argv[i]);
        srcstack[nsrc++] = f;
        interactive = 0;
    } else {
        srcstack[nsrc++] = stdin;
        interactive = isatty(0);
    }

    if (login) {
        startup("/.login");
        startup("/.sh");
    }

    while (nextline(line, sizeof(line))) {
        if (verbose)
            fputs(line, stderr);
        if (parse(line, &pipe1) > 0)
            execute(&pipe1);
    }

    exit(status);
}
