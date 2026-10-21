pluggable boards: a sketch, not a spec
======================================

This is a design sketch for turning hwsim's board support into something a
machine can be assembled from, rather than the one Morrow Decision 1 that
the source tree happens to describe.  It is written for discussion: where a
fact is known it is cited, where an implementer would have to choose it is
said so, and where the current code contradicts a natural assumption that
is said too.  Nothing here has been built.

The direction of travel is stated up front because it decides everything
else.  The point is not to clean up the Morrow wiring; it is to make the
machine under the simulator *any* S-100 machine, so that the real driver
code can be asked the questions it must survive - two of the same card at
different addresses, a card at a non-default strap, an unknown board at an
unknown port.  A machine is then a list of board instances plus a wiring
table, and the existing d1 machine is one such list that has to keep
booting the distribution floppies at every step of the way.


what is already pluggable
-------------------------

The beginnings are real and worth naming precisely, because the sketch
builds on them rather than replacing them.

There is a driver table.  `struct driver` (src/hwsim/hwsim.h:31) holds a
name and four hooks - usage, prearg, startup, poll - and
`register_driver()` (src/hwsim/hwsim.c:687) appends to a fixed array of
sixteen (hwsim.c:676).  Every board registers itself from a constructor
before main: `register_djdma_driver()` at djdma.c:1773 is the pattern, and
adding a board to the link is all it takes to have it (d1/GNUmakefile:23
lists them).  The hooks run at three points: prearg before argument
processing so defaults can be set (hwsim.c:1793), startup before the
emulation loop (hwsim.c:2292), and poll once per loop iteration
(hwsim.c:2313).

Port dispatch is already a lookup, not a chain of tests.  Two 256-entry
tables of function pointers, `input_handler[]` and `output_handler[]`
(s100.c:93), are filled by `register_input()`/`register_output()`
(s100.c:192) and dispatched by `s100_input()`/`s100_output()` (s100.c:178).
The tables are initialised to the "undefined port" handlers by a
constructor (s100.c:214).  What the prompt's phrase "a lookup rather than a
chain of tests" is really pointing at is not this table, which already
exists, but the address it is keyed on: every registration and every
dispatch masks the port to eight bits (s100.c:181, 187, 195, 206), so the
table cannot tell two boards apart that decode the same low eight bits of a
sixteen-bit Z80 I/O address.  S-100 has eight I/O address lines, so
masking is not wrong, but the decision to expose eight bits rather than
sixteen is made for every board at once and cannot be revisited per board.

The real chains of tests live elsewhere.  `drivearg()` dispatches a drive
argument over a strcmp chain of controller names (hwsim.c:1567-1617),
`bootdev()` looks a boot name up the same way (hwsim.c:1410), and
`wiring_vi_line()` scans the card table for a name (s100.c:353).  These are
the places where a new board has to be edited into the framework, and they
are what a board registry replaces.

The interrupt wiring is already a table with profiles.  s100.c keeps a
`struct wiring` - card name to VI line, IR pin to source, and the name of
the board driving INT - with three profiles, `d1`, `multio` and `ss1`
(s100.c:277-334).  A constructor resolves the default (s100.c:342), and
`-w profile[,key=value]...` re-selects and overrides it (s100.c:467,
hwsim.c:1886).  Boards look their line up by name at init
(`wiring_vi_line("hdca")` at hdca.c:498) instead of carrying a number.
This is the right shape already; it is not yet tied to instances.

The interrupt path is three function pointers.  The CPU installs
`int_change` (mpz80.c:1518); the interrupt controller installs `vi_change`
and `get_intack` (multio.c:1781, ss1.c:639).  `set_vi()` (s100.c:67) models
the open-collector VI lines with a per-line eight-bit mask and a card id,
and the CPU's `int_ack()` (s100.c:54) calls whatever the controller
installed.  This is a clean seam; its weakness is the card id, covered
below.

There is also a shared library already.  libsim.a (lib/GNUmakefile:12) holds
the disassembler, the monitor, the IMD image code, the filesystem library,
and the drive abstraction `hdcdmadisk.c` that `drive_open()` and friends
come from.  The d1 drivers split two ways here: ide.c talks to the drive
module (`drive_open`, ide.c:229), the others call `imd_load()` directly
(hdca.c:213, hddma.c:182, djdma.c:1715, disk1.c:136, disk2.c:94,
disk3.c:104, ncr5380.c:307).  Two backing-store paths
with the same shape is exactly the kind of inconsistency a board interface
should absorb.

And one board already has the target shape.  nic.c is the network card core
and it makes no simulator calls at all - "only POSIX sockets" (nic.c:12) -
while nicd1.c is the twenty-line wiring that hangs its two ports on the bus
and runs the poll (nicd1.c:55).  Every driver in the sketch is being asked
to end up split the way nic.c and nicd1.c already are.


what is not pluggable
---------------------

The CPU card is not a board, and it is the hard part.  mpz80.c is the
processor, the memory accessor, the MMU, and the trap model in one file.
It owns the z80 instance and the cycle counter (mpz80.c:42-44), the tick
callback that decodes MREQ/IORQ/M1/HALT (mpz80.c:51), `get_byte`/`put_byte`
which contain the page-map walk (mpz80.c:1090, 1341, with `getpte` at
mpz80.c:911), the task register and the sixteen maps (mpz80.c:132, 162,
178), the trap sequence (mpz80.c:699), and the switch register's meaning of
the boot bits (mpz80.c:219-227).  It is a singleton by nature - there is
one processor - but its knowledge is not naturally a singleton's: the MMU
and the switch register are cards on the real machine, and the memory
accessor is the bus's job, not the CPU's.

The memory system is one flat array.  `physmem[16*1024*1024]` (s100.c:103)
with `ram_size` gating reads above the top to the open-bus value 0xff
(s100.c:112), and the boot ROM shadowed at 0xff0000 (hwsim.h:25, 192).
`physread()`/`physwrite()` are inline in hwsim.h (192-213) and are the
choke point every DMA path already goes through - which is why the physical
watchpoint works for controller transfers too (mpz80.c:655).  What is
missing is any notion of who owns a region of the 24-bit space: a memory
board, a ROM board, or a bus master are all the same flat array today, and
a board that wants to answer a memory cycle instead of an I/O cycle has no
hook at all.

Every board is a pile of file-static globals.  hdca.c keeps its buffer,
ram pointer, status and drive handles as statics (hdca.c:77-154); hddma.c
its channel and command (hddma.c:45-48, 150-154); djdma.c its dparams,
channel, and eight image handles (djdma.c:183-214).  Two instances would
share every one of them, which is the concrete reason "two of the same
card" cannot be posed today.

Port numbers are #defines, and the TODO is right about them.  hdca is
0x50-0x53 (hdca.c:99), hddma 0x54-0x55 (hddma.c:40), the MultIO 0x48 and
0x40 (multio.c:81), djdma 0xef (djdma.c:43), ide 0x30-0x34 (ide.c:92),
scsi 0x20 (ncr5380.c:114), disk1 0xc0 (disk1.c:44), disk2 0xc8 (disk2.c:33),
disk3 0x90 (disk3.c:28), selector 0xe0 (selector.c:17), nic 0x60
(nicd1.c:15).  The real cards were strapped, and nothing in the simulator
can move one.  The d1/TODO entry asks for an option per emulation or one
`<card>:<addr>` option, and notes the shape already exists: `-j <ab|bc>`
is the djdma's J2 strap (hwsim.c:1921), documented in usage() at
hwsim.c:715.  That is the shape to generalise, and the TODO's idea is
absorbed below rather than left open.

The boot path is a name-to-switch table plus a comment.  `bootdevs[]`
(hwsim.c:1401) maps hdca and ide to switch 0x00, hdcdma to 0x08, djdma to
0x10, and `-B` turns a name into `config_sw` with bit 0x04 set to skip the
monitor (hwsim.c:1906, bootdev() at 1409).  The switch bits themselves are
the MPZ80's (SW_DJDMA/SW_HDDMA/SW_HDCA, mpz80.c:219-227), and the rom
decides what 0x00 means - mon447 boots the hdca, mon500 the ide, and
nothing tells the simulator that except the comment (hwsim.c:1385-1396) and
mpz80_usage() (mpz80.c:1524).  A machine with a different boot card would
have to edit three places that are really one fact.

Two small corrections to the framing, before the sketch proper.  First,
there is no s100.h; the bus lives in s100.c plus hwsim.h.  Second, the
option that re-selects the wiring profile is lowercase `-w` (hwsim.c:1886);
the comments in s100.c:240 and hwsim.h:73 still call it `-W`, which is the
write-watchpoint option (hwsim.c:1985).  The comments are what have
drifted.


the bus interface
-----------------

The sketch is built around one object - the bus - that every board holds a
pointer to, and one set of entry points that is the whole of what a board
may do.  The list below is the proposal; the types are illustrative.

Port I/O.  A board declares the ports it decodes and hands the bus a
handler plus a base, rather than registering absolute numbers one at a
time.  The bus owns the dispatch table, decides the address width, and can
report a collision between two boards that decode the same port - which
today is silent: the last `register_output()` wins (s100.c:203).  Whether
the width is eight or sixteen bits is the first decision an implementer
has to make; S-100 is eight, the Z80 drives sixteen, and the MultIO's
group-select register is the proof that a board can decode a window and
re-point it at runtime (multio.c:1516), which a static table has to either
represent or forbid.

Memory.  `physread`/`physwrite` stay the choke point but become methods on
the bus, so a region can be owned by a board instead of answered by the
flat array.  The boot ROM shadow is the one existing example of a board
answering memory (hwsim.h:196, 208), and it is currently special-cased
inline rather than registered; making it the first registered memory owner
is the test of the idea.  Whether to keep physread/physwrite inline (the
hot path pays a call otherwise, which is why they are inline today,
hwsim.h:189) or to accept the call in exchange for board-owned regions is
the second decision.

DMA.  The bus masters - hddma, djdma, disk1, disk3, and disk2 through the
selector - all move data between an image and a 24-bit address they hold in
a register or a command block.  Today each of them decodes that address
itself, and three of them repeat the same special case: a zero high byte is
a flat sixteen-bit boot-loader address, a nonzero one is a 4K segment over
a 12-bit offset, exactly the MMU's own arithmetic (hddma.c:274, djdma.c:344,
disk1.c:177).  That decode is the MMU's, and it is duplicated because the
bus offers no way to ask the memory system to interpret a DMA address.  The
interface wants a `bus_dma` entry that runs the address through the MMU the
way a fetch does, and then the three copies collapse into one.

Arbitration, ready/wait, refresh.  The honest state is that none of it is
modelled.  A controller's DMA runs inside the guest's OUT instruction as a
memcpy - "everything being done synchronously" (hddma.c:7) - and the one
board that exists to arbitrate says so outright: "the arbitration is not
modelled cycle by cycle; the channel is just the holder of the transfer's
address" (selector.c:6).  There is no pHLDA, no cycle stealing, no
contention, and no RDY/WAIT line anywhere.  Halt is modelled, but as CPU
state and not as a bus state: the tick sets S_HLTA from Z80_HALT
(mpz80.c:66) and the main loop charges a nop's worth of cycles so a halted
processor does not stop the clock (hwsim.c:2420).  Refresh is absent.  The
sketch should reserve the names for these in the interface - `bus_hold`,
`bus_ready`, `bus_tick` - but not build them until a board needs one,
because the existing drivers do not.  Pretending otherwise would be
specifying machinery with no user.

Time and console are already bus-shaped.  The timeout facility
(hwsim.h:106) and console arbitration (`console_claim`, hwsim.h:157) are
the two cross-board services that already work and should simply be part
of the bus handle a board is handed, since djdma and multio already share
the console through them (djdma.c:1013, multio.c:1009).

One latent contract to fix while in there.  `set_vi()` computes the new
line state but never stores it; it relies on the `vi_change` callback to
store the value it is handed (s100.c:85, and the three installers all do -
default at s100.c:23, multio at multio.c:653, ss1 at ss1.c:224).  A fourth
installer that forgets would leave `set_vi`'s wire-or reading a stale base
on the next call.  The bus should own the storage and hand the controller a
notification, not a storage duty.


instance and address
--------------------

A board becomes a struct, constructed once per instance with its strapping
set, and the driver file becomes the type - the functions that know how the
board behaves - rather than the instance.  The djdma's statics are the
clearest candidate to convert first: dparams, channel, imdp[8], and the
serial state all move into one struct, and `pulse_djdma` etc. take it as an
argument.  The existing `struct ace` in multio.c:46 is already an instance
of exactly this shape inside one file; the sketch is to give every board
one and stop at one per file.

The address and the straps are set at construction, and the command line
overrides them afterwards.  The TODO's two spellings - an option per
emulation, or one option taking `<card>:<addr>` pairs - should resolve to
one option, because per-emulation options do not compose: a machine with
two hdca cards has nowhere to put the second address.  The shape is `-j`
generalised, so `-j bc` remains a working spelling and becomes sugar for a
`djdma:j2=bc` entry.  An implementer has to decide the character that
does not collide with the existing alphabet (hwsim.c:710-751 is the
current usage text) and whether the value grammar is one word or a
comma list; the sketch's preference is `<card>:<strap>[=<value>]` repeated,
the way `-w` already takes `key=value` overrides (s100.c:418).

The port number is just one more strap.  The d1/TODO's "the port numbers
want to be overridable" folds into the same mechanism: `hdca:base=0x70`,
and the board's defaults stay the #defines for the machines that do not say
otherwise.  The strap set a board announces is part of what it declares
about itself, so usage() prints it from the registry instead of hand-writing
the `-j` line (hwsim.c:715).

Two instances also force the card-id question, which is the next section.


interrupts and the shared line
------------------------------

The VI lines are open collector and already modelled as a wire-or.  The
d1/TODO's first note - "the HDCA clearing it won't necessarily clear the
interrupt, since the HDDMA might also have the level up" - describes
behaviour `set_vi()` already provides: each line keeps an eight-bit mask,
one bit per card, and the line is up while any card's bit is set
(s100.c:76).  The hddma and hdca share vi_0 in the `d1` profile with
distinct card ids, 1 and 0, and the case works (s100.c:277, hddma.c:260,
hdca.c:163).  So the shared line is not unhandled in the model; what is
unhandled is that the model's bookkeeping is hand-assigned.

Every driver hard-codes its card id.  The set, read from the call sites:

    hdca    0   hdca.c:163
    hddma   1   hddma.c:260,508
    djdma   0   djdma.c:256,294
    disk1   1   disk1.c:42,118
    disk2   2   disk2.c:66,150
    disk3   1   disk3.c:250
    nic     0   nicd1.c:24
    ncr     1   ncr5380.c:225,370
    ide     0   ide.c:160,287

These happen to be distinct on every line of every profile today, because
whoever picked them checked the profiles.  Nothing enforces it.  Two hdca
instances would both claim id 0 and each would clear the other's level;
hdca and djdma are both id 0 and are safe only because the profiles put
them on different lines.  The fix is that the bus assigns the id when a
board claims a line, and refuses a second claim that would share a bit -
so a board stops passing a number at all and the id becomes the bus's
property of the instance, like a slot number.

The second instance problem is the static line number.  Each driver copies
its VI line into a file-static at init - `hdca_vi_line` (hdca.c:18, set at
498), `hddma_vi_line` (hddma.c:38) and so on - so two instances of one
board would write the same static and the second would clobber the first's
line.  Moving the line into the instance struct is the same edit as moving
the ports, and is the part of the conversion that cannot be skipped.

The sharing check in main is evidence the model is not trusted.  hwsim.c
refuses a machine that attaches drives to two disk controllers whose names
map to one VI line, with "two disk controllers share a VI line"
(hwsim.c:2097-2117).  A real Morrow box could hold the HD-DMA and the HDCA
both on vi_0 - the `d1` profile exists to describe it - and the driver
code is exactly what has to survive that.  The check should come out once
the bus owns card assignment, because it is currently standing in for the
invariant the bus will enforce.

One more piece of the interrupt path belongs in the sketch but has no
current user: NMI.  `nmi_line` and `nmi_pin` exist (hwsim.h:61, 68) and
nothing drives them.  The bus interface should carry them for the same
reason it carries INT, and the first board that needs NMI will be the one
that tests the plumbing.


registration and configuration
------------------------------

What a board announces, gathered from what the drivers already know about
themselves and what the framework currently discovers by other means:

    name             the word -B, -w and a drive argument all use
    ports            base plus the range it decodes
    memory           any region it answers (only the boot ROM today)
    vi line          looked up by name now; becomes a claim at construction
    intack / fold    for the one board that is the interrupt controller
    boot device      whether it can bootstrap, and its switch value
    monitor rom      which rom knows it (mon447/mon500 today)
    straps           j2, base, and any others, with defaults
    usage hook       already exists in struct driver
    resources        the image or rom file a board loads at startup

Most of that list already exists scattered across three places: the
driver struct (hwsim.h:31), the wiring table (s100.c:262), and bootdevs[]
(hwsim.c:1401).  The sketch's move is to give a board one declaration and
derive the rest, so a new board is added in one file and linked, and
usage(), -B, -w and drivearg all pick it up without further edits.

The command line and the resource files then both drive the same
construction.  `-b` names the monitor rom (hwsim.c:1894), `-M` the level 1
boot rom (hwsim.c:1900), `-d` the drive directory (hwsim.c:1942), `-e` the
media manifest (hwsim.c:1954), and the positional arguments name drives
through `drivearg()` (hwsim.c:2075).  The resources directory
(src/hwsim/resources/) holds the roms and the DJDMA firmware, and the
default rom paths are probed by access() at startup (hwsim.c:1806).  None
of that changes shape; what changes is that the board registry supplies the
names, so `drivearg`'s strcmp chain and `bootdev`'s loop become lookups in
the registry instead of lists that have to be extended by hand.

What -B becomes.  `bootdevs[]` maps a name to a switch value, and the
switch's meaning lives in the CPU card.  In the sketch the switch decode is
the machine's, not the MPZ80's: the machine says "switch bits 5-7 select
the boot card, bit 2 skips the monitor" and each board that can bootstrap
announces which value selects it.  The rom coupling - 0x00 is hdca under
mon447 and ide under mon500 (hwsim.c:1385) - stops being a comment and
becomes a fact the registry can check: a `-B` name whose board's rom does
not match the `-b` rom is a mistake the simulator can report instead of
booting "whatever that rom does have" (hwsim.c:1395).

The one genuinely hard config problem stays hard.  The console cannot be
configured because it is not discoverable from a disk - it is assembled
into the bios - so hwsim discovers it from use (CONSOLE, and
`console_claim`).  A pluggable machine does not make that easier; the
discovery mechanism is already the right one and the sketch keeps it.


what each driver needs, honestly
--------------------------------

Per driver, what the conversion actually costs.  "A rewrite" means the
static-global structure is so interwoven that moving it into an instance
struct touches most of the file.

mpz80.c - a rewrite, and the gate for everything else.  The CPU must
continue to own the z80 instance, the cycle counter and the trap model; the
MMU, the page maps and the switch-register decode must come out into
separate pieces the bus and the machine own.  get_byte/put_byte stop being
the memory system and become the CPU's two calls into the bus's fetch and
store.  This is not two instances of anything; it is the recognition that
the file is three cards today (processor, memory management, front panel)
and one of them is the machine itself.  It also carries the only genuine
surprise in the tree for an implementer: the halt, the trap window and the
task-switch countdown are timed against instruction fetches, and the
comments record that getting the count wrong by one instruction does not
boot (mpz80.c:1420, 1434) - so the split has to preserve the exact cycle
behaviour, which is why it is the last thing, not the first.

multio.c - mostly mechanical, with one duplication to retire first.  It
already has instance structs (ace[3], pics[2]) but a #defined base
(multio.c:81), a runtime group-select that re-registers ports
(multio.c:1516), and, crucially, its own inline copy of the 8259 that
pic.c was factored to replace - compare the duplicated register-set and
priority logic at multio.c:230-382 with pic.c:83-115.  The conversion is:
finish adopting pic.c (ss1.c:576 shows the thin-wrapper shape), parameterise
the base, and let the two PICs be two instances of the chip.  The console
and clock stay, but as members of the instance.

hdca.c - moderate, and the DISKS document already named the buried part:
the 512-byte sector is not just SECLEN but the 510/2 shuffle in the read
path (hdca.c:293, and DISKS).  Parameterising the base and moving the
statics into a struct is straightforward; the sector-size question is a
separate piece of work and should not be smuggled into the conversion.

hddma.c - moderate.  The channel program, command block and drives move
into a struct; the DMA address decode (hddma.c:274) collapses into the
bus's DMA entry.  It is one of the two clean bus masters to convert after
the bus entry exists, because its DMA is already entirely inside the
attention handler.

djdma.c - the largest of the disk controllers, and the one the strap
option generalises from.  It is a bus master, a serial console, a media
changer (port 0xd2, djdma.c:1270, which is a simulator service and not
really the card), and the carrier of the `-j` shape.  The conversion is the
same struct move as hddma, plus separating the media changer out.  Its
interrupt is the one that taught the tree to deliver events after the
instruction that caused them (djdma.c:443-489), so the timing comments are
part of the value and must survive the edit.

ide.c - already closest to done.  Its state is a tidy block of statics
(ide.c:198), it uses the drive module rather than imd_load, its interrupt
latch is deliberately idempotent (ide.c:284), and it documents why the
guests agree on the protocol (ide.c:48).  Parameterising the base and
moving the statics into a struct is the whole job.

ncr5380.c, disk1.c, disk2.c, disk3.c - the same struct move, each with a
base #define and a hard-coded card id to lose.  disk2 and selector together
are the one existing board-to-board relationship - disk2 reads the transfer
address the selector was programmed with (disk2.c:42, 119) - and they are
the pair that should become the first users of the bus's DMA entry, since
the selector is literally the temporary-master channel.

selector.c - trivial, but it is the board that proves the bus needs a
memory-owning category: it exists to generate bus cycles for another board,
and today that is one shared global (sc_addr, selector.c:19).

nic.c/nicd1.c - already the target shape; nothing to do except make the
bus handle the nicd1 wiring takes explicit.

kdump.c, micronix.c, pic.c, nic.c - these are linked into the d1 binary
but register no driver (kdump.c and micronix.c are in d1/GNUmakefile:27 and
have no `struct driver`; pic.c and nic.c are chip cores called by their
owners).  The sketch should give the debug helpers their own category in
the registry - a "debugger" role with init and poll - so they stop being
called by name from main (hwsim.c:2266, 2355).


the two simulators
------------------

usersim shares the library and the chips, not the board framework.  It
links libsim.a and includes the same headers (sim.h, disz80.h, mnix.h,
fslib.h, imd.h, gui.h, z80glue.h - usersim.c:43-79) and it includes
nic.h to reach the same network-card core the d1 uses (usersim.c:51).
But it does not use hwsim.h or s100.c: it has its own main (usersim.c:1290)
and no `struct driver`, `register_driver`, or `register_input/output`.  It
is a kernel/process simulator answering the syscall interface directly, not
an S-100 machine, so the board registry has nothing to offer it.

The one piece the two already share is the model the sketch wants to
reproduce: nic.c is the self-contained core, nicd1.c the d1 wiring, and
usersim reaches the same core through nic.h.  A board interface that wants
to be shared should therefore live where nic.c lives - a core with no bus
knowledge - and each simulator keeps its own wiring layer.  Whether the
bus itself (s100.c's dispatch and interrupt lines) should move into lib is
a question the sketch answers no to, for now: the d1 is the only S-100
machine, the bus is cheap to keep in hwsim, and moving it into lib before a
second machine exists would be generality with one user.  What does belong
in lib is whatever a board core needs that is not the bus - the drive
abstraction is already there - and the shared bits of the registry's
declarations, if usersim ever wants to enumerate its devices the same way.

There is a third simulator in the tree's history worth naming: lib/unused/
and hwsim/unused/ hold the earlier abstractions - an 8250 chip model
(unused/8250.c), an 8259 extraction (unused/8259.c), and a cpro/ tree with
a separate cpuz.c and i8272.c that are the road not taken.  pic.c is the
one of those that made it out; the ACE is the one that did not, and multio
still carries the inline version.  The lesson the sketch takes from
unused/ is that a chip model succeeds when it is extracted with its owner
doing the port decode (pic.c's line_changed callback, pic.h:101) and stalls
when the owner keeps its own copy.


a migration path
----------------

The constraint is the one that matters: the simulator must keep booting the
distribution floppies and generating them at every step, because that is
the thing that must not break.  The order below is chosen so that no step
changes behaviour before the mechanism it depends on exists.

First, the bus object, with no behaviour change.  Introduce the bus handle
and the registration entry points, keep physread/physwrite inline, keep the
eight-bit dispatch, and keep set_vi exactly as it is.  This step is pure
wiring and can be verified by booting the usual floppy unchanged; nothing
should look different, which is the test.

Second, card ids become bus-assigned, and the sharing check comes out.
This is the step that makes two boards on one line a supported fact instead
of a refused one, and it must come before any driver is made multi-instance
or the new instances would collide with the hand-picked ids.  The proof is
the `d1` profile with both the hdca and the hddma attached: today refused
(hwsim.c:2117), afterwards booting, which is the real Morrow case the TODO
names.

Third, one driver becomes an instance struct end to end, and it should be
the smallest disk controller rather than the largest: disk2, or ide if the
first strap to parameterise should be a base address.  The `-p`/`-j`
generalisation lands here, as the one strap option, with `-j bc` kept as
the compatibility spelling.  The test is two instances of the converted
card at two addresses answering their own ports and holding their own VI
lines.

Fourth, the DMA entry lands, and disk2+selector are converted onto it, then
hddma and djdma.  Each driver loses its private address-decode special
case as it moves, and the three copies of the segment/offset arithmetic
collapse to one.  The dist floppy boot is the regression check each time,
because the floppy controller is the boot path and its DMA decode is the
one the loaders depend on.

Fifth, the boot path moves into the registry: bootdevs[] and the switch
decode become data a board declares and the machine assembles, and the
mon447/mon500 rom coupling becomes a checkable fact.  `-B` keeps its
spelling.  This is late because it is where a mistake stops every boot, so
it wants the machinery beneath it settled first.

Sixth, and last, the mpz80 split: MMU out, memory accessor into the bus,
switch decode into the machine.  It is last because it is the only step
that can change instruction timing, and the timing comments (mpz80.c:1420)
say exactly how sensitive it is.  Only after it does a different machine
become possible - a different CPU card, a different memory map - and by
then the floppy boot has been the yardstick at every earlier step.

What the sketch deliberately does not order: the MultIO's adoption of
pic.c, the hdca sector-size work from DISKS, and the media changer's
separation from djdma.  Each is worth doing and each can slot in anywhere
after the first step, but none of them gates the others.

The open decisions an implementer has to make, collected: eight or sixteen
bit port addresses; inline physread versus board-owned memory regions and
what that costs in the hot path; the option letter and value grammar for
the generalised strap; whether the bus moves into lib before a second
S-100 machine exists; and how much of the arbitration/ready/refresh surface
to reserve in the interface now versus build when a board needs it.  All
of those are choices with trade-offs, not conclusions the tree has already
reached.

vim: tabstop=8 shiftwidth=8 noexpandtab:
