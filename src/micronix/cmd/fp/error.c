/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/error.c
 */

#include "fp.h"

char *usemessage =
    "usage:\n\n\tfp [rxtcelviqsf] [device] [filename ... ]\n\n";

void
usage()
{
    fatal(usemessage);
}

void
fatal(a)
    char *a;
{
    prs(a);
    exit(NO);
}
