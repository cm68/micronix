/*
 * the stock micronix pwd is busted, so this is a rewrite
 *
 * cmd/pwd/pwd.c
 * Changed: <2022-01-06 16:37:35 curt>
 */

#include <types.h>
#include <std.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>

char dot[] = ".";
char dotdot[] = "..";
char root[] = "/";
char path[512] = 0;
int file = 0;
int off = -1;

struct stat rootstat = 0;
struct stat statbuf = 0;
struct dir dirent = 0;

/*
 * print out the accumulated name
 */
prname()
{
    if (off < 0) {
        off = 0;
	}
    path[off] = '\n';
	write(1, root, 1);
    write(1, path, off + 1);
    exit(0);
}

/*
 *
 */
cat()
{
    int i, j;

	/* how long is our name */
    i = -1;
    while (dirent.name[++i] != 0);

	/* do we have room */
    if ((off + i + 2) > 511)
        prname();

	/* make space for the name by shifting right */
    for (j = off + 1; j >= 0; --j)
        path[j + i + 1] = path[j];

    off = i + off + 1;

    path[i] = root[0];

    for (--i; i >= 0; --i) {
        path[i] = dirent.name[i];
	}
}

main()
{
    int n;

	stat(root, &rootstat);

	while (1) {
		/*
		 * find out our inode
		 */
		stat(dot, &statbuf);

		/*
		 * open our parent directory
		 */
		if ((file = open(dotdot, 0)) < 0) {
			prname();
		}

		/*
		 * read directory entries until we find our inode
		 */
		do {
			if ((n = read(file, &dirent, 16)) < 16) {
				prname();
			}
		} while (dirent.ino != statbuf.st_ino);

		close(file);

		if (dirent.ino == rootstat.st_ino) {
			prname();
		}

		/* prepend our found name to path */
		cat();

		chdir(dotdot);
	}
	exit(0);
}

#ifdef notdef
ckroot()
{
    int i, n;

    if ((n = stat(y.name, &statbuf)) < 0)
        prname();
    i = statbuf.devn;
    if ((n = chdir(root)) < 0)
        prname();
    if ((file = open(root, 0)) < 0)
        prname();
  loop:
    if ((n = read(file, &dirent, 16)) < 16)
        prname();
    if (y.jnum == 0)
        goto loop;
    if ((n = stat(y.name, &statbuf)) < 0)
        prname();
    if (statbuf.devn != i)
        goto loop;
    statbuf.i[0] &= 060000;
    if (statbuf.i[0] != 040000)
        goto loop;
    if (y.name[0] == '.')
        if (((y.name[1] == '.') && (y.name[2] == 0)) || (y.name[1] == 0))
            goto pr;
    cat();
  pr:
    write(1, root, 1);
    prname();
}
#endif

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

