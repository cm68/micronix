/*
 * Copyright (c) 1985 by Morris Code Works
 *
 * make.h
 */

#define	INMAX	132             /* longest line read */
#define	TRUE	1
#define	FALSE	0

typedef char boolean;

/*
 * makefile record types
 */
#define	UNKNOWN	0               /* unknown record */
#define	TARGET	1               /* target record */
#define	RECIPE	2               /* how to make record */
#define	MACRO	3               /* macro definition record */
#define	COMMENT	4               /* a line to ignore */

/*
 * a build target, along with all it's dependencies
 * when we look at what we want to build, we traverse
 * all the dependencies for the target, and if any of them
 * are out of date, we run the recipe
 */
struct target {
    char *name;
    boolean current;
    unsigned long modified;
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
