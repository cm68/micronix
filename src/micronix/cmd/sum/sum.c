/*
 * sum [-r] [file ...]
 *
 * v7 sum (usr/src/cmd/sum.c), ported to micronix.
 *
 * cmd/sum/sum.c
 *
 * The check sum is v7's, unchanged: rotate the sixteen bit sum right
 * by one, add the byte, mask to sixteen bits, so that a byte's
 * position in the file counts as much as the byte.  It is the sum
 * that sum (1) has always printed, and it agrees byte for byte with
 * the sum on the machine this was ported on.  The block count is
 * v7's too, (nbytes + BUFSIZ - 1) / BUFSIZ, and BUFSIZ is 512 here,
 * which is what the page says a block is.
 *
 * What the port had to say differently:
 *
 *	-r		recursive mode, which sum (1) documents and v7
 *			does not have.  A named directory is descended,
 *			and only regular files are summed - a directory
 *			and a special file are walked past, which is what
 *			the page asks for.  This is new code: rsum()
 *			below, and the name buffer and the directory
 *			handling it needs.
 *
 *	the file name	v7 printed the name only when more than one
 *			file was named.  sum (1) says a sum and a block
 *			count are printed "for each named file", and the
 *			1982 command prints the name for one file as
 *			well, so it is printed whenever there is a name
 *			at all.
 *
 *	the shape	v7's single do/while over argv is now main
 *			walking the arguments and sumstream() summing one
 *			open stream, because -r has to sum a file reached
 *			by a walk and a file named on the command line
 *			with the same code.
 *
 * One thing to know before trusting a sum against the 1982 binary:
 * the /bin/sum on the 1.6 disk does NOT compute this sum.  It adds
 * the file as sixteen bit little endian words - sum += c << (8 * (i
 * & 1)), odd bytes into the high half - so "sum f1" there answers
 * 6067 where sum (1), this port and the host all answer 47926.  The
 * page says only that the answer is in the range 0 to 65535, so
 * nothing in sum (1) contradicts this port; the 1982 binary is what
 * does.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

int	errflg;
int	rflg;

/*
 * the name being walked, and the name printed with its sum.  One
 * buffer, shared, with each descent remembering where its own name
 * ended so it can put it back - the way du keeps its paths.
 */
char	namebuf[BUFSIZ];

DIR	*opendir();
struct	direct *readdir();
long	telldir();

main(argc, argv)
	int argc;
	char **argv;
{
	register int i;

	i = 1;
	while (i < argc && argv[i][0] == '-' && argv[i][1] != '\0') {
		if (strcmp(argv[i], "-r") == 0)
			rflg++;
		else {
			fprintf(stderr, "sum: bad flag %s\n", argv[i]);
			exit(1);
		}
		i++;
	}
	if (i >= argc)
		sumstream(stdin, (char *)NULL);
	else
		for (; i < argc; i++) {
			if (rflg) {
				if (strlen(argv[i]) >= BUFSIZ) {
					cantopen(argv[i]);
					continue;
				}
				strcpy(namebuf, argv[i]);
				rsum();
			} else
				sumfile(argv[i]);
		}
	exit(errflg);
}

/*
 *	sum one open stream, and print the answer.  name is printed
 *	with it, and is NULL for the standard input, which has none.
 */
sumstream(f, name)
	register FILE *f;
	char *name;
{
	register unsigned sum;
	register int c;
	long nbytes;

	sum = 0;
	nbytes = 0;
	while ((c = getc(f)) != EOF) {
		nbytes++;
		if (sum & 01)
			sum = (sum >> 1) + 0x8000;
		else
			sum >>= 1;
		sum += c;
		sum &= 0xFFFF;
	}
	if (ferror(f)) {
		errflg++;
		fprintf(stderr, "sum: read error on %s\n",
			name != NULL ? name : "-");
	}
	printf("%05u%6ld", sum, (nbytes + BUFSIZ - 1) / BUFSIZ);
	if (name != NULL)
		printf(" %s", name);
	printf("\n");
}

sumfile(path)
	char *path;
{
	register FILE *f;

	if ((f = fopen(path, "r")) == NULL) {
		cantopen(path);
		return;
	}
	sumstream(f, path);
	fclose(f);
}

cantopen(path)
	char *path;
{

	fprintf(stderr, "sum: Can't open %s\n", path);
	errflg += 10;
}

/*
 *	recursive mode.  namebuf holds a file or a directory; a file is
 *	summed, a directory is walked and each of its entries looked at
 *	in turn, and anything else is nothing to sum.
 *
 *	Only one directory is open at a time: it is closed before the
 *	descent and reopened and seeked back afterwards, because the
 *	machine has twelve file slots for the whole program.  That is
 *	why telldir and seekdir are in libc - see du (1).
 */
rsum()
{
	register DIR *dirp;
	register struct direct *dp;
	struct stat stb;
	char *ebase;
	long pos;
	int len, slash;

	if (stat(namebuf, &stb) < 0) {
		cantopen(namebuf);
		return;
	}
	if ((stb.st_mode & S_IFMT) == S_IFREG) {
		sumfile(namebuf);
		return;
	}
	if ((stb.st_mode & S_IFMT) != S_IFDIR)
		return;

	dirp = opendir(namebuf);
	if (dirp == NULL) {
		cantopen(namebuf);
		return;
	}
	len = strlen(namebuf);
	ebase = namebuf + len;
	slash = len > 0 && namebuf[len-1] == '/' ? 0 : 1;
	while (dp = readdir(dirp)) {
		if (dp->d_ino == 0)
			continue;
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;
		if (len + strlen(dp->d_name) + 2 > BUFSIZ) {
			fprintf(stderr, "sum: name too long: %s/%s\n",
				namebuf, dp->d_name);
			errflg++;
			continue;
		}
		if (slash)
			sprintf(ebase, "/%s", dp->d_name);
		else
			sprintf(ebase, "%s", dp->d_name);
		pos = telldir(dirp);
		closedir(dirp);
		dirp = NULL;
		rsum();
		namebuf[len] = '\0';
		if ((dirp = opendir(namebuf)) == NULL) {
			cantopen(namebuf);
			return;
		}
		seekdir(dirp, pos);
	}
	closedir(dirp);
}
