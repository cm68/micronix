
#include <stdio.h>
#include <stdlib.h>

#include "../micronix/include/types.h"
#include "../micronix/include/sys/fs.h"
#include "../micronix/include/sys/dir.h"
#include "../include/fslib.h"
#include "../include/util.h"

int traceflags;
extern int trace_fs;
int verbose;
int extra;

struct super *fs;
int sflg;
int aflg;

#define	NI	20
#define	NDIRS	787

void pass1(), pass2(), pass3();

int iilist[NI] = { -1 };

struct htab {
    int hino;
    int hpino;
    char hname[14];
} htab[NDIRS];

int nhent = 10;
void (*pass[])() = { pass1, pass2, pass3 };

char *lasts;
int ino;
int nerror;
int nfiles;
struct htab *hlookup();

void
check(file)
    char *file;
{
    struct dsknod *dp;
    int i, j, pno;

    i = openfs(file, &fs);
    if (i < 0) {
        printf("cannot open %s\n", file);
        return;
    }
    printf("%s:\n", file);

    nfiles = fs->s_isize * I_PER_BLK;
    for (i = 0; i < NDIRS; i++)
        htab[i].hino = 0;
    for (pno = 0; pno < 3; pno++) {
        for (ino = 1; ino < nfiles; ino++) {
            dp = iget(fs, ino);
            (*pass[pno]) (dp);
            ifree(dp);
        }
    }
    closefs(fs);
}

void
pass1(struct dsknod *dp)
{
    if ((dp->d_mode & IALLOC) == 0 || (dp->d_mode & IFMT) != IFDIR)
        return;
    hlookup(ino, 1);
}

int
dotname(struct dir *dp)
{
    if (dp->name[0] == '.')
        if (dp->name[1] == 0 || dp->name[1] == '.' && dp->name[2] == 0)
            return (1);
    return (0);
}

void
pass2(dp)
    struct dsknod *dp;
{
    int doff;
    struct htab *hp;
    struct dir *dirp;
    int i;

    if ((dp->d_mode & IALLOC) == 0 || (dp->d_mode & IFMT) != IFDIR)
        return;

    for (doff = 0; (dirp = getdirent(dp, doff)) != 0; doff++) {
        if (dirp->ino == 0)
            continue;
        if ((hp = hlookup(dirp->ino, 0)) == 0)
            continue;
        if (dotname(dirp))
            continue;
        hp->hpino = ino;
        for (i = 0; i < 14; i++)
            hp->hname[i] = dirp->name[i];
    }
}

void
pname(int i, int lev)
{
    struct htab *hp;

    if (i == 1)
        return;
    if ((hp = hlookup(i, 0)) == 0) {
        printf("???");
        return;
    }
    if (lev > 10) {
        printf("...");
        return;
    }
    pname(hp->hpino, ++lev);
    printf("/%.14s", hp->hname);
}

void
pass3(dp)
    struct dsknod *dp;
{
    int doff;
    struct dir *dirp;
    int *ilp;

    if ((dp->d_mode & IALLOC) == 0 || (dp->d_mode & IFMT) != IFDIR)
        return;

    for (doff = 0; (dirp = getdirent(dp, doff)) != 0; doff++) {
        if (dirp->ino == 0)
            continue;
        if (aflg == 0 && dotname(dirp))
            continue;
        for (ilp = iilist; *ilp >= 0; ilp++)
            if (*ilp == dirp->ino)
                break;
        if (ilp > iilist && *ilp != dirp->ino)
            continue;
        printf("%d	", dirp->ino);
        pname(ino, 0);
        printf("/%.14s\n", dirp->name);
    }
}

struct htab *
hlookup(int i, int ef)
{
    struct htab *hp;

    for (hp = &htab[i % NDIRS]; hp->hino;) {
        if (hp->hino == i)
            return (hp);
        if (++hp >= &htab[NDIRS])
            hp = htab;
    }
    if (ef == 0)
        return (0);
    if (++nhent >= NDIRS) {
        printf("Out of core-- increase NDIRS\n");
        exit(1);
    }
    hp->hino = i;
    return (hp);
}

int
main(argc, argv)
    int argc;
    char **argv;
{
    char **p;
    int n, *lp;

    while (--argc) {
        argv++;
        if (**argv == '-')
            switch ((*argv)[1]) {
            case 'v':
                if (traceflags & verbose) traceflags |= trace_fs;
                traceflags |= verbose;
                continue;
            case 's':
                sflg++;
                continue;

            case 'a':
                aflg++;
                continue;

            case 'i':
                lp = iilist;
                while (lp < &iilist[NI - 1] && (n = atoi(argv[1]))) {
                    *lp++ = n;
                    argv++;
                    argc--;
                }
                *lp++ = -1;
                continue;

            default:
                printf("Bad flag\n");
            }
        check(*argv);
    }
    return (nerror);
}

__attribute__((constructor))
void
main_init()
{
    verbose = register_trace("verbose");
    extra = register_trace("extraverbose");
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
