#include <stdlib.h>
#include "util.h"

int traceflags;

int
main(int argc, char **argv)
{
	char buf[512];

	blockedit(buf, 512);

	exit(0);
}

