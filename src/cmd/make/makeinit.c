/*
 * Copyright (c) 1985 by Morris Code Works
 *
 * makeinit.c
 */
#include	<stdio.h>
#include	"make.h"

extern boolean debug;
extern boolean execute;

extern struct target *targets;
extern struct macro *macros;

/*
 * initialize structures 
 */
init(argc, argv)
    int argc;
    char *argv[];
{
    int i;                      /* parameter index */
    boolean usedefault = TRUE;  /* assume dflt file */
    boolean readmakefile();     /* process a 'make' file */

    /*
     * scan thru all supplied parameters 
     */
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            /*
             * decode this option 
             */
            switch (toupper(argv[i][1])) {
            case 'D':
                /*
                 * show everything on the next 'make' file 
                 */
                debug = TRUE;
                break;

            case 'F':
                /*
                 * use a special 'make' file 
                 */
                if (++i < argc) {
                    if (readmakefile(argv[i], 1) == FALSE)
                        exit(1);
                    usedefault = FALSE;
                } else
                    usage();
                break;

            case 'N':
                /*
                 * this is only a dry run 
                 */
                execute = FALSE;
                break;

            default:
                /*
                 * give a hint at how to use 
                 */
                usage();
                break;
            }
        } else
            /*
             * add this file to do list 
             */
            add_to(argv[i]);
    }

    /*
     * check if any files were read 
     */
    if (usedefault) {
        /*
         * read the default file if not 
         */
        if (readmakefile("makefile", 0) == FALSE) {
            if (readmakefile("Makefile", 1) == FALSE) {
                exit(1);
            }
        }
    }
}

/*
 * display all definitions 
 */
debugmode()
{
    struct macro *m;
    struct target *t;
    struct dep *d;
    struct command *cmd;
    char *ListTime();
    char time[20];
    int i;

    /*
     * first do the macro definitions 
     */
    fprintf(stderr, "\nMacros:\n");
    for (m = macros; m; m = m->next)
        fprintf(stderr, "\t%s = %.60s\n", m->name, m->text);

    /*
     * now do the target definitions 
     */
    for (t = targets; t; t = t->next) {
        /*
         * tell which file we're talking about 
         */
        ListTime(t->modified, time);
        fprintf(stderr, "\nFile(%s): Modified(%s)\n  depends on:",
            t->name, time);

        /*
         * display the dependencies 
         */
        i = 0;
        for (d = t->need; d; d = d->next) {
            fprintf(stderr, "%c%s", (i == 0 ? '\t' : ' '), d->name);
            if ((i += strlen(d->name) + 1) > 44) {
                if (d->next)
                    fprintf(stderr, "\n\t");
                i = 0;
            }
        }

        /*
         * display the associated commands 
         */
        fprintf(stderr, "\n  commands:\n");
        for (cmd = t->recipe; cmd; cmd = cmd->next)
            fprintf(stderr, "\t%.70s\n", cmd->text);
    }
}

/*
 * explain how to use make 
 */
usage()
{
    fprintf(stderr,
        "usage: make [-d] [-n] [-f makefile] [filename ...]\n");
    exit(1);
}

/*
 * add_to : add name to do_list 
 */
add_to(s)
    char *s;
{
    struct work *w;             /* ptr to this 'do' record */
    extern dolist;              /* list of names to 'make' */
    char *lc();                 /* convert to lowercase */

    /*
     * get a pointer to newly allocated structure 
     */
    if (!(w = (struct work *) calloc(1, sizeof(struct work))))
        OutOfMem();

    w->name = lc(s);

    w->next = dolist;
    dolist = w; 
}

char *
lc(s)                           /* convert string to lower case */
    char *s;
{
    char *str;

    for (str = s; *str = tolower(*str); ++str);

    return (s);
}

/*
 * ok, let's look through the suffix rules to see if any of them can be made
 * to match any targets that we don't have a recipe for.
 */
suffixes()
{
    struct target *t;
    struct dep *d;

    for (t = targets; t; t = t->next) {
        if (*t->name != '%')
            continue;
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
