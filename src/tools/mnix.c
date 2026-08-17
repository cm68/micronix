/*
 * a command line micronix filesystem reader/writer/lister
 *
 * tools/mnix.c
 * Changed: <2023-06-19 19:16:00 curt>
 *
 * the interface really needs to look like tar: 
 *
 * mar -xf <image> micronix       - extract /micronix
 * mar -rf <image> micronix       - replace or add /micronix
 * mar -df <image> micronix       - delete /micronix
 * mar -tf <image> [file list]    - list of files
 * mar -cf <image> [ <size> ]     - create an empty filesystem 
 *
 * with the usual -v option for verbosity
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "../micronix/include/types.h"
#include "../micronix/include/sys/fs.h"
#include "../micronix/include/sys/dir.h"
#include "../include/fslib.h"
#include "../include/util.h"
#include "../include/disklabel.h"
#include "../micronix/cmd/mkfs/mkfs.h"

int traceflags;

char *filesystem;
int nerror;
char **command;

int ls();
int cat();
int emptycmd();
int infocmd();
int dumpcmd();
int readcmd();
int writecmd();
int rmcmd();
int mkdircmd();
int rmdircmd();
int fsinfo();
int iinfo();
int blkcmd();
int setblkcmd();
int mknodcmd();
int devlistcmd();
int imagemkdir(char *path);
int tarcmd();
int initcmd();
int mkfscmd();

struct cmdtab
{
    char *name;
    int (*handler)(int n, char **a);
    char *usage;
} cmds[] = {
    {"inode", iinfo, "inode [-f] <inum>" },
    {"fsinfo", fsinfo, "fsinfo" },
    {"empty", emptycmd, "empty <file>" },
    {"info", infocmd, "info <file>" },
    {"dump", dumpcmd, "dump <file>" },
    {"ls", ls, "ls [-a] <path>" },
    {"cat", cat, "cat <file>" },
    {"read", readcmd, "read <src> <dest>" },
    {"write", writecmd, "write [-k] <src> <dest>" },
    {"rm", rmcmd, "rm <file>" },
    {"mkdir", mkdircmd, "mkdir <directory>" },
    {"rmdir", rmdircmd, "rmdir <directory>" },
    {"block", blkcmd, "block [-e] <blkno>" },
    {"setblk", setblkcmd, "setblk <path> <blkno> ..." },
    {"mknod", mknodcmd, "mknod <path> <b|c> <major> <minor>" },
    {"devlist", devlistcmd, "devlist" },
    {"tar", tarcmd, "tar x [-C prefix] <tarfile> | tar c <tarfile> [path ...]" },
    {"initialize", initcmd, "initialize <medium> <image>" },
    {"mkfs", mkfscmd, "mkfs <image> [size|-exclude] [-i bootfile] [-f]" }
};

void
usage(char *e)
{
    int i;
    printf("%s [-f <filesystem>] <command>\n", e);
    printf("commands:\n");
    for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        printf("\t%s\n", cmds[i].usage);
    }
}

/*
 * a seperate command arg processor
 */
char opt[26];       // indexed by 'a' to 'z'

void
readopt(int *argc, char ***argv, char *allowed)
{
    char c;
    char *s;

    (*argv)++;
    s = **argv;
    (*argc)--;
    if (*s == '-') {
        while (c = *++s) {
            if ((c >= 'a') && (c <= 'z') && index(allowed, c)) {
                opt[c-'a']++;
            } else {
                printf("bogus option: %c\n", c);
                break;
            }
        }
        (*argc)--;
        (*argv)++;
    }
    return;
}

struct super *fs;

int
main(argc, argv)
    int argc;
    char **argv;
{
    int i;
    char *pname = argv[0];

    putenv("TZ=GMT");
    tzset();

    filesystem = getenv("MNIXFILESYSTEM");

    while (--argc) {
        argv++;
        if (**argv == '-')
            switch ((*argv)[1]) {
            case 'v':
                traceflags = -1;
                continue;
            case 'f':
                filesystem = *++argv;
                argc--;
                continue;
            default:
                printf("Bad flag\n");
        } else {
            break;
        }
    }

    if (!argc) {
        usage(pname);
        exit(0);
    }
    /* initialize and mkfs make a filesystem; there is nothing to open yet */
    if (strcmp(*argv, "initialize") == 0)
        return initcmd(argc, argv);
    if (strcmp(*argv, "mkfs") == 0)
        return mkfscmd(argc, argv);
    if (!filesystem) {
        filesystem = "testfs";
    }
    if (traceflags) {
        printf("filesystem: %s\n", filesystem);
    }
    i = openfsrw(filesystem, &fs, 1);
    if (i < 0) {
        printf("can't open %s\n", filesystem);
        exit(errno);
    }

    if (traceflags) {
        printf("superblock\n");
        dumpsb(fs);
    }

    nerror = -1;
    for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        if (strcmp(cmds[i].name, *argv) == 0) {
            nerror = (*cmds[i].handler) (argc, argv);
            break;
        }
    }
    if (nerror == -1) {
        usage(pname);
    }
    closefs(fs);
    return (nerror);
}

int
iinfo(int c, char **a)
{
    int inum;
    struct dsknod *dp;
    char na[20];

    readopt(&c, &a, "f");

    if (!c) return 1;
    inum = atoi(*a);

    dp = iget(fs, inum);
    if (!dp) return 2;
    sprintf(na, "inum %d\n", inum);
    idump(na, dp, opt['f'-'a']);
    ifree(dp); 
    return 0;
}

void
list(char *name, int opts)
{
    struct dsknod *dp, *f;
    struct dir *dirp;
    int i;

    dp = namei(fs, name);
    if (!dp) {
        printf("%s: not found\n", name);
        return;
    }

    if ((dp->d_mode & IFMT) == IFDIR) {
        for (i = 0; i < ((dp->d_size0 << 16) + dp->d_size1) / 16; i++) {
            dirp = getdirent(dp, i);
            f = iget(fs, dirp->ino);
            isummary(dirp->name, f);
            ifree(f);
        }
    } else {
        isummary(name, dp);
    }
    ifree(dp);
}

int
ls(int c, char **a)
{
    int i;
    char *s;
    char o;
    int opts = 0;

    c--;
    a++;

    while (c) {
        if (**a != '-') {
            break;
        }
        s = *a;
        s++;
        while (*s) {
            switch (o = *s++) {
            default:
                printf("unknown option %c\n", o);
                break;
            }
        }
        a++;
        c--;
    }
    if (c) {
        while (c--) {
            list(*a++, opts);
        }
    } else {
        list("/", opts);
    }
    return 0;
}

void
catfile(char *name, int opts)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;

    dp = namei(fs, name);
    if (!dp) {
        printf("%s: not found\n", name);
        return;
    }

    if ((dp->d_mode & IFMT) == IFDIR) {
        printf("%s: is directory\n", name);
        return;
    }

    size = (dp->d_size0 << 16) + dp->d_size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(dp, i, buf);
        write(1, buf, valid);
    }
    ifree(dp);
}

int
cat(int c, char **a)
{
    int i;
    char *s;
    char o;
    int opts = 0;

    c--;
    a++;

    while (c--) {
        catfile(*a++, opts);
    }
    return 0;
}

void
dumpfile(char *name, int opts)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;

    dp = namei(fs, name);
    if (!dp) {
        printf("%s: not found\n", name);
        return;
    }

    if ((dp->d_mode & IFMT) == IFDIR) {
        printf("%s: is directory\n", name);
        return;
    }

    size = (dp->d_size0 << 16) + dp->d_size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(dp, i, buf);
        hexdump(buf, valid);
    }
    ifree(dp);
}

int
dumpcmd(int c, char **a)
{
    int i;
    char *s;
    char o;
    int opts = 0;

    c--;
    a++;

    while (c--) {
        dumpfile(*a++, opts);
    }
    return 0;
}

int
readcmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int outfd;

    a++;
    c--;

    if (c != 2) {
        return -1;
    }

    dp = namei(fs, *a++);
    if (!dp) {
        printf("can't find file\n");
        return 2;
    }

    if ((dp->d_mode & IFMT) != IFREG) {
        printf("need regular file\n");
        return 2;
    }

    outfd = open(*a, O_WRONLY | O_CREAT, 0777);
    if (outfd < 0) {
        printf("can't open %s for writing: %d\n", *a, errno);
        ifree(dp);
        return 2;
    }
    printf("write to %s\n", *a);
    size = (dp->d_size0 << 16) + dp->d_size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(dp, i, buf);
        write(outfd, buf, valid);
    }
    ifree(dp);
    close(outfd);
    return 0;
}

int
/*
 * write <src> <dest>, and write -k <src> <dest>.
 *
 * Without -k the destination is emptied first and the contents land on
 * whatever the free list hands out.  That is what you want for an
 * ordinary file and is exactly wrong for a boot area, which has to stay
 * on the blocks the rom will read: freeing it puts those blocks back on
 * the free list and the write then lands somewhere else entirely.
 *
 * -k keeps the block list and writes through it, so the bytes go where
 * the file already is.  It is the other half of setblk - set the blocks,
 * then write -k onto them - and it refuses to grow the file, because a
 * block that had to be allocated would not be part of the area and the
 * boot would be half in it.
 */
writecmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int infd;
    char *destname;
    int keep = 0;
    int have;
    struct stat sb;

    a++;
    c--;

    if (c && strcmp(*a, "-k") == 0) {
        keep = 1;
        a++;
        c--;
    }

    if (c != 2) {
        return -1;
    }

    infd = open(*a, O_RDONLY);
    if (infd < 0) {
        /*
         * And stop.  This used to say so and carry on, which meant
         * every read below returned -1, nothing was written, and the
         * size was set from a byte count that had gone negative - so
         * a mistyped source name replaced the destination with a file
         * of 16777215 bytes.  /etc/init went that way once.
         */
        printf("can't open %s for reading: %d\n", *a, errno);
        return 2;
    }
    
    destname = *++a;
    dp = namei(fs, destname);
    if (!dp) {
        /*
         * The destination is not there yet, which is the usual case
         * for write - it creates what it writes.  Nothing to say
         * about it: filecreate reports a real failure below.
         */
        dp = filecreate(fs, destname);
        if (!dp) {
            printf("can't create file %s\n", destname);
            return 2;
        }
    }

    if ((dp->d_mode & IFMT) != IFREG) {
        printf("need regular file\n");
        return 2;
    }

    if (keep) {
        /*
         * How much room the file already has.  bmap with alloc 0
         * answers 0 for a block that is not there, so this stops at
         * the first hole and counts what is behind it.
         */
        have = 0;
        while (bmap(dp, have * 512, 0)) {
            have++;
        }
        if (fstat(infd, &sb) == 0 && sb.st_size > have * 512) {
            printf("%s holds %d blocks, %s needs %d - refusing to grow it\n",
                destname, have, *(a - 1), (int)((sb.st_size + 511) / 512));
            close(infd);
            return 2;
        }
    } else {
        filefree(dp);
    }
    i = 0;
    do {
        valid = read(infd, buf, 512);
        if (valid < 0) {
            /*
             * Whatever has been written stays; the size is what was
             * actually read, not what the count reached after a -1.
             */
            printf("read error on the source: %d\n", errno);
            break;
        }
        if (filewrite(dp, i, buf) != 512) {
            printf("write failed\n");
        }
        i += valid;
    } while (valid == 512);
    dp->d_size0 = i >> 16;
    dp->d_size1 = i & 0xffff;
    iput(dp);
    close(infd);
    return 0;
}

int 
rmcmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int infd;
    char *dirname;
    char *filename;

    a++;
    c--;

    if (c != 1) {
        return -1;
    }

    fileunlink(fs, *a);
    return 0;
}

int 
infocmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int infd;
    char *dirname;
    char *filename;

    a++;
    c--;

    if (c != 1) {
        return -1;
    }

    dp = namei(fs, *a);
    idump(*a, dp, 0);

    return 0;
}

int 
emptycmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int infd;
    char *dirname;
    char *filename;

    a++;
    c--;

    if (c != 1) {
        return -1;
    }

    dp = namei(fs, *a);
    filefree(dp);

    return 0;
}

int 
rmdircmd(int c, char **a)
{
    return 1;
}

int
mkdircmd(int c, char **a)
{
    a++;
    c--;
    if (c != 1)
        return -1;
    if (imagemkdir(a[0]) < 0)
        return 2;
    return 0;
}


/*
 * Give a file a block list of our choosing.
 *
 * This exists for one job: a boot area has to be at a fixed place on the
 * platter, and the only thing in a v6 filesystem that keeps a block from
 * being handed out is an inode claiming it.  So the file is made to own
 * the blocks the hardware will read, and then writing the file writes
 * the boot area, through the same mapping the kernel and the rom both
 * use rather than a second opinion about where the sector is.
 *
 * Two things this does not do, deliberately.  It does not take the
 * blocks off the free list - run icheck -s afterwards, which rebuilds
 * the list from what the inodes claim, and until you do the blocks are
 * both owned and queued to be handed out.  And it does not write any
 * contents; set the blocks first, then write the file, and the write
 * lands on them.
 */
int
setblkcmd(int c, char **a)
{
    struct dsknod *dp;
    UINT blocks[256];
    UINT iblk[256];
    int nblk = 0;
    int iblkno;
    char *path;
    int i;

    a++;
    c--;
    if (c < 2) {
        return -1;
    }
    path = *a++;
    c--;

    while (c-- && nblk < 256) {
        blocks[nblk++] = atoi(*a++);
    }

    dp = namei(fs, path);
    if (!dp) {
        printf("can't find %s\n", path);
        return 2;
    }
    if ((dp->d_mode & IFMT) != IFREG) {
        printf("%s is not a regular file\n", path);
        return 2;
    }

    /*
     * Eight addresses in an inode.  Up to eight blocks they are the
     * blocks; past that they are indirect blocks holding 256 apiece, and
     * the file has to say so with ILARG.  A cylinder is more than eight
     * blocks on every drive we care about, so the second case is the
     * usual one here, not the exception.
     */
    if (nblk <= 8) {
        dp->d_mode &= ~ILARG;
        for (i = 0; i < 8; i++) {
            dp->d_addr[i] = (i < nblk) ? blocks[i] : 0;
        }
    } else {
        iblkno = balloc(fs);
        if (!iblkno) {
            printf("no block for the indirect\n");
            return 2;
        }
        bzero((char *)iblk, 512);
        for (i = 0; i < nblk; i++) {
            iblk[i] = blocks[i];
        }
        writeblk(fs, iblkno, (char *)iblk);
        for (i = 0; i < 8; i++) {
            dp->d_addr[i] = 0;
        }
        dp->d_addr[0] = iblkno;
        dp->d_mode |= ILARG;
    }

    /*
     * And the size, or it is an empty file with a block list: reads
     * return nothing and a write goes to the allocator instead of to the
     * blocks sitting right there in the inode.
     */
    dp->d_size0 = 0;
    dp->d_size1 = nblk * 512;
    iput(dp);

    printf("%s: %d blocks, %d bytes%s", path, nblk, nblk * 512,
        (nblk > 8) ? ", ILARG" : "");
    if (nblk > 8) {
        printf(", indirect in %d", iblkno);
    }
    printf("\nnow run icheck -s to rebuild the free list\n");
    return 0;
}

int
blkcmd(int c, char **a)
{
    unsigned char buf[512];
    unsigned char newbuf[512];
    int blkno;

    readopt(&c, &a, "e");

    if (!c) return 1;
    blkno = atoi(*a);

    readblk(fs, blkno, buf);
    if (opt['e'-'a']) {
        bcopy(buf, newbuf, 512);
        blockedit(newbuf, 512);
        if (bcmp(buf, newbuf, 512) != 0) {
            printf("dirty\n");
            writeblk(fs, blkno, newbuf);
        }
    } else {
        hexdump(buf, 512);
    } 
    return 0;
}

int
fsinfo(int c, char **a)
{
    unsigned short buf[256];
    int b;
    unsigned short fl[100];
    int fi;
    int k;

    dumpsb(fs);
    printf("free list: \n");

    bcopy(fs->s_free, fl, sizeof(fl));
    fi = fs->s_nfree - 1;

    /* scan through whole freelist from end */
    k = 0;
    while ((b = fl[fi]) != 0) {
        if (!fi) {
            readblk(fs, b, (char *)buf);
            fi = buf[0] - 1;
            bcopy(&buf[1], fl, sizeof(fl));
            printf(" ...");
        } else {
            fi--;
        }
        printf("%6d ", b);
        k++;
        if (k == 8) {
            k = 0;
            printf("\n");
        }
    }
    if (k) printf("\n");
}

/*
 * tar archive support, using the classic V7 tar format that cmd/tar
 * reads and writes: a 512-byte header, the data padded out to 512-byte
 * blocks, and a zero block to end.
 */

#define TBLOCK	512
#define NAMSIZ	100

union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[NAMSIZ];
	} dbuf;
};

union hblock dblock;
char *tarprefix;	/* -C: import the archive under this prefix */

int
checksum()
{
	register int i;
	register char *cp;

	for (cp = dblock.dbuf.chksum;
	     cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return i;
}

/*
 * make a directory at path.  The parent must already be there - a tar
 * archive lists a directory before its contents, so it always is.
 */
int
imagemkdir(char *path)
{
	char *dir, *name;
	char *save;
	struct dsknod *parent;
	struct dsknod *dp;
	struct dir dirbuf[32];
	int inum;

	save = dir = strdup(path);
	name = rindex(dir, '/');
	if (!name) {
		parent = iget(fs, 1);
		name = dir;
	} else {
		*name++ = 0;
		parent = namei(fs, dir);
	}
	if (!parent) {
		printf("tar: %s: parent directory not found\n", path);
		free(save);
		return -1;
	}

	inum = ialloc(fs, IFDIR | 0777);
	if (inum == 0) {
		ifree(parent);
		free(save);
		return -1;
	}

	filelink(fs, path, inum);

	dp = iget(fs, inum);
	memset(dirbuf, 0, sizeof dirbuf);
	dirbuf[0].ino = inum;
	dirbuf[0].name[0] = '.';
	dirbuf[1].ino = ((struct i_node *)parent)->inum;
	dirbuf[1].name[0] = '.';
	dirbuf[1].name[1] = '.';
	filewrite(dp, 0, (char *)dirbuf);
	dp->d_size0 = 0;
	dp->d_size1 = 2 * sizeof(struct dir);
	iput(dp);
	ifree(dp);
	ifree(parent);
	free(save);
	return 0;
}

/*
 * mkdir -p: make path and every directory in it that is not there yet.
 * This is what tar x -C's prefix wants - the prefix names directories
 * that an archive, which lists its own files, does not contain.
 */
int
imagemkdirs(char *path)
{
	char buf[512];
	char *p;

	strcpy(buf, path);
	for (p = buf + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			if (!namei(fs, buf))
				imagemkdir(buf);
			*p = '/';
		}
	}
	if (!namei(fs, buf))
		imagemkdir(buf);
	return 0;
}

/*
 * mknod <path> <b|c> <major> <minor>: make a device node.
 *
 * The device number is (major << 8) | minor, the same word the kernel's
 * cio/bio open routines read out of i_addr[0].  The node is made the way
 * mkfs and tar make a file - ialloc the inode, then link it into the
 * directory - except the type is IFBLK or IFCHR rather than IFREG and
 * the first address word carries the device number instead of a block.
 */
int
mknodcmd(int c, char **a)
{
	char *path;
	int isblk, major, minor;
	int dev, inum;
	struct dsknod *dp;

	a++;
	c--;
	if (c != 4)
		return -1;
	path = a[0];
	if (a[1][0] == 'b')
		isblk = 1;
	else if (a[1][0] == 'c')
		isblk = 0;
	else
		return -1;
	major = atoi(a[2]);
	minor = atoi(a[3]);

	if (namei(fs, path)) {
		printf("mknod: %s already exists\n", path);
		return 2;
	}
	dev = (major << 8) | minor;
	inum = ialloc(fs, (isblk ? IFBLK : IFCHR) | 0666);
	if (inum == 0) {
		printf("mknod: no inode for %s\n", path);
		return 2;
	}
	dp = iget(fs, inum);
	dp->d_addr[0] = dev;
	iput(dp);
	ifree(dp);
	filelink(fs, path, inum);
	return 0;
}

/*
 * devlist: print the device nodes in /dev, one per line, as
 *
 *	type major minor name
 *
 * where type is b or c.  This is the prototype device list, read from a
 * working filesystem and replayed onto a new one with mknod.
 */
int
devlistcmd(int c, char **a)
{
	struct dsknod *devdir;
	struct dsknod *dp;
	struct dir *dirp;
	int i, type, major, minor;

	devdir = namei(fs, "/dev");
	if (!devdir) {
		printf("devlist: no /dev\n");
		return 2;
	}
	for (i = 0; (dirp = getdirent(devdir, i)) != 0; i++) {
		if (dirp->ino == 0 || dirp->name[0] == '.')
			continue;
		dp = iget(fs, dirp->ino);
		if ((dp->d_mode & IFMT) == IFBLK)
			type = 'b';
		else if ((dp->d_mode & IFMT) == IFCHR)
			type = 'c';
		else {
			ifree(dp);
			continue;
		}
		major = (dp->d_addr[0] >> 8) & 0xff;
		minor = dp->d_addr[0] & 0xff;
		printf("%c %d %d %s\n", type, major, minor, dirp->name);
		ifree(dp);
	}
	ifree(devdir);
	return 0;
}

/*
 * tar x <tarfile>: extract the archive into the image.
 */
int
tarx(char *tarfile)
{
	int infd;
	int i;
	long size;
	int mode;
	int nblocks;
	char buf[TBLOCK];
	char fullname[512];
	struct dsknod *dp;
	int inum;

	infd = open(tarfile, O_RDONLY);
	if (infd < 0) {
		printf("tar: can't open %s: %d\n", tarfile, errno);
		return 2;
	}

	if (tarprefix)
		imagemkdirs(tarprefix);

	for (;;) {
		if (read(infd, &dblock, TBLOCK) != TBLOCK)
			break;
		if (dblock.dbuf.name[0] == '\0')
			break;
		sscanf(dblock.dbuf.mode, "%o", &mode);
		sscanf(dblock.dbuf.size, "%lo", &size);

		/* a directory's name ends with a slash, as cmd/tar writes it */
		{
			int nlen = strlen(dblock.dbuf.name);
			int isdir = 0;

			if (nlen > 0 && dblock.dbuf.name[nlen - 1] == '/') {
				dblock.dbuf.name[nlen - 1] = '\0';
				isdir = 1;
			}
			/* a host tar roots the archive at "." and names every
			 * entry "./..."; strip that, matching what tar c writes */
			if (dblock.dbuf.name[0] == '.' && dblock.dbuf.name[1] == '/')
				memmove(dblock.dbuf.name, dblock.dbuf.name + 2,
				    strlen(dblock.dbuf.name + 2) + 1);
			/* the archive's "." root is not an entry to create */
			if (dblock.dbuf.name[0] == '.' && dblock.dbuf.name[1] == '\0')
				continue;
			/* the full path: -C's prefix, then the name */
			if (tarprefix && dblock.dbuf.name[0] != '\0')
				sprintf(fullname, "%s/%s", tarprefix, dblock.dbuf.name);
			else
				strcpy(fullname, dblock.dbuf.name);
			/* the root is already there; skip it */
			if (fullname[0] == '\0')
				continue;
			if (isdir) {
				imagemkdir(fullname);
				continue;
			}
		}

		dp = namei(fs, fullname);
		if (!dp) {
			inum = ialloc(fs, IFREG | (mode & 07777));
			if (inum == 0) {
				printf("tar: can't create %s\n", fullname);
				lseek(infd, ((size + TBLOCK - 1) / TBLOCK) * TBLOCK, SEEK_CUR);
				continue;
			}
			filelink(fs, fullname, inum);
			dp = namei(fs, fullname);
		}
		if (!dp) {
			printf("tar: can't create %s\n", fullname);
			lseek(infd, ((size + TBLOCK - 1) / TBLOCK) * TBLOCK, SEEK_CUR);
			continue;
		}

		filefree(dp);
		nblocks = (size + TBLOCK - 1) / TBLOCK;
		for (i = 0; i < nblocks; i++) {
			read(infd, buf, TBLOCK);
			filewrite(dp, i * TBLOCK, buf);
		}
		dp->d_size0 = (size >> 16) & 0xff;
		dp->d_size1 = size & 0xffff;
		iput(dp);
		ifree(dp);
	}
	close(infd);
	return 0;
}

/*
 * walk one path into the archive, recursing into directories.
 */
void
tarput(char *path, int outfd)
{
	struct dsknod *dp;
	struct dir *dirp;
	long size;
	int i;
	int entries;
	int isdir;
	char child[512];
	char tarname[NAMSIZ];
	char buf[TBLOCK];

	dp = namei(fs, path);
	if (!dp) {
		printf("tar: %s: not found\n", path);
		return;
	}

	isdir = ((dp->d_mode & IFMT) == IFDIR);
	size = ((long)dp->d_size0 << 16) + dp->d_size1;

	/*
	 * the root is not itself an entry; its children are the top level
	 * of the archive, and a tar name has no leading slash.
	 */
	if (path[0] == '/' && path[1] == '\0') {
		entries = size / sizeof(struct dir);
		for (i = 0; i < entries; i++) {
			dirp = getdirent(dp, i);
			if (dirp->ino == 0)
				continue;
			if (strcmp(dirp->name, ".") == 0 || strcmp(dirp->name, "..") == 0)
				continue;
			strncpy(child, dirp->name, sizeof child - 1);
			child[sizeof child - 1] = 0;
			tarput(child, outfd);
		}
		ifree(dp);
		return;
	}

	/* the archive name: a directory carries a trailing slash */
	strncpy(tarname, path, sizeof tarname - 1);
	tarname[sizeof tarname - 1] = 0;
	if (isdir && tarname[strlen(tarname) - 1] != '/')
		strcat(tarname, "/");

	memset(&dblock, 0, TBLOCK);
	strncpy(dblock.dbuf.name, tarname, NAMSIZ - 1);
	sprintf(dblock.dbuf.mode, "%6o ", dp->d_mode & 07777);
	sprintf(dblock.dbuf.uid, "%6o ", dp->d_uid);
	sprintf(dblock.dbuf.gid, "%6o ", dp->d_gid);
	sprintf(dblock.dbuf.size, "%11lo ", isdir ? 0L : size);
	sprintf(dblock.dbuf.mtime, "%11lo ", (long)dp->d_mtime);
	sprintf(dblock.dbuf.chksum, "%6o", checksum());
	write(outfd, &dblock, TBLOCK);

	if (isdir) {
		entries = size / sizeof(struct dir);
		for (i = 0; i < entries; i++) {
			dirp = getdirent(dp, i);
			if (dirp->ino == 0)
				continue;
			if (strcmp(dirp->name, ".") == 0 || strcmp(dirp->name, "..") == 0)
				continue;
			if (path[0] == '/' && path[1] == '\0')
				sprintf(child, "/%s", dirp->name);
			else
				sprintf(child, "%s/%s", path, dirp->name);
			tarput(child, outfd);
		}
	} else {
		for (i = 0; i < size; i += TBLOCK) {
			memset(buf, 0, TBLOCK);
			fileread(dp, i, buf);
			write(outfd, buf, TBLOCK);
		}
	}
	ifree(dp);
}

/*
 * tar c <tarfile> [path ...]: write the image, or the named paths, to
 * the archive.
 */
int
tarc(int c, char **a)
{
	int outfd;
	int i;

	outfd = open(a[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (outfd < 0) {
		printf("tar: can't create %s: %d\n", a[0], errno);
		return 2;
	}

	if (c == 1)
		tarput("/", outfd);
	else
		for (i = 1; i < c; i++)
			tarput(a[i], outfd);

	memset(&dblock, 0, TBLOCK);
	write(outfd, &dblock, TBLOCK);
	write(outfd, &dblock, TBLOCK);
	close(outfd);
	return 0;
}

/*
 * The drives FORMATMW formats, keyed by name.  Geometry is sys/mw.c's
 * specs[]; the sector size is 512, the only size Micronix reads.
 */
struct medium {
    char *name;
    int tracks;
    int heads;
    int spt;
} mediums[] = {
    {"m5",  153, 4, 17},
    {"m10", 306, 4, 17},
    {"m16", 306, 6, 17},
    {"m32", 640, 6, 17},
    {"m40", 733, 5, 17},
};

/*
 * initialize <medium> <image>: simulate what FORMATMW does.  A format
 * writes the out-of-band volume label - the geometry and the sector
 * header values - and leaves the data area blank for mkfs.  This is
 * the whole of it.
 */
int
initcmd(int c, char **a)
{
    struct disklabel label;
    struct medium *m;
    long nblocks;
    long size;
    int fd;
    int i;

    a++;
    c--;

    if (c != 2)
        return -1;

    for (i = 0; i < sizeof(mediums) / sizeof(mediums[0]); i++)
        if (strcmp(mediums[i].name, a[0]) == 0)
            break;
    if (i == sizeof(mediums) / sizeof(mediums[0])) {
        printf("initialize: unknown medium %s\n", a[0]);
        return 2;
    }
    m = &mediums[i];

    fd = open(a[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        printf("initialize: can't create %s: %d\n", a[1], errno);
        return 2;
    }

    memset(&label, 0, sizeof label);
    label.magic = MAGIC;
    label.secsize = 512;
    label.cylinders = m->tracks;
    label.heads = m->heads;
    label.spt = m->spt;
    label.firstsec = 0;
    label.seccode = 3;      /* 512-byte sectors, the specify form */
    label.gap3 = 43;
    label.fill = 0xe5;
    label.formatted = 1;

    if (write(fd, &label, sizeof label) != sizeof label) {
        printf("initialize: can't write label on %s: %d\n", a[1], errno);
        close(fd);
        return 2;
    }

    nblocks = (long)m->tracks * m->heads * m->spt;
    size = DATAOFF + nblocks * 512;
    if (ftruncate(fd, size) < 0) {
        printf("initialize: can't size %s: %d\n", a[1], errno);
        close(fd);
        return 2;
    }
    close(fd);

    printf("initialized %s: %d cylinders, %d heads, %d sectors, %ld blocks\n",
        a[1], m->tracks, m->heads, m->spt, nblocks);
    return 0;
}

/*
 * The block i/o for the host: fslib's rotation-aware read/write against
 * the simulated drive image.  These are the same two names mkfs.c's
 * driver defines against the raw unix device; mkfsfunc.c calls them.
 */
void
rdblk(bn, buf)
    UINT bn;
    char *buf;
{
    readblk(fs, bn, buf);
}

void
wrblk(bn, buf)
    UINT bn;
    char *buf;
{
    if (bn == 0)
        return;     /* block 0 is reserved and always reads zero */
    writeblk(fs, bn, buf);
}

/*
 * mkfs <image> [size|-exclude] [-i bootfile] [-f]: make a filesystem on
 * a drive image initialize made.  The geometry comes out of the label,
 * so the medium is not named again.
 */
int
mkfscmd(int c, char **a)
{
    struct disklabel label;
    char *image;
    char *bfile;
    UINT dsize;
    UINT fsize;
    UINT isize;
    UINT bootfirst;
    UINT bootnblk;
    UINT spc;
    UINT given;
    UINT exclude;
    int type;
    int f;
    int fd;
    int i;

    a++;
    c--;

    image = 0;
    bfile = DEFBOOT;
    given = 0;
    exclude = 0;
    f = 0;

    while (c > 0) {
        char *arg = *a++;

        c--;
        if (arg[0] == '-' && arg[1] >= '0' && arg[1] <= '9') {
            exclude = atoi(arg + 1);
        } else if (arg[0] == '-') {
            if (arg[1] == 'f' && arg[2] == 0) {
                f = 1;
                continue;
            }
            if (arg[1] != 'i' || arg[2] != 0 || c < 1)
                return -1;
            c--;
            bfile = *a++;
        } else if (arg[0] >= '0' && arg[0] <= '9') {
            given = atoi(arg);
        } else {
            if (image)
                return -1;
            image = arg;
        }
    }
    if (!image)
        return -1;
    if (given && exclude) {
        printf("mkfs: give a size or an exclusion, not both\n");
        return 2;
    }

    /* the geometry, out of the label initialize wrote */
    fd = open(image, O_RDONLY);
    if (fd < 0 || read(fd, &label, sizeof label) != sizeof label) {
        printf("mkfs: can't read the label on %s\n", image);
        if (fd >= 0)
            close(fd);
        return 2;
    }
    close(fd);
    if (label.magic != MAGIC || !label.formatted) {
        printf("mkfs: %s is not formatted - run initialize first\n", image);
        return 2;
    }
    for (type = 0; type < NDRIVE; type++)
        if (dtracks[type] == label.cylinders && dheads[type] == label.heads
            && dsecs[type] == label.spt)
            break;
    if (type == NDRIVE) {
        printf("mkfs: %d/%d/%d is not a drive this tree knows\n",
            label.cylinders, label.heads, label.spt);
        return 2;
    }

    dsize = (UINT) label.cylinders * label.heads * label.spt;
    if (given) {
        fsize = given;
        if (fsize > dsize) {
            printf("mkfs: that is bigger than the device\n");
            return 2;
        }
    } else {
        if (exclude >= dsize) {
            printf("mkfs: nothing left after the exclusion\n");
            return 2;
        }
        fsize = dsize - exclude;
    }
    if (fsize < 50) {
        printf("mkfs: too small to be a filesystem\n");
        return 2;
    }
    isize = fsize / 43 + fsize / 1000;
    if (isize < 1)
        isize = 1;

    spc = (UINT) label.heads * label.spt;
    bootfirst = (label.cylinders - (label.cylinders >> 1)) * spc;
    bootnblk = spc;

    i = openfsrw(image, &fs, 1);
    if (i < 0) {
        printf("mkfs: can't open %s\n", image);
        return 2;
    }

    pname = "mkfs";
    domkfs(fsize, isize, bootfirst, bootnblk, dsize, type, bfile, f);
    closefs(fs);
    return 0;
}

int
tarcmd(int c, char **a)
{
	a++;
	c--;
	if (c == 0)
		return -1;
	if (strcmp(a[0], "x") == 0) {
		a++;
		c--;
		tarprefix = 0;
		if (c >= 2 && strcmp(a[0], "-C") == 0) {
			tarprefix = a[1];
			a += 2;
			c -= 2;
		}
		if (c != 1)
			return -1;
		return tarx(a[0]);
	}
	if (strcmp(a[0], "c") == 0) {
		if (c < 2)
			return -1;
		return tarc(c - 1, a + 1);
	}
	return -1;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
