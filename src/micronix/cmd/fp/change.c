/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/change.c
 */

#include "fp.h"

char *reason[] = {
    "[part 1] must be copied first and is on another volume of this set.\n",
    "Space on this volume is exhausted, a fresh one is needed to continue.\n",
    "There are more files or parts of files yet to be copied on other volumes.\n",
    "Failure on read-back.  We'll try again on the next volume.\n"
};

char *changemessage = "Change volume, press return\7";

/*
 * change - ask for the next volume, then reopen and reread it.  In a
 * replace the current directory is marked CONTINUED and written out
 * before the prompt, so the next volume chains onto it.
 */
void
change(a)
    int a;
{
    char buf[BUFSIZE];

    haschanged = YES;

    if (rflag) {
        super.flags |= CONTINUED;
        superdirty = YES;
        writedir();
    }

    if (vflag)
        prs(reason[a]);

    for (;;) {
        closedev();

        free(tapedir);
        tapedir = NULL;

        prs(changemessage);
        gets(buf);

afterformat:
        if (!devopen()) {
            if (fd < 0 && errno == EIO) {
                doformat();
                goto afterformat;
            }

            continue;      /* couldn't open the device */
        }

        if (!readdir())    /* couldn't read directory */
            continue;

        break;
    }
}
