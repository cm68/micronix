/*
 * halt - stop the machine
 *
 * cmd/halt/halt.c
 *
 * With no argument, halt asks init to shut the machine down: a SIGTERM
 * to process 1, which runs init's shutdown - kill every process, reap
 * them with the logout bookkeeping, unmount the filesystems, print the
 * epitaph, and halt.
 *
 * With an argument, halt dives straight into the kernel: the reboot
 * system call flushes the buffer pool and drops the cpu, with none of
 * init's grace.  It is the one to use when the system is too sick to
 * shut down the long way.
 *
 * Only the super-user may halt the machine, whichever way.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
#include <sys/reboot.h>
#include <sys/signal.h>

main(argc, argv)
    int argc;
    char **argv;
{
    if (argc > 1) {
        reboot(RB_HALT);        /* die now */
        exit(1);                /* reboot returns only on failure */
    }
    kill(1, SIGTERM);           /* ask init to shut down gracefully */
    exit(0);
}
