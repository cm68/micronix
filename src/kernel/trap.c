/*
 * trap.c 
 */
#include "sys.h"
#include "proc.h"

/*
 * Firmware constants, defined in uhdr.s
 */
UCHAR status, cmask, ctask, mask, oldstack, rst1[];
UINT trapvec, trapstack;

UCHAR resched;                  /* sleep.c */

/*
 * Permission masks.
 * Bit 7        8080 io mode (A0-A7 = A8-A15)
 * Bit 6        io disable
 * Bit 5        special interrupt enable
 * Bit 4        halt disable
 * Bit 3        run enable
 * Bit 2        interrupt disable
 * Bit 1        aux enable
 * Bit 0        stop switch disable
 */
#define KERMASK 0230
#define USRMASK 0135

/*
 * Trap identification bits
 * All but MTRAP are active low
 */
#define MTRAP	0100            /* reference to yet-to-be allocated memory */
#define IOTRAP	0002
#define HALTRAP 0004
#define INTRAP	0010
#define ILLEGAL (IOTRAP | HALTRAP)

/*
 * System interface instructions
 */
#define HLT	0166
#define EXIT	"\166\317\001"  /* hlt; sys; exit */

/*
 * Control arrives here from the bootstrap (see uhdr.s).
 * We are in task 1, and the memory maps
 * for tasks 1 and 0 are identical.
 * The stack pointer has been set.
 */
start()
{
    extern int task0();

    /*
     * Set the trap vector so that the next
     * trap will go to task0() in task 0.
     */
    trapvec = &task0;
    /*
     * Now trap there.
     */
    hlt();
    /*
     * Task0() returns to here.
     * We are now back in task 1,
     * but in a new memoy bank.
     * Set up a system vector
     * and execute init.
     */
    rst1[0] = HLT;
    xinit();
}

/*
 * Control arrives here after the first trap,
 * from the hlt() above. We are now in task 0,
 * but we still are executing from the same
 * physical memory.
 */
task0()
{
    extern int trap();

    /*
     * Set traps appropriately for
     * task 0.
     */
    set0();

    /*
     * Copy the stack from the place where the
     * firmware put it to the user structure,
     * and arrange for the firmware to keep it there.
     */
    copy(&oldstack, &u.stack, (UINT) & u.save - (UINT) & u.stack);
    setframe(&u.stack, &u.stack);
    trapstack = &u.save;

    /*
     * Main() initializes the kernel
     * and forks the swap process.
     * On return, the kernel code will
     * have been copied into a new bank,
     * and task 1's map will have been
     * re-written to point to that bank.
     */
    main();
    /*
     * We are still in task 0, and in
     * the original memory bank.
     * Set the saved task-permissions
     * so that on return to task 1,
     * the appropriate set of user
     * traps will be enabled.
     */
    u.mask = USRMASK;
    /*
     * Set the trap vector so that all
     * Subsequent traps to task 0 will
     * go to trap() below.
     */
    trapvec = &trap;
    /*
     * Since this routine was invoked by
     * a trap, it returns thru the firmware
     * to the parallel code in task 1.
     */
    di();
}

/*
 * Control arrives here after the Decision cpu firmware
 * has done the preliminary trap processing.
 * All the registers from the trapped process have
 * been saved on the trap stack.
 * The firmware picks up this stack pointer
 * from ram, so that the system can set it.
 * There are two features that simplify this situation:
 * further traps are disabled until explicitly
 * re-enabled below, and traps will never be nested
 * more than two deep. The only level-2 trap occurs
 * when the kernel (which can only be entered via a trap
 * from a user task) itself encounters a debugging
 * trap. This invokes the monitor, which does not
 * allow further traps.
 *
 * The firmware pushes the registers onto the beginning
 * of the trap stack, which coincides with the r structure
 * of saved registers.
 */

char cause = 0;                 /* saved trap status */

trap()
{
    cause = status;             /* save status for debugging */
    set0();

    /*
     * Process any waiting interrupts
     * (This is the most likely cause of the trap)
     */
    enable();

    /*
     * Check the trap type and proceed accordingly
     */

    if ((cause & HALTRAP) == 0)
        r_system();
    else if ((cause & IOTRAP) == 0)
        send(u.p, SIGILL);
    else if ((cause & MTRAP) == MTRAP)
        fault();

    /*
     * If the rescheduling flag has been set,
     * switch processes
     */
    if (resched)
        next();

    /*
     * Process any waiting signal
     */
    if (u.p->slist[0])
        sig();
    /*
     * Return to the firmware,
     * which will restore the
     * registers and the permission mask
     * and continue the trapped task.
     */
    di();
}

/*
 * Set traps and firmware registers
 * appropriately for task 0
 */
set0()
{
    disable();
    cmask = KERMASK;
    mask = KERMASK;
    ctask = 0;
    wtask();
}

/*
 * Put an exit system call into the user's
 * address space, and return its address.
 * Used by sig() in sig.c to kill
 * the current process.
 */
setexit()
{
    copyout(EXIT, &rst1[0], 3);
    return (&rst1[1]);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
