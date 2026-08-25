# How interrupts are vectored

How an interrupt gets from a board on the S-100 bus into a kernel handler,
across the MPZ80's supervisor/user boundary.

File references are relative to `src/`: `micronix/sys/` is the kernel,
`hwsim/` is the hardware simulator.

The short version: the Z80 runs a real 8080-mode acknowledge cycle and the
8259 feeds it a synthesized `CALL`. The kernel never computes a vector and
never reads the interrupt controller to find out what happened. What makes
this look complicated is that in user mode the interrupt cannot be delivered
directly, so the MPZ80 converts it into a trap first and the acknowledge
happens afterwards, once supervisor code has re-enabled interrupts.

```
device                                  set_vi()
  |
  v  VI0-VI7, open collector             hwsim/s100.c
8259 on the MultIO                       hwsim/d1/multio.c
  |
  v  S-100 PINT                          int_change hook
MPZ80 mask register                      hwsim/d1/mpz80.c
  |
  v  Z80 INT pin                         int_pin
Z80, IM 0                                z80_tick, M1+IORQ
  |
  v  CD lo hi  ->  CALL vectors+n        int_ack -> multio_intack
kernel dispatch                          micronix/sys/intrpt.s
```

## 1. Devices pull a vectored interrupt line

Drivers call `set_vi(line, card, value)` in `hwsim/s100.c`. VI0-VI7 are open
collector on the real bus, so two cards can share a line and neither may
release it on the other's behalf. `s100.c` models this with `vi_masks[8]`,
one bit per card per line:

```c
vi_masks[line] &= 0xff ^ (1 << card);
vi_masks[line] |= value << card;
value = (vi_masks[line] == 0) ? 0 : 1;
```

That `card` argument is why `hwsim/d1/hdca.c` can say
`#define HDCA_INTERRUPT 0 // same as hddma!` — both hard disk controllers
sit on VI0 with different card ids, and clearing one does not clear the
other. `set_vi` calls the `vi_change` hook only on an actual transition.

| Line | Owner | Registered at |
|------|-------|---------------|
| VI0 | hard disk (hdca / hddma) | `multio.c` `reg_intbit(0, "hd")` |
| VI1 | djdma floppy | `reg_intbit(1, "djdma")` |
| VI2 | slave MultIO | `reg_intbit(2, "slave")` |
| VI3-5 | the three 8250 ACEs | `ace_init(i, name, 3 + i)` |
| VI6 | master parallel port | disabled at the PIC, see `mio.s` |
| VI7 | RTC clock tick | `reg_intbit(7, "clock")` |

## 2. The 8259 on the MultIO card

The MultIO owns the interrupt controller, so at init it claims both bus
hooks (`multio.c`, `multio_init`):

```c
vi_change   = &multio_vi_change;
get_intack  = &multio_intack;
```

`multio_vi_change` diffs the old and new line state, sets or clears the
corresponding bits in the **IRR**, and calls `multio_set_int_line`:

```c
if ((irr & ~imr) & ~isr)  line = 1; else line = 0;
(*int_change)(line);
```

The S-100 `PINT` line is asserted when a request is pending, unmasked in the
IMR, and not already in service in the ISR.

Note that `micronix/sys/mio.s` programs the master with `ICWORD1 := 037` —
level triggered, 4-byte vector interval, single controller. Edge triggering
is decoded but not implemented in `multio.c`; it logs
`"edge triggered not supported"`. Everything assumes level triggered, which
is consistent with the open-collector VI modelling above.

## 3. The MPZ80 decides whether the Z80 ever sees it

`int_change` is claimed by the CPU card (`mpz80_startup`:
`int_change = mpz80_intr`), which stores `int_line` and runs
`interrupt_check()`. This is the supervisor/user gate:

```c
if (!int_line) {
    int_pin = 0;
    return;
}
if (!super()) {
    if (maskreg & MASK_TINT) {      // no user interrupts, trap!
        // this goes to supervisor, which will assert int_pin later
        int_pending_trap = 1;
        return;
    }
} else {
    if (maskreg & MASK_SINT) {   // block interrupts
        return;
    }
}
int_pin = 1;
```

Three outcomes:

- **Supervisor, `MASK_SINT` set** — blocked. `int_pin` is not set, and note
  it is not cleared either; the request stays latched in the 8259's IRR.
- **User, `MASK_TINT` set** — user code may not take interrupts directly, so
  the card forces a trap into supervisor instead and sets `int_pending_trap`.
- **Otherwise** — `int_pin = 1` and the Z80 will take it.

The pending trap is deliberately deferred to an instruction boundary:
`hwsim/hwsim.c` calls `take_pending_trap()` after `z80_run()` returns. The
comment above `int_pending_trap` in `mpz80.c` explains why this is safe for
an interrupt and would be wrong for a page fault — an interrupt is
recognised between instructions, so deferring it loses nothing, whereas a
fault must trap from inside the cycle and carry its own address.

`interrupt_check()` is re-run wherever deliverability could have changed:
when the trap window drains (`trapcount == 0`), when the task register
switch countdown completes, and on mask register writes.

## 4. The Z80 runs the acknowledge cycle

The kernel puts the CPU in interrupt mode 0 (`micronix/sys/sub8.s`):

```
_enable:
	xor	a
	ld	(dicount),a
	im	0
	ei
	ret
```

In IM 0 the CPU asserts M1+IORQ and executes whatever instruction the bus
hands back. `z80_tick` in `mpz80.c` implements exactly that:

```c
} else if (pins & Z80_IORQ) {
    if (pins & Z80_M1) {
        Z80_SET_DATA(pins, int_ack() & 0xff);
```

`int_ack()` in `s100.c` trampolines through `get_intack` to
`multio_intack()`, which is the 8259's acknowledge state machine. On the
first cycle of a sequence (`ivecstate == IV_EMPTY`) it picks a winner by
rotating priority scan and builds a three byte instruction:

```c
for (i = 0; i < 8; i++) {
    level = (priority + i) % 8;
    mask = 1 << level;
    if ((mask & irr) && !(mask & imr)) break;
}
vecaddr = (icw1 & ICW1_VECL) + (level * ((icw1 & ICW1_ADI) ? 4 : 8)) + (icw2 << 8);
vector[0] = 0xcd;                    // CALL
vector[1] = vecaddr & 0xff;
vector[2] = (vecaddr >> 8) & 0xff;
```

The three bytes are handed out one per acknowledge cycle, stepping
`IV_FILLED -> IV_1SENT -> IV_2SENT -> IV_EMPTY`. Two things happen on the
last byte:

```c
if (ivecstate == IV_EMPTY) {
    isr |= iv_isrbit;         // now in service
    multio_set_int_line();    // drops PINT if nothing else is pending
}
```

The vector is therefore computed **at acknowledge time** from whatever is
pending then, not from what was pending when the line first went low. That
is what makes the user-mode detour in the next section cost nothing.

Worth being precise about who does what: the Z80 is bus master and *runs*
the cycle; the 8259 is the responder that supplies the bytes. In hwsim the
asymmetry is literal — `z80_tick` drives it, `multio_intack`'s `ivecstate`
just counts which byte it has been asked for.

## 5. Dispatch and EOI

The `CALL` lands in the kernel's dispatch table, `micronix/sys/intrpt.s`:

```
; Interrupt dispatch table
; Extra space allows us to move the vectors to a 32-byte
; boundry, as required by the interrupt controller.
vectors:
	jp	int0
	.defb	0
	jp	int1
	...
```

Four bytes per entry, eight levels, 32 bytes total — matching the
`ICW1_ADI` interval of 4 that `ICWORD1` selects. ICW2 supplies the high byte
of `vectors`.

| Level | Handler | Purpose |
|-------|---------|---------|
| 0 | `_mwint` | hard disk |
| 1 | `_djint` | floppy disk |
| 2 | `slint` | slave MultIO |
| 3 | `m1int` | master ACE 1 |
| 4 | `m2int` | master ACE 2 |
| 5 | `m3int` | master ACE 3 |
| 6 | `m0int` | master parallel port |
| 7 | `clkint` | clock |

Each `intN` is a `call intrupt` followed by the inline address of the real
service routine. `intrupt` in `micronix/sys/mio.s` is the common wrapper: it
saves registers, pulls the handler address out of the return address, calls
it, and then issues the EOI:

```
	ld	a,PENABLE
	out	(MSELECT),a
	ld	a,ENDINT
	out	(ICNTRL),a
	pop	af
	pop	bc
	pop	de
	pop	hl
	ei
	ret
```

`PENABLE` (0x28) selects the PIC bank on the MultIO's select register;
`ENDINT` is **0xA0**, non-specific EOI with rotate. That is the
`OCW2_NSEOIR` case in `multio.c`:

```c
case OCW2_NSEOIR:
    intlevel = bitnum(isr);
    priority = (intlevel + 1) % 8;
    isr ^= (1 << intlevel);
    break;
```

The level just serviced drops to lowest priority, which is what makes the
rotating scan in `multio_intack` fair across devices.

The master is not in auto-EOI mode — `ICWORD4 := 014` is master + buffered
with the auto-EOI bit clear, so the explicit write above is required. Only
the slave uses auto-EOI (`SLICW4 := 016`).

## 6. The user-mode path, end to end

This is the part that is easy to misread as the kernel doing the vectoring.
It is not; the trap only exists to get the machine somewhere the interrupt
can be delivered.

```
device asserts VI, 8259 sets IRR, PINT goes true
  MPZ80: user mode and MASK_TINT set
      -> int_pin stays low, int_pending_trap = 1
  take_pending_trap() at the next instruction boundary
      -> trap window at 0xbf0, taskreg = 0, now in supervisor
  trapcount reaches 0 -> interrupt_check() runs again
      -> super() is now true, int_line is still asserted
         (nobody has acknowledged the 8259)
      -> int_pin = 1
  kernel trap() -> enable() -> ei
  Z80 takes INT: M1+IORQ x3 -> CD lo hi -> CALL vectors+n
  handler runs -> intrupt writes ENDINT -> isr cleared, priority rotates
```

The kernel's own trap handler makes the intent explicit
(`micronix/sys/trap.c`):

```c
trap()
{
    cause = status;             /* save status for debugging */
    set0();

    /*
     * Process any waiting interrupts
     * (This is the most likely cause of the trap)
     */
    enable();
```

`enable()` is the `im 0 / ei` above, and it is the handler's first real act.

More telling still is what `trap()` does *not* do. For an interrupt the card
calls `trap(ST_RESET & ~ST_INT)`, giving `cause = 0xB7`, and every dispatch
arm falls through:

| Test | Evaluates | Taken? |
|------|-----------|--------|
| `(cause & HALTRAP) == 0` | `0xB7 & 0x04` = 4 | no, not a system call |
| `(cause & IOTRAP) == 0` | `0xB7 & 0x02` = 2 | no, not illegal I/O |
| `(cause & MTRAP) == MTRAP` | `0xB7 & 0x40` = 0 | no, not a page fault |

Nothing matches. `cause` is saved for debugging and otherwise unused. The
kernel performs no interrupt dispatch in `trap()` at all — the interrupt is
delivered by hardware straight to `vectors + level*4`, behind the trap
handler's back, the moment the `ei` retires.

The request survives the whole detour latched in the 8259's IRR. Nothing has
to be remembered on the MPZ80 side except the one `int_pending_trap` flag.

## Notes and gotchas

- **Dead code lives in `hwsim/unused/` and is not in any build.** It used to
  sit in the tree next to the live drivers, where a grep for interrupt
  handling would hit it first. `unused/8259.c` is an older copy of the
  multio driver — its header comment still says so — and the live 8259 is
  inside `d1/multio.c`. `unused/cpro/` is the Cromemco machine, which was
  never wired up: `hwsim/Makefile` builds `DIRS = d1` only. This document
  describes `d1`, which is the only machine there is.
- **The kernel cannot poll the PIC.** `rd_pic_port_0` and `rd_pic_port_1`
  return 0 unconditionally, and OCW3 poll mode is `#define`d but not
  implemented. Cause is only ever learned from which vector was called.
- **`ARMMASTER` (0103) is the boot-time mask, not the final one.** It arms
  levels 2-7; `mio.s` has runtime paths that set and clear individual IMR
  bits as drivers come and go. Remember the IMR's sense is inverted —
  `multio.c` notes "enables are low".
- **`_status` and `_mask` are the same address** (0x403), read and write
  respectively: reading gives `trapstat`, writing sets the MPZ80 mask
  register. See `rregp[]` and `wregp[]` in `mpz80.c`.
- **`MASK_TINT` and `MASK_SINT` are not symmetric.** `MASK_TINT` converts a
  user-mode interrupt into a trap; `MASK_SINT` simply blocks in supervisor
  and leaves the request latched.
