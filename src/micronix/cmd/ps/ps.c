/*
 * ps - process status
 *
 * cmd/ps/ps.c
 *
 * The 1982 original.  It came out of sys/UNUSED - now sys/attic - and
 * before that off a floppy owned by a Micronix developer.  This is
 * that program ported: the logic is unchanged, and what follows is
 * every place the twenty years between the source and this tree
 * forced a difference.
 *
 *	- The includes.  The original asked for <sys/types.h> and the
 *	  flat /usr/include of 1982.  There is no <sys/types.h> here -
 *	  the host build of cmd/ar wants it and guards it with #ifdef
 *	  linux - and the kernel's headers are under <sys/>.
 *
 *	- "int *s = state; s[0] = '--';" blanked two letters of the
 *	  state string with a single sixteen-bit store, six times.  ccc
 *	  has no multi-character constant and stops there with "bad
 *	  numeric constant", which is as far as it gets on the original.
 *	  The same bytes are now written two at a time.
 *
 *	- stat.  The one this was written against had flags, addr[] and
 *	  S_TYPE/S_ISCHAR.  The tree's has st_mode, st_addr and
 *	  S_IFMT/S_IFCHR.  Same fields, new spellings.
 *
 *	- findtty() read the /dev directory sixteen bytes at a time
 *	  itself.  That still works - a micronix directory IS a file of
 *	  sixteen byte entries, and readdir() is a call to read() with
 *	  that size - but readdir() terminates the name, which the
 *	  original relied on the following byte for, and the rest of the
 *	  tree reads directories that way.
 *
 *	- lenstr() and cpystr() were old-Micronix libc routines that
 *	  this libc does not carry.  strlen() does lenstr()'s job, and
 *	  cpystr() is copied from cmd/upm, which had already solved it
 *	  the same way - see the note on it below.
 *
 * The kernel side did not have to change, and that is the point of the
 * port being possible at all: sys/uhdr.s still emits ".defw _plist" at
 * 0x1003 under a comment calling it the "Hook for ps", so the table
 * address this program seeks is exactly where it looks for it.
 *
 * One thing is carried over rather than fixed, because it is a
 * decision about the program and not about the port: the original
 * exits YES - which is 1 - from main, and NO - which is 0 - from
 * fail().  ps therefore reports success as a failure status, the
 * opposite of every other command in the tree.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <dirent.h>

/*
 * Mnemonics. See detail[], quick[], and pro[] below.
 */
#define PID	0
#define COMMAND 1
#define TERM	2
#define UID	3
#define PARENT	4
#define SIZE	5
#define NICE	6
#define PRI	7

/*
 * #define CPU 8
 */
#define EVENT	8
#define STATE	9
#define PC	10

/*
 * Order of presentation for short listing
 */
char quick[] = { PID, COMMAND, TERM };

/*
 * Order for long listing
 */
char detail[] = {
    PID, COMMAND, TERM, UID,
    PARENT, SIZE, NICE, PRI,
    /*
     * CPU,
     */
    EVENT, STATE, PC
};

/*
 * Format strings and titles for the proc entries. Each format
 * contains a field width
 * for that entry. The titles should have the same length as
 * this field width.
 *
 * value carries both numbers and strings - the original relies on the
 * two being the same width, which on this machine they are.
 */
struct
{
    char *format;
    char *title;
    int value;
} pro[] = {
    {"%6u", "   PID"},
    {"%-8.8s", "COMMAND "},
    {"%-4.4s", "TTY "},
    {"%3u", "UID"},
    {"%3u", "PAR"},
    {"%3u", "SIZ"},
    {"%3d", "NIC"},
    {"%3u", "PRI"},
    /*
     * {"%3u", "CPU"},
     */
    {"%4x", "WAIT"},
    {"%-12.12s", "STATE       "},
    {"%5x", "  PC "}
};

#define SEPERATOR	"  "    /* between entries */
#define NOTTY		"    "  /* for procs without a tty */

#define MEMORY		"/dev/mem"      /* memory device */

/*
 * Satisfy the definitions of u and plist (proc.h)
 */
struct proc plist[NPROC] = 0;

/*
 * Command line flags
 */
int aflag = 0,                  /* show processes belonging to all ttys */
    lflag = 0,                  /* give long listing */
    pflag = 0,                  /* display the current PC - if in memory */
    xflag = 0;                  /* show processes not belonging to any tty */

int myterm = 0;                 /* device number of local tty */

struct user u = { 0 };

main(ac, av)
    int ac;
    char **av;
{
    init(ac, av);
    ps();
    exit(1);
}

init(ac, av)
    register int ac;
    register char **av;
{
    static char *arg;
    static int n;
    struct stat s;

    for (n = 1; n < ac; n++) {
        arg = av[n];
        doflag(arg);
    }

    findtty();

    if (fstat(0, &s) < 0)       /* stdin - the tree spells it 0 */
        myterm = 0;
    else
        myterm = s.st_addr[0];
}

ps()
{
    static int mem, addr;
    struct proc *p, *ptab;
    struct tty tty;
    char *tname;

    if ((mem = open(MEMORY, O_RDONLY)) < 0 || seek(mem, 0x1003, 0) < 0      /* pointer
                                                                             * to
                                                                             * process
                                                                             * table
                                                                             */
        || read(mem, &ptab, sizeof ptab) != sizeof ptab)
        fail();

    seek(mem, ptab, 0);
    read(mem, plist, sizeof plist);

    heading();

    for (p = plist; p < plist + NPROC; p++) {
        /*
         * discard empty entries
         */

        if (!(p->mode & ALLOC))
            continue;

        /*
         * get the tty structure (for the device number)
         */

        if (p->tty) {           /* proc. assoc. with terminal */
            if (seek(mem, p->tty, 0) < 0)
                fail();
            if (read(mem, &tty, sizeof tty) != sizeof tty)
                fail();
            if (myterm != tty.dev && !aflag)
                continue;       /* not my terminal */
            tname = ttyname(tty.dev);
        } else {
            if (!xflag)         /* don't list detached procs. */
                continue;
            tname = NOTTY;
        }

        pro[TERM].value = (int)tname;

        /*
         * Numerical values
         */
        {
            pro[UID].value = p->uid;
            pro[PID].value = p->pid;

            pro[PARENT].value = (p->pid) ? plist[p->parent - ptab].pid : 0;

            pro[SIZE].value = (p->pid) ? p->nsegs * 4 : 64;
            pro[NICE].value = 128 - p->nice;
            pro[PRI].value = p->pri;
            /*
             * pro[CPU].value         = p->cpu;
             */
            pro[EVENT].value = p->event;

            if (p->mem[16].seg) {
                seek(mem, 8 * p->mem[16].seg, 3);
                read(mem, &u, sizeof u);
                pro[PC].value = (int)u.pc;
            } else {
                pro[PC].value = 0;
            }
        }

        /*
         * Process state
         */
        {
            static char state[13];
            char *s = state;

            cpystr(state, "AlAwLdSwLkSy", 0);
            if (!(p->mode & ALIVE)) {
                s[0] = '-';
                s[1] = '-';
            }
            if (!(p->mode & AWAKE)) {
                s[2] = '-';
                s[3] = '-';
            }
            if (!(p->mode & LOADED)) {
                s[4] = '-';
                s[5] = '-';
            }
            if (!(p->mode & SWAPPED)) {
                s[6] = '-';
                s[7] = '-';
            }
            if (!(p->mode & LOCKED)) {
                s[8] = '-';
                s[9] = '-';
            }
            if (!(p->mode & SYS)) {
                s[10] = '-';
                s[11] = '-';
            }

            pro[STATE].value = (int)state;
        }

        /*
         * Command
         */
        {
            if (p->pid == 0)
                pro[COMMAND].value = (int)"System";
            else if (p->pid == 1)
                pro[COMMAND].value = (int)"Init";
            else if ((p->mode & ALIVE) == 0)
                pro[COMMAND].value = (int)"DEFUNCT";
            else
                pro[COMMAND].value = (int)p->args;
        }

        /*
         * Print
         */
        {
            char *pp;
            int ss, j;

            pp = lflag ? detail : quick;
            ss = lflag ? sizeof(detail) : sizeof(quick);

            for (j = 0; j < ss; j++) {
                printf(pro[pp[j]].format, pro[pp[j]].value);
                printf(SEPERATOR);
            }
            printf("\n");
        }
    }
}

heading()
{
    char *pp;
    int ss, j;

    pp = lflag ? detail : quick;
    ss = lflag ? sizeof(detail) : sizeof(quick);

    for (j = 0; j < ss; j++) {
        printf(pro[pp[j]].title);
        printf(SEPERATOR);
    }
    printf("\n");
}

fail()
{
    perror(MEMORY);
    exit(0);
}

doflag(a)
    register char *a;
{
    for (; *a; a++) {
        switch (*a) {
        case 'a':
            aflag = 1;
            break;

        case 'l':
            lflag = 1;
            break;

        case 'x':
            xflag = 1;
            break;

        case 'p':
            pflag = 1;
            break;
        }
    }
}

struct table
{
    int number;
    char *name;
    struct table *next;
};

struct table *table = 0;

/*
 * ttyname - find name of terminal
 * return a file name for the given file descriptor
 */

char *
ttyname(a)
{
    struct table *t;

    for (t = table; t; t = t->next) {
        if (a == t->number) {
            return t->name;
        }
    }

    return "?";
}

/*
 * read the contents of the "/dev" directory into memory
 *
 * This is the one routine whose shape changed rather than its
 * spelling. The original read sixteen bytes at a time into its own
 * struct mydir and passed dir.name to stat() on the strength of the
 * byte after it being zero. readdir() returns the same entry with that
 * byte written for it.
 */

findtty()
{
    DIR *f;
    struct dir *dp;
    struct stat found;
    static struct table *t;
    char buf[32];

    if ((f = opendir("/dev")) == 0)
        return;                 /* can't read dev directory */

    while ((dp = (struct dir *)readdir(f)) != 0) {

        if (!dp->ino)
            continue;

        cpystr(buf, "/dev/", dp->name, 0);

        if (stat(buf, &found) < 0)
            continue;           /* can't stat it */

        if ((found.st_mode & S_IFMT) != S_IFCHR)
            continue;

        t = calloc(1, sizeof(*t));
        t->name = save(dp->name);
        t->number = found.st_addr[0];
        t->next = table;
        table = t;
    }

    closedir(f);
}

save(a)
    char *a;
{
    char *b;

    b = malloc(strlen(a) + 1);
    cpystr(b, a, 0);
    return b;
}

/*
 * cpystr - concatenate the source strings into dst.  The sources are a
 * variable number of char * arguments, ended by a null argument; each is
 * copied in turn, a single nul is written, and the position after it is
 * returned.
 *
 * Copied from cmd/upm, which carries the same routine for the same
 * reason: it was a Micronix libc routine and this libc does not have
 * it.  It reads its arguments by walking the stack from the address of
 * the first one, which is what the original did too.
 */
char *
cpystr(dst, s)
char *dst;
char *s;
{
    char **ap;
    char *q;

    ap = &s;
    while ((q = *ap++) != 0) {
        while (*q)
            *dst++ = *q++;
    }
    *dst = 0;
    return dst;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
