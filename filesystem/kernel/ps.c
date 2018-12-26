#include <stdio.h>
#include <sys/types.h>
#include <stat.h>
#include "sys.h"
#include "proc.h"
#include "tty.h"

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
/* #define CPU	8 */
#define EVENT	8
#define STATE	9
#define PC	10

	/*
	 * Order of presentation for short listing
	 */
char	quick[] =
	{PID, COMMAND, TERM};

	/*
	 * Order for long listing
	 */
char	detail[] =
{PID, COMMAND, TERM, UID, PARENT, SIZE, NICE, PRI, /* CPU, */ EVENT, STATE, PC};

	/*
	 * Format strings and titles for the proc entries. Each format
	 * contains a field width
	 * for that entry. The titles should have the same length as
	 * this field width.
	 */
struct
	{
	char *	format;
	char *	title;
	int	value;
	}

	pro[] =
	{
	{"%6u",		"   PID",}
	{"%-8.8s",	"COMMAND ",}
	{"%-4.4s",	"TTY ",}
	{"%3u",		"UID",}
	{"%3u",		"PAR",}
	{"%3u",		"SIZ",}
	{"%3d",		"NIC",}
	{"%3u",		"PRI",}
	/* {"%3u",		"CPU",} */
	{"%4x",		"WAIT",}
	{"%-12.12s",	"STATE       ",}
	{"%5x",		"  PC ",}
	};

# define SEPERATOR	"  "		/* between entries */
# define NOTTY		"    "		/* for procs without a tty */

# define MEMORY		"/dev/mem"	/* memory device */

	/*
	 * Satisfy the definitions of u and plist (proc.h)
	 */
struct proc plist[NPROC] = 0;

	/*
	 * Command line flags
	 */
BOOL	aflag = NO,	/* show processes belonging to all ttys */
	lflag = NO,	/* give long listing */
	pflag = NO,	/* display the current PC - if in memory */
	xflag = NO;	/* show processes not belonging to any tty */

int	myterm = 0;	/* device number of local tty */

struct user u = {0};

main (ac, av)
	int ac;
	char **av;
	{
	init (ac, av);
	ps ();
	exit (YES);
	}

init (ac, av)
	register int ac;
	register char **av;
	{
	static char *arg;
	static n;
	struct stat s;

	for (n = 1; n < ac; n++)
		{
		arg = av[n];
		doflag (arg);
		}

	findtty ();

	if (fstat (STDIN, &s) < 0)
		myterm = 0;
	else
		myterm = s.addr[0];
	}

ps ()
	{
	static mem, addr;
	struct proc *p, *ptab;
	struct tty tty;
	char *tname;

	if ((mem = open (MEMORY, READ)) < 0
		||
	     seek (mem, 0x1003, 0) < 0	/* pointer to process table */
		||
	     read (mem, &ptab, sizeof ptab) != sizeof ptab)
		fail ();

	seek (mem, ptab, 0);
	read (mem, plist, sizeof plist);

	heading();

	for (p = plist; p < plist + NPROC; p++)
		{
		/* discard empty entries */

		if (!(p->mode & ALLOC))
			continue;

		/* get the tty structure (for the device number) */

		if (p->tty) /* proc. assoc. with terminal */
			{
			if (seek (mem, p->tty, 0) < 0)
				fail ();
			if (read (mem, &tty, sizeof tty) != sizeof tty)
				fail ();
			if (myterm != tty.dev && !aflag)
				continue; /* not my terminal */
			tname = ttyname (tty.dev);
			}
		else
			{
			if (!xflag)	/* don't list detached procs. */
				continue;
			tname = NOTTY;
			}

		pro[TERM].value = tname;

		/* Numerical values */
			{
			pro[UID].value	  = p->uid;
			pro[PID].value	  = p->pid;

			pro[PARENT].value 
				= (p->pid)? plist [p->parent - ptab].pid : 0;

			pro[SIZE].value	  = (p->pid)? p->nsegs * 4 : 64;
			pro[NICE].value	  = 128 - p->nice;
			pro[PRI].value	  = p->pri;
			/* pro[CPU].value	  = p->cpu; */
			pro[EVENT].value  = p->event;

			if (p->mem[16].seg)
				{
				seek (mem, 8 * p->mem[16].seg, 3);
				read (mem, &u, sizeof u);
				pro[PC].value = u.pc;
				}
			else
				{
				pro[PC].value = 0;
				}
			}

		/* Process state */
			{
			static char state[13];
			int * s = state;

			cpystr(state, "AlAwLdSwLkSy", 0);
			if (!(p->mode & ALIVE))
				s[0] = '--';
			if (!(p->mode & AWAKE))
				s[1] = '--';
			if (!(p->mode & LOADED))
				s[2] = '--';
			if (!(p->mode & SWAPPED))
				s[3] = '--';
			if (!(p->mode & LOCKED))
				s[4] = '--';
			if (!(p->mode & SYS))
				s[5] = '--';

			pro[STATE].value = state;
			}

		/* Command */
			{
			if (p->pid == 0)
				pro[COMMAND].value = "System";
			else if (p->pid == 1)
				pro[COMMAND].value = "Init";
			else if ((p->mode & ALIVE) == 0)
				pro[COMMAND].value = "DEFUNCT";
			else
				pro[COMMAND].value = p->args;
			}

		/* Print */
			{
			unsigned char * pp;
			int ss, j;

			pp = lflag? detail : quick;
			ss = lflag? sizeof(detail) : sizeof(quick);

			for (j = 0; j < ss; j++)
				{
				printf(pro[pp[j]].format, pro[pp[j]].value);
				printf(SEPERATOR);
				}
			printf("\n");
			}
		}
	}

heading ()
	{
	unsigned char * pp;
	int ss, j;

	pp = lflag? detail : quick;
	ss = lflag? sizeof(detail) : sizeof(quick);

	for (j = 0; j < ss; j++)
		{
		printf(pro[pp[j]].title);
		printf(SEPERATOR);
		}
	printf("\n");
	}

fail ()
	{
	perror (MEMORY);
	exit (NO);
	}

doflag (a)
	register char *a;
	{
	for (; *a; a++)
		{
		switch (*a)
			{
			case 'a':
				{
				aflag = YES;
				break;
				}

			case 'l':
				{
				lflag = YES;
				break;
				}

			case 'x':
				{
				xflag = YES;
				break;
				}

			case 'p':
				{
				pflag = YES;
				break;
				}
			}
		}
	}

struct table
	{
	int number;
	char *name;
	struct table *next;
	};

struct table *table = NULL;

struct mydir
	{
	COUNT ino;
	TEXT name [16];
	};

INTERN struct mydir dir = {0};

/*
 * ttyname - find name of terminal
 * return a file name for the given file descriptor
 */

	char *
ttyname (a)
	{
	struct table *t;

	for (t = table; t; t = t->next)
		{
		if (a == t->number)
			{
			return t->name;
			}
		}

	return "?";
	}

/*
 * read the contents of the "/dev" directory into memory
 */

findtty ()
	{
	static char f;
	struct stat found;
	static struct table *t;

	if ((f = open ("/dev", READ)) < 0)
		return;		/* can't read dev directory */

	for (;;)
		{
		char buf [32];

		if (read (f, &dir, 16) != 16)
			break;	/* end of file */

		if (!dir.ino)
			continue;

		cpystr (buf, "/dev/", dir.name, NULL);

		if (stat (buf, &found) < 0)
			continue;	/* can't stat it */

		if ((found.flags & S_TYPE) != S_ISCHAR)
			continue;

		t = calloc (1, sizeof (*t));
		t->name = save (dir.name);
		t->number = found.addr[0];
		t->next = table;
		table = t;
		}
	}

save (a)
	char *a;
	{
	char *b;

	b = alloc (lenstr (a) + 1);
	cpystr (b, a, NULL);
	return b;
	}
