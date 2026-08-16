#include <types.h>
#include <sys/dir.h>
#include <dirent.h>

DIR *
opendir(dirname)
char *dirname;
{
	DIR *ret;
	short fd;

	fd = open(dirname, 0);
	if (fd < 0) 
		return 0;
	
	ret = calloc(sizeof(DIR), 1);
	ret->fd = fd;

	return (ret);
}

int
closedir(dirp)
DIR *dirp;
{
	int ret;

	ret = close(dirp->fd);
	free(dirp);
	return ret;
}

rewinddir(dirp)
DIR *dirp;
{
	seek(dirp->fd, 0, 0);
}

struct dirent *
readdir(dirp)
DIR *dirp;
{
	int len;

	len = read(dirp->fd, &dirp->d, 16);
	if (len == 0)
		return 0;
	dirp->pad = '\0';
	return &dirp->d;
}

/*
 * where the scan is, and putting it back.  A micronix directory is a
 * file of 16 byte entries read one at a time, so the position is the
 * fd's offset and nothing more.  tar is why these exist: it closes a
 * directory while it recurses into a subdirectory and seeks back to
 * where it was, which on a system with 16 open file slots is the
 * difference between depth costing one fd and costing one per level.
 */
long
telldir(dirp)
DIR *dirp;
{
	long lseek();

	return lseek(dirp->fd, 0L, 1);
}

seekdir(dirp, pos)
DIR *dirp;
long pos;
{
	long lseek();

	lseek(dirp->fd, pos, 0);
}
