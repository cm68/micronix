/*
 * update
 *
 * v7 update (usr/src/cmd/update.c), ported to micronix, with the
 * thirty-second timer done with sleep(2) in place of v7's signal(SIGALRM)
 * and alarm(2), and v7's fillst[] of open directories dropped - the
 * inode list is LRU, so the hot directories stay in core without
 * pinning.  What is left is the v6 loop, which is what the distributed
 * micronix binary does.
 *
 * cmd/update/update.c
 *
 * The program forks, drops the three standard descriptors, syncs, and
 * then syncs again every thirty seconds for the rest of the session.
 * The 1982 page agrees on the whole of it, half minute included.
 *
 * sync(2) is a library call, as it is for the sync command.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>

main()
{
	if (fork())
		exit(0);
	close(0);
	close(1);
	close(2);
	for (;;) {
		sync();
		sleep(30);
	}
}
