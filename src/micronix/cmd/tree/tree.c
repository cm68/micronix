/*
 * tree - draw a picture of the file system
 *
 * cmd/tree/tree.c
 *
 * Walks the directory tree from a starting directory (the root by
 * default) and draws it with |- for each entry and |  for a level that
 * still has entries to come.  Only directory names are listed unless
 * -a is given, in which case files are too.  Entries are sorted.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

#define	MAXENT	256
#define	MAXLEVEL 32
#define	PATHLEN	512

static char	names[MAXENT][14];
static char	isdir[MAXENT];
static int	order[MAXENT];
static char	path[PATHLEN];
static char	more[MAXLEVEL];
static int	aflag;

DIR		*opendir();
struct direct	*readdir();
int		closedir();

static void
prname(name)
	char *name;
{
	int i;

	for (i = 0; i < 14 && name[i] != '\0'; i++)
		putchar(name[i]);
}

static void
sortent(n)
	int n;
{
	int i, j, t;

	for (i = 1; i < n; i++) {
		t = order[i];
		for (j = i - 1; j >= 0 &&
		     strncmp(names[order[j]], names[t], 14) > 0; j--)
			order[j + 1] = order[j];
		order[j + 1] = t;
	}
}

/*
 * Read the directory named by path, keep the entries to show (the
 * directories, and the files if -a), and sort them.  path is built up
 * and put back as it was.
 */
static int
collect()
{
	DIR *dp;
	struct direct *de;
	struct stat stb;
	int n, len, clen, i;

	dp = opendir(path);
	if (dp == NULL)
		return 0;
	len = strlen(path);
	n = 0;
	while ((de = readdir(dp)) != NULL && n < MAXENT) {
		if (de->d_ino == 0)
			continue;
		if (de->d_name[0] == '.' && de->d_name[1] == '\0')
			continue;
		if (de->d_name[0] == '.' && de->d_name[1] == '.' &&
		    de->d_name[2] == '\0')
			continue;
		/* the child's full name, for stat */
		clen = len;
		if (clen == 0 || path[clen - 1] != '/')
			path[clen++] = '/';
		for (i = 0; i < 14 && de->d_name[i] != '\0'; i++)
			path[clen + i] = de->d_name[i];
		path[clen + i] = '\0';
		isdir[n] = 0;
		if (stat(path, &stb) >= 0 &&
		    (stb.st_mode & S_IFMT) == S_IFDIR)
			isdir[n] = 1;
		path[len] = '\0';
		if (!isdir[n] && !aflag)
			continue;
		for (i = 0; i < 14; i++)
			names[n][i] = de->d_name[i];
		order[n] = n;
		n++;
	}
	path[len] = '\0';
	closedir(dp);
	sortent(n);
	return n;
}

static void
walk(depth)
	int depth;
{
	int n, i, d, e, plen;

	n = collect();
	plen = strlen(path);
	for (i = 0; i < n; i++) {
		e = order[i];
		for (d = 0; d < depth; d++) {
			if (more[d])
				printf("|  ");
			else
				printf("   ");
		}
		printf("|- ");
		prname(names[e]);
		printf("\n");
		if (isdir[e]) {
			/* child path = path/name */
			if (plen > 0 && path[plen - 1] != '/')
				path[plen] = '/';
			/* the '/' is at plen, name follows */
			{
				int k, base;
				base = (plen > 0 && path[plen - 1] == '/')
					? plen : plen + 1;
				for (k = 0; k < 14 && names[e][k] != '\0'; k++)
					path[base + k] = names[e][k];
				path[base + k] = '\0';
			}
			more[depth] = (i < n - 1);
			walk(depth + 1);
			path[plen] = '\0';
		}
	}
}

main(argc, argv)
	int argc;
	char **argv;
{
	char *root;
	int i;

	root = "/";
	aflag = 0;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-a") == 0)
			aflag = 1;
		else
			root = argv[i];
	}
	strcpy(path, root);
	printf("%s\n", root);
	walk(0);
	exit(0);
}
