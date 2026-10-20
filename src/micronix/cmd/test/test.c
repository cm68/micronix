/*
 * test expression
 * [ expression ]
 *
 * cmd/test/test.c
 *
 * V7 test.c, ported to micronix.  The shell wants a test that takes
 * its arguments apart the way V7's did - the grammar here is the one
 * bsh's IF and WHILE were written against - so this is the V7 source
 * and not the 2.11BSD rewrite, which has grown primaries this system
 * has no use for.
 *
 * The one change is the file size: micronix has no st_size, the size
 * is split across st_size0 and st_size1, so -s asks whether either
 * half is nonzero.  Everything else sits on the v6 interface this
 * system still speaks - open(name, 0) reads and open(name, 1)
 * writes.
 *
 * The program is also [ - main() looks at argv[0] and a [ call must
 * end with a ].  There are no symbolic links here, so the install
 * puts a second copy in /bin/[ rather than a link.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#define EQ(a,b)	((tmp=a)==0?0:(strcmp(tmp,b)==0))

#define DIR 1
#define FIL 2

int	ap;
int	ac;
char	**av;
char	*tmp;

main(argc, argv)
char *argv[];
{
	ac = argc; av = argv; ap = 1;
	if (EQ(argv[0], "[")) {
		if (!EQ(argv[--ac], "]"))
			synbad("] missing", "");
	}
	argv[ac] = 0;
	if (ac <= 1)
		exit(1);
	exit(exp() ? 0 : 1);
}

char *nxtarg(mt)
{
	if (ap >= ac) {
		if (mt) {
			ap++;
			return (0);
		}
		synbad("argument expected", "");
	}
	return (av[ap++]);
}

exp()
{
	int p1;

	p1 = e1();
	if (EQ(nxtarg(1), "-o"))
		return (p1 | exp());
	ap--;
	return (p1);
}

e1()
{
	int p1;

	p1 = e2();
	if (EQ(nxtarg(1), "-a"))
		return (p1 & e1());
	ap--;
	return (p1);
}

e2()
{
	if (EQ(nxtarg(0), "!"))
		return (!e3());
	ap--;
	return (e3());
}

e3()
{
	int p1;
	register char *a;
	char *p2;
	int int1, int2;

	a = nxtarg(0);
	if (EQ(a, "(")) {
		p1 = exp();
		if (!EQ(nxtarg(0), ")"))
			synbad(") expected", "");
		return (p1);
	}

	if (EQ(a, "-r"))
		return (tio(nxtarg(0), 0));

	if (EQ(a, "-w"))
		return (tio(nxtarg(0), 1));

	if (EQ(a, "-d"))
		return (ftype(nxtarg(0)) == DIR);

	if (EQ(a, "-f"))
		return (ftype(nxtarg(0)) == FIL);

	if (EQ(a, "-s"))
		return (fsizep(nxtarg(0)));

	if (EQ(a, "-t"))
		if (ap >= ac)
			return (isatty(1));
		else
			return (isatty(atoi(nxtarg(0))));

	if (EQ(a, "-n"))
		return (!EQ(nxtarg(0), ""));
	if (EQ(a, "-z"))
		return (EQ(nxtarg(0), ""));

	p2 = nxtarg(1);
	if (p2 == 0)
		return (!EQ(a, ""));
	if (EQ(p2, "="))
		return (EQ(nxtarg(0), a));

	if (EQ(p2, "!="))
		return (!EQ(nxtarg(0), a));

	if (EQ(a, "-l")) {
		int1 = length(p2);
		p2 = nxtarg(0);
	} else {
		int1 = atoi(a);
	}
	int2 = atoi(nxtarg(0));
	if (EQ(p2, "-eq"))
		return (int1 == int2);
	if (EQ(p2, "-ne"))
		return (int1 != int2);
	if (EQ(p2, "-gt"))
		return (int1 > int2);
	if (EQ(p2, "-lt"))
		return (int1 < int2);
	if (EQ(p2, "-ge"))
		return (int1 >= int2);
	if (EQ(p2, "-le"))
		return (int1 <= int2);

	synbad("unknown operator ", p2);
}

tio(a, f)
char *a;
int f;
{
	f = open(a, f);
	if (f >= 0) {
		close(f);
		return (1);
	}
	return (0);
}

ftype(f)
char *f;
{
	struct stat statb;

	if (stat(f, &statb) < 0)
		return (0);
	if ((statb.st_mode & S_IFMT) == S_IFDIR)
		return (DIR);
	return (FIL);
}

fsizep(f)
char *f;
{
	struct stat statb;

	if (stat(f, &statb) < 0)
		return (0);
	return (statb.st_size0 != 0 || statb.st_size1 != 0);
}

synbad(s1, s2)
char *s1, *s2;
{
	write(2, "test: ", 6);
	write(2, s1, strlen(s1));
	write(2, s2, strlen(s2));
	write(2, "\n", 1);
	exit(255);
}

length(s)
char *s;
{
	char *es = s;

	while (*es++)
		;
	return (es - s - 1);
}
