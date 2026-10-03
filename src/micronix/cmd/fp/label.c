/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/label.c
 *
 * The volume label, stored at LABEL on the volume and protected by the
 * xor byte that follows its text.
 */

#include "fp.h"

void
readlabel()
{
    seek(fd, LABEL, BYBYTE);

    if (read(fd, &label, sizeof label) != sizeof label
        || xor(&label, sizeof label) != 0) {
        *label.text = 0;
    }
}

void
writelabel()
{
    label.xor = xor(label.text, sizeof label.text);
    seek(fd, LABEL, BYBYTE);
    write(fd, &label, sizeof label);
}

void
getlabel()
{
    prs("Enter volume label: ");
    gets(label.text);
}
