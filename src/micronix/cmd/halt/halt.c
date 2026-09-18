/*
 * halt - stop the machine
 *
 * cmd/halt/halt.c
 *
 * Calls the reboot system call with the RB_HALT flag, which drops the
 * cpu in kernel mode.  The simulators see that halt and end the run;
 * real hardware traps to the monitor.
 *
 * The kernel enforces that only the super-user may halt the system.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
#include <sys/reboot.h>

main()
{
	reboot(RB_HALT);
	exit(1);		/* reboot returns only on failure */
}
