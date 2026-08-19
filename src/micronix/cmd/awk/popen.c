/* popen/pclose stubs: micronix has no shell to pipe through. */
#include "stdio.h"
FILE *popen(cmd, mode) char *cmd, *mode; { return NULL; }
int pclose(fp) FILE *fp; { return 0; }
