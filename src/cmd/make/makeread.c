/*
 * Copyright (c) 1985 by Morris Code Works
 *
 * makeread.c
 */
#include	<stdio.h>
#include	<ctype.h>
#include	"make.h"

extern struct target *targets;
extern struct macro *macros;

int lineno = 1;

/*
 * a word about memory management:
 * input and macro expansions in the parse path are in fixed length static arrays
 * dependency trees and macro definitions are malloc'd.
 */
char inbuf[1024];       /* input buffer */
char exbuf[1024];       /* macro expanded */

/*
 * a definition line can have up to NDEFS targets before the colon
 */
#define NDEFS   32

#define NLEN
char namebuf[NLEN];

/*
 * place a name into the namebuf
 * updating the incoming pointer
 * return pointing at a delimiter:  null, space, : or = 
 */
snagname(char **in)
{
    char c;
    char *s = namebuf;

    while (1) {
        c = **in;
        if ((c == '\0') || ( c == ':') || (c == '=') || (c == ' '))
            break;
        (*in)++;
        *s++ = c;
    }
    *s = '\0';
}

/*
 * read the makefile 
 */
readmakefile(s)
    char *s;                    /* filename to read */
{
    FILE *infile;               /* fp for input file */
    char *p;                    /* input line pointer */
    struct command *rp;         /* recipe we are constructing */
    struct command *rt;         /* tail of recipe */
    int tcnt;
    struct target *lt[NDEFS];   /* targets on line */
    struct target *t;
    struct macro *m;
    struct dep *d;
    int i;

    /*
     * try to open specified file 
     */
    if ((infile = fopen(s, "r")) == 0) {
        fprintf(stderr, "make: unable to open %s.\n", s);
        return (FALSE);
    }

    rt = NULL;

    /*
     * process the file 
     */
    while (readline(infile)) {

        p = inbuf;
 
        /*
         * check for definitions and macros 
         */
        switch (linetype()) {

        /* grammar: <whitespace> [# comment] */
        case COMMENT:
            break;              /* ignore comments */

        /* grammar: <macro> = <definition> */
        case MACRO:
            tcnt = 0;
            snagname(&p);

            /*
             * search if we already know this definition
             */
            for (m = macros; m; m = m->next) {
                if (!strcmp(m->name, namebuf))
                    break;
            }

            /*
             * if a macro is already defined, redefine 
             */
            if (m) {
                free(m->text);
                m->text = 0;
            } else {
                /*
                 * add new struct to the end of maclist 
                 */
                if (!(m = (struct macro *) calloc(1, sizeof(struct macro))))
                    OutOfMem();
                if (!(m->name = strdup(namebuf)))
                    OutOfMem();
                m->next = macros;
                macros = m;
            }

            /*
             * copy in macro definition - we know there is an '=' in the line
             */
            p = index(p, '=') + 1;
            while (isspace(*p)) p++;
            if (!(m->text = strdup(p)))
                OutOfMem();
            break;

        /*
         * grammar: <target> [target] ... : <dependency> [<dependency>] ...
         */
        case TARGET:
            rt = NULL;
            tcnt = 0;

            /* expand any macros in the targets or dependencies */
            expand(inbuf, "", "");
            p = exbuf;

            /* we know there is a colon, so guaranteed to terminate */
            do {

                snagname(&p);

                while (isspace(*p)) p++;

                /*
                 * search to see if we know anything about this target
                 */
                for (t = targets; t; t = t->next) {
                    if (!strcmp(t->name, namebuf))
                        break;
                }

                /*
                 * create a NEW definition record 
                 */
                if (tcnt == NDEFS) {
                    fprintf(stderr, "too many definitions on line %d\n", lineno);
                    return (FALSE);
                }

                if (!t) {
                    if (!(t = (struct target *)calloc(1, sizeof(struct target))))
                        OutOfMem();
                    if (!(t->name = strdup(namebuf)))
                        OutOfMem();
                    t->current = FALSE;
                    t->modified = FileTime(t->name);
                    t->next = targets;
                    targets = t;
                }

                lt[tcnt++] = t;
            } while (*p != ':');

            p++;
            while (isspace(*p)) p++;

            /*
             * now go thru all of the dependencies
             */
            while (*p) {
                while (isspace(*p)) p++;

                if (*p == '\0') break;

                snagname(&p);

                /* add them to each entry in the lt array */
                for (i = 0; i < tcnt; i++) {
                    /* make sure it's not already there */
                    for (d = lt[i]->need; d; d = d->next) {
                        if (!strcmp(d->name, namebuf)) {
                            break;
                        }
                    }
                    if (d) {
                        /* might want to complain - it will use the first recipe */
                        continue;
                    }
                    if (!(d = (struct dep *) calloc(1, sizeof(struct dep))))
                        OutOfMem();
                    if (!(d->name = strdup(namebuf)))
                        OutOfMem();
                    d->next = lt[i]->need;
                    lt[i]->need = d;
                }
            }

            break;

        /*
         * these are all attached to the current targets
         * and consecutive RECIPE lines append to the current recipe
         */
        case RECIPE:
            if (!tcnt) {
                fprintf(stderr, "make: line %d recipe without target.\n", lineno);
                break;
            }

            while (isspace(*p)) ++p;

            /*
             * if there is something there, allocate mem and copy 
             */
            if (strlen(p)) {
                if (!(rp = (struct command *) calloc(1, sizeof(struct command))))
                    OutOfMem();
                if (!(rp->text = strdup(p)))
                    OutOfMem();
                if (rt) {
                    rt->next = rp;
                } else {
                    for (i = 0; i < tcnt; i++) {
                        lt[i]->recipe = rp;
                    }
                }
                rt = rp;
            }
            break;

        case UNKNOWN:
        default:
            /*
             * unrecognized line 
             */
            fprintf(stderr, "make: unrecognized line %d\n", lineno);
            break;
        }
    }
    return (TRUE);
}

/*
 * expand macros on line 
 */
expand(str, name, mod)
    char *str;                  /* string to expand */
    char *name;                 /* name we're making */
    char *mod;                  /* names that caused make */
{
    char *c, *c1, *c2;          /* working buffer pointers */
    char *mp, mac[32];          /* macroo expansion ptr, buffer */
    char *index();              /* locate character function */
    struct macro *macp;        /* working macro struct ptr */

    /*
     * see if any expansion needed 
     */
    if (!index(str, '$')) {
        strcpy(exbuf, str);
        return;
    }

    /*
     * let's get the show on the road 
     */
    c1 = exbuf;
    c2 = str;

    /*
     * while there is any data left... 
     */
    while (*c2) {
        if (*c2 != '$')
            *c1++ = *c2++;
        else {
            /*
             * assume there is no true macro definition 
             */
            mac[0] = NULL;

            /*
             * decode the character following the '$' 
             */
            switch (*++c2) {
            case '$':          /* $$ = just a dollar sign */
                *c1++ = '$';
                break;

            case '@':          /* $@ = copy 'make' name */
                for (c = name; *c;)
                    *c1++ = *c++;
                break;

            case '*':          /* $* = copy 'make' name prefix */
                for (c = name; *c && *c != '.';)
                    *c1++ = *c++;
                break;

            case '?':          /* $? = copy the cause of the make */
                for (c = mod; *c;)
                    *c1++ = *c++;
                break;

            case '(':          /* $(xx) = a long macro definition */
                for (mp = mac; *++c2 != ')'; *mp++ = *c2);      /* copy the
                                                                 * macro name 
                                                                 * (up to
                                                                 * ')') */
                *mp = NULL;     /* terminate macro name */
                break;

            default:           /* $<other> = a short macro definition */
                mac[0] = *c2;   /* move in the macro definition */
                mac[1] = NULL;  /* terminate the string */
                break;
            }

            /*
             * bypass the character which caused the termination 
             */
            ++c2;

            /*
             * expand macro if one has been defined 
             */
            if (strlen(mac)) {
                /*
                 * look for matching macro definition 
                 */
                macp = macros;
                while (macp) {
                    /*
                     * look for matching string name 
                     */
                    if (!strcmp(macp->name, mac)) {
                        mp = macp->text;
                        /*
                         * copy in macro definition 
                         */
                        while (*mp)
                            *c1++ = *mp++;
                        break;
                    }
                    macp = macp->next;
                }
            }
        }
    }

    /*
     * terminate the EXPANDED string 
     */
    *c1 = NULL;
}

/*
 * read logical input line 
 * this simply reads physical lines until we get one that is not
 * terminated by a backslash
 * return 0 for eof or overflow
 */
readline(fp)
    FILE *fp;                   /* input file pointer */
{
    char *fgets();              /* read input from file */
    char *p;                    /* working string pointer */
    int i;

    /*
     * initialize buffer ptr 
     */
    p = inbuf;

    while (p < &inbuf[sizeof(inbuf)]) {

        /*
         * get a single physical line 
         */
        if (!fgets(p, sizeof(inbuf) - (p - inbuf), fp)) {
            return 0;
        }
        lineno++;

        i = strlen(inbuf);

        /*
         * get rid of trailing newline 
         */
        inbuf[i - 1] = 0;

        /*
         * see if more data to read 
         */
        if (inbuf[i - 2] != '\\') {
            return 1;
        }

        p = &inbuf[i - 1];
    }
    fprintf(stderr, "line %d: too long\n", lineno);
    return 0;
}

/*
 * return type of input 
 * if it's got an equal, then it's a macro definition
 * if it's got a colon, it's a dependency list
 * if starts with a tab, it's part of a recipe
 */
linetype()
{
    char *cp = inbuf;
    char *index();

    /*
     * decode buffer type... simple rules 
     */
    /* empty line and comments */
    if (*cp == NULL || *cp == '#')
        return (COMMENT);

    /* starting tab is a recipe line */
    else if (*cp == '\t')
        return (RECIPE);

    /* leading non-tab is an error */
    else if (isspace(*cp))
        return (UNKNOWN);

    /* look for an equals sign */
    if (index(cp, '=')) {
        return MACRO;
    }

    if (index(cp, ':')) {
        return TARGET;
    }
    return (UNKNOWN);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
