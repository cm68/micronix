/*
 * sh.h - what the shell passes between its pieces
 *
 * micronix/cmd/sh/sh.h
 *
 * The parser is not written yet.  This defines the shape it has to
 * produce, so that everything downstream of it can be written and
 * read now.  See NOTES for where each of these came from in the
 * binary.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#define MAXLINE     512     /* the line buffer, 0x200 in the binary */
#define MAXARG      64      /* words in one command */
#define MAXCMD      16      /* commands in one pipeline */
#define MAXSRC      16      /* nesting of source/scripts - the stack at */
                            /*   0x9648 holds one FILE * per level */
#define MAXALIAS    32
#define MAXPATH     128

/*
 * One command: its words, and where its three streams go.  A
 * redirection that is not asked for leaves the name null and the fd
 * inherited.
 */
struct cmd {
    char    *argv[MAXARG];  /* null terminated */
    int     argc;
    char    *in;            /* < name    */
    char    *out;           /* > name    */
    int     append;         /* >> rather than > */
    int     both;           /* the & forms: stderr goes where
                             * stdout goes.  |& >& >>& */
    int     bg;             /* & on the end of the pipeline */
};

/*
 * A pipeline is what one input line turns into.  "a | b | c" is three
 * commands; the plain case is one.
 */
struct pipeline {
    struct cmd  cmd[MAXCMD];
    int         ncmd;
};

/*
 * The builtins, numbered as the binary numbers them - the table at
 * 0x9755 pairs each name with one of these.  7 and 14 are not used;
 * they are gaps rather than anything missing.  cd and chdir are the
 * same builtin, which is why there are eighteen names and seventeen
 * numbers.
 */
#define B_CD        1
#define B_DIR       2
#define B_ERA       3
#define B_WAIT      4
#define B_EXIT      5
#define B_ECHO      6
#define B_SYNC      8
#define B_PROMPT    9
#define B_PID       10
#define B_TYPE      11
#define B_SOURCE    12
#define B_PATH      13
#define B_HOME      15
#define B_KILL      16
#define B_ALIAS     17
#define B_UNALIAS   18
#define B_NICE      19

struct builtin {
    char    *name;
    int     code;
};

extern struct builtin builtins[];

/* sh.c */
extern int  login;          /* argv[0] began with '-'  (0x9004) */
extern int  verbose;        /* -v */
extern int  status;         /* what exit is given      (0x9008) */
extern char *prompt;
extern char *homedir;
extern char pathlist[MAXPATH];

int  lookup();
int  dobuiltin();
int  execute();
int  runpipeline();
int  nextline();
char *strsave();
void fatal();
void warn();

/* parse.c - NOT WRITTEN.  See NOTES.
 *
 * parse() takes one input line and fills in a pipeline.  It returns
 * the number of commands, 0 for a blank line, or -1 on a syntax
 * error, having already complained.  The metacharacters it has to
 * know are "<>&|" - the binary keeps them as a string at 0x17cd -
 * plus quoting and backquote substitution.
 */
int  parse(/* char *line, struct pipeline *p */);
