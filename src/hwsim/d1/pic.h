/*
 * pic.h
 *
 * The 8259A interrupt controller, factored out of multio.c so the
 * MultIO/Wunderbus and the CompuPro SS1 can share the one chip model.
 * Every function takes the struct pic it works on; nothing here knows
 * how a board decodes its ports or folds a cascaded slave's INT into the
 * master's request pins - that is the owner's wiring, and it arrives as
 * the line_changed callback and the irbits name table.
 */

#ifndef PIC_H
#define PIC_H

/*
 * The 8259A interrupt controller, factored out of multio.c so the
 * MultIO/Wunderbus and the CompuPro SS1 can share the one chip model.
 * Every function takes the struct pic it works on; nothing here knows
 * how a board decodes its ports or folds a cascaded slave's INT into the
 * master's request pins - that is the owner's wiring, and it arrives as
 * the line_changed callback and the irbits name table.
 *
 * Like hwsim.h, this assumes sim.h (byte) and util.h (l/trace/bitdef)
 * were included by the translation unit first; it includes neither, so a
 * file can include sim.h once and this header after it without sim.h's
 * unguarded typedefs and enums being read twice.
 */

/*
 * ICW1 bits (the low five; the top three are the vector base, ICW1_VECL)
 */
#define ICW1_I4     0x01        // icw4 needed
#define ICW1_SNG    0x02        // single, no cascade
#define ICW1_ADI    0x04        // address interval = 4
#define ICW1_LTIM   0x08        // level triggered mode
#define ICW1_VECL   0xe0        // bits 7-5 of interrupt vector

/*
 * OCW2 bits
 */
#define OCW2_LEVEL  0x07        // level mask
#define OCW2_CMD    0xe0        // command mask (R|SL|EOI)
#define OCW2_ROTCLR 0x00        // clear rotate in auto-eoi mode
#define OCW2_NSEOI  0x20        // non-specific eoi
#define OCW2_SEOI   0x60        // specific eoi
#define OCW2_ROTSET 0x80        // set rotate in auto-eoi mode
#define OCW2_NSEOIR 0xa0        // non-specific eoi with rotate
#define OCW2_SETPRI 0xc0        // set priority (rotate, no eoi)
#define OCW2_SEOIR  0xe0        // specific eoi with rotate

/*
 * OCW3 bits
 */
#define OCW3_RIS    0x01        // register to read
#define OCW3_RR     0x02        // register to read
#define OCW3_POLL   0x04        // poll command
#define OCW3_SMM    0x20        // set special mask mode
#define OCW3_ESMM   0x40        // affect special mask mode

/*
 * The bit that distinguishes a write to the command register from one to
 * the data register, and inside the command register the ICW1 vs OCW
 * markers.  A0 (the register-select line) is what really tells them apart,
 * but the chip also looks at bit 4/bit 3 of a command write.
 */
#define PIC0_ICW1   0x10
#define PIC0_OCW3   0x08

/*
 * The ICW1/2/3/4 init sequence states.
 */
#define PS_UNDEF    0
#define PS_ICW2     1
#define PS_ICW3     2
#define PS_ICW4     3
#define PS_READY    4

/*
 * The whole of one 8259's state.  Two of these - a master and a cascaded
 * slave - are one 16-pin interrupt space, but each carries the full
 * register set, so the priority resolver is per chip.
 */
struct pic {
    byte icw1, icw2, icw3, icw4;
    byte ocw2, ocw3;
    byte imr, irr, isr;
    int priority;
    byte auto_eoi, rotate;
    byte poll;                  // OCW3 poll command is pending a read
    int pic_state;

    /*
     * Board context, filled in by the owner.  name and trace_flag are for
     * the trace()/l() messages; irbits names the eight interrupt sources
     * for bitdef(); line_changed is the board's fold - it re-evaluates the
     * INT line after a write that may have changed it (an EOI, an IMR).
     */
    char *name;
    int trace_flag;
    char **irbits;
    void (*line_changed)(void);
};

void pic_init(struct pic *p);
void pic_write_cmd(struct pic *p, byte v);      // A0=0 write: ICW1/OCW2/OCW3
void pic_write_data(struct pic *p, byte v);     // A0=1 write: ICW2/3/4/IMR
byte pic_read_cmd(struct pic *p);               // A0=0 read: poll/ISR/IRR
byte pic_read_data(struct pic *p);              // A0=1 read: IMR
void pic_request(struct pic *p, int ir, int value); // assert/deassert one IR pin
int pic_serviceable(struct pic *p);             // highest pending, -1 if none
int pic_highest_isr(struct pic *p);             // highest in-service, -1 if none
int pic_rank(struct pic *p, int level);         // 0 highest .. 7 lowest
int pic_vector(struct pic *p, int level);       // the 16 bit vector for a level

#endif
