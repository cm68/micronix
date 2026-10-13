/*
 * down - take the system down
 *
 * cmd/down/down.c
 *
 * The orderly way to bring Micronix down, and only for the super-user.
 * Sync the disks, then ask init to shut down: a SIGTERM to process 1,
 * which init turns into the full shutdown - kill every process, reap
 * them, unmount the filesystems, halt.  The reboot call that halt(1)
 * makes is the ungraceful way and is deliberately not what down does.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>

main()
{
	if (getuid() != 0) {
		fprintf(stderr, "down: not super-user\n");
		exit(1);
	}
	sync();
	kill(1, SIGTERM);	/* init shuts the machine down */
	exit(0);
}
