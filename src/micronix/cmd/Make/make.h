/*
 * Copyright (c) 1985 by Morris Code Works
 *
 * make.h
 */

#ifdef linux
#define INIT
#else
#define INIT = 0
#endif

extern char *PTime();
extern long FileTime();
extern long CurrTime();

#ifndef linux
extern char *strdup();
#endif

/*
 * verbosity:
 * 0: print commands executed
 * 1: dump out definitions
 * 2: trace make reasons
 * 3: kitchen sink
 * 4: truly extreme
 */
extern char verbose;

/*
 * makefile record types
 * whitesmith's c predates enum!
 */
#ifdef linux
enum rectype {
    UNKNOWN, TARGET, RECIPE, MACRO, COMMENT
};
#else
#define UNKNOWN 0
#define TARGET  1
#define RECIPE  2
#define MACRO   3
#define COMMENT 4
#endif

/*
 * a build target, along with all it's dependencies
 * when we look at what we want to build, we traverse
 * all the dependencies for the target, and if any of them
 * are out of date, we run the recipe
 */
struct target {
    char *name;
    char current;                       /* we made it */
    long modified;
    struct dep *need;
    struct command *recipe;
    struct target *next;
};

/*
 * this is a chain of dependencies associated with a single target
 */
struct dep {
    char *name;
    struct target *def;
    struct dep *next;
};

/*
 * a list of commands to run.  these are in order
 * also, the text of the command can have macros to expand
 * also, all the targets on the same dependency line link to the same
 * recipe.  however, the recipe is interpreted differently via
 * macro expansion when building each recipe.
 */
struct command {
    char *text;
    struct command *next;
};

/*
 * we can redefine macros
 */
struct macro {
    char *name;
    char *text;
    struct macro *next;
};

/*
 * this is the list of targets that we want to build
 */
struct work {
    char *name;
    struct work *next;
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
