/*
 * Stub driver's interrupt handler.
 *
 * sys/stub_intr.c
 *
 * The handler the header's line/intr fields name (sys/ovl.h).  The
 * kernel's dispatch table (sys/intrpt.s) CALLs the common wrapper
 * (sys/mio.s), which saves the registers, calls this routine, and writes
 * the EOI on the way out - so a handler here reads the controller's
 * status, decides whether it has anything to say, and returns; it neither
 * saves registers nor touches the 8259 itself.  INTERRUPTS.md is the full
 * story from the board to this door.
 *
 * A handler runs with its module's page mapped (the kernel puts it in
 * place before calling), but it must not sleep and must not reach a
 * buffer header or the window: an interrupt is not a context that can be
 * put to bed, and the window's discipline (uio.c) is not in force here.
 * It sets the driver's done flag and wakeup()s the sleeper, and that is
 * all.
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/ovl.h>

/*
 * stubint(): the interrupt handler.  A real one:
 *   - reads the controller's status register;
 *   - checks whether it is the one that raised the shared line, and
 *     returns without touching anything if it is not;
 *   - checks the bit that says it has work (DRQ, done, error);
 *   - sets the driver's done word and wakeup()s whoever is waiting on it.
 *
 * The wrapper does not look at the return value, but the interface says
 * int, so return 0.
 */
stubint()
{
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
