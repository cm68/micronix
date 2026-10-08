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
char *rindex();
char *malloc();

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
    if (cflag)
        rc = rcopy(argv[0], argv[1]);
    else
        for (i = 0; i < argc - 1; i++)
            rc |= copy(argv[i], argv[argc - 1]);
    exit(rc);
  usage:
    fprintf(stderr, "Usage: cp [-i] f1 f2; or: cp [-ir] f1 ... fn d2\n");
    exit(1);
}

copy(from, to)
    char *from, *to;
{
    int fold, fnew, n, exists;
    char *last, *destname = 0;
    static char buf[MAXBSIZE];
    struct stat stfrom, stto;

    fold = open(from, 0);
    if (fold < 0) {
        Perror(from);
        return (1);
    }
    if (fstat(fold, &stfrom) < 0) {
        Perror(from);
        close(fold);
        return (1);
    }
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
            close(fold);
            return (1);
        }
        sprintf(destname, "%s/%s", to, last);
        to = destname;
    }
    if (vflag)
        fprintf(stderr, "%s\n", to);
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
        if (write(fnew, buf, n) != n) {
            Perror(to);
            close(fold);
            close(fnew);
            free(destname);
            return (1);
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

