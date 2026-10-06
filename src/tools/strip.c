/*
 * strip the symbol table out of a Whitesmiths image
 *
 * tools/strip.c
 *
 * A Whitesmiths image is a 16 byte header, then text, then data, then the
 * symbol table, and the header's `table` field says how many bytes of
 * symbol table there are.  Stripping is dropping that table: zero the
 * field and cut the file down to header + text + data.  The field is the
 * only thing that changes - the conf byte keeps its symbol length, which
 * no reader consults once the table is gone.
 *
 * The kernel is not stripped.  Its overlays ride in its own file and the
 * inode the boot hands it, and the symbol table is part of what it opens;
 * a stripped kernel would not find its modules.  The diskette's userland
 * is stripped instead, which is where the space matters - a diskette is a
 * fixed number of blocks and every one of them is spoken for.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
    unsigned char hdr[16];
    unsigned text, data;
    long size;
    FILE *f;
    int i;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <image> ...\n", argv[0]);
        exit(1);
    }
    for (i = 1; i < argc; i++) {
        f = fopen(argv[i], "r+b");
        if (!f) {
            perror(argv[i]);
            exit(1);
        }
        if (fread(hdr, 1, sizeof hdr, f) != sizeof hdr) {
            fprintf(stderr, "%s: too short to be an image\n", argv[i]);
            exit(1);
        }
        text = hdr[4] | (hdr[5] << 8);
        data = hdr[6] | (hdr[7] << 8);
        hdr[2] = 0;             /* the symbol table is gone */
        hdr[3] = 0;
        rewind(f);
        if (fwrite(hdr, 1, sizeof hdr, f) != sizeof hdr) {
            perror(argv[i]);
            exit(1);
        }
        size = 16L + text + data;
        if (ftruncate(fileno(f), size) != 0) {
            perror(argv[i]);
            exit(1);
        }
        fclose(f);
    }
    return 0;
}
