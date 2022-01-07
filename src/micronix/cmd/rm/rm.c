/*
 * ported the V7 rm
 *
 * cmd/rm/rm.c
 * Changed: <2022-01-06 16:38:16 curt>
 */
int errcode = 0;

#include <stdio.h>
#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>

char *sprintf();

main(argc, argv)
    char *argv[];
{
    register char *arg;
    int fflg, iflg, rflg;

    fflg = 0;
    if (isatty(0) == 0)
        fflg++;
    iflg = 0;
    rflg = 0;
    if (argc > 1 && argv[1][0] == '-') {
        arg = *++argv;
        argc--;
        while (*++arg != '\0')
            switch (*arg) {
            case 'f':
                fflg++;
                break;
            case 'i':
                iflg++;
                break;
            case 'r':
                rflg++;
                break;
            default:
                printf("rm: unknown option %s\n", *argv);
                exit(1);
            }
    }
    while (--argc > 0) {
        if (!strcmp(*++argv, "..")) {
            fprintf(stderr, "rm: cannot remove `..'\n");
            continue;
        }
        rm(*argv, fflg, rflg, iflg, 0);
    }

    exit(errcode);
}

rm(arg, fflg, rflg, iflg, level)
    char arg[];
{
    struct stat buf;
    struct dir direct;
    char name[100];
    int d;

    if (stat(arg, &buf)) {
        if (fflg == 0) {
            printf("rm: %s nonexistent\n", arg);
            ++errcode;
        }
        return;
    }
    if ((buf.st_mode & S_IFMT) == S_IFDIR) {
        if (rflg) {
            if (access(arg, 02) < 0) {
                if (fflg == 0)
                    printf("%s not changed\n", arg);
                errcode++;
                return;
            }
            if (iflg && level != 0) {
                printf("directory %s: ", arg);
                if (!yes())
                    return;
            }
            if ((d = open(arg, 0)) < 0) {
                printf("rm: %s: cannot read\n", arg);
                exit(1);
            }
            while (read(d, (char *) &direct,
                    sizeof(direct)) == sizeof(direct)) {
                if (direct.ino != 0 && !dotname(direct.name)) {
                    sprintf(name, "%s/%.14s", arg, direct.name);
                    rm(name, fflg, rflg, iflg, level + 1);
                }
            }
            close(d);
            errcode += rmdir(arg, iflg);
            return;
        }
        printf("rm: %s directory\n", arg);
        ++errcode;
        return;
    }

    if (iflg) {
        printf("%s: ", arg);
        if (!yes())
            return;
    } else if (!fflg) {
        if (access(arg, 02) < 0) {
            printf("rm: %s %o mode ", arg, buf.st_mode & 0777);
            if (!yes())
                return;
        }
    }
    if (unlink(arg) && (fflg == 0 || iflg)) {
        printf("rm: %s not removed\n", arg);
        ++errcode;
    }
}

dotname(s)
    char *s;
{
    if (s[0] == '.')
        if (s[1] == '.')
            if (s[2] == '\0')
                return (1);
            else
                return (0);
        else if (s[1] == '\0')
            return (1);
    return (0);
}

rmdir(f, iflg)
    char *f;
{
    int status, i;
	char namebuf[100];
	sprintf(namebuf, "%s/..", f);
	status = 0;

    if (dotname(f))
        return (0);
    if (iflg) {
        printf("%s: ", f);
        if (!yes())
            return (0);
    }
	status += unlink(namebuf);
	i = strlen(namebuf);
	namebuf[i] = '\0';
	status += unlink(namebuf);
	status += unlink(f);
	return status;
}

yes()
{
    int i, b;

    i = b = getchar();
    while (b != '\n' && b != EOF)
        b = getchar();
    return (i == 'y');
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

