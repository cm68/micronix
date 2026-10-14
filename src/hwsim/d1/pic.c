/*
 * pic.c
 *
 * The 8259A interrupt controller, factored out of multio.c.  This is the
 * chip's own state machine - the ICW1/2/3/4 init sequence, OCW2 EOI and
 * rotate commands, OCW3 poll and read-register, and the rotating priority
 * resolver.  A board owns the port decode and the cascading: how the
 * slave's INT reaches one of the master's request pins is the board's
 * line_changed callback, never this file's business.
 */

#include "sim.h"
#include "util.h"
#include "pic.h"
#include <string.h>

/*
 * Names for the ICW/OCW bits, for bitdef() in the trace lines.  The
 * interrupt-source names live on the board (struct pic.irbits) because
 * they are different on every card.
 */
static char *icw1_bits[] = { "icw4need", "single", "interval4", "level",
    "icw1", 0, 0, 0 };
static char *ocw2_bits[] = { 0, 0, 0, 0, 0, "eoi", "spec", "rotate" };
static char *ocw3_bits[] = { "ris", "rr", "poll", "ocw3", 0, "smm", "esmm", 0, 0 };
static char *icw4_bits[] = { "8086", "autoeoi", "master", "buffered",
    "special", 0, 0, 0 };
static char *nobits[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void
pic_init(struct pic *p)
{
    memset(p, 0, sizeof(*p));
    p->pic_state = PS_UNDEF;
}

/*
 * The 8259 priority is a rotating scheme: `priority` names the level that
 * is currently highest, and levels wrap around, so the rank of a level -
 * 0 being highest, 7 lowest - is its distance from `priority`.
 */
int
pic_rank(struct pic *p, int level)
{
    return (level - p->priority + 8) & 7;
}

/*
 * The highest-priority in-service level, or -1 when none is in service.
 * In fully-nested mode an in-service level blocks every request of equal
 * or lower priority, so this is the level a pending request must beat.
 */
int
pic_highest_isr(struct pic *p)
{
    int i;
    int best = -1;
    int bestrank = 8;

    for (i = 0; i < 8; i++) {
        if (p->isr & (1 << i)) {
            int r = pic_rank(p, i);

            if (r < bestrank) {
                bestrank = r;
                best = i;
            }
        }
    }
    return best;
}

/*
 * The 8259 priority resolver: the highest-priority pending request that
 * is unmasked and higher priority than the highest-priority in-service
 * level.  A lower-priority request is held until the in-service level is
 * ended.  Returns the level, or -1 when nothing is serviceable.
 */
int
pic_serviceable(struct pic *p)
{
    int i;
    int best = -1;
    int bestrank = 8;
    int isr_level = pic_highest_isr(p);
    int isrrank = (isr_level < 0) ? 8 : pic_rank(p, isr_level);

    for (i = 0; i < 8; i++) {
        int mask = 1 << i;

        if (!(mask & p->irr))           // not pending
            continue;
        if (mask & p->imr)              // masked
            continue;
        if (pic_rank(p, i) >= isrrank)  // not higher priority than in-service
            continue;
        if (pic_rank(p, i) < bestrank) {
            bestrank = pic_rank(p, i);
            best = i;
        }
    }
    return best;
}

/*
 * The interrupt vector for a level: the ICW2 base in the top byte, the
 * vector base from ICW1 and the level's slot in the bottom.
 */
int
pic_vector(struct pic *p, int level)
{
    return (p->icw1 & ICW1_VECL)
        + (level * ((p->icw1 & ICW1_ADI) ? 4 : 8))
        + (p->icw2 << 8);
}

/*
 * A write to the command register (A0 = 0): ICW1 starts the init
 * sequence, OCW3 handles poll/read-register, anything else is an OCW2
 * command.  The EOI commands clear the in-service bit and then ask the
 * board to re-fold the INT line, exactly as the original multio did.
 */
void
pic_write_cmd(struct pic *p, byte v)
{
    char **bdec;
    byte *rname;
    int intlevel;

    if (v & PIC0_ICW1) {                // ICW1
        bdec = icw1_bits;
        rname = "icw1";
        p->pic_state = PS_ICW2;
        p->icw1 = v;
        p->priority = 0;
        if (!(p->icw1 & ICW1_LTIM)) {
            l("%s: edge triggered not supported\n", p->name);
        }
    } else if (v & PIC0_OCW3) {         // OCW3
        bdec = ocw3_bits;
        rname = "ocw3";
        p->ocw3 = v;
        if (v & OCW3_POLL)              // poll: next read returns the word
            p->poll = 1;
    } else {                            // OCW2
        bdec = ocw2_bits;
        rname = "ocw2";
        p->ocw2 = v;
        switch (p->ocw2 & OCW2_CMD) {
        case OCW2_NSEOIR:
            /* non-specific EOI, then rotate: the serviced level becomes
             * the new lowest priority */
            intlevel = pic_highest_isr(p);
            trace(p->trace_flag, "%s: NSEOIR isr:%02x lvl:%d pri:%d\n",
                p->name, p->isr, intlevel, p->priority);
            if (intlevel >= 0) {
                p->isr ^= (1 << intlevel);
                p->priority = (intlevel + 1) % 8;
            }
            p->line_changed();
            break;
        case OCW2_NSEOI:
            /* non-specific EOI clears the highest-priority in-service
             * level, without rotating the priorities */
            intlevel = pic_highest_isr(p);
            if (intlevel >= 0)
                p->isr ^= (1 << intlevel);
            p->line_changed();
            break;
        case OCW2_SEOI:
            /* specific EOI clears one named level, no rotation */
            p->isr &= ~(1 << (p->ocw2 & OCW2_LEVEL));
            p->line_changed();
            break;
        case OCW2_SEOIR:
            /* specific EOI, then rotate so the named level is lowest */
            intlevel = p->ocw2 & OCW2_LEVEL;
            p->isr &= ~(1 << intlevel);
            p->priority = (intlevel + 1) % 8;
            p->line_changed();
            break;
        case OCW2_ROTSET:
            /* rotate in auto-eoi mode: each acknowledge clears the isr
             * and rotates, with no EOI command from the CPU */
            p->auto_eoi = 1;
            p->rotate = 1;
            break;
        case OCW2_ROTCLR:
            p->auto_eoi = 0;
            p->rotate = 0;
            break;
        case OCW2_SETPRI:
            /* set priority: the named level becomes the lowest, no EOI */
            p->priority = ((p->ocw2 & OCW2_LEVEL) + 1) % 8;
            break;
        default:
            l("%s: pic bogus ocw2 write command %x\n", p->name, p->ocw2);
        }
    }

    trace(p->trace_flag, "%s: write pic0 %02x %s: %s %x\n",
        p->name, v, rname, bitdef(v, bdec), p->isr);
}

/*
 * A write to the data register (A0 = 1): the ICW2/3/4 bytes while the
 * init sequence is running, or the IMR once it is complete.
 */
void
pic_write_data(struct pic *p, byte v)
{
    char **bdec;
    byte *reg;

    switch (p->pic_state) {
    case PS_ICW2:
        reg = "icw2";
        p->icw2 = v;
        bdec = nobits;
        if (!(p->icw1 & ICW1_SNG)) {
            p->pic_state = PS_ICW3;
        } else if (p->icw1 & ICW1_I4) {
            p->pic_state = PS_ICW4;
        }
        break;
    case PS_ICW3:
        reg = "icw3";
        p->icw3 = v;
        bdec = nobits;
        if (p->icw1 & ICW1_I4) {
            p->pic_state = PS_ICW4;
        } else {
            p->pic_state = PS_READY;
        }
        break;
    case PS_ICW4:
        reg = "icw4";
        p->icw4 = v;
        bdec = icw4_bits;
        p->pic_state = PS_READY;
        break;
    case PS_READY:
        reg = "imr";
        p->imr = v;
        v = (~p->imr) & 0xff;
        bdec = p->irbits;
        break;
    default:
        reg = "unknown";
        bdec = nobits;
    }
    trace(p->trace_flag, "%s: write pic1 %s %02x, %s\n",
        p->name, reg, v, bitdef(v, bdec));
}

/*
 * A read from the command register: the OCW3 poll word, or the
 * read-register IRR/ISR, or nothing.
 */
byte
pic_read_cmd(struct pic *p)
{
    trace(p->trace_flag, "%s: read pic0\n", p->name);

    /*
     * OCW3 poll: return the poll word and acknowledge the level it names.
     * Reading it drops the request, exactly as an intack does.
     */
    if (p->poll) {
        int level = pic_serviceable(p);

        p->poll = 0;
        if (level >= 0) {
            p->irr &= ~(1 << level);
            trace(p->trace_flag, "%s: poll -> level %d\n", p->name, level);
            return (0x80 | level);
        }
        return 0;
    }

    /* OCW3 read-register command: RIS selects ISR over IRR. */
    if (p->ocw3 & OCW3_RR)
        return (p->ocw3 & OCW3_RIS) ? p->isr : p->irr;
    return 0;
}

/*
 * A read from the data register: the IMR.
 */
byte
pic_read_data(struct pic *p)
{
    trace(p->trace_flag, "%s: read pic1\n", p->name);
    return p->imr;
}

/*
 * Assert or deassert one interrupt-request pin.
 */
void
pic_request(struct pic *p, int ir, int value)
{
    if (value)
        p->irr |= (1 << ir);
    else
        p->irr &= ~(1 << ir);
}
