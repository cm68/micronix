/*
 * setdev - show or set a micronix kernel's root and swap devices
 *
 * src/tools/setdev.c
 *
 *	setdev <kernel>				print them
 *	setdev <kernel> <root> <swap>		set them
 *
 * A device is written either as major/minor - 3/0 - or as a plain
 * number, decimal or 0x hex, so 3/0 and 0x300 and 768 all agree.
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
 * file by 16 + text + (addr - dataoff).
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
#define SYMLEN      12          /* 2 value + 1 type + 9 name */
#define NAMEOFF     3
#define NAMELEN     9
#define WS_IDENT    0x99

#define STDOUT      1
#define STDERR      2

static unsigned k_text, k_data, k_table, k_dataoff;
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
 * Where a symbol's data lives in the file, or -1 for no such symbol.
 * The name carries the underscore that C puts in front, so callers
 * spell it that way.
 */
static long
symoffset(want)
    char *want;
{
    long base, off, end;
    unsigned char ent[SYMLEN];
    char name[NAMELEN + 1];
    unsigned addr;
    int i;

    base = (long) HDRLEN + k_text + k_data;
    end = base + k_table;
    for (off = base; off + SYMLEN <= end; off += SYMLEN) {
        readat(off, ent, SYMLEN);
        for (i = 0; i < NAMELEN; i++)
            name[i] = ent[NAMEOFF + i];
        name[NAMELEN] = '\0';
        if (!sameName(name, want))
            continue;
        addr = get16(ent);
        if (addr < k_dataoff || addr + 2 > k_dataoff + k_data)
            die("that symbol is not in the data segment");
        return (long) HDRLEN + k_text + (addr - k_dataoff);
    }
    return -1;
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
    unsigned char hdr[HDRLEN];
    long rootoff, swapoff;
    unsigned rootdev, swapdev;
    int setting;

    if (argc != 2 && argc != 4) {
        say(STDERR, "usage: setdev <kernel> [<rootdev> <swapdev>]\n");
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

    if ((fd = open(argv[1], setting ? O_RDWR : O_RDONLY)) < 0)
        die("cannot open the kernel");

    readat(0L, hdr, HDRLEN);
    if (hdr[0] != WS_IDENT)
        die("not a whitesmiths object");
    k_table = get16(hdr + 2);
    k_text = get16(hdr + 4);
    k_data = get16(hdr + 6);
    k_dataoff = get16(hdr + 14);
    if (k_table == 0)
        die("no symbol table - do not strip the kernel");

    if ((rootoff = symoffset("_rootdev")) < 0)
        die("no _rootdev in the symbol table");
    if ((swapoff = symoffset("_swapdev")) < 0)
        die("no _swapdev in the symbol table");

    say(STDOUT, argv[1]);
    say(STDOUT, ":\n");
    show("rootdev", rootoff);
    show("swapdev", swapoff);

    if (setting) {
        writeat(rootoff, rootdev);
        writeat(swapoff, swapdev);
        say(STDOUT, "now:\n");
        show("rootdev", rootoff);
        show("swapdev", swapoff);
    }
    close(fd);
    return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
