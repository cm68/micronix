/*
 * df - disk free space
 *
 * cmd/df/df.c
 *
 * THIS IS A RECONSTRUCTION.  There is no surviving source for /bin/df;
 * this file was written from the disassembly of the /bin/df binary on the
 * Micronix 1.6 filesystem - see df.dist, df.dis and df.ctl beside this
 * file, and "make disas" to regenerate the disassembly from the binary.
 * Every function below corresponds to one function in that binary, and
 * the names here are the ones df.ctl assigns.
 *
 * It is Whitesmith's C, the way every 1.6 command is: c.ent and c.ret at
 * the top of the library, the three register variables in fixed cells,
 * and the syscall stubs in the data segment rather than the text.
 *
 * What it does, per filesystem named on the command line: opens the
 * device, makes sure it holds a Micronix filesystem, counts the free
 * blocks by walking the free list, and prints the count and the name.
 * A name that is not already a device path - one that does not stat, is
 * shorter than 15 characters, and has no slash - gets "/dev/" put in
 * front of it, so "df root" and "df /dev/root" mean the same thing.
 *
 * The free list is walked the way the kernel's alloc() pops it: off the
 * top of s_free[] until s_nfree runs out, then into the block the list
 * came from, which holds the next count and the next 100 block numbers.
 *
 * The odd bits are reproduced rather than tidied away, because they are
 * what the system shipped with:
 *
 *   -v lists every free block, right-justified in a five-column field,
 *	one to a line, ahead of the summary.  It is not in the man page.
 *
 *	The exit status is 1 after a normal run and 0 only from the usage
 *	message - the wrong way round - and nothing in the system looks.
 *
 *	A block number or count above 32767 prints as a negative number:
 *	the value is 16 bits and is printed signed, though the block
 *	numbers themselves are compared unsigned, as they must be.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define	SUPERBLK 1		/* the superblock */
#define	INOSTART 2		/* the first inode block */

int	vflag;			/* -v: list every free block */

struct super sblock;		/* the superblock, and the free list */

void parse();
void usage();
void process();
int isfs();
int countfree();

main(argc, argv)
    int argc;
    char **argv;
{
    struct stat stb;
    char buf[32];
    register char *np;
    register int i;

    parse(argc, argv);
    for (i = 1; i < argc; i++) {
        np = argv[i];
        if (strcmp(np, "-v") == 0)
            continue;
        if (stat(np, &stb) < 0 && strlen(np) < 15 && strchr(np, '/') == 0) {
            strcpy(buf, "/dev/");
            strcat(buf, np);
            np = buf;
        }
        process(np);
    }
    exit(1);
}

/*
 * Look for -v, and print the usage message when there is nothing else.
 * The flag is scanned here and the arguments are skipped again in main,
 * so "df -v root" and "df root -v" both work.
 */
void
parse(argc, argv)
    int argc;
    char **argv;
{
    register int i;

    if (argc == 1)
        usage();
    for (i = 1; i < argc; i++)
        if (strcmp(argv[i], "-v") == 0)
            vflag = 1;
}

void
usage()
{
    fprintf(stderr, "usage: df filesystem ...\n");
    exit(0);
}

/*
 * One filesystem: open it, make sure it is one, count its free blocks,
 * and print the count and the name.  It is opened twice - once here and
 * once by isfs, which opens its own descriptor and closes it again.
 */
void
process(name)
    char *name;
{
    int fd;
    int count;

    if ((fd = open(name, 0)) < 0) {
        perror(name);
        return;
    }
    if (!isfs(name)) {
        fprintf(stderr, ": Not a file system\n");
        close(fd);
        return;
    }
    count = countfree(name, fd);
    printf("%5d %s\n", count, name);
    close(fd);
}

/*
 * Is the device a Micronix filesystem?  Read the superblock, then the
 * root inode - the first inode in the first inode block - and its data
 * block, the first data block, and ask for a directory whose first two
 * entries are "." and "..", both with inumber 1.  That is what mkfs
 * writes and nothing else is.
 */
isfs(name)
    char *name;
{
    struct dsknod root;
    struct dir dot[2];
    int fd;

    if ((fd = open(name, 0)) < 0)
        return 0;
    seek(fd, SUPERBLK, 3);
    if (read(fd, (char *)&sblock, sizeof sblock) != sizeof sblock)
        goto bad;
    seek(fd, INOSTART, 3);
    if (read(fd, (char *)&root, sizeof root) != sizeof root)
        goto bad;
    if ((root.d_mode & S_IFMT) != S_IFDIR)
        goto bad;
    seek(fd, sblock.s_isize + INOSTART, 3);
    if (read(fd, (char *)dot, sizeof dot) != sizeof dot)
        goto bad;
    close(fd);
    if (dot[0].ino != 1 || dot[1].ino != 1)
        return 0;
    if (strcmp(dot[0].name, ".") != 0 || strcmp(dot[1].name, "..") != 0)
        return 0;
    return 1;

bad:
    close(fd);
    return 0;
}

/*
 * Count the free blocks.  The superblock is read into the global sblock
 * and the free list is popped off s_free[]; when s_nfree runs out, the
 * block the list came from holds the next list, in exactly the layout of
 * s_nfree and s_free, and is read straight back into them.
 */
countfree(name, fd)
    char *name;
    int fd;
{
    int count;
    UINT b;

    count = 0;
    seek(fd, SUPERBLK, 3);
    if (read(fd, (char *)&sblock, sizeof sblock) != sizeof sblock) {
        perror(name);
        return count;
    }
    for (;;) {
        if (sblock.s_nfree == 0)
            return count;
        sblock.s_nfree--;
        b = sblock.s_free[sblock.s_nfree];
        if (b == 0)
            return count;
        if (b >= sblock.s_fsize) {
            fprintf(stderr, "Bad block in free list\n");
            return count;
        }
        count++;
        if (count == 0xffff) {
            fprintf(stderr, "Block count overflow\n");
            return count;
        }
        if (vflag)
            printf("%5d\n", b);
        if (sblock.s_nfree == 0) {
            seek(fd, b, 3);
            if (read(fd, (char *)&sblock.s_nfree,
                sizeof sblock.s_nfree + sizeof sblock.s_free)
                != sizeof sblock.s_nfree + sizeof sblock.s_free) {
                perror(name);
                return count;
            }
        }
    }
}
