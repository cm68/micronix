/*
 * strip - remove the symbol table from a linked image
 *
 * cmd/strip/strip.c
 *
 * A Whitesmiths image is a 16 byte header, then text, then data, then the
 * symbol table, and the header's `table` field says how many bytes of
 * symbol table there are.  Stripping is dropping that table: zero the
 * field and cut the file down to header + text + data.  The field is the
 * only thing that changes - the conf byte keeps its symbol length, which
 * no reader consults once the table is gone.
 *
 * The first byte of the header is the object magic (0x99) and the high
 * bit of the second is CONF_NORELO, set only on a linked image.  A
 * relocatable object, an archive, and an ordinary text file all fail one
 * of the two, and are left alone - so the caller can hand strip a whole
 * tree and only the runnable images come back smaller.  A file shorter
 * than the header is not an image either.
 *
 * The kernel is not stripped.  Its overlays ride in its own file and the
 * inode the boot hands it, and the symbol table is part of what it opens;
 * a stripped kernel would not find its modules.  The diskette's userland
 * is stripped instead, which is where the space matters - a diskette is a
 * fixed number of blocks and every one of them is spoken for.
 *
 * Ported from the host tool in ../tools/strip.c, which strips images on
 * the build machine.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <unistd.h>
#include <obj.h>

int
main(argc, argv)
int argc;
char **argv;
{
    unsigned char hdr[HDR_SIZE];
    unsigned text, data;
    long size;
    FILE *f;
    int i;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <image> ...\n", argv[0]);
        exit(1);
    }
    for (i = 1; i < argc; i++) {
        f = fopen(argv[i], "rb");
        if (!f) {
            perror(argv[i]);
            exit(1);
        }
        if (fread(hdr, 1, sizeof hdr, f) != sizeof hdr) {
            fclose(f);          /* short file: not an image */
            continue;
        }
        fclose(f);
        if (hdr[HDR_MAGIC] != MAGIC || !(hdr[HDR_CONFIG] & CONF_NORELO))
            continue;           /* object, archive, or text */
        text = hdr[HDR_TEXT] | (hdr[HDR_TEXT + 1] << 8);
        data = hdr[HDR_DATA] | (hdr[HDR_DATA + 1] << 8);
        hdr[HDR_SYMTAB] = 0;    /* the symbol table is gone */
        hdr[HDR_SYMTAB + 1] = 0;
        f = fopen(argv[i], "r+b");
        if (!f) {
            perror(argv[i]);
            exit(1);
        }
        if (fwrite(hdr, 1, sizeof hdr, f) != sizeof hdr) {
            perror(argv[i]);
            exit(1);
        }
        size = (long) HDR_SIZE + text + data;
        if (ftruncate(fileno(f), size) != 0) {
            perror(argv[i]);
            exit(1);
        }
        fclose(f);
    }
    return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
