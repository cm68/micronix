/*
 * directory name to inode translation
 * 
 * sys/name.c 
 * Changed: <>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <errno.h>
extern void ilock();
extern void irelse();
extern void brelse();
extern unsigned char direqu();
extern unsigned char access();

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
    register struct inode *ip, *parent;
    register int inum;

    u.iparent = 0;
    for (ip = u.cdir; getbyte(name) == '/'; name++)
        ip = rootdir;
    ilock(ip);
    for (;;) {
        if (getbyte(name) == 0)
            return (ip);
        if ((ip->i_mode & IFMT) != IFDIR) {
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
        ip = iget(inum, parent->i_dev);
        parent->count--;
        if (ip == 0)
            return 0;
    }

    irelse(ip);                 /* no sleeps between now and create */
    return 0;
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
    register UINT nblks, tail, log;
    static UINT phys, end, dummy;
    static UINT8 nents;
    static char c, *p, *nambuf;
    static struct buf *b;
    static struct dir *d;
    /*
     * The place of an entry is its byte offset in the directory, not an
     * address.  It used to be a struct dir * advanced alongside d, so
     * that place++ carried the 16 bytes per entry for free; but a
     * directory may be larger than the 16 bits such a pointer holds,
     * and storing it in u.offset made the conversion of a pointer to
     * the 32-bit offset depend on the compiler widening it.  Count the
     * block and the entry instead: place is the offset of the block,
     * and the entry's own offset is i entries of 16 bytes in.
     */
    UINT32 place;               /* byte offset of the block being read */
    UINT32 slot;                /* byte offset of the first free entry */
    UINT inum;
    UINT i;

    nambuf = u.dir.name;
    copyin(*np, nambuf, 14);
    for (p = nambuf, end = &nambuf[14]; p < end; p++)
        if (*p == 0 || *p == '/') {
            *p = 0;
            break;
        }
    p = *np + (p - nambuf);
    while (!((c = getbyte(p)) == 0 || c == '/'))
        p++;
    while (getbyte(p) == '/')
        p++;
    *np = p;

    nblks = (ip->size >> 9);    /* whole blocks */
    tail = ip->size & 511;
    if (tail)
        nblks++;
    slot = 0;
    for (log = 0; log < nblks; log++) {
        place = (UINT32) log << 9;      /* this block's byte offset */
        if ((phys = imap(ip, log, &dummy, IREAD)) == 0)
            continue;
        if ((b = bread(phys, ip->i_dev)) == 0)
            continue;
        d = (struct dir *)bhold(b);
        /*
         * Scan by entry count, not by an end pointer: a buffer's data can
         * sit at 0xee00, the top of the BUFSEG window, where b->data + 512
         * wraps past 0xffff and the old `d < end` test was never true, so
         * the whole block was skipped.  The last block may be short.
         */
        nents = ((tail && log == nblks - 1) ? tail : 512) >> 4;   /* 16-byte entries */
        for (i = 0; i < nents; i++, d++) {
            if (d->inum == 0) {
                if (slot == 0)
                    slot = place + (i << 4);
            } else if (direqu(d->name, u.dir.name)) {
                u.offset = place + (i << 4);
                inum = d->inum;     /* read it before the window goes */
                brel();
                brelse(b);
                return (inum);
            }
        }
        brel();
        brelse(b);
    }
    /*
     * Name not found.
     */
    u.offset = (slot) ? slot : ip->size;    /* a free slot, or the end */
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
unsigned char
direqu(a, b)
    char *a, *b;
{
    static char *x, *y;
    static UINT8 i;

    x = a;
    y = b;
    for (i = 0; i < 14; i++) {
        if (*x != *y)
            return 0;
        if (*x == 0)
            return 1;
        x++;
        y++;
    }
    return 1;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
