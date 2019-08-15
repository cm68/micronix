
/*
 * this snarfs an imd file into memory, suitable for a floppy driver to
 * to read it by cyl/head/sector
 * an imd file has the following format:
 * a text header, up to a 0x1a (control-z)
 * repeated for each track:
 * mode 	1 byte
 * cylinder	1 byte
 * head		1 byte
 * numsecs	1 byte
 * secsize	1 byte in shifts of 128
 * secmap	numsecs of sector numbers
 * cylmap	numsecs of sector numbers - never seen this
 * headmap	numsecs of sector numbers - never seen this
 * a data code:  0: no data 1: secsize bytes 2: a fill byte
 */

#include "sim.h"
#include <fcntl.h>

int imd_fd = 0;

struct imd {
    char *comment;
    int tracks;
    struct imd_trk **track;
};

struct imd_trk {
    struct {
        unsigned char mode;
        unsigned char cyl;
        unsigned char head;
        unsigned char nsec;
        unsigned char size;
    } fixed;
    int secsize;
    char *secmap;
    char *cylmap;
    char *headmap;
    char **data;
};

static void hexdump(char *addr, unsigned short len);
static void dump_track(struct imd *imd, int t);

/*
 * read the imd file for track data and build the data structure
 * which then can be interrogated for format information and data
 */
struct imd_trk *
get_track()
{
    struct imd_trk *tp;
    char c;
    int nsec;
    int s;
    int i;

    tp = malloc(sizeof(*tp));

    if (read(imd_fd, tp, sizeof(tp->fixed)) != sizeof(tp->fixed))
        return 0;

    tp->secsize = 0x80 << tp->fixed.size;

    nsec = tp->fixed.nsec;
    tp->secmap = malloc(nsec);

    if (read(imd_fd, tp->secmap, nsec) != nsec)
        return 0;

    if (tp->fixed.head & 0x80) {
        tp->cylmap = malloc(nsec);
        if (read(imd_fd, tp->cylmap, nsec) != nsec)
            return 0;
    } else {
        tp->cylmap = 0;
    }

    if (tp->fixed.head & 0x40) {
        tp->headmap = malloc(nsec);
        if (read(imd_fd, tp->headmap, nsec) != nsec)
            return 0;
    } else {
        tp->headmap = 0;
    }

    tp->data = malloc(sizeof(char *) * nsec);

    for (s = 0; s < nsec; s++) {
        if (read(imd_fd, &c, 1) != 1)
            return 0;
        if (c) {
            tp->data[s] = malloc(tp->secsize);

            if (c == 2) {
                if (read(imd_fd, &c, 1) != 1)
                memset(tp->data[s], c, tp->secsize);
            } else {
                if (read(imd_fd, tp->data[s], tp->secsize) != tp->secsize)
                    return 0;
            }
        } else {
            tp->data[s] = 0;
        }
    }
    return tp;
}

/*
 * load an imd file and return the struct
 */
void *
load_imd(char *fname)
{
    struct imd *ip;
    char c = 0;
    int clen = 0;
    struct imd_trk *t[256];
    
    imd_fd = open(fname, O_RDONLY);
    // what's the size of the comment?
    do {
        if (read(imd_fd, &c, 1) != 1)
            return 0;
        clen++;
    } while (c != 0x1a);

    ip = malloc(sizeof(*ip));
    ip->comment = malloc(clen);
    ip->tracks = 0;

    lseek(imd_fd, SEEK_SET, 0);
    read(imd_fd, ip->comment, clen);
    ip->comment[clen-1] = 0;

    while ((t[ip->tracks] = get_track()) != 0) {
        ip->tracks++;
    }
    ip->track = malloc(sizeof(struct imd_trk *) * ip->tracks);
    memcpy(ip->track, t, sizeof(struct imd_trk *) * ip->tracks);
    return ip;
}

void
imd_trkinfo(void *vp, int trk, int *secs, int *secsize)
{
    struct imd *ip = (struct imd *)vp;
    struct imd_trk *tp = ip->track[trk];
    *secs = tp->fixed.nsec;
    *secsize = tp->secsize;
}

/*
 * copy the data from a sector in imd to a buffer
 * return the number of bytes
 */
int
imd_read(void *vp, int drive, int trk, int side, int sec, char *buf)
{
    struct imd *ip = (struct imd *)vp;
    struct imd_trk *tp = ip->track[trk];
    int i;
    int mysec = -1;

    if (side) {
        printf("side nonzero\n");
        verbose |= V_IMD;
        stop();
    }
    if (verbose & V_IMD) printf("imd_read drive %d trk %d side %d sec %d\n",
        drive, trk, side, sec);
    if (tp->secmap) {
        for (i = 0; i < tp->fixed.nsec; i++) {
            if (tp->secmap[i] == sec) {
                mysec = i;
            }
        }
    }
    if (mysec == -1) {
        if (verbose & V_IMD) {
            printf("imd_read: sector not found %d\n", sec);
            dump_track(ip, trk);
        }
        return 0;
    }
    memcpy(buf, tp->data[mysec], tp->secsize); 
    if (verbose & V_IMD) hexdump(buf, tp->secsize);
    return (tp->secsize);
}

static void
dump_secmap(char *label, char *m, int s)
{
    int i;

    printf("%s: ", label);
    if (!m) {
        printf("absent\n");
        return;
    }
    for (i = 0; i < s; i++) {
        printf("%d ", m[i]);
    }
    printf("\n");
}

static void
dump_track(struct imd *imd, int t)
{
    struct imd_trk *tp = imd->track[t];
    int s;

    printf("track %d\n", t);
    printf("mode: %x\n", tp->fixed.mode);
    printf("cyl: %d\n", tp->fixed.cyl);
    printf("head: %x\n", tp->fixed.head);
    printf("nsec: %d\n", tp->fixed.nsec);
    printf("size: %x\n", tp->fixed.size);
    printf("secsize: %d\n", tp->secsize);
    dump_secmap("secmap", tp->secmap, tp->fixed.nsec);
    dump_secmap("cylmap", tp->cylmap, tp->fixed.nsec);
    dump_secmap("headmap", tp->headmap, tp->fixed.nsec);

#ifdef notdef
    for (s = 0; s < tp->fixed.nsec; s++) {
        printf("sector: %d\n", s);
        if (tp->data[s]) {
            hexdump(tp->data[s], tp->secsize);
        } else {
            printf("absent\n");
        }
    }
#endif
}

void
dump_imd(struct imd *imd)
{
    int t;

    printf("comment: %s\n", imd->comment);
    printf("%d tracks\n", imd->tracks);

    for (t = 0; t < imd->tracks; t++) {
        dump_track(imd, t);
   }
}

#ifdef standalone
main(int argc, char **argv)
{
    struct imd *ip;
    ip = (struct imd *)load_imd("/dev/stdin");
    dump_imd(ip);
    exit(0);
}
#endif

static unsigned char pchars[16];
static int pcol;

static
dp()
{
    int i;
    char c;

    for (i = 0; i < pcol; i++) {
        c = pchars[i];
        if ((c <= 0x20) || (c >= 0x7f))
            c = '.';
        printf("%c", c);
    }
    printf("\n");
}

static
void
hexdump(char *addr, unsigned short len)
{
    int i;

    pcol = 0;
    int k;

    k = 0;
    while (len) {
        if (pcol == 0)
            printf("%04x: ", k);
        printf("%02x ", pchars[pcol] = addr[k++]);
        len--;
        if (pcol++ == 15) {
            dp();
            pcol = 0;
        }
    }
    if (pcol != 0) {
        for (i = pcol; i < 16; i++)
            printf("   ");
        dp();
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
