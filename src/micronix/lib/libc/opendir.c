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
