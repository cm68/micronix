# CCC_LIMITS — the compiler's syntax and resource limits, measured

The regression matrix compiles the tree and its own sources, which is a
good exercise but not a probe: nothing in it approaches the compiler's
tables.  This file records what the limits actually are, where they come
from, and what happens when a source crosses one.

The limits fall into two kinds:

- **Fixed tables and counters in the passes.**  These are the same on
  the host cross compiler and on the machine, because both are the same
  pass sources; only the run-time stack differs.
- **Run-time recursion.**  Parenthesis depth, include depth and macro
  expansion depth are bounded by the stack the pass runs on, so the host
  passes (a 32-bit host process) and the guest passes (a Z80 process
  under the kernel) have different practical maxima.

Everything below was measured on the host `mxccc` built from this tree,
by generating sources that push one dimension at a time and bisecting:

```
mxccc -m micronix -O -i<tree>/lib/include -I<tree>/include -c probe.c
```

Compile-only for the structural limits.  Where a limit could fail
silently the probe was also linked and run under `usersim` to check the
result, because "it compiled" is not the same as "it is right" — the
frame-slot bug below was exactly that shape.

## Summary

| Construct | Safe maximum measured | Enforced by | Crossing it |
|---|---|---|---|
| Nested brace blocks in a function body | **32** | c0 `MAX_BLK_DEPTH 32` | 33: fatal `too many blocks` |
| Sibling blocks with declarations | **254** | c0 block-entry table (`MAX_BLKCNTS 256`) | 255–256: c1 segfault; 257: fatal `too many blocks` |
| Sibling blocks, statements only | 256 (tested) | same | 257: fatal `too many blocks` |
| `if` statements (braceless bodies) per file | **256** | c0 block entries; `MAX_IFS` is 512 | 257: fatal `too many blocks` |
| Nested `switch` statements | **8** (correct) | c0 `MAX_SWDEPTH 8`, c1 `MAXSWNEST 8` | 9: compiles, binary broken; 10: c0 segfault |
| `case` labels in one switch | **254** | byte-sized case counts | 255: c0 `out of memory` fatal |
| Case values in switches open at once | **384** | c1 `SWPOOL 384` | assembler `.error more than 384 case values in switches open at once` |
| `goto` labels per function | **~10,000** | asz symbol machinery | ~10,550+: assembler `unexpected symbol`, garbled name |
| Scalar locals per function | **60 ints** (120-byte frame) | c0 `ER_D_FL`, frame > 120 | 61: `locals too large` |
| Parameters per function | **16** | parameter table (`MAXPARMS`) | 17: `too many parameters` |
| Array dimensions in one declarator | **6** | c0 `MAXDIM 6` | 7: `array dim` |
| Bitfield width | **15** | c0 `MAXBITS 15`; helper's 4-bit width | 16+: `bitfield big` |
| Identifier length in the object | **15 characters** | assembler symbol name field | >15: warning + truncation; two names sharing 15 characters: `symbol defined twice` |
| Macro parameters | ~**20** observed | pass0 parameter array | 21: pass0 overflows its array (`stack smashing detected`) |
| Source line length | **141 characters** | c0 line buffer | 142: c0 `bad op` (truncated line desynchronises the parse) |
| Functions per file | ≥4000 (tested) | — | not found in this shape |
| Statements per function | ≥2000 (tested) | byte counters | not found in simple shapes |
| `#include` nesting | ≥128 (tested) | — | not found |
| Nested struct definitions | ≥40 (tested) | — | not found |
| Parenthesis nesting (host compiler) | ≥12,000 (tested) | host stack | — |
| Parenthesis nesting (guest `.mx` c0) | ~300 | guest process stack/heap | between 300 and 600: c0 fails |

"Safe maximum measured" is the largest value that compiled and, where it
matters, produced the right answer.  The numbers are boundaries of the
*tested shapes*, not necessarily the first value that fails.

## The limits in detail

### Block nesting — 32

A function body may contain 32 nested brace blocks:

```
f()
{
        { . . . { int x; x = 1; } . . . }
}
```

The 33rd is `fatal(ER_S_BLK)` from c0's block-depth stack
(`MAX_BLK_DEPTH 32` in `c0/swcnt.c`).  Locals declared at a depth above
`MAXBLKLVL` (24, `c0/p1name.h`) are no longer given shared slots in the
frame allocator; the frame grows instead.  That is safe — correctness
held at depth 30 — but it is where the allocator's level tracking has to
be right, and it was not: a descent that skipped a level (a `for`
statement's scope plus its body block makes a body local level 4)
rewound the frame offset to a `mark[]` entry that had never been set,
placing a nested local on top of an outer byte variable.  `c1/docall`
was the victim; its `direct` byte was overwritten by a nested block's
word, and the compiler built for the Z80 then emitted direct calls for
indirect ones.  See `c0/regalloc.c`, `assignFrmOff`, where every level
crossed on a descent is now marked.

### Sibling blocks and `if` statements — 254 to 256

A block gets an entry in a file-wide table when it starts.  Blocks that
declare locals and blocks that only hold statements do not use it
identically, and the boundary shows it:

- **254 sibling blocks with declarations** compile.  The 255th makes
  c1 crash (a byte-sized statement count wraps); 257 is caught earlier
  by c0 with `fatal(ER_S_BLK)`.
- **256 sibling blocks of plain statements** compile; 257 is the c0
  `too many blocks` fatal.
- **256 braceless `if` statements** compile; the 257th is `too many
  blocks`, because each `if` body is a block entry.  `MAX_IFS` is 512
  (`c0/parse.c`), but the block table is reached first, so an `if`-heavy
  file hits blocks and never ifs.

The `MAX_BLKCNTS` and `funcCnts` tables in `c0/swcnt.c` are file-wide
because phase 1 counts every function before phase 2 emits any of them;
running out desynchronises the AST rather than truncating it.  They are
sized with room over the largest source in the tree (76 block entries of
256, 46 functions of 128), but the byte-sized counts inside them are the
sharp edge at the top of the range.

### Switch nesting — 8, and it fails silently at 9

`c0` has a nesting stack of `MAX_SWDEPTH 8`; when it is full,
`pushSwitch()` returns without pushing and the switch bookkeeping silently
diverges.  `c1` has its own `MAXSWNEST 8` guard, but it only sees the
switches c0 managed to emit.

Measured: 8 nested switches compile and run correctly; 9 compile, and the
resulting binary produces no output; 10 crash c0 (SIGSEGV).  **Eight is
the limit; nine is not a diagnostic, it is a wrong binary.**

### Case labels — 254 per switch, 384 open at once

Case values are bytes in this compiler, and the counts are byte-sized:

- **254 `case` labels in one switch** compile.  The 255th makes c0 exit
  with `out of memory` — a byte-sized count overflows the allocation,
  so the diagnostic is misleading but the limit is real.
- A nested set of switches shares one case-value pool of `SWPOOL 384`
  (`c1/parseast.c`).  More than 384 values across the switches open at
  that moment is caught by c1 and emitted as an assembler `.error`:
  `more than 384 case values in switches open at once`.  Two open
  switches of 200 cases each already cross it.

### `goto` labels — around 10,000 per function

Labels and their `goto`s are emitted as assembler symbols.  Up to about
10,000 labels in one function compile; at roughly 10,550 the assembler
starts failing with

```
ld: ... unexpected symbol
_<garbage>
Error: assembler failed
```

The boundary is not a clean count: 10,600 labels compiled in one run
while 10,550 and 10,650 failed, so what matters is the symbol spellings
and the assembler's tables rather than the number itself.  Treat 10,000
labels per function as the practical ceiling.

### Locals — a 120-byte scalar frame

c0 assigns frame slots to scalar locals below the saved registers and
complains at `ER_D_FL` when the scalar area passes 120 bytes:

- **60 `int` locals** (120 bytes) compile.
- **61** is `locals too large`.

Arrays are placed after the scalars, below the callee-save slots, and
are not counted in that 120-byte sum; they have the whole 64K to
themselves, less whatever the scalars and the stack take.  Register
allocation can move locals out of the frame, so the count is a maximum
for locals that stay on the stack, not a count of declarations.

### Parameters — 16

Sixteen parameters compile; the 17th is `too many parameters`.  The
first parameter rides in HL (or A for an 8-bit prototype) and the rest
are pushed; `c1` tracks them as a short.

### Array dimensions — 6

`MAXDIM 6` in `c0/p1base.h`.  Six dimensions compile; seven are
rejected with `array dim`.  `sizeof` of a fully used 6-dimensional
`[2][2][2][2][2][2]` array is 128 bytes, as it should be (64 elements
of `int`).

### Bitfields — 15 bits

`bfins`/`bfext` (`lib/libc/bitfield.s`) take one info byte after the
call:

```
bit  |7    4|3   0|
     |offset|width|
```

Four bits of width, four bits of offset — so a field is 1..15 bits and
`MAXBITS` is 15.  A wider one is `bitfield big`.

That cap is new.  It was 32, and c1 wrote the info byte as
`(offset << 4) | width` without masking, so a width above 15 spilled
into the offset nibble.  Measured then, by compiling each declaration,
reading the `.db` operand out of the assembly, and running the field
back under `usersim`:

| Declared width | `.db` | decoded | read back |
|---|---|---|---|
| 8 | 8 | offset 0, width 8 | correct |
| 15 | 15 | offset 0, width 15 | correct |
| 16 | 16 | offset 1, width 0 | 0 — a dead field |
| 17 | 17 | offset 1, width 1 | 1 — a one-bit field one bit along |
| 31 | 31 | offset 1, width 15 | a 15-bit field one bit along |
| 32 | 32 | offset 2, width 0 | 0 — a dead field |

Fields that cross a 16-bit word are handled correctly: the address
steps by two and the offset stays inside the nibble.  `unsigned a:8,
b:8, c:8, d:4` lays `a`/`b` in the first word (offsets 0 and 8) and
`c`/`d` in the second (offsets 0 and 8), and all four round-trip.

A field on the **right** of a binary operator used to fail c1 with `no
rule could build code for the output`: `p->a + p->b`, `x + p->b`,
`p->b * p->b` and `p->a == p->b` were all unbuildable.  The extract is a
call to `bfext`, whose value comes back in HL; the register estimate
scored it like a load, so the left operand stayed in HL while the right
was lowered on top of it, and the operator was handed `(HL) op (HL)`.
The right-hand extract now takes the spill path — the left is parked
first — as the call it is (`c1/lower.c`, the `e->regs >= 3` guard).
Checked by running: `v.a+v.b`, `x+v.b`, `v.b*v.b`, `v.a==v.b`,
`h()+v.b`, `v.a+v.a`, and `char`/`unsigned char` operands all return
the right answers.

### Identifiers — 15 characters in the object

The compiler's own name buffer is 32 characters (`MAXSYMLEN`), so longer
identifiers parse.  The object format's symbol name field holds 15, and
the assembler truncates:

```
warning: symbol '_aaaaaaaaaaaaaaY' longer than 15 characters
```

Two identifiers that agree in their first 15 characters then collide:
`symbol defined twice`, and assembly fails.  **Names that must stay
distinct should differ within the first 15 characters.**

The preprocessor's own symbol buffer is 16 bytes (15 characters,
`pass0/cpp.h`).

### Macro parameters — about 20, then a stack overflow

A macro with 20 parameters expands; 21 makes pass0 die with
`*** stack smashing detected ***` — the parameter array is overrun, not
a diagnostic.  (`cpp.h` says `MAXPARMS 16`; the expansion path that was
tested held 20, so the array that overruns is not the one that constant
names.)  Do not rely on more than 16.

### Source line length — 141 characters

A line of 141 characters parses; 142 is truncated by c0's input buffer
and the parser desynchronises (`bad op`, then a fatal).  The probe was a
string literal whose line was `char *p = "<n chars>";`; the limit is the
line, not the string.  Long string literals should be split across
adjacent literals or built at run time.

### Recursion — expression nesting is stack-bound, not table-bound

The host compiler's passes are ordinary 32-bit processes and handled
12,000 nested parentheses.  The guest `.mx` passes run under the
user-mode simulator on the Z80's small stack: a probe of nested
parentheses compiled at 300 and failed in c0 by 600.  This is a
run-time-stack limit, not a compiler table, so it moves with whatever
stack the pass is given.  The same applies to `#include` nesting and
macro expansion depth; those were not pushed to failure on the guest.

## The lessons

1. **The filters in the compiler are not all diagnostics.**  Switch
   nesting 9, block counts near 255 and identifier collisions produce a
   binary or an assembler failure rather than the pass saying no; the
   bitfield width was one of these until `MAXBITS` was capped at what
   the runtime can encode.  The regression matrix would not catch a
   source that crossed them; only a run that checks the answer would.
2. **Counts stored in bytes are the sharp edges** — case values,
   per-block statement counts, case counts.  The tables were sized down
   from generous values to save bss, and the constants at the top of
   this file are the measured worst cases of that trade.
3. **The frame allocator's level tracking must match the parser's block
   levels.**  A skipped level is not just a missed share; it is a
   rewound offset, and the offsets are what keep live locals apart.
   That was a real bug, fixed in `c0/regalloc.c`, and it is why this
   file says "measured and run", not "measured".
