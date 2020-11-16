/*
 * Copyright (c) 1985 by Morris Code Works
 * UNIX like 'make' command
 * Revision History
 *      When    Who     Description
 * 06 Aug 85    TGM     Module created
 * 27 Aug 85    TGM     Released for use
 * 29 Oct 20    CMM     substantially rewritten for correctness,
 *                      unix portability and speed
 * 30 Oct 20    CMM     runs on micronix, upward compatible
 *                      with stock make
 * make.c
 */
#include	"make.h"

#include	<stdio.h>

#ifdef linux
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <stat.h>
#endif

extern struct stat statb;

/*
 * global data definitions 
 */
struct target *targets = NULL;
struct macro *macros = NULL;
struct work *dolist = NULL;
extern char exbuf[];

char execute = 1;               /* actually build stuff */
char silent = 0;                /* show commands */

char knowhow = 0;               /* know how to make file */
char madesomething = 0;         /* actually made a file */

#ifndef linux
char *
strdup(s)
char *s;
{
    char *d;
    int i;
    i = strlen(s);
    d = calloc(1, i+1);
    strcpy(d, s);
    return d;
}

char *
index(s, c)
char *s;
char c;
{
    while (*s) {
        if (*s == c) return s;
        s++;
    }
    return 0;
}

#endif

/*
 * malloc failed - big lose
 */
OutOfMem()
{
    fprintf(stderr, "Insufficient memory for make.\n");
    fprintf(stderr, "Try reducing size of makefile.\n");
    exit(1);
}

/*
 * make - a  Unix-like utility
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    long make();                /* 'make' files */
    FILE *fopen();              /* standard file-open */
    struct target *t;

    /*
     * go initialize data structures 
     */
    init(argc, argv);

    if (!targets) {
        fprintf(stderr, "no targets\n");
        exit(1);
    }

    /*
     * if no name was given use the first definition 
     */
    if (!dolist) {
        if (!(dolist = (struct work *) calloc(1, sizeof(struct work))))
            OutOfMem();
        for (t = targets; t->next; t = t->next);
        if (!(dolist->name = strdup(t->name)))
            OutOfMem();
    }

    /*
     * now fall down the dolist and do them all 
     */
    while (dolist) {
        /*
         * haven't made anything yet 
         */
        madesomething = 0;
        /*
         * try to make something 
         */
        make(dolist->name);
        if (!madesomething) {
            if (knowhow)
                fprintf(stderr, "make: %s is up to date.\n", dolist->name);
            else
                fprintf(stderr, "make: don't know how to make %s.\n",
                    dolist->name);
        }
        dolist = dolist->next;
    }
}

#define MAXARGS 50

/*
 * this function replaces system(), which fires off a subshell
 * that's just a silly waste of time and memory
 * the hard part is globbing
 */
char *paths[] = {
    ".",
    "/bin",
    "/usr/bin",
    0
};

int
docmd(s)
    char *s;
{
    int ret;
    int child;
    int status;
    int pid = 0;
    char *args[MAXARGS];
    register unsigned char i;
    char *fn = s;
    char path[30];
    char *p;

    /*
     * if there's a something a simple exec can't do, use system() 
     */
    for (p = s; *p; p++) {
        if ((*p == '*') || (*p == ';') || (*p == '>') || (*p == '<'))
            break;
    }
    if (*p) {
        ret = system(s);
        return ret;
    }

    child = fork();
    if (child == 0) {

        nice(0);
        for (i = 0; i < MAXARGS; i++) {
            args[i] = s;
            /*
             * skip to null or space 
             */
            while (*s && (*s != ' ')) {
                s++;
            }
            if (*s) {
                *s++ = '\0';
            }
            if (!*s)
                break;
        }
        args[++i] = 0;
        for (i = 0; paths[i]; i++) {
            sprintf(path, "%s/%s", paths[i], fn);
            if (access(path, 1) == -1)
                continue;
            if (stat(path, &statb) == -1)
                continue;
            execv(path, args);
        }
        fprintf(stderr, "could not locate %s\n", fn);
        exit(1);
    } else {
        while (pid != child) {
            pid = wait(&status);
        }
        ret = status >> 8;
    }
    return (ret);
}

long
make(s)                         /* returns the modified date/time */
    char *s;
{
    struct target *t;
    struct dep *d;
    struct command *howp;
    long latest;                /* current 'latest' file time */
    char *expand();             /* expand macros */
    char *mod;                  /* list of files needing 'making' */
    char *cmd;
    long tt;
    int ret;

    /*
     * run through the explict targets and see if we find a rule
     */
    for (t = targets; t; t = t->next) {
        if (!strcmp(t->name, s)) {
            break;
        }
    }

    if (!t) {
        knowhow = 0;
        latest = FileTime(s);
        if (latest == 0) {
            /*
             * doesn't exist but don't know how to make it 
             */
            fprintf(stderr, "make: don't know how to make %s\n", s);
            exit(1);
        } else {
            if (verbose > 2)
                printf("%s exists but no recipe or dependencies\n", s);
            return (latest);
        }
    }

    if (verbose > 1)
        printf("making: %s\n", s);

    /*
     * if file is up to date 
     */
    if (t->current) {

        if (verbose > 1)
            printf("current\n");

        /*
         * return actual modification time 
         */
        return (t->modified);
    }

    /*
     * make sure everything it depends on is up to date 
     */
    latest = 0;

    /*
     * no files need 'making' yet 
     */
    if (!(mod = calloc(1, 1)))
        OutOfMem();

    /*
     * grunge through the dependencies
     */
    for (d = t->need; d; d = d->next) {
        unsigned long tv;

        if (verbose > 1)
            printf("%s depends on %s\n", t->name, d->name);

        tt = make(d->name);
        tv = FileTime(d->name);
        if (tt > latest)
            latest = tt;
        if (tv > t->modified) {

            if (verbose > 1)
                printf("build %s because %s is newer (%s < %s)\n",
                    t->name, d->name, 
                    PTime(t->modified), PTime(tv));

            if (!(mod = realloc(mod, strlen(mod) + strlen(d->name) + 2)))
                OutOfMem();

            /*
             * add this name to the list 
             */
            if (strlen(mod) > 0)
                strcat(mod, " ");
            strcat(mod, d->name);
        }
    }

    /*
     * has dependencies therefore we know how 
     */
    knowhow = 1;

    /*
     * if necessary, execute all of the commands to make it 
     * if ( out of date ) || ( depends on nothing ) 
     */
    if ((latest > t->modified) || (!t->need)) {
        /*
         * make these guys 
         */
        for (howp = t->recipe; howp; howp = howp->next) {

            expand(howp->text, s, mod);
            cmd = exbuf;
            if (*cmd != '@') {
                printf("%s\n", exbuf);
            }
            if (execute) {
                if (*cmd == '@')
                    ++cmd;
                ret = docmd(cmd);
            }
        }
        /*
         * file has now been modified 
         */
        t->modified = CurrTime();

        if (verbose > 1)
            printf("set time of %s to (%s)\n", t->name, PTime(t->modified));

        t->current = 1;
        if (t->recipe) {
            madesomething = 1;
        }
    }

    /*
     * return the update file time 
     */
    free(mod);
    return (t->modified);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
