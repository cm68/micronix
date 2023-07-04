/*
 * Copyright (c) 1985 by Morris Code Works
 *
 * cmd/Make/makeread.c
 * Changed: <2023-07-02 23:45:34 curt>
 */
 
#include	<stdio.h>
#ifdef nodef
#include	<ctype.h>
#endif
#include	"make.h"

extern char knowhow;

extern struct target *targets;
extern struct macro *macros;

extern char verbose;

int lineno = 1;

/*
 * a word about memory management:
 * input and macro expansions in the parse path are in fixed length
 * static arrays; dependency trees and macro definitions are malloc'd.
 */
#define MAXLINE 1024
char inbuf[MAXLINE] INIT;       /* input buffer */
char exbuf[MAXLINE] INIT;       /* macro expanded */

/*
 * a definition line can have up to NDEFS targets before the colon
 */
#define NDEFS   100

#define NLEN    32
char namebuf[NLEN] INIT;
char workbuf[NLEN] INIT;

/*
 * all whitespace inside a line is identical. 
 * a tab at the start of a line is significant
 */
whitespace(c)
char c;
{
    if ((c == ' ') || (c == '\t'))
        return 1;
    return 0;
}

/*
 * place a name into the namebuf
 * updating the incoming pointer
 * return pointing at a delimiter:  null, space, : or = 
 */
snagname(in)
char **in;
{
    char c;
    char *s = namebuf;

    while (1) {
        c = **in;
        if ((c == '\0') || ( c == ':') || 
            (c == '=') || (c == ' ') || ( c == '\t'))
            break;
        (*in)++;
        *s++ = c;
    }
    *s = '\0';
}

/*
 * read the makefile 
 */
readmakefile(s, report)
    char *s;                    /* filename to read */
    int report;
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
    long ft;
    char *semi;
    char *w;

    /*
     * try to open specified file 
     */
    if ((infile = fopen(s, "r")) == 0) {
        if (report) {
            fprintf(stderr, "make: unable to open %s.\n", s);
            exit(1);
        }
        return 0;
    }

    rt = NULL;

    /*
     * process the file 
     * there's one syntactic oddity:
     * lines of the form:  foo.o : foo.c ; cc -c foo.c
     * are valid, even with no dependencies 
     */
    while (readline(infile)) {

        /* a # anywhere in the line becomes a line end */
        for (p = inbuf; *p; p++) {
            if (*p == '#') {
                *p = 0;
                break;
            }
        }

        semi = NULL;
        p = inbuf;
         
        i = linetype();
        if (verbose > 2) {
            printf("linetype: %d %s\n", i, inbuf);
        }

        /*
         * check for definitions and macros 
         */
        switch (i) {

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
             * replace redundant white space in macro def with a space
             */
            p = index(p, '=') + 1;
            while (whitespace(*p)) p++;
            if (!(m->text = calloc(1, strlen(p)+1)))
                OutOfMem();
            for (w = m->text; *p;) {
                if (whitespace(*p)) {
                    *w++ = ' ';
                    while (whitespace(*p)) p++;
                } else {
                    *w++ = *p++;
                }
            }
            *w = '\0';
            if (verbose > 2) {
                printf("macdef %s\n", m->name);
                /* hexdump(m->text, strlen(m->text)); */
            }
            break;

        /*
         * grammar: <target> [target] ... : [<dependency> ...] [; recipe ]
         */
        case TARGET:
            rt = NULL;
            tcnt = 0;

            /*
             * put a null at the semicolon, and we'll handle after as recipe
             */
            if ((semi = index(inbuf, ';'))) {
                *semi++ = '\0';
            }
            
            /* expand any macros in the targets or dependencies */
            expand(inbuf, 0, 0);

            if (verbose > 2) {
                printf("expanded: %s\n", exbuf);
                /* hexdump(exbuf, strlen(exbuf)); */
            }
            p = exbuf;

            /* we know there is a colon, so guaranteed to terminate */
            do {

                snagname(&p);

                while (whitespace(*p)) p++;

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
                    fprintf(stderr, "line %d\: too many definitions", lineno);
                    return (0);
                }

                if (!t) {
                    if (!(t = (struct target *)calloc(1, sizeof(struct target))))
                        OutOfMem();
                    if (!(t->name = strdup(namebuf)))
                        OutOfMem();
                    t->current = 0;
                    ft = FileTime(t->name);
                    t->modified = ft;
                    if (verbose > 1)
                        fprintf(stderr, "set time of %s to (%s)\n", 
                            t->name, PTime(ft));
                    t->next = targets;
                    targets = t;
                }

                lt[tcnt++] = t;
            } while (*p != ':');

            p++;
            while (whitespace(*p)) p++;

            /*
             * now go thru all of the dependencies
             */
            while (*p) {
                while (whitespace(*p)) p++;

                if (*p == '\0') break;

                snagname(&p);

                /* printf("dep: %s(%d)\n", namebuf, strlen(namebuf)); */
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

                    /*
                     * we do a little magic here if the dependency looks like $*<something>
                     * in that we substitute the target prefix in for the $*
                     * this turns  foo.o : $*.c into foo.o : foo.c
                     * this is useful when you have a target list
                     */
                    w = namebuf;
                    if ((namebuf[0] == '$') && (namebuf[1] == '*')) {
                        strcpy(workbuf, lt[i]->name);
                        if (verbose > 2)
                            printf("magic: pat: %s target: %s\n", namebuf, workbuf);
                        for (w = workbuf; *w && (*w != '.'); w++)
                            ;
                        strcpy(w, &namebuf[2]);
                        w = workbuf;
                    }

                    /* if we have foo.o: foo.o, just skip it */
                    if (strcmp(w, lt[i]->name) == 0) {
                        printf("circular dependency line %d: %s\n",
                            lineno, d->name);
                        continue;
                    }
                    if (!(d = (struct dep *) calloc(1, sizeof(struct dep))))
                        OutOfMem();
                    if (!(d->name = strdup(w)))
                        OutOfMem();
                    d->next = lt[i]->need;
                    lt[i]->need = d;
                }
            }

            if (semi) {
                p = semi;
                /* fall through */
            } else {
                break;
            }

        /*
         * these are all attached to the current targets
         * and consecutive RECIPE lines append to the current recipe
         */
        case RECIPE:
            if (!tcnt) {
                fprintf(stderr, "make: line %d recipe without target.\n", lineno);
                break;
            }

            while (whitespace(*p)) ++p;

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
            fprintf(stderr, inbuf);
            break;
        }
    }
    return (1);
}

char *
mactext(mname)
char *mname;
{
    struct macro *macp;
    char *ret = 0;

    for (macp = macros; macp; macp = macp->next) {
        if (strcmp(macp->name, mname) == 0) {
            ret = macp->text;
            break;
        }
    }
    if (verbose > 2) {
        printf("mactext: %s\n", mname, ret ? ret : "undefined");
        /* if (ret) hexdump(ret, strlen(ret)); */
    }
    return ret;
}

/*
 * expand macros on line 
 */
expand(str, name, mod)
    char *str;                  /* string to expand */
    char *name;                 /* name we're making */
    char *mod;                  /* names that caused make */
{
    char mac[32];               /* macro name */
    char *index();              /* locate character function */
    char expanded;
    char *s;
    char *mtext;
    char *dest;
    char *src;
    char c;

    if (verbose > 2) {
        printf("\nexpand \"%s\" name: \"%s\" mod: \"%s\"\n", 
            str, name?name:"", mod?mod:"");
    }

    /*
     * since macros can contain macros, we need to loop
     * this extra buffer copying is not ideal
     */
    strcpy(exbuf, str);

    do {

        expanded = 0;

        strcpy(inbuf, exbuf);

        if (verbose > 3) {
            printf("expand pass\n");
            /* hexdump(inbuf, strlen(inbuf)); */
        }

        /*
         * let's get the show on the road 
         */
        dest = exbuf;
        src = inbuf;

        /*
         * while there is any data left... 
         */
        while (*src) {

            if (*src != '$') {
                *dest++ = *src++;
                *dest = '\0';
                continue;
            }

            /*
             * now we know we have a '$'.
             */
            mtext = 0;
            src++;
            c = *src++;

            switch (c) {
            case '$':          /* $$ = just a dollar sign */
                mtext = "$$";
                break;

            case '@':          /* $@ = copy 'make' name */
                if (name) {
                    mtext = name;
                } else {
                    mtext = "$@";
                }
                break;

            case '*':          /* $* = copy 'make' name prefix */
                if (name) {
                    mtext = mac;
                    s = name;
                    while (*s && *s != '.') {
                        *mtext++ = *s++;
                    }
                    *mtext = '\0'; 
                    mtext = mac;
                } else {
                    mtext = "$*";
                }
                break;

            case '?':          /* $? = copy the cause of the make */
                if (mod) {
                    mtext = mod;
                } else {
                    mtext = "$?";
                }
                break;

            case '(':          /* $(xx) = a long macro definition */
                s = mac;
                *s = '\0';
                while (*src != ')') {
                    if (!*src) {
                        fprintf(stderr, "unterminated macro call\n");
                        fprintf(stderr, "%s\n", inbuf);
                        *dest = '\0';
                        return;
                    }
                    *s++ = *src++;
                    *s = '\0';
                }
                src++;
                mtext = mactext(mac);
                if (mtext) expanded++;
                break;

            default:           /* $<other> = a short macro definition */
                mac[0] = c;
                mac[1] = '\0';
                mtext = mactext(mac);
                if (mtext) expanded++;
                break;
            }

            /*
             * expand macro
             */
            if (mtext) {
                while (*mtext) {
                    *dest++ = *mtext++;
                }
                *dest = '\0';
            }
        }
    } while (expanded);
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

        p = &inbuf[i - 2];
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
    if (*cp == '\0' || *cp == '#' || (*cp == ' ' && cp[1] == '\0'))
        return (COMMENT);

    /* starting tab is a recipe line */
    else if (*cp == '\t')
        return (RECIPE);

    /* leading non-tab is an error */
    else if (*cp == ' ')
        return (UNKNOWN);

    /* look for an equals sign */
    if (index(cp, '=')) {
        return MACRO;
    }

    /* do we have a target */
    if (index(cp, ':')) {
        return TARGET;
    }
    return (UNKNOWN);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
