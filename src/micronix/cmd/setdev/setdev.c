/*
 * setdev - show or set a micronix kernel's root and swap devices, and
 *          stamp the root device's driver into the kernel
 *
 * src/tools/setdev.c
 *
 *	setdev <kernel>				print them
 *	setdev <kernel> <root> <swap>		set them
 *	setdev -i <kernel>			stamp the root driver only
 *
 * A device is written either as major/minor - 3/0 - or as a plain
 * number, decimal or 0x hex, so 3/0 and 0x300 and 768 all agree.
 *
 * Setting them also stamps a driver page into the kernel's 4K slot
 * (sys/OVERLAY-DRIVERS.md, "The slot, and setdev").  The kernel is
 * linked with no disk drivers and a page-sized hole in its text; the
 * driver it needs to mount root is chosen at install time, and this is
 * where.  The driver is not named on the command line: it is the module
 * whose header says it serves the major `rootdev` names, and the two are
 * written in one pass so they cannot disagree.
 *
 * -i is that stamp on its own, for a build that wants the kernel's own
 * default root device and nothing else: it takes the major from the
 * _rootdev already in the kernel and writes no device numbers at all, so
 * the default is stated in one place (sys/main.c) and not restated here.
 * It is the last step of building a bootable kernel, and it is why a
 * kernel with its modules appended is still not bootable until it has
 * been run.
 *
 * The modules are whole 4K pages appended past the object's extent, one
 * per driver, each starting with the header `struct ovlhdr`
 * (include/sys/ovl.h) whose first word is the major it serves.  That is
 * the whole of the ordering agreement - the slot gets the page whose
 * major matches, whatever order the build wrote them in.
 *
 * This exists because the install script patched the kernel with ddt,
 * searching for the bytes it expected to find:
 *
 *	ddt /b/micronix / c 2 0 0 . s 0 3 0 3 .
 *
 * which is "find 0c 02 00 00 and make it 00 03 00 03" - root djdma/12
 * and no swap, becoming 3/0 for both.  1.6's kernel has djdma/12.  1.67
 * has djdma/76, so the pattern is not there, ddt says "Pattern not
 * found", and the install finishes having quietly not done it.  A disk
 * built that way boots a kernel that goes looking for the floppy.
 *
 * The kernel carries a symbol table, so none of that guessing is
 * needed: _rootdev and _swapdev say where they are.
 *
 * The object is Whitesmiths: a 16 byte header, then text, then data,
 * then a symbol table of 12 byte entries - a 2 byte value, a 1 byte
 * type, and a 9 byte name padded with NULs.  A data address maps to the
 * file by 16 + text + (addr - dataoff); a text address by
 * 16 + (addr - textoff).  Note that the object's extent is all three of
 * text, data and symbol table: a kernel's symbol table is thousands of
 * bytes, and the appended module region starts past it.
 *
 * read and write rather than stdio, and its own formatting, because this
 * has to run on the machine it patches and there stdio is most of the
 * program: with it, 7955 bytes of text for what is a seek, two reads and
 * a two byte write.  Nothing holds the file in memory either - a kernel
 * is larger than the address space there.
 */
#ifndef CCC
#include <stdlib.h>
#endif
#include <unistd.h>
#include <fcntl.h>

#define HDRLEN      16
#define MAXSYMLEN   15          /* longest name: (conf & 7) * 2 + 1, conf==7 */
#define NAMEOFF     3
#define WS_IDENT    0x99

#define STDOUT      1
#define STDERR      2

#define PAGESZ      4096        /* a driver page, and the slot it goes in */
#define MAJOROFF    0           /* struct ovlhdr's first word is the major */
#define SLOTSYM     "_ovlslot"  /* the slot the root driver is stamped into */
#define BOUNCE      512         /* a page is copied through this much at a time */

static unsigned k_text, k_data, k_table, k_textoff, k_dataoff, k_symlen;
static int fd;

/*
 * Output.  printf is not worth linking for six lines of it.
 */
static void
say(f, s)
    int f;
    char *s;
{
    char *p;

    for (p = s; *p; p++)
        ;
    write(f, s, p - s);
}

/*
 * A number in the given base, right justified in width with pad.  The
 * buffer fills from the right, which is why it is handed over by index.
 */
static void
saynum(f, v, base, width, pad)
    int f;
    long v;
    int base, width;
    char pad;
{
    char buf[16];
    int i = sizeof(buf);
    int d;

    if (v == 0)
        buf[--i] = '0';
    while (v > 0 && i > 0) {
        d = (int) (v % base);
        buf[--i] = d < 10 ? '0' + d : 'a' + d - 10;
        v /= base;
    }
    while (sizeof(buf) - i < width && i > 0)
        buf[--i] = pad;
    write(f, &buf[i], sizeof(buf) - i);
}

static void
die(s)
    char *s;
{
    say(STDERR, "setdev: ");
    say(STDERR, s);
    say(STDERR, "\n");
    exit(1);
}

static unsigned
get16(p)
    unsigned char *p;
{
    return p[0] | (p[1] << 8);
}

static void
readat(off, buf, n)
    long off;
    unsigned char *buf;
    int n;
{
    if (lseek(fd, off, 0) == -1L)
        die("seek failed");
    if (read(fd, buf, n) != n)
        die("short read");
}

static int
sameName(a, b)
    char *a, *b;
{
    while (*a && *a == *b)
        a++, b++;
    return *a == '\0' && *b == '\0';
}

/*
 * A symbol's address, or -1 for no such symbol.  The name carries the
 * underscore that C puts in front, so callers spell it that way.  Where
 * that address is in the file depends on which segment it is in, which
 * is the caller's business: a data symbol maps by
 * 16 + text + (addr - dataoff), a text one by 16 + (addr - textoff).
 */
static long
symaddr(want)
    char *want;
{
    long base, off, end;
    unsigned char ent[MAXSYMLEN + 3];
    char name[MAXSYMLEN + 1];
    int i;

    base = (long) HDRLEN + k_text + k_data;
    end = base + k_table;
    for (off = base; off + k_symlen + 3 <= end; off += k_symlen + 3) {
        readat(off, ent, k_symlen + 3);
        for (i = 0; i < k_symlen; i++)
            name[i] = ent[NAMEOFF + i];
        name[k_symlen] = '\0';
        if (sameName(name, want))
            return (long) get16(ent);
    }
    return -1;
}

/*
 * Where a data symbol lives in the file, or -1 for no such symbol.
 */
static long
symoffset(want)
    char *want;
{
    long a = symaddr(want);

    if (a < 0)
        return -1;
    if (a < k_dataoff || a + 2 > k_dataoff + k_data)
        die("that symbol is not in the data segment");
    return (long) HDRLEN + k_text + (a - k_dataoff);
}

static long
filesize()
{
    long n;

    if ((n = lseek(fd, 0L, 2)) == -1L)
        die("seek failed");
    return n;
}

/*
 * Where the appended module region starts: the first page boundary past
 * the object's extent.  All three of text, data and symbol table are in
 * that extent - leaving the symbol table out is the mistake to make
 * here, because a kernel's runs to thousands of bytes and the region
 * would then start inside it.
 */
static long
modbase()
{
    long ext = (long) HDRLEN + k_text + k_data + k_table;

    return (ext + PAGESZ - 1) & ~(long) (PAGESZ - 1);
}

/*
 * Module page n's major, from the header that is its first bytes, or -1
 * if there is no such page.  A page that is not one - padding, or a hole
 * the build left - reads as major 0, which is nodev and matches nothing.
 */
static long
modmajor(n, base, size)
    long n, base, size;
{
    unsigned char b[2];
    long off = base + n * PAGESZ;

    if (off + 2 > size)
        return -1;
    readat(off, b, 2);
    return (long) get16(b);
}

/*
 * Copy one module page onto the slot, a bounce buffer at a time: this
 * runs on the machine it patches, and a page is not something to put on
 * the stack there.
 */
static void
stamp(from, to, size)
    long from, to, size;
{
    unsigned char buf[BOUNCE];
    long n;

    if (to + PAGESZ > size)
        die("the slot runs past the end of the kernel");
    for (n = 0; n < PAGESZ; n += BOUNCE) {
        readat(from + n, buf, BOUNCE);
        if (lseek(fd, to + n, 0) == -1L)
            die("seek failed");
        if (write(fd, buf, BOUNCE) != BOUNCE)
            die("write failed");
    }
}

/*
 * major/minor, or a plain number - 0x for hex.  Written out rather than
 * calling strtol, which would want another library.
 */
static unsigned
number(s, endp)
    char *s;
    char **endp;
{
    unsigned v = 0;
    int base = 10, any = 0, d;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        base = 16, s += 2;
    for (;; s++) {
        if (*s >= '0' && *s <= '9')
            d = *s - '0';
        else if (base == 16 && *s >= 'a' && *s <= 'f')
            d = *s - 'a' + 10;
        else if (base == 16 && *s >= 'A' && *s <= 'F')
            d = *s - 'A' + 10;
        else
            break;
        v = v * base + d;
        any++;
    }
    if (!any)
        die("that is not a number");
    *endp = s;
    return v;
}

static unsigned
parsedev(s)
    char *s;
{
    char *end;
    unsigned maj, min;

    maj = number(s, &end);
    if (*end == '/') {
        min = number(end + 1, &end);
        if (*end != '\0')
            die("bad device - want major/minor");
        if (maj > 255 || min > 255)
            die("major and minor are one byte each");
        return (maj << 8) | min;
    }
    if (*end != '\0')
        die("bad device number");
    return maj;
}

static void
show(what, off)
    char *what;
    long off;
{
    unsigned char b[2];
    unsigned v;

    readat(off, b, 2);
    v = get16(b);
    say(STDOUT, "  ");
    say(STDOUT, what);
    say(STDOUT, " ");
    saynum(STDOUT, (long) (v >> 8), 10, 4, ' ');
    say(STDOUT, "/");
    saynum(STDOUT, (long) (v & 0xff), 10, 0, ' ');
    say(STDOUT, "  (0x");
    saynum(STDOUT, (long) v, 16, 4, '0');
    say(STDOUT, ")  at file offset 0x");
    saynum(STDOUT, off, 16, 5, '0');
    say(STDOUT, "\n");
}

static void
writeat(off, v)
    long off;
    unsigned v;
{
    unsigned char b[2];

    b[0] = v & 0xff;
    b[1] = (v >> 8) & 0xff;
    if (lseek(fd, off, 0) == -1L)
        die("seek failed");
    if (write(fd, b, 2) != 2)
        die("write failed");
}

int
main(argc, argv)
    int argc;
    char **argv;
{
    unsigned char hdr[HDRLEN], b[2];
    long rootoff, swapoff, slot, slotaddr, base, size;
    unsigned rootdev, swapdev, major;
    int setting, install, n, i, found;

    install = 0;
    if (argc > 1 && sameName(argv[1], "-i")) {
        install = 1;
        argc--;
        argv++;
    }
    if (argc != 2 && argc != 4) {
        say(STDERR, "usage: setdev [-i] <kernel> [<rootdev> <swapdev>]\n");
        say(STDERR, "       a device is 3/0 or 0x300 or 768\n");
        return 1;
    }
    setting = (argc == 4);

    /*
     * The devices are parsed before anything is opened for writing, so
     * that a typo costs nothing.
     */
    rootdev = swapdev = 0;
    if (setting) {
        rootdev = parsedev(argv[2]);
        swapdev = parsedev(argv[3]);
    }

    if ((fd = open(argv[1], (setting || install) ? O_RDWR : O_RDONLY)) < 0)
        die("cannot open the kernel");

    readat(0L, hdr, HDRLEN);
    if (hdr[0] != WS_IDENT)
        die("not a whitesmiths object");
    k_table = get16(hdr + 2);
    k_text = get16(hdr + 4);
    k_data = get16(hdr + 6);
    k_textoff = get16(hdr + 12);
    k_dataoff = get16(hdr + 14);
    k_symlen = (hdr[1] & 0x7) * 2 + 1;
    if (k_table == 0)
        die("no symbol table - do not strip the kernel");

    if ((rootoff = symoffset("_rootdev")) < 0)
        die("no _rootdev in the symbol table");
    if ((swapoff = symoffset("_swapdev")) < 0)
        die("no _swapdev in the symbol table");

    size = filesize();
    base = modbase();

    /*
     * The slot is a hole in the kernel's text, at a page boundary: its
     * whole segment has to be the driver's page and nothing else.  The
     * alignment that matters is the address, not the file offset - a
     * text address maps to the file by 16 + (addr - textoff), so an
     * aligned address gives an offset 16 bytes past a boundary.
     */
    slotaddr = symaddr(SLOTSYM);
    if (slotaddr < 0) {
        slot = -1;
    } else {
        if (slotaddr < k_textoff || slotaddr >= k_dataoff)
            die(SLOTSYM " is not in the text segment");
        if (slotaddr & (PAGESZ - 1))
            die(SLOTSYM " is not page aligned");
        slot = (long) HDRLEN + (slotaddr - k_textoff);
    }

    say(STDOUT, argv[1]);
    say(STDOUT, ":\n");
    show("rootdev", rootoff);
    show("swapdev", swapoff);

    n = (size > base) ? (int) ((size - base) / PAGESZ) : 0;
    say(STDOUT, "  modules  ");
    saynum(STDOUT, (long) n, 10, 0, ' ');
    say(STDOUT, " x 4K at file offset 0x");
    saynum(STDOUT, base, 16, 5, '0');
    say(STDOUT, "\n");

    if (slot < 0) {
        say(STDOUT, "  slot     none in this kernel\n");
    } else {
        readat(slot, b, 2);
        major = get16(b);
        say(STDOUT, "  slot     ");
        if (major == 0)
            say(STDOUT, "empty");
        else
            saynum(STDOUT, (long) major, 10, 0, ' ');
        say(STDOUT, "  at file offset 0x");
        saynum(STDOUT, slot, 16, 5, '0');
        say(STDOUT, "\n");
    }

    if (setting || install) {
        /*
         * The stamp and, when the devices are being set, the two devices
         * go in one pass.  The driver is not named: it is the module
         * whose header serves the major rootdev names, so the stamp and
         * rootdev are the same fact stated twice rather than two facts
         * that can disagree.  -i is this without the setting: the major
         * comes out of the kernel instead of off the command line, so
         * there is only one statement of it either way.
         */
        if (slot < 0)
            die("no " SLOTSYM " in the kernel to stamp");

        if (install) {
            readat(rootoff, b, 2);
            rootdev = get16(b);
        }

        found = -1;
        for (i = 0; i < n; i++)
            if (modmajor((long) i, base, size) == (long) (rootdev >> 8)) {
                found = i;
                break;
            }
        if (found < 0)
            die("no module for that major");

        stamp(base + (long) found * PAGESZ, slot, size);

        if (setting) {
            writeat(rootoff, rootdev);
            writeat(swapoff, swapdev);
            say(STDOUT, "now:\n");
            show("rootdev", rootoff);
            show("swapdev", swapoff);
        }
        say(STDOUT, "  stamped module ");
        saynum(STDOUT, (long) found, 10, 0, ' ');
        say(STDOUT, " for major ");
        saynum(STDOUT, (long) (rootdev >> 8), 10, 0, ' ');
        say(STDOUT, "\n");
    }
    close(fd);
    return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
