/*
 * name.c 
 */
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/inode.h>

extern struct inode *rootdir;

/*
 * Turn a pathname into a locked inode.
 * Leaves u.offset and u.segflg ready
 * for a directory write.
 */
struct inode *
iname(name)
    char *name;
{
    fast struct inode *ip, *parent;
    fast int inum;

    u.iparent = NULL;
    for (ip = u.cdir; getbyte(name) == '/'; name++)
        ip = rootdir;
    ilock(ip);
    for (;;) {
        if (getbyte(name) == 0)
            return (ip);
        if ((ip->mode & ITYPE) != IDIR) {
            u.error = ENOTDIR;
            break;
        }
        u.iparent = parent = ip;
        if (!access(ip, IEXEC))
            break;
        if ((inum = dlook(ip, &name)) == 0)
            break;
        parent->count++;
        irelse(parent);
        ip = iget(inum, parent->dev);
        parent->count--;
        if (ip == NULL)
            return NULL;
    }

    irelse(ip);                 /* no sleeps between now and create */
    return NULL;
}

/*
 * Search the directory for the next name in the pathname.
 * Advance *np past the name and any /'s, and leave a 0
 * terminated copy of the name in u.dir.name. If found:
 * leave u.offset ready to overwrite the entry, leave
 * a copy of the entry in u.dirent, and return the inumber.
 * If not found: leave u.offset at a free slot or at the end,
 * and return 0.
 */
dlook(ip, np)
    struct inode *ip;
    char **np;
{
    fast UINT nblks, tail, log;
    static UINT phys, end, dummy;
    static char c, *p, *nambuf;
    static struct buf *b;
    static struct dir *d;
    struct dir *place;
    UINT slot;

    nambuf = u.dir.name;
    copyin(*np, nambuf, 14);
    for (p = nambuf, end = &nambuf[14]; p < end; p++)
        if (*p == 0 || *p == '/') {
            *p = 0;
            break;
        }
    p = *np + (p - nambuf);
    until((c = getbyte(p)) == 0 || c == '/')
        p++;
    while (getbyte(p) == '/')
        p++;
    *np = p;

    nblks = (ip->size >> 9);    /* whole blocks */
    tail = ip->size & 511;
    if (tail)
        nblks++;
    slot = 0;
    place = 0;
    for (log = 0; log < nblks; log++) {
        if ((phys = imap(ip, log, &dummy)) == 0)
            continue;
        if ((b = bread(phys, ip->dev)) == 0)
            continue;
        end = b->data + ((tail && log == nblks - 1) ? tail : 512);
        for (d = b->data; d < end; d++, place++) {
            if (d->inum == 0) {
                if (slot == 0)
                    slot = place;
            } else if (direqu(d->name, u.dir.name)) {
                u.offset = place;
                brelse(b);
                return (d->inum);
            }
        }
        brelse(b);
    }
    /*
     * Name not found.
     */
    u.offset = (slot) ? slot : place;
    if (!u.error) {
        if (getbyte(*np) == '\0')
            u.error = ENOENT;
        else
            u.error = ENOTDIR;  /* for imkfile */
    }
    return (0);
}

/*
 * Check equality of two directory entries
 */
direqu(a, b)
    char *a, *b;
{
    static char *x, *y;
    static int i;

    x = a;
    y = b;
    for (i = 0; i < 14; i++) {
        if (*x != *y)
            return NO;
        if (*x == 0)
            return YES;
        x++;
        y++;
    }
    return YES;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
