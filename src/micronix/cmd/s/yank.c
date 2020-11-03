/*
 * yank.c - yank buffer
 *
 * Simple linked list.  Storage is allocated dynamically with malloc()
 * (not ckalloc()) so execution can continue if storage is exhausted.
 */

#include "s.h"

struct y_line
{
    char *y_text;
    struct y_line *next;
};

struct y_line *start = NULL;

/*
 *      do_put(way)
 *      int way;
 *              Copy the yank buffer to the main buffer.  If way = 1, the lines
 *              go after the current line; otherwise, they go before it.
 */
do_put(way)
    int way;
{
    struct y_line *p;
    int cur_line, cur_pos, line, size;

    if (start == NULL) {
        UNKNOWN;
        return;
    }
    b_getcur(&cur_line, &cur_pos);
    if (way == 1)
        ++cur_line;
    for (line = cur_line, p = start; p != NULL; p = p->next)
        b_insert(line++, p->y_text);

    /*
     * move to first nonwhite character 
     */
    b_setline(cur_line);
    if ((size = line - cur_line) >= 5)
        s_savemsg("%d lines added", size);
}

/*
 *      int do_yank(line1, line2)
 *      int line1, line2;
 *              Copy the block of lines from line1 to line2, inclusive, to the
 *              yank buffer; tell if successful.  Line1 cannot exceed line2.
 */
int
do_yank(line1, line2)
    int line1, line2;
{
    struct y_line *p, *q;
    char *r, text[MAXTEXT - 1];

    p = NULL;

    free_ybuf();

    for (; line1 <= line2; ++line1) {
        b_gets(line1, text);
        q = (struct y_line *) malloc(sizeof(struct y_line));
        r = malloc((unsigned) strlen(text) + 1);
        if (q == NULL || r == NULL) {
            free_ybuf();
            return (0);
        }
        q->y_text = strcpy(r, text);
        q->next = NULL;
        /*
         * link the line in at the end of the list 
         */
        if (start == NULL)
            start = q;
        else
            p->next = q;
        p = q;                  /* p points to the end of the list */
    }
    return (1);
}

/*
 * free_ybuf - free the storage for the yank buffer
 */
free_ybuf()
{
    struct y_line *p, *q;

    for (p = start; p != NULL; p = q) {
        free(p->y_text);
        q = p->next;
        free((char *) p);
    }
    start = NULL;
}
