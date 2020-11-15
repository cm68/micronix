/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * cp
 */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

#define	MAXPATHL	512
#define	MAXBSIZE	512

#ifdef linux
#define INIT
#else
#define INIT = 0
#endif

int iflag INIT;
int rflag INIT;
char *rindex();

main(argc, argv)
    int argc;
    char **argv;
{
    struct stat stb;
    int rc, i;

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
    char *last, destname[MAXPATHLEN + 1], buf[MAXBSIZE];
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
        if (strlen(to) + strlen(last) >= sizeof destname - 1) {
            fprintf(stderr, "cp: %s/%s: Name too long", to, last);
            close(fold);
            return (1);
        }
        sprintf(destname, "%s/%s", to, last);
        to = destname;
    }
    if (rflag && (stfrom.st_mode & S_IFMT) == S_IFDIR) {
        int fixmode = 0;        /* cleanup mode after rcopy */

        close(fold);
        if (stat(to, &stto) < 0) {
            if (mkdir(to, (stfrom.st_mode & 0777) | 0700) < 0) {
                Perror(to);
                return (1);
            }
            fixmode = 1;
        } else if ((stto.st_mode & S_IFMT) != S_IFDIR) {
            fprintf(stderr, "cp: %s: Not a directory.\n", to);
            return (1);
        }
        n = rcopy(from, to);
        if (fixmode)
            chmod(to, stfrom.st_mode & 0777);
        return (n);
    }

    if ((stfrom.st_mode & S_IFMT) == S_IFDIR)
        fprintf(stderr,
            "cp: %s: Is a directory (copying as plain file).\n", from);

    exists = stat(to, &stto) == 0;
    if (exists) {
        if (stfrom.st_dev == stto.st_dev && stfrom.st_ino == stto.st_ino) {
            fprintf(stderr,
                "cp: %s and %s are identical (not copied).\n", from, to);
            close(fold);
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
                return (1);
            }
        }
    }
    fnew = creat(to, stfrom.st_mode & 0777);
    if (fnew < 0) {
        Perror(to);
        close(fold);
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
            return (1);
        }
        if (write(fnew, buf, n) != n) {
            Perror(to);
            close(fold);
            close(fnew);
            return (1);
        }
    }
    close(fold);
    close(fnew);
    return (0);
}

rcopy(from, to)
    char *from, *to;
{
    DIR *fold = opendir(from);
    struct direct *dp;
    int errs = 0;
    char fromname[MAXPATHLEN + 1];

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
        if (strlen(from) + 1 + strlen(dp->d_name) >= sizeof fromname - 1) {
            fprintf(stderr, "cp: %s/%s: Name too long.\n", from, dp->d_name);
            errs++;
            continue;
        }
        sprintf(fromname, "%s/%s", from, dp->d_name);
        errs += copy(fromname, to);
    }
}

Perror(s)
    char *s;
{

    fprintf(stderr, "cp: ");
    perror(s);
}
