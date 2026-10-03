/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/link.c
 *
 * The link table: files with more than one name are extracted as links,
 * so the first name is remembered and every later one is hard-linked to
 * it rather than copied again.
 */

#include "fp.h"

struct list *list = NULL;

/*
 * lookup - run a file through the link table, remembering the primary
 * (first) name.  Return that name if the file has been seen before,
 * else NULL after recording it.
 */
char *
lookup(a, b)
    char *a;
    struct stat *b;
{
    static struct list *l;

    for (l = list; l; l = l->next) {
        if (l->major != (b->st_dev >> 8))
            continue;
        if (l->minor != (b->st_dev & 0xff))
            continue;
        if (l->inumber != b->st_ino)
            continue;

        return l->name;
    }

    l = calloc(1, sizeof(*l));

    if (!l) {
        perror("link table");
        exit(NO);
    }

    l->major = b->st_dev >> 8;
    l->minor = b->st_dev & 0xff;
    l->inumber = b->st_ino;
    l->name = strdup(a);
    l->next = list;
    list = l;

    return NULL;
}
