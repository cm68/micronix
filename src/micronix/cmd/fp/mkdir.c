/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/mkdir.c
 */

#include "fp.h"

#define DIRMODE (S_IFDIR | 0755)

char *
parent(a)
    char *a;
{
    static char buf[PATHSIZE];
    char *p;

    if (!strchr(a, '/'))
        return ".";

    strcpy(buf, a);

    p = buf + strlen(buf) - 1;     /* last character */

    while (buf < p && *p == '/')   /* remove trailing slashes */
        *p-- = 0;

    while (buf < p && *p != '/')   /* remove trailing component */
        *p-- = 0;

    while (buf < p && *p == '/')   /* remove trailing slashes */
        *p-- = 0;

    return buf;
}

/*
 * mkdir - make a new directory, making sure all the path components
 * exist first.
 */
void
mkdir(new, mode, owner)
    char *new;
    int mode, owner;
{
    char buf[PATHSIZE], *p;

    if (!*new || !strcmp(new, "/") || !strcmp(new, "."))
        return;

    if (mode == 0)
        mode = DIRMODE;

    /*
     * make your parent first, then make yourself
     */
    strcpy(buf, parent(new));
    mkdir(buf, mode, owner);

    if (fexists(new))              /* all done */
        return;

    if (mknod(new, mode, 0) < 0) { /* create directory */
        perror(new);
        return;
    }

    chown(new, owner);

    strcpy(buf, new);
    strcat(buf, "/.");
    if (link(new, buf) < 0) {
        unlink(new);               /* back out */
        perror(new);
        return;
    }

    strcpy(buf, new);
    strcat(buf, "/..");
    if (link(parent(new), buf) < 0) {
        unlink(buf);               /* back out */
        unlink(new);

        perror(new);
        return;
    }
}
