/*
 * sh.h - what the shell passes between its pieces
 *
 * micronix/cmd/sh/sh.h
 *
 * This is the shape the parser produces and everything downstream of
 * it consumes.  See NOTES for where each of these came from in the
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
#define MAXPATHV    8       /* the image has three: . /bin /usr/bin */

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
 * A pipeline is what one statement turns into.  "a | b | c" is three
 * commands; the plain case is one.  A line can hold several
 * statements, separated by ";" - see runline().
 */
struct pipeline {
    struct cmd  cmd[MAXCMD];
    int         ncmd;
};

/*
 * The builtins, numbered as the binary numbers them - the table at
 * 0x9755 pairs each name with one of these.  cd and chdir are the
 * same builtin, which is why the binary has eighteen names and
 * seventeen numbers.
 *
 * The numbers are kept where the binary put them, so 2, 3, 7, 8 and
 * 14 are gaps.  7 and 14 were already: the binary numbers around them
 * and nothing is missing.  2, 3 and 8 are ours - dir, era and sync
 * have left.
 *
 * What earns a place here is needing the shell's own state.  cd moves
 * the shell, exit ends it, source redirects where it reads from, and
 * alias, path, prompt and home are its tables; wait and pid are about
 * its children and itself; nice sets what its children inherit.  dir
 * and era needed none of it - they are cp/m spellings, seeded into
 * the alias table in main() where a user can redefine them - and sync
 * is a system call with a program of its own in cmd/sync.
 */
#define B_CD        1
#define B_WAIT      4
#define B_EXIT      5
#define B_ECHO      6
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

/*
 * What dobuiltin says when the word names a builtin but the work is
 * an ordinary command after all: dir is ls, and nice sets the
 * priority and then wants the rest of the line run.  Both rewrite the
 * command and leave the running to execute().
 *
 * Not 1, which is what it returned for an unknown code as well.
 * execute() handed that straight back to its caller, so dir - whose
 * whole body was "return 1" with a comment saying the external ls
 * would pick it up - did nothing at all.
 */
#define B_ASCOMMAND (-2)

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
extern char *pathv[MAXPATHV];

int  isbuiltin();
int  dobuiltin();
int  execute();
int  sourcefile();
void aliasexpand();
int  pushredir();
void popredir();
int  runpipeline();
void runline();
int  nextline();
char *strsave();
void fatal();
void warn();

/* parse.c
 *
 * parse() takes ONE STATEMENT off the front of a line and fills in a
 * pipeline, leaving *pp where the next one starts.  It returns the
 * number of commands, 0 when there is no statement left, or -1 on a
 * syntax error, having already complained.  runline() is the loop
 * around it.
 *
 * It knows "<>&|", ";" and double quotes.  The set the tokeniser
 * really scans against is at 0x17c0 and has a backtick and an open
 * paren in it as well; 0x17cd, the four operators, is a pointer into
 * the tail of that same string.  So backticks and "( )" are still
 * missing here, and single quoting is here without being in the image
 * at all.  NOTES has the behaviour of each, read by running it.
 */
int  parse(/* char **pp, struct pipeline *p */);
