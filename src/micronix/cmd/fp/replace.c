/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/replace.c
 *
 * The r option: write files onto the volume set.  A file too large for
 * one volume is split into chapters; the directory entry's CONTINUED
 * flag and chapter number describe how the chapters chain.
 */

#include "fp.h"

void repname();
void repone();
void repdir();
int repfile();
void repspecial();
void fillin();

int
replace()
{
    char **f;

    if (!readdir())
        return NO;

    if (!cflag)
        reel = super.reel;

    if (lflag && cflag)
        getlabel();

    if (nfiles <= 0) {
        repname(".");
    } else {
        for (f = files; f < &files[nfiles]; f++)
            repname(*f);
    }

    writedir();
}

/*
 * repname - replace a single file that was named by the user.
 */
void
repname(a)
    char *a;
{
    struct stat s;
    struct dtab d;

    if (a == NULL)
        return;        /* does not represent a file */

    if (stat(a, &s) < 0) {
        perror(a);
        return;
    }

    /*
     * for incremental dumps set the time threshhold from the central
     * dump table
     */
    if (iflag) {
        threshhold = tabread(s.st_dev, s.st_ino);
    }

    repone(a, &s);

    if (iflag) {
        time(&d.date);
        d.inode = s.st_ino;
        d.device = s.st_dev;
        tabwrite(&d);
    }
}

void
repone(a, s)
    char *a;
    struct stat *s;
{
    switch (ftype(s)) {
    /*
     * special files are handled by creating only a directory entry for
     * them
     */
    case S_IFCHR:
    case S_IFBLK:
        repspecial(a, s);
        break;

    case S_IFDIR:
        repdir(a, s);
        break;

    case S_IFREG:
        repfile(a, s);
        break;
    }
}

void
repdir(a, s)
    char *a;
    struct stat *s;
{
    char path[2 * PATHSIZE], *p, *buf;
    int f, bufsize, n;
    struct entry *t;

    struct {
        int number;
        char name[16];
    } dir;

    /*
     * make an entry for the directory itself
     */
    if (!tapestat(a) && !strcmp(a, ".")) {
        for (;;) {
            t = ealloc();

            if (t)
                break;

            change(VOLFULL);
        }

        fillin(a, s, t);

        if (vflag)
            display("r", t);
    }

    /*
     * now work on all the descendents
     */
    dir.name[14] = 0;

    bufsize = fsize(s);        /* measure the directory */

    buf = malloc(bufsize);     /* allocate space */

    if (buf == NULL) {
        perror("subdirectory  buffer");
        return;
    }

    f = open(a, READ);

    if (f < 0) {
        perror(a);
        return;
    }

    bufsize = read(f, buf, bufsize);   /* read the directory */

    if (bufsize < 0)
        perror(a);

    close(f);

    strcpy(path, a);
    strcat(path, "/");
    p = path + strlen(path);

    /* process the directory, piece by piece */
    for (n = 0; n < bufsize; n += UDIRSIZE) {
        struct stat st;

        memcpy(&dir, buf + n, UDIRSIZE);

        if (dir.number == 0)
            continue;

        if (!strcmp(dir.name, "."))
            continue;

        if (!strcmp(dir.name, ".."))
            continue;

        strcpy(p, dir.name);

        if (stat(path, &st) >= 0)
            repone(path, &st);
        else
            perror(path);
    }

    free(buf);
}

/*
 * repfile - copy a single file over, possibly as many chapters on many
 * volumes.  a is the file name, b its stat information.
 */
int
repfile(a, b)
    char *a;
    struct stat *b;
{
    char linkonly;

    int f;

    unsigned chapter, bleft, btogo, offset;

    long cleft, ctogo;

    struct entry *t;

    /*
     * incremental exclusion
     */
    if (iflag && b->st_mtime < threshhold)
        return YES;

    /*
     * Decide whether to copy the file or just record an entry name for
     * a file with multiple links.  If this is not the first time we've
     * seen this file, link it only.
     */
    linkonly = b->st_nlink > 1 && lookup(a, b) != NULL;

    f = open(a, READ);

    if (f < 0) {
        perror(a);
        return NO;
    }

    chapter = 0;
    offset = 0;

    bleft = bsize(b);      /* measure the file */
    cleft = fsize(b);

    do {
        t = ealloc();

        if (!t || freespace <= 0) {
            change(VOLFULL);
            continue;
        }

        fillin(a, b, t);

        if (linkonly) {
            t->flags |= LINK;
            break;
        }

        if (bleft <= freespace) {      /* will fit on this volume */
            btogo = bleft;
            ctogo = cleft;
        } else {                       /* will need to split across volumes */
            btogo = freespace;
            ctogo = (long) BUFSIZE * freespace;
            t->flags |= CONTINUED;
        }

        t->length = ctogo;
        t->chapter = chapter;
        t->block = balloc(btogo);
        t->offset = offset;

        if (vflag)
            display("r", t);

        seek(f, t->offset, BYBLOCK);
        seek(fd, t->block, BYBLOCK);

        fcopy(f, fd, t->length, t->name, SQUARE);

        setchecksum(t);

        if (!qflag) {
            seek(fd, t->block, BYBLOCK);

            if (!fcopy(fd, READONLY, t->length, t->name, ROUND)) {
                *t->name = 0;
                change(BADMEDIA);
                continue;
            }
        }

        cleft -= ctogo;
        bleft -= btogo;
        offset += btogo;
        chapter++;
    } while (cleft > 0);

    close(f);
}

void
repspecial(a, b)
    char *a;
    struct stat *b;
{
    struct entry *t;

    if (iflag)
        return;

    for (;;) {
        t = ealloc();

        if (t)
            break;

        change(VOLFULL);
    }

    fillin(a, b, t);

    if (vflag)
        display("r", t);

    t->device = *b->st_addr;

    if (t->links > 1 && lookup(a, b))
        t->flags |= LINK;
}

void
fillin(a, b, c)
    char *a;
    struct stat *b;
    struct entry *c;
{
    memset(c, 0, sizeof *c);

    if (strlen(a) > PATHSIZE - 1) {
        prs(a);
        prs(": file name too long - substituting simple name\n");

        while (*a && strchr(a, '/'))
            a++;
    }

    strcpy(c->name, a);

    /*
     * clear the high-order bit (S_ALLOC), which the kernel keeps in the
     * inode but which is not part of the file's mode
     */
    c->mode = b->st_mode & 077777;

    c->time = b->st_mtime;
    c->user = b->st_uid;
    c->group = b->st_gid;
    c->links = b->st_nlink;
    c->major = b->st_dev >> 8;
    c->minor = b->st_dev & 0xff;
    c->inumber = b->st_ino;

    dirdirty = YES;
}
