/*
 * Copyright (c) 1985 by Morris Code Works
 * UNIX like 'make' command
 * Revision History
 *      When    Who     Description
 * 06 Aug 85    TGM     Module created
 * 27 Aug 85    TGM     Released for use
 *
 * 29 Oct 20    CMM     substantially rewritten for correctness,
 *                      unix portability and speed, suffixes
 * make.c
 */
#include	"make.h"
#include	<stdio.h>

/*
 * global data definitions 
 */
struct target *targets = NULL;
struct macro *macros = NULL;
struct work *dolist = NULL;
extern char exbuf[];

boolean execute = TRUE;         /* build submit file */
boolean debug = FALSE;          /* not in debug mode */
boolean silent = FALSE;         /* show commands */

boolean knowhow = 0;                /* know how to make file */
boolean madesomething = 0;          /* actually made a file */

#ifdef CPM
FILE *mfp;                      /* 'make' submit file pointer */
#endif

#ifndef linux
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

char *
malloc(i)
int i;
{
    char *r;

    r = alloc(i);
    return r;
}

char *
strdup(s)
char *s;
{
    int i = strlen(s);
    char *r;
    r = malloc(i);
    strcpy(r, s); 
    return r;
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

    /*
     * process magic suffix rules
     */
    suffixes();

    /*
     * display structures if desired 
     */
    if (debug)
        debugmode();

#ifdef CPM
    /*
     * create submit file 
     */
    if ((mfp = fopen(EXECFILE, "w")) == 0) {
        fprintf(stderr, "make: can't create %s.\n", EXECFILE);
        exit(1);
    }
    if (silent)
        fprintf(mfp, "put console output to file null [system]\n");
#endif

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
        for (t = targets; t->next; t = t->next)
            ;        
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
        madesomething = FALSE;
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

#ifdef CPM
    /*
     * finish up the command file 
     */
    if (silent)
        fprintf(mfp, "put console output to console\n");
    fprintf(mfp, "erase %s\n", EXECFILE);
    fclose(mfp);

    /*
     * see if we made anything 
     */
    if (execute && madesomething) {
        /*
         * chain to command file 
         */
        sprintf(0x0080, "SUBMIT %s\r", EXECFILE);
        bdos(47, 0);
    }
#endif
}

long
make(s)                         /* returns the modified date/time */
    char *s;
{
    struct target *t;
    struct dep *d;
    struct command *howp;
    unsigned long latest;       /* current 'latest' file time */
    unsigned long FileTime();   /* return file 'modified' time */
    unsigned long CurrTime();   /* return 'current' time */
    char *expand();             /* expand macros */
    char *mod;                  /* list of files needing 'making' */
    char *cmd;
    long tt;

    /*
     * look for the definition 
     */
    for (t = targets; t; t = t->next) {
        if (!strcmp(t->name, s))
            break;
    }

    /*
     * don't know how to make it 
     */
    if (!t) {
        knowhow = FALSE;
        latest = FileTime(s);
        if (latest == 0) {
            /*
             * doesn't exist but don't know how to make it 
             */
            fprintf(stderr, "make: can't make %s.\n", s);
            exit(1);
        } else
            return (latest);
    }

    /*
     * if file is up to date 
     */
    if (t->current)
        /*
         * return actual modification time 
         */
        return (t->modified);

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
        tt = make(d->name);
        if (tt > latest) latest = tt;
        if (FileTime(d->name) > t->modified) {
            if (!(mod =
                    realloc(mod,
                        strlen(mod) + strlen(d->name) + 2)))
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
    knowhow = TRUE;

    /*
     * if necessary, execute all of the commands to make it 
     */
    /*
     * if ( out of date ) || ( depends on nothing ) 
     */
    if ((latest > t->modified) || (!t->need)) {
        /*
         * make these guys 
         */
        howp = t->recipe;
        for (howp = t->recipe; howp; howp = howp->next) {
            expand(howp->text, s, mod);
            cmd = exbuf;
            if ((*cmd != '@') || !execute) {
                printf("%s\n", exbuf);

                if (*cmd == '@')
                    ++cmd;
                system(cmd);
#ifdef CPM
                fprintf(mfp, "%s\n", cmd);
#endif
            }
        }
        /*
         * file has now been modified 
         */
        t->modified = CurrTime();
        t->current = TRUE;
        if (t->recipe) {
            madesomething = TRUE;
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
