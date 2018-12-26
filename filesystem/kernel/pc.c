#include <stdio.h>
#include "sys.h"
#include "proc.h"


# define MEMORY		"/dev/mem"	/* memory device */

int mem = 0, ourpid = 0;


struct proc plist[NPROC] = 0;

struct user u = {0};

main (ac, av)
	int ac;
	char **av;
	{
	init (ac, av);
	pc ();
	return 0;
	}

init (ac, av)
	int ac;
	char **av;
	{
	if (ac != 2)
		exit (1);

	ourpid = atoi (av[1]);
	}

pc ()
	{
	struct proc *p, *ptab;

	mem = open (MEMORY, READ);

	if (mem < 0)
		{
		perror (MEMORY);
		exit (1);
		}

	seek (mem, 0x1003, 0);
	read (mem, &ptab, sizeof ptab);
	seek (mem, ptab, 0);
	read (mem, plist, sizeof plist);

/*
 watch (plst + pid);
*/


	for (p = plist; p < plist + NPROC; p++)
		if (p->pid == ourpid)
			watch (p);

	prs ("No such pid.\n");
	return 1;
	}

watch (p)
	struct proc *p;
	{
	if (!p->mem[16].seg)
		{
		return NO;
		}

	for (;;)
		{
		seek (mem, 8 * p->mem[16].seg, 3);
		read (mem, &u, sizeof u);
		px (u.pc);
		putchar (' ');
		fflush (stdout);
		}
	}

px (a)
	unsigned a;
	{
	unsigned n;

	n = a / 16;

	if (n)
		px (n);
	
	putchar ("0123456789ABCDEF" [a % 16]);
	}

prs (a)
	char *a;
	{
	fputs (a, stdout);
	}
