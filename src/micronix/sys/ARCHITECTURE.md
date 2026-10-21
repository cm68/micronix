# Kernel architecture

Micronix runs in one 64K address space per task, built from 4K
segments that the Morrow MPZ80's memory management unit maps through
sixteen page registers.  The kernel is one flat resident image low in that
space, the drivers are single pages swapped into one window, the buffer
cache's data lives in segments outside the kernel's own and is reached
through a second window, and the code that runs only at boot is parked
above the resident image and turned into buffer headers once it is dead.

That is the whole shape:

- the resident kernel at 0x1000, up to `_ebss` - text, data, bss, and
  the first eight buffer headers at the end;
- the init-only code (`highmem.o`) parked at `_ebss`, reclaimed as more
  buffer headers after `main()` returns;
- four fixed windows: scratch (0xc000), driver (0xd000), buffer
  (0xe000), and the u page (0xf000);
- every disk driver a 4K module, linked at 0xd000 and mapped there one
  at a time;
- every user process a set of 4K segments of its own, above the
  kernel's sixteen, with a per-process u page;
- every piece of hardware an interrupt line discovered at boot, not
  named in the driver.

The sections below take those one at a time.  The code comments carry
much more detail than this file: `include/sys/ovl.h` for the module
interface, the makefile for the link that fixes the layout,
`include/sys/proc.h` for the process and user structures.

## The overall memory map

### The 64K the kernel sees (task 0)

| address | segment | contents |
|---|---|---|
| 0x0000 | 0 | the CPU board's low page: firmware variables and trap references, the 8259 vector table, the MMU map registers at 0x600, and a block of memory the boot left free |
| 0x1000 | 1 | the kernel entry header: a jump, `_trapvec`, and the process and mount table pointers ps and df read |
| 0x1000-`_ebss` | 1-11 | the resident kernel: text, data, bss |
| `_ebss`-0xc000 | 11-12 | `highmem.o`, the init-only code, parked here until `main()` returns and then overwritten by buffer headers |
| 0xc000 | 12 | the scratch window: one segment borrowed for user memory and by `swin()` |
| 0xd000 | 13 | the driver window: one module page, `OVLBASE` |
| 0xe000 | 14 | the buffer window: one segment of eight buffers held by `bhold()` |
| 0xf000 | 15 | the u page: leaf runtime, the system stack, and `struct user` |

The exact numbers move with the kernel.  At the time of writing
`_ebss` is 0xb60a, so the resident image is 0x1000..0xb60a (41.5K),
`highmem.o` runs from there to just below the driver page, and the
buffer pool's header area is 0xb60a..0xc000.  `nm sys/unix` (or
`bin/mxnm unix.link`) names all of them.

`win.o`, the part of the resident kernel that can live in the buffer
window's page (0xe000), is linked there as data: `open`, `sys2`,
`rdwri`, `memdev`, `link` and `access`.  Its 4,012 bytes would
otherwise have cost 211 buffer headers below 0xc000.  The trade is
only sound because that code runs only while no buffer is held - see
the pool section.

### Physical memory

The machine's memory is counted in 4K segments, up to `MAXSEG` (256,
one megabyte).  Segments 0-15 are the kernel's address space.  Segment
0 is the CPU board's low page; the rest are what the kernel's own
image and windows occupy.  Processes and the buffer cache come from
segments 16 and up.

`meminit()` (main_init.c) finds the top by writing and reading back the
first byte of each segment, starting at 16, mapped temporarily into the
0x1000 window.  The first segment that does not answer is the end of
memory; every segment past it is marked used in `segmap[]`, because a
"free" segment past the end would alias real memory onto open bus.

`segalloc()` and `segfree()` (malloc.c) keep `segmap[]`, one byte per
segment, with `lastseg` as a hint.  `nsegs` is the count free.  The
allocator is shared by processes and the buffer cache; a shortage is a
panic, not a swapout, because returning a garbage segment aliases two
owners' pages and shows up later as a block read that is not the block
asked for.

### Where the layout is fixed

The makefile builds five objects and places them by address, then an
ordinary link puts them in order:

- `low.o` - the entry header, the resident C, the hand-written
  assembly, and `textpad.o`, whose empty `.bss` defines `_ebss` after
  every other object's.
- `highmem.o` - every section folded to data (`-Sdata`), no address of
  its own; the final link's `-Shigh` parks it at `_ebss`.
- `ovlslot.o` - one page of data at `OVLBASE`.
- `win.o` - the window-page code, `-Sdata -Tdata=0xe000`.
- `upage.o` - the leaf code and `user.o` at `-Tdata=0xf000`.

The final link is `-Ttext=0x1000` with no data origin, so the data
follows the text and the resident image ends at `_ebss`.  The three
fixed pages are objects with an address in their own header, not
special cases in the final link.

## Memory mapping policy

### The map registers

Each hardware task has a 32-byte table of sixteen page registers in
map RAM: task 0's at 0x600, task n's at 0x600 + 32n, selected by the
task register.  Task 0's are the ones the kernel writes as it runs.
A register holds the physical segment a 4K page maps to, with that
page's permission bits.

The registers are write-only.  Reading one back is bus garbage, not
the segment in place.  Everything that remaps a window therefore
tracks the current segment in a variable or restores a literal:

- `bufseg` (uio.c) for the buffer window,
- `scratchseg` (uio.c) for the scratch window,
- `u.drv` (proc.h) for the driver window,
- the literal 12 for mem.s's scratch-window borrows,
- the literal `OVLSEG` for the slot's identity page.

The monitor's readable copy of task 0's map (`IMAGE0`, 0x200 in the
old layout) is not read by the kernel any more; 0x200 is the master
8259's vector table now.

### The kernel's identity map

Task 0 starts with segment n mapped at n * 0x1000: the kernel's linear
addresses are physical segments, which is what lets it be linked flat
at 0x1000 and run wherever the loader put it.  Four pages are remapped
through the kernel's life, and the 0x1000/0x2000 borrows below are the
only other exceptions:

- **0xc000, the scratch window (SCRSEG 12).**  The kernel's own code
  and `swin()`/`srel()` borrow it to reach a user page or a driver's
  segment.  `mem.s` maps the page a user address falls in - found in
  `curmem`, the current process's `mem[]` - writes through the window,
  and puts 12 back.  Interrupts are off across the borrow, so nothing
  can be entered with the register half changed.
- **0xd000, the driver window (OVLSEG 13).**  One module page.
  `ovlmap()` and `ovlcall()` record the segment in `u.drv` before
  writing the register; `ovlremap()` puts it back at every process
  switch.  See the driver section.
- **0xe000, the buffer window (BUFSEG 14).**  `bhold()` maps one
  buffer's segment and returns its 512 bytes; `brel()` restores the
  previous segment from a stack.  See the pool section.
- **0xf000, the u page (USERSEG 15).**  Task 0's entry is remapped at
  every process switch to that process's u page segment (`mem[16]`).

`mem.s` also uses the 0x1000 and 0x2000 windows (`segcopy`, `memrw`,
`zerouser`) to map two arbitrary segments at once, restoring them
afterwards.  Those are the kernel's own text and data pages, so the
copies keep interrupts off across the whole transfer.

### User memory

A process gets sixteen user pages, 0x0000-0xffff, plus a seventeenth
for its u page.  `struct proc.mem[17]` holds a segment and a
permission byte per page:

- `FULL` - a real segment is there,
- `GROW` - the segment is shared with the process's growing data and
  the page may still be claimed,
- `NONE` - not a page this process may touch.

The user task's map RAM holds `mem[0..15]`; `mem[16]` is the u page and
is never seen by user code.  When a page marked `GROW` is touched, the
hardware takes an `MTRAP` to `fault()`, which promotes the page to
`FULL` and allocates a new grow segment if needed.  `grow()` refuses
the gap between the program break and the stack pointer - a fault there
is `SIGSEGV` - and `valid()` applies the same growth to addresses a
system call is about to read or write.

The kernel never switches to a user task to reach user memory.  It
reads and writes user addresses through the scratch window with
`getbyte`, `putbyte`, `copyin` and `copyout`, all of which crack at 4K
boundaries and keep the window correct across the change.  The u page
is the exception that proves the rule: it is mapped into task 0 at
0xf000, so the kernel's stack and `struct user` are always reachable
without a window.

## proc and task

A process has two identities, and they are not the same thing.

**`struct proc`** is the software object, of which there are `NPROC`
(20).  It holds the mode bits, priority, pid, parent, controlling tty,
page table (`mem[17]`), open files (through `u`), swap address, signal
dispositions, accounting, and the two words that make a context switch
possible: `frmptr` and `stkptr`, the saved stack frame.  The table is
published to ps through the word at 0x1003.

**A hardware task** is one of sixteen slots.  Task 0 is the supervisor
and is the only one the kernel runs in.  Tasks 1-15 belong to
processes, and each has its own 32-byte map table in map RAM.  A task
slot is what installs a user process's map; it is not the process's
own identity: a process that cannot return to user mode does not need
one, and `taskalloc()` takes a slot back from the stalest process when
all fifteen are in use.

The relationship is deliberately loose:

- `taskmap` is the allocation bitmap; `proc.task` is the slot a
  process holds, or 0.
- `p->mem[]` is the master copy of the map.  The map RAM is a cache of
  it, loaded by `taskload()` whenever the map changes and when a slot
  is reacquired.  The map RAM can therefore be taken away and reloaded
  without losing anything.
- `newmap(p)` runs at every switch.  It allocates a slot if p has none,
  loads the map, remaps task 0's 0xf000 to p's u page (`mem[16]`), sets
  `u.p`, `u.task`, `curmem`, and puts back the driver page in `u.drv`.
- `u.task` and `u.mask` travel in the trap frame.  The firmware's
  return to user mode (`gotask` in the monitor) takes the task and
  mask from that frame and writes the task register, so the process
  resumes in its own task and mask.

The switch itself does not touch the hardware's task register.
`sched()` picks the highest-priority awake loaded process; `next()`
saves the current frame in its `proc`, remaps the u page to the new
process with `newmap()`, and resumes through the new process's saved
frame.  The u page carries the kernel stack, so switching processes is
switching kernel stacks.

The trap entry is the other half.  User code traps (`rst 1`, a fault,
or an interrupt) into task 0; the firmware saves the register set in
the u page, from `&u.save` down to `&u.task`; `trap()` dispatches and
returns through the firmware, which restores the mask and writes the
task register.  `start()` and `task0_body()` set this up once at boot:
the kernel first runs as task 1, traps itself into task 0, copies the
loader's stack into `u.stack`, and never leaves task 0.

`fork()` copies the `struct proc`, sets the child's `task` to 0, gives
it a new map with `mget()` (17 fresh segments) and clones the `FULL`
pages with `segcopy()`.  The child is loaded but not current; its slot
is allocated at its first switch.  So a process may outnumber the
tasks, and the map RAM is never the only copy of anything.

## Driver overlays

### The build

Every disk driver is built as a module, not linked into the kernel:

    mxld -s -Sdata -Tdata=0xd000 -Aunix.link -o driver.mod driverhdr.o driver.o

`-Aunix.link` links it against the kernel's symbol table, so a driver
can name anything in the kernel and get a real address.  The kernel
names nothing in the driver - not an entry point, not a data object -
which is why a kernel with no disk drivers can be linked at all.

The header object is named first, and `-Sdata` folds every section into
the data segment, so `struct ovlhdr` is the first thing in the page.
The page is what the module link produced after its 16-byte object
header; the build pads it to 4K and appends it to `unix`.

`unix` is therefore `unix.link` rounded up to a page, with one page per
driver appended, and the root driver's page stamped into the empty slot
at 0xd000 by `setdev -i`.  The slot is part of the kernel's own image;
the appended pages are reachable only by reading the file back.

### The header

`struct ovlhdr` (include/sys/ovl.h) is the entire interface:

    struct ovlhdr {
        UINT major;
        int (*init) ();         /* once, with the page in place */
        int (*tick) ();         /* once a second, or 0 */
        char *data;             /* the driver's data, in its segment */
        struct biovec *bvec;    /* block entries, or 0 */
        struct ciovec *cvec;    /* character entries, or 0 */
        int (*probe) ();        /* tickle the card's interrupt */
        int (*intr) ();         /* the handler, or 0 */
        char name[12];
        UINT8 ndrv;
    };

The rules that matter:

- `init` runs first, with the page mapped and nothing reachable.  A
  non-zero return means "this card is not in the machine": nothing is
  registered, the major goes on answering as `nodev` does, and the
  only cost is the page.  That is how one kernel boots every board.
- `data` is a pointer into the driver's own segment.  It is how a
  resident `init` reaches the driver's state at all: resident kernel
  code can name an address in the module, but not a symbol in it.
- `name` is a character list, not a string literal.  ccc emits a copy
  of a literal ahead of the object's data, which would push the header
  off the front of the page.
- `probe` and `intr` are how the card is found and served; a driver
  names no interrupt line.

### Placement at boot

1. `cus()` registers the root driver: `setdev` stamped its page into
   the slot, and `ovlstart()` calls `ovlattach(OVLSEG)`.  That maps
   0xd000 to itself and registers the header.
2. `main()` runs `ovlplaceall()` after the root device is mounted.  It
   opens the inode the loader left in `kino`, computes the kernel
   object's extent from its header (the same arithmetic setdev used),
   and reads every page appended past it.  A page's first bytes say
   which major it serves, so no table of modules is needed; a major
   already placed is skipped, because a second placement would double
   the interrupt handler and the tick.
3. Each new page is copied into a segment from `segalloc()` - through
   the driver window itself, not the scratch window, because the
   init-only code doing the copy is parked over the scratch page - and
   `ovlattach(seg)` places it.

Registration (`ovlplace`) takes the header's entries only after
`init()` succeeds, fills `ovlbvec`/`ovlcvec` for that major, arms the
tick if one was named, and gives the probe and handler to the
interrupt attach.

### At run time

The kernel's device switch tables point at trampolines, not drivers:
`ovlopen`, `ovlclose`, `ovlstrat`, `ovlioctl` and the character
equivalents in ovl.c.  Each recovers the major from the device and
calls `ovlmap()`, which maps that major's segment (or, for a resident
driver, does nothing) and returns the header.  An entry that is zero
means no driver: the request completes as `nodev`/`ENXIO` rather than
hanging, which is what a major that may be loaded later has to do.

`ovlcall(fn, seg)` is the same idea for a function that is not a
device entry: map the segment, call, restore.  Interrupt handlers and
driver ticks both run through it, because they arrive at moments when
the caller's page is not necessarily mapped.

The window is one page for the whole machine, but the driver a process
is inside is the process's own business.  A strategy routine sleeps on
its board, the process is switched out, another process maps another
driver - and when the first wakes, the page must be its own.  That is
what `u.drv` is for: it is written before the map register on every
entry to a module, survives the switch in the u page, and `ovlremap()`
puts it back at every `newmap()`.  A driver must still not assume its
page is mapped across a sleep it did not enter through one of these
doors - the doors are the whole contract.

`ovltick()` walks the modules that asked for a tick once a second and
calls each with its page mapped, via `ovlcall`.  One timeout serves all
of them, so a driver's watchdog costs the kernel no timer of its own.

## Interrupt discovery and dispatch

Nothing about the interrupt wiring is compiled into a driver.  The
board is discovered at boot and the line is discovered by tickling the
card.

### The controller

`pic_init()` (pic_init.c, init-only) probes for the controllers:

- the SS1's two CAS-cascaded 8259s at 0x70-0x73,
- the Wunderbus master at 0x48-0x4f, selected through a group latch,
- the add-on MultIO's slave at 0x40-0x47, either polled or
  CAS-vectored.

Presence is a round trip: the IMR (OCW1) is written with 0x00 and
0xff and read back.  A missing 8259 reads open bus, the same value
whatever was written, so both values have to come back.  The slave's
INT is wired to the master's IR2; the polled variant is acknowledged by
reading its poll word (the cascade stub in intrpt.s does this) and the
vectored one has its own vector table.

What the probe leaves behind is a resident table (`picdata.c`): the
PIC descriptors, `linepic[]` and `lineir[]` mapping a line number to a
PIC and an IR, and `npics`/`nlines`.  The resident code (`pic.c`, leaf
code in the u page) walks that table; it knows nothing about which
board it is on.  Each 8259's 32-byte vector table is copied to its
home before the ICWs name it (`picvec.c`): the master's at 0x200, a
CAS slave's at 0x220.

### Attaching a driver

Drivers do not call `inton()` and do not know their line.  Each header
says how to tickle its card (`probe`, with `PROBE_TICKLE`) and how to
quiet it (`PROBE_CLEAR`).  `ovlplace()` passes both, with the segment,
to `intrset()`; `intr_attach()` does the wiring:

1. mask every line;
2. for each unprocessed driver: `PROBE_CLEAR` to remove whatever the
   boot left standing, read the PIC's request register as a baseline,
   `PROBE_TICKLE`, read it again;
3. the difference is the one line this card raised.  Install the
   handler in `ovlint[line]`; a second card on the same line is a
   wiring error and panics.  A card that was not there raised nothing
   and is left unwired - the tickle is the presence test;
4. `pic_eoi` the tickled request, `PROBE_CLEAR` again;
5. unmask every line that has a handler.

The baseline matters because a card holds its line until acknowledged,
and a masked line is never acknowledged.  Without the difference, the
next probe would find the previous card's pending bit and wire its own
handler onto the wrong line.

Attach runs twice.  `cus()` places the stamped root driver and attaches
it before `main()` can open the root device; `main()` places the rest
from the kernel's own file and attaches those.  `nintrdone` is how far
the first pass got, so the second pass tickles only the new drivers and
opens from `ovlint[]` rather than from what it just found - masking is
machine-wide.

### Dispatch

A vector table entry jumps to a stub in intrpt.s (the Wunderbus uses
`vectors`, the SS1 its two tables in ss1vec.s).  The stubs that serve
discovered cards hand the line number to `ovlntr(line)` - `ovlint0`,
`ovlint1`, `ovlint5` through `ovlint7`, and the SS1's slave stubs; the
clock, console and spurious handlers are resident and do not.  On the
two-PIC Wunderbus, master IR2 is the MultIO slave's cascade: that stub
polls the slave, reads the line number out of the poll word, and
dispatches lines 2-4.  `ovlntr()` (probe.c) calls the handler with its
module mapped (`ovlcall`) and then `pic_eoi(line)`.  The EOI happens
even when nothing claimed the line: the 8259 put the line in service
when it acknowledged it, and a line left in service blocks every
lower-priority request, including the clock.

One card per line is the policy.  The kernel carries one handler per
line, and the attach loop refuses a second; a card that shares a line
must be serviced by the one driver that owns it.  (This is also why
the driver's `intr` is in the header rather than registered from the
driver's own code: the kernel must know which segment to map before it
can call the handler.)

## The overlaid buffer pool

### Headers and data

The cache is split in two.  The **headers** are ordinary kernel
memory, contiguous from `blist` to `SCRWIN`: 19 bytes each, with
`data` computed as `BUFWIN + (i & 7) * 512` and `xmem` naming the
physical segment that holds that group of eight.
The **data** is 512-byte blocks in unmapped physical segments, eight
blocks to a segment, reached by mapping the segment into the buffer
window at 0xe000.

`binit()` seeds eight headers from one segment.  `expand_bufs()`
(main.c) mints the rest after `main()` has returned: it takes a
segment from `segalloc()` for each group of eight headers and zeroes
them into place.  The pool grows to `SCRWIN` (0xc000), capped at
`MAXBUFS` (256; `nbuf` is one byte).  At the time of writing `_ebss`
is 0xb60a, which leaves room for 142 headers - eight boot headers and
134 minted.  The count is a function of where `.bss` ends, so every
byte added to the resident kernel costs a buffer; it is printed by
`signon()` and published one byte at 0x3eb for kstat (256 reads as 0).

`make DEBUGBUFS=1` clamps the pool to 32 buffers, so a cache bug shows
up in seconds rather than an hour into a recompile.

### The window and the hold stack

`bhold(b)` saves the segment currently in the window on a small stack,
maps `b->xmem`, and returns `b->data`.  `brel()` pops the stack back
into the register.  Holds nest (up to `NHOLD`, 8), so a callee's hold
does not lose its caller's, and an interrupt's hold does not lose the
process's.

The stack stores `bufseg`, never the register: the register is
write-only, and saving it once left the window mapped to page 0 - the
rom's page - after the first hold.

Two rules keep the window usable as kernel memory when no hold is up,
which is what lets `win.o` live in that page:

- **Never sleep while holding one.**  `sleep()` panics if `bholdn` is
  non-zero; the next process would run with this one's buffer over the
  window's page.
- **Never reach a buffer header, or anything that maps the window for
  its own use, while one is held.**  `bget`/`brelse`/`bdwrite` are
  out, and `imapb()` is the shape to copy: it drops its hold before
  `plug()`, which can reach `balloc()` and sleep.

`bget()` keeps the cache in a 64-bucket hash on `(dev, blk)`.  The
buffer's identity **is** that pair: change either on a linked header
without unhashing it first and the block is stranded in two buckets
and `bget()` spins.

`swin(seg)`/`srel()` are the same window discipline on the scratch
page (0xc000), for a driver that has to see a segment of its own and a
buffer at the same time.  They share SCRSEG with `mem.s`, which is safe
because `mem.s` holds interrupts off across its borrow; a process-side
`swin()` hold must not be held across anything that reaches `mem.s`.

### Why the ceiling is 0xc000

A header cannot live at or above `SCRWIN`, because `mem.s` and
`swin()` borrow that page for arbitrary segments at any moment - a
header there would be read back as user bytes.  A header also cannot
live in the buffer page, because `bhold()` maps a buffer's segment
over it for the whole of an access.  `win.o` is allowed there precisely
because its code is only entered when no hold is up, and the hold
sites' call graph is checked for that.

The scratch page is also where the tail of `highmem.o` ends up; that is
harmless because the init-only code never borrows the scratch window,
and it is dead before the pool is expanded.

## High memory and its reclaim

`highmem.o` is everything that runs once at boot and never again:
`main()`, the board bring-up (`cus`), the MIO/console init in
`inits.s`, `bcd2unix`, the disk drivers' own `init()` splits
(`djinit`, `ncrinit`), the PIC discovery and attach (`pic_init`,
`probe_init`, `picvec`), and the clock bring-up.  It is linked with
`-Sdata`, so it has no text and no bss; the final link's `-Shigh` parks
an object of exactly that shape after bss, at `_ebss`.

That placement is the reclaim.  `main()` returns into `task0_body()`,
which is resident; the first fork makes process 1 (init) and the
parent becomes the swapper; the swapper then calls `expand_bufs()`,
which mints buffer headers straight over `highmem.o` from `_ebss` up to
`SCRWIN`.  Nothing marks the object as init-only in its header - the
test is its shape (data only, no address), and the timing is the
promise that its code is finished.

The part of the park above `SCRWIN` (inside the scratch page) is not
reclaimed as headers; it is simply dead bytes.  The pool cannot use
them for the reason above: the scratch window is borrowed for other
segments.

Why it is built this way:

- Resident text is the scarcest thing in the machine, and the kernel
  has grown into its text budget.  Moving init code to the buffer
  region turns dead boot code into cache: every 19 bytes is a buffer.
- A driver's `init()` cannot live in the driver's page, because the
  page is 4K and the driver needs it at run time.  Split into
  `highmem.o`, the init is reclaimed and the page holds only what the
  running driver needs.
- Nothing in `highmem.o` is needed after `main()` returns.  The rule
  for a new function is therefore: if it runs only between entry and
  that return, it belongs there; if anything calls it later, it does
  not.

## Boot order, in one pass

1. The loader puts the kernel's inode in HL and jumps to 0x1000.
   `boot` saves HL in `_kino`, sets the initial stack, and enters
   `_start`.
2. `start()` runs in task 1, points the trap vector at `task0`, and
   `hlt`s.  The firmware traps into task 0 and calls `task0_body()`.
3. `task0_body()` sets task 0's traps and mask, moves the stack into
   `u.stack`, and calls `main()`.
4. `main()`: `cus()`, clear the freed low bss, enable interrupts,
   console, logo, `pinit()`, `meminit()`, `binit()`, `pcon()`, open
   and mount the root device, `ovlplaceall()`, `intr_attach()`,
   enumerate drives, `swapinit()`.
5. `main()` returns to `task0_body()`, which forks.  The parent
   becomes the swapper: `expand_bufs()` then `swap()`, never
   returning.  The child becomes process 1 and runs `/etc/init`.

`setdev` has already stamped the root driver into the slot (with `-i`
it writes the driver and no device numbers, so the defaults in main.c
stand).  A kernel that was linked but never stamped has a zero major
in the slot; `ovlstart()` refuses it, and the mount of the root device
fails loudly.

## Invariants worth knowing

- Map registers are write-only; track the segment in the variable that
  owns the window, or restore a literal.
- Never sleep holding the buffer or scratch window; `sleep()` panics
  on the buffer window.
- One driver page for the whole machine; a process's page is in
  `u.drv`, and it is restored at every switch.
- No kernel symbol names a driver entry point; the header is the whole
  interface, in one direction only.
- A driver's `init()` runs once, before anything can reach it; a
  non-zero return means the card is absent, and nothing has been taken.
- One handler per interrupt line; the attach loop panics on a second.
- User memory is reached through a window with interrupts off; the
  u page is the only per-process page mapped into task 0.
- Buffer identity is `(dev, blk)`: unhash before changing either.
- The buffer pool and the buffer-page code trade one currency: 19
  bytes below `SCRWIN` per header, and `win.o`'s code is only safe
  there while no hold is up.
- `highmem.o` is dead the moment `main()` returns; anything that must
  outlive that call is resident.

See also: `include/sys/ovl.h` and `INTR.md` for the driver interface and
interrupt wiring, `sys/TODO` for what is still being worked on,
`static-cleanup.md` for the split between resident, window, highmem and
module code.
