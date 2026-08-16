/*
 * errors, messages, and recovery
 */
/*
 * Not cc1.h: this file needs the error codes and three globals, and
 * including the whole of pass1's world costs cpp the room it needs to
 * hold the source.  See p1expr.h for what the split is for.
 */
#include <stdlib.h>
#ifdef CCC
#include <unixio.h>
#else
#endif
#include "debug.h"
#include "token.h"		/* token_t */
#undef DEF_ERRMSG
#include "error.h"		/* error_t, ER_* */

#include "libutil.h"		/* fmtstr */
#include "p1lex.h"		/* struct token, cur, gettoken */

extern void fatal(error_t e);
extern char *filename;
#include <unistd.h>

#define DEF_ERRMSG
#include "error.h"

int error;
int exitCode = 0;  /* Global exit code: 0=success, 1=errors occurred */

static char errbuf[128];

void
gripe(error_t errcode)
{
    int i = errcode - 1;  /* error codes start at 1, array at 0 */
    char *p;
    if (i < 0) i = 0;
    if (i > ER_WTF - 1) i = ER_WTF - 1;
    p = fmtstr(errbuf, "%s:%d: %s\n", filename, lineno, errmsg[i]);
    write(2, errbuf, p - errbuf);
    error = errcode;
    exitCode = 1;
}

/*
 * some errors are too nasty to fix
 */
void
fatal(error_t errcode)
{
    gripe(errcode);
    write(2, "fatal\n", 6);
    exit(-errcode);
}

/*
 * checked allocation - parser data structures come from here.
 * on a 64K machine we will run out; die cleanly rather than let
 * a NULL return scribble over low memory.
 */
char *
galloc(unsigned int size)
{
    char *p = calloc(1, size);
    if (!p)
        fatal(ER_NOMEM);
    return p;
}

/*
 * throw an error message and discard tokens until we see the token we specify
 */
void
recover(error_t errcode, token_t skipto)
{
    gripe(errcode);
    while ((cur.type != skipto) && (cur.type != E_O_F)) {
        gettoken();
    }
}

/*
 * the next token must be 'check'.  if it isn't, gripe about it and skip
 * until we find 'skipto'
 */
void
need(token_t check, token_t skipto, error_t errcode)
{
    if (cur.type == check) {
        gettoken();
        return;
    }
    recover(errcode, skipto);
}

/*
 * expect: simplified token checking - gripe if wrong, advance regardless
 * Used to reduce code duplication where error recovery isn't needed
 */
void
expect(token_t check, error_t errcode)
{
    if (cur.type != check) {
        gripe(errcode);
    }
    gettoken();
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

