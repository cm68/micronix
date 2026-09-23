/*
 * xargs - run a command with arguments read from stdin
 *
 * cmd/xargs/xargs.c
 *
 * xargs reads words from stdin - find's output, one name per line - and
 * runs the named command with them as arguments, in batches that stay
 * under two limits at once: the shell's word limit, and the kernel's
 * argument-block size.  "find . -name "*.o" -print ... | xargs rm -f" is
 * how a makefile's clean rule removes any number of files without
 * tripping the shell's "too many arguments" or exec's E2BIG.
 *
 * The word limit keeps a command under MAXARG words; the byte limit
 * keeps the argument block under the kernel's exec limit.  exec copies
 * the arguments (each string plus its NUL, the command's own name
 * included) into NBLKS blocks of 512 bytes - see sys/exec.c, NBLKS is 4
 * - and returns E2BIG past 2048 bytes.  Counting words alone is not
 * enough: a batch of short names is fine at 62 words, but 62 long
 * pathnames can overrun the 2048-byte block while still under 64 words,
 * and then the command never runs.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/access.h>

#define MAXARG 64		/* the shell's limit, in words per command */
#define ARGMAX 2048		/* exec's arg block: NBLKS(4) * 512 bytes */

char *pathv[] = { ".", "/bin", "/usr/bin", 0 };

char *cmdv[MAXARG];		/* the command and its accumulated args */
int nav;			/* how many of cmdv[] are live */
int nfixed;			/* the command plus its own arguments */
int nbytes;			/* strlen+1 of every live cmdv[] entry */

char *savestr();
char *findcmd();
int cmdbytes();
void run();

main(argc, argv)
    int argc;
    char **argv;
{
    char word[256];
    int i;

    if (argc < 2) {
        fprintf(stderr, "usage: xargs command [argument ...]\n");
        exit(1);
    }
    cmdv[nav++] = findcmd(argv[1]);
    for (i = 2; i < argc; i++)
        cmdv[nav++] = argv[i];
    nfixed = nav;
    nbytes = cmdbytes();

    while (fgets(word, sizeof word, stdin) != 0) {
        i = strlen(word);
        if (i > 0 && word[i - 1] == '\n')
            word[i - 1] = 0;
        if (nav >= MAXARG - 1 || nbytes + strlen(word) + 1 > ARGMAX)
            run();
        cmdv[nav++] = savestr(word);
        nbytes += strlen(word) + 1;
    }
    if (nav > nfixed)
        run();
    exit(0);
}

/*
 * The argument block this batch would build: each live word plus its
 * NUL terminator.  The kernel counts the command's own name in this too,
 * and cmdv[0] is that name, so it is summed here along with the rest.
 */
int
cmdbytes()
{
    int i, n;

    for (n = 0, i = 0; i < nav; i++)
        n += strlen(cmdv[i]) + 1;
    return n;
}

/*
 * Run the accumulated command, then drop the stdin words and start the
 * batch over from the fixed prefix.
 */
void
run()
{
    int pid;
    int i;

    if ((pid = fork()) == 0) {
        cmdv[nav] = 0;
        exec(cmdv[0], cmdv);
        exit(1);
    }
    wait((int *)0);
    for (i = nfixed; i < nav; i++)
        free(cmdv[i]);
    nav = nfixed;
    nbytes = cmdbytes();
}

char *
savestr(s)
    char *s;
{
    char *p;

    p = malloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

/*
 * Find a command: a name with a slash is used as-is, otherwise the
 * directories of pathv are searched, the way the shell does.
 */
char *
findcmd(name)
    char *name;
{
    static char buf[128];
    int i;

    if (strchr(name, '/'))
        return name;
    for (i = 0; pathv[i]; i++) {
        strcpy(buf, pathv[i]);
        strcat(buf, "/");
        strcat(buf, name);
        if (access(buf, A_EXEC) == 0)
            return buf;
    }
    return name;
}
