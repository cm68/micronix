/*
 * du - summarize disk usage
 *
 * 2.11BSD du (du.c 4.11, Berkeley 7/1/83), ported to micronix.
 *
 * cmd/du/du.c
 *
 * What the port had to change:
 *
 *	st_blocks	the micronix inode does not report blocks, so
 *			usage is figured from the size, (size+511)/512,
 *			the way ls does - and printed in those 512 byte
 *			blocks, the way du always did before Berkeley
 *			started reporting kilobytes.  Indirect blocks
 *			are not counted; neither was the truth of
 *			st_blocks worth much on a machine that has
 *			never had it.
 *
 *	symbolic links	micronix has none; lstat is stat.
 *
 *	devices		a device inode keeps its device number where a
 *			file keeps its size, so a special file counts
 *			as zero rather than as whatever the number
 *			says.
 *
 * The original's one-open-directory discipline - close the parent
 * around each descent, reopen and seek back after - is kept as it
 * was; it is why telldir and seekdir are in libc.  The fork per
 * argument is kept too: each argument's walk chdirs freely and dies
 * in its own process, which is cheaper than finding the way back.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

#define	fsize(sp)	(((long)(sp)->d.d_size0 << 16) + (sp)->d.d_size1)

char	path[BUFSIZ], name[BUFSIZ];
char	aflg;		/* truth values */
char	sflg;
char	*dot = ".";

/*
 * files already counted once through another link
 */
#define ML	200
struct {
	int	dev;
	UINT	ino;
} ml[ML];
unsigned char	mlx;		/* 0..ML, and ML is 200 */

long	descend();
long	telldir();
char	*rindex(), *strcpy();

main(argc, argv)
	int argc;
	char **argv;
{
	long blocks = 0;
	register char *np;
	int pid;

	argc--, argv++;
again:
	if (argc && !strcmp(*argv, "-s")) {
		sflg++;
		argc--, argv++;
		goto again;
	}
	if (argc && !strcmp(*argv, "-a")) {
		aflg++;
		argc--, argv++;
		goto again;
	}
	if (argc == 0) {
		argv = &dot;
		argc = 1;
	}
	do {
		if (argc > 1) {
			pid = fork();
			if (pid == -1) {
				fprintf(stderr, "No more processes.\n");
				exit(1);
			}
			if (pid != 0)
				wait((int *)0);
		}
		if (argc == 1 || pid == 0) {
			(void) strcpy(path, *argv);
			(void) strcpy(name, *argv);
			if (np = rindex(name, '/')) {
				*np++ = '\0';
				if (chdir(*name ? name : "/") < 0) {
					perror(*name ? name : "/");
					exit(1);
				}
			} else
				np = path;
			blocks = descend(path, *np ? np : ".");
			if (sflg)
				printf("%ld\t%s\n", blocks, path);
			if (argc > 1)
				exit(1);
		}
		argc--, argv++;
	} while (argc > 0);
	exit(0);
}

DIR	*dirp = NULL;

long
descend(base, name)
	char *base, *name;
{
	char *ebase0, *ebase;
	struct stat stb;
	int i;
	long blocks = 0;
	long curoff = 0;
	register struct direct *dp;

	ebase0 = ebase = base + strlen(base);
	if (ebase > base && ebase[-1] == '/')
		ebase--;
	if (stat(name, &stb) < 0) {
		perror(base);
		*ebase0 = 0;
		return (0);
	}
	if (stb.st_nlink > 1 && (stb.st_mode&S_IFMT) != S_IFDIR) {
		for (i = 0; i < mlx; i++)
			if (ml[i].ino == stb.st_ino && ml[i].dev == stb.st_dev)
				return (0);
		if (mlx < ML) {
			ml[mlx].dev = stb.st_dev;
			ml[mlx].ino = stb.st_ino;
			mlx++;
		}
	}
	if ((stb.st_mode&S_IFMT) == S_IFREG ||
	    (stb.st_mode&S_IFMT) == S_IFDIR)
		blocks = (fsize(&stb) + 511) >> 9;
	if ((stb.st_mode&S_IFMT) != S_IFDIR) {
		if (aflg)
			printf("%ld\t%s\n", blocks, base);
		return (blocks);
	}
	if (dirp != NULL)
		closedir(dirp);
	dirp = opendir(name);
	if (dirp == NULL) {
		perror(base);
		*ebase0 = 0;
		return (0);
	}
	if (chdir(name) < 0) {
		perror(base);
		*ebase0 = 0;
		closedir(dirp);
		dirp = NULL;
		return (0);
	}
	while (dp = readdir(dirp)) {
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		(void) sprintf(ebase, "/%s", dp->d_name);
		curoff = telldir(dirp);
		blocks += descend(base, ebase+1);
		*ebase = 0;
		if (dirp == NULL) {
			dirp = opendir(".");
			if (dirp == NULL) {
				perror(".");
				return (0);
			}
			seekdir(dirp, curoff);
		}
	}
	closedir(dirp);
	dirp = NULL;
	if (sflg == 0)
		printf("%ld\t%s\n", blocks, base);
	if (chdir("..") < 0) {
		(void) sprintf(base + strlen(base), "/..");
		perror(base);
		exit(1);
	}
	*ebase0 = 0;
	return (blocks);
}
