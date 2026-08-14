/*
 * freopen.c - stdio freopen 
 *
 */

#include	<stdio.h>

extern int	open(char *, int), creat(char *, int);

FILE *
freopen(name, mode, iob)
char *	name, * mode;
register FILE *	iob;
{
	uchar		c;
	char *		p;
	int		plus;

	fclose(iob);
	c = 0;
	plus = 0;
	iob->_flag &= _IONBF;
	switch (*mode) {

	case 'w':
		c++;
	case 'a':
		c++;
	case 'r':
		break;

	}

	/*
	 * The modifiers come in either order - "w+b" and "wb+" mean the
	 * same thing - so read all of them.
	 *
	 * "b" is accepted and recorded nowhere.  It used to set
	 * _IOBINARY, which nothing ever tested; that bit is _IORW now,
	 * and setting it here would tell fseek that every "rb" stream
	 * was read-write.  There is no text mode on this system to be
	 * the other half of the distinction.
	 */
	for (p = mode + 1; *p; p++) {
		if (*p == '+')
			plus++;
	}

	/*
	 * access mode.  0 is read, 1 is write, 2 is read-write; a "+"
	 * mode is the last of those whichever letter it followed.
	 */
	switch(c) {

	case 0:
		iob->_file = open(name, plus ? 2 : 0);
		break;

	case 1:
		if ((iob->_file = open(name, plus ? 2 : 1)) >= 0)
			break;
		/* else fall through: it does not exist yet */
	case 2:
		iob->_file = creat(name, 0666);
		/*
		 * creat hands back a descriptor open for writing only,
		 * so a "w+" stream could be written and never read -
		 * which is not what it promises.  The file exists and
		 * is empty now, so open it again for both.
		 *
		 * asz is what wanted this: it assembles into a "w+b"
		 * temp file and then seeks back over it to copy it into
		 * the object.
		 */
		if (iob->_file >= 0 && plus) {
			close(iob->_file);
			iob->_file = open(name, 2);
		}
		break;
	}
	if (iob->_file < 0)
		return (FILE *)NULL;
	if (!(iob->_flag & (_IONBF|_IOMYBUF)))
		iob->_base = _bufallo();
	if (iob->_base == (char *)-1) {
		iob->_base = (char *)0;
		close(iob->_file);
		iob->_flag = 0;
		return (FILE *)NULL;
	}
	iob->_ptr = iob->_base;
	iob->_cnt = 0;
	if (c)
		iob->_flag |= _IOWRT;
	else
		iob->_flag |= _IOREAD;
	/*
	 * A "+" stream is both, which is what lets fseek put it back to
	 * undecided and let the next operation pick a direction.
	 */
	if (plus)
		iob->_flag |= _IORW;
	if (iob->_base && c)
		iob->_cnt = BUFSIZ;
	if (c == 1)
		fseek(iob, 0L, 2);
	return iob;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
