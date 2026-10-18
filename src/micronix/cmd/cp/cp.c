/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * cmd/cp/cp.c
 * Changed: <2022-01-06 16:31:58 curt>
 */

/*
 * cp
 */
#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

#define	MAXBSIZE	512

#ifdef linux
#define INIT
#else
#define INIT = 0
#endif

/* truth values, so bytes */
char iflag INIT;
char rflag INIT;
char vflag INIT;
char cflag INIT;
char sflag INIT;
char *rindex();
char *malloc();
long lseek();

/*
 * The cptree destination, remembered before the walk.  Copying / into /b
 * (the install) walks / and meets /b on the way down; descending into it
 * would copy what is being written, then that copy, without end.
 */
static UINT wormdev;
static UINT wormino;
static char wormset;

main(argc, argv)
    int argc;
    char **argv;
{
    struct stat stb;
    int rc, i;
    char *base, *slash;

    /* cptree is this program under a second name: it is always
     * recursive, because a tree copy is the whole point of it.  The
     * alias is a link to this binary, so the name is all it takes.
     */
    base = argv[0];
    slash = rindex(argv[0], '/');
    if (slash)
        base = slash + 1;
    if (strcmp(base, "cptree") == 0) {
        rflag++;
        cflag++;
    }

    argc--, argv++;
    while (argc > 0 && **argv == '-') {
        (*argv)++;
        while (**argv)
            switch (*(*argv)++) {

            case 'i':
                iflag++;
                break;

            case 'R':
            case 'r':
                rflag++;
                break;

            case 'v':
                vflag++;
                break;

            case 's':
                sflag++;
                break;

            default:
                goto usage;
            }
        argc--;
        argv++;
    }
    if (argc < 2)
        goto usage;
    if (argc > 2) {
        if (stat(argv[argc - 1], &stb) < 0)
            goto usage;
        if ((stb.st_mode & S_IFMT) != S_IFDIR)
            goto usage;
    }
    rc = 0;
    /*
     * cptree copies the source directory's *contents* into the
     * destination, so the destination names each entry directly rather
     * than a copy of the source directory under its own name.  rcopy
     * walks the source and copy() appends each entry's basename, which
     * is exactly the two-level behaviour; calling copy() on the top
     * would name a subdirectory of the destination after the source.
     */
    if (cflag) {
        /*
         * The destination's inode is its identity wherever the walk
         * finds it: it is an entry of the source whenever the source
         * contains it, and that entry is what must be skipped.
         */
        if (stat(argv[1], &stb) == 0) {
            wormdev = stb.st_dev;
            wormino = stb.st_ino;
            wormset = 1;
        }
        rc = rcopy(argv[0], argv[1]);
    } else
        for (i = 0; i < argc - 1; i++)
            rc |= copy(argv[i], argv[argc - 1]);
    exit(rc);
  usage:
    fprintf(stderr, "Usage: cp [-is] f1 f2; or: cp [-irs] f1 ... fn d2\n");
    exit(1);
}

/*
 * Is every byte zero?  In sparse mode a whole block that is asks to be
 * left as a hole in the copy rather than written.
 */
static
allzero(buf, n)
register char *buf;
register int n;
{
    while (n--)
        if (*buf++)
            return 0;
    return 1;
}

copy(from, to)
    char *from, *to;
{
    int fold, fnew, n, exists;
    char *last, *destname = 0;
    static char buf[MAXBSIZE];
    struct stat stfrom, stto;

    /*
     * Stat before opening.  A device node is answered from its inode, and
     * opening it is what a console or a printer port blocks or spins on -
     * the copy below decides what to do with it without ever opening it.
     */
    if (stat(from, &stfrom) < 0) {
        Perror(from);
        return (1);
    }
    /*
     * The destination met on the way down: skip it.  Descending would
     * copy the directory being written into itself - /b becomes /b/b,
     * and the walk never ends.
     */
    if (wormset && (stfrom.st_mode & S_IFMT) == S_IFDIR &&
        stfrom.st_dev == wormdev && stfrom.st_ino == wormino)
        return (0);
    if (stat(to, &stto) >= 0 && (stto.st_mode & S_IFMT) == S_IFDIR) {
        last = rindex(from, '/');
        if (last)
            last++;
        else
            last = from;
        /*
         * The destination name is the destination directory plus this
         * entry's basename.  Size it for exactly that, not PATH_MAX, and
         * hand the pointer to the recursive walk; each level frees its
         * own on the way back up.
         */
        destname = malloc((unsigned)(strlen(to) + strlen(last) + 2));
        if (destname == 0) {
            fprintf(stderr, "cp: out of memory\n");
            return (1);
        }
        sprintf(destname, "%s/%s", to, last);
        to = destname;
    }
    if (vflag)
        fprintf(stderr, "%s\n", to);
    /*
     * A device is its mode and its rdev, not its contents.  Reading one
     * to copy it is at best pointless and at worst a hang: reading
     * /dev/console waits for a byte that nobody is going to type, which
     * is what a tree copy of / into a new root used to do.
     */
    if ((stfrom.st_mode & S_IFMT) == IFCHR ||
        (stfrom.st_mode & S_IFMT) == IFBLK) {
        if (stat(to, &stto) >= 0)
            unlink(to);
        if (mknod(to, stfrom.st_mode, stfrom.st_addr[0]) < 0) {
            Perror(to);
            free(destname);
            return (1);
        }
        free(destname);
        return (0);
    }
    fold = open(from, 0);
    if (fold < 0) {
        Perror(from);
        free(destname);
        return (1);
    }
    if (rflag && (stfrom.st_mode & S_IFMT) == S_IFDIR) {
        int fixmode = 0;        /* cleanup mode after rcopy */

        close(fold);
        if (stat(to, &stto) < 0) {
            if (mkdir(to, (stfrom.st_mode & 0777) | 0700) < 0) {
                Perror(to);
                free(destname);
                return (1);
            }
            fixmode = 1;
        } else if ((stto.st_mode & S_IFMT) != S_IFDIR) {
            fprintf(stderr, "cp: %s: Not a directory.\n", to);
            free(destname);
            return (1);
        }
        n = rcopy(from, to);
        if (fixmode)
            chmod(to, stfrom.st_mode & 0777);
        free(destname);
        return (n);
    }

    if ((stfrom.st_mode & S_IFMT) == S_IFDIR)
        fprintf(stderr,
            "cp: %s: Is a directory (copying as plain file).\n", from);

    exists = stat(to, &stto) == 0;
    if (exists) {
        if (stfrom.st_dev == stto.st_dev && 
			stfrom.st_ino == stto.st_ino &&
			stfrom.d.d_size0 == stto.d.d_size0 && 
			stfrom.d.d_size1 == stto.d.d_size0 && 
			stfrom.st_mode == stto.st_mode) {
            fprintf(stderr,
                "cp: %s and %s are identical (not copied).\n", from, to);
            close(fold);
            free(destname);
            return (1);
        }
        if (iflag && isatty(fileno(stdin))) {
            int i, c;

            fprintf(stderr, "overwrite %s? ", to);
            i = c = getchar();
            while (c != '\n' && c != EOF)
                c = getchar();
            if (i != 'y') {
                close(fold);
                free(destname);
                return (1);
            }
        }
    }
    fnew = creat(to, stfrom.st_mode & 0777);
    if (fnew < 0) {
        Perror(to);
        close(fold);
        free(destname);
        return (1);
    }

    for (;;) {
        n = read(fold, buf, sizeof buf);
        if (n == 0)
            break;
        if (n < 0) {
            Perror(from);
            close(fold);
            close(fnew);
            free(destname);
            return (1);
        }
        /*
         * A whole block of zeros is not written: the copy seeks over it,
         * so its block list carries no block and a reader that knows holes
         * (the boot loader) skips it.  A short block is written even if it
         * is zeros, because a seek does not extend the file.
         */
        if (sflag && n == sizeof buf && allzero(buf, n)) {
            if (lseek(fnew, (long)n, SEEK_CUR) < 0) {
                Perror(to);
                close(fold);
                close(fnew);
                free(destname);
                return (1);
            }
        } else if (write(fnew, buf, n) != n) {
            Perror(to);
            close(fold);
            close(fnew);
            free(destname);
            return (1);
        }
    }
    /*
     * A trailing hole leaves the copy short, because the seek did not
     * extend it.  Bring the length up to the source's so the copy reads
     * back the same number of bytes.
     */
    if (sflag) {
        long sz = ((long)stfrom.st_size0 << 16) | stfrom.st_size1;

        if (lseek(fnew, 0L, SEEK_END) < sz) {
            buf[0] = 0;
            lseek(fnew, sz - 1, SEEK_SET);
            write(fnew, buf, 1);
        }
    }
    close(fold);
    close(fnew);
    free(destname);
    return (0);
}

rcopy(from, to)
    char *from, *to;
{
    DIR *fold = opendir(from);
    struct direct *dp;
    int errs = 0;
    char *fromname;

    if (fold == 0) {
        Perror(from);
        return (1);
    }
    for (;;) {
        dp = readdir(fold);
        if (dp == 0) {
            closedir(fold);
            return (errs);
        }
        if (dp->d_ino == 0)
            continue;
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;
        /*
         * The source path is this directory plus the entry's name, sized
         * for exactly that.  copy() recurses for a subdirectory and reads
         * the pointer, so it is freed here once copy() has returned.
         */
        fromname = malloc((unsigned)(strlen(from) + strlen(dp->d_name) + 2));
        if (fromname == 0) {
            fprintf(stderr, "cp: out of memory\n");
            errs++;
            continue;
        }
        sprintf(fromname, "%s/%s", from, dp->d_name);
        errs += copy(fromname, to);
        free(fromname);
    }
}

Perror(s)
    char *s;
{

    fprintf(stderr, "cp: ");
    perror(s);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

