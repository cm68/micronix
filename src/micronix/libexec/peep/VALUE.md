# peep value numbering

A value-tracking layer for the peephole pass.  It replaces the ad-hoc
per-rule whitelists (r_h0's "is H zero", r_orclr's "is C clear") with
one structure that knows, at every point in a straight-line run, what
each register and the carry flag contain.  Rules become reads of that
structure.

This is static single assignment in miniature.  A straight-line run is
the one place SSA needs no phi-functions, which is why the Z80 backend
wants it: every definition gets a fresh name, and two registers holding
the same name hold the same value.

## The model

Each of the seven 8-bit registers A B C D E H L holds a *value number*.
A value number is an integer; a side table maps some numbers to the
constant they are known to hold.  "H is 0" is "H's number maps to 0".

A definition mints a fresh number for its destination:

  * `ld r,n`          - a fresh number, known = n
  * `ld r,r'`         - r takes r' 's *current* number (a copy, a snapshot)
  * `ld r,(mem)`      - a fresh number, unknown
  * `xor a`           - a fresh number, known = 0
  * anything else     - a fresh number, unknown

The snapshot rule is the whole of the SSA analogy.  `ld b,a` gives B the
number A holds *now*; a later write to A mints a new number for A and
leaves B's number alone.  A copy chain `ld b,a ; ld c,b` lands B and C
on one number, so C is A.

The carry flag is tracked separately, as one of CLEAR / SET / UNKNOWN.
The other flags (Z, S, H, P/V, N) are not tracked yet; they are only
needed for the `ld a,0 -> xor a` class of rewrite, which is deferred.

## next(): the transition

A pure function `next(state, insn)` that returns the state after one
instruction.  This is the correctness-critical table; a wrong entry
costs bytes the way the `or a` case nearly did, so every flag effect is
spelled out.  Notation: `r←n` constant, `r←r'` copy, `r←⊥` fresh
unknown, `C←0` clear, `C←1` set, `C←C` untouched, `C←⊥` unknown.

```
ld r,n              r←n                    flags untouched
ld r,r'             r←r'                   flags untouched
ld r,(hl|ix+d|iy+d) r←⊥                    flags untouched
ld a,(nn)           a←⊥                    flags untouched
ld (mem),r          -                      flags untouched
ld hl,nn            h←hi(nn), l←lo(nn)     flags untouched
ld hl,rr            h←hi(rr), l←lo(rr)     flags untouched   (copies)
ld hl,(mem)         h←⊥, l←⊥               flags untouched
ld de,nn / ld bc,nn as for hl
push rr             -                      flags untouched
pop rr              rr←⊥                   flags untouched
pop af              a←⊥, all flags ←⊥

add a,r|n           a←⊥  C←⊥               (Z S H P/V also ⊥, untracked)
add hl,rr           hl←⊥  C←⊥              (16-bit: Z S untouched)
adc a,r             a←⊥  C←⊥
adc hl,rr           hl←⊥  C←⊥
sub r|n             a←⊥  C←⊥
sbc a,r             a←⊥  C←⊥
sbc a,a             a←(0 if C=0, FF if C=1) C←C   (the sign-extension idiom)
sbc hl,rr           hl←⊥  C←⊥
cp r|n              -    C←⊥
neg                 a←⊥  C←⊥               (C = (a != 0))

and r|n             a←⊥  C←0               (Z S ⊥)
and a               -    C←0               (a unchanged)
or  r|n             a←⊥  C←0
or  a               -    C←0               (a unchanged)
xor r|n             a←⊥  C←0
xor a               a←0  C←0               (Z=1 S=0, all known)

inc r (8-bit)       r←⊥  C←C               (Z S H P/V set, C untouched)
inc rr (16-bit)     rr←⊥ flags untouched
dec r / dec rr      as inc

rla / rra           a←⊥  C←⊥               (a shifts, C = the bit moved)
rlca / rrca         a←⊥  C←⊥
rl r / rr r         r←⊥  C←⊥
rlc r / rrc r       r←⊥  C←⊥
sla r / sra r       r←⊥  C←⊥
sll r / srl r       r←⊥  C←⊥

bit n,r             -    C←C               (Z←⊥, C untouched)
res n,r             r←⊥  flags untouched
set n,r             r←⊥  flags untouched

ex de,hl            de←hl, hl←de           flags untouched   (swap numbers)
ex (sp),hl          hl←⊥                   flags untouched
ex af,af'           a←⊥  all flags ←⊥
exx                 bc←⊥, de←⊥, hl←⊥       flags untouched

scf                 C←1
ccf                 C←⊥                    (C←~C)
cpl                 a←⊥  C←C               (N H set, C Z S untouched)
daa                 a←⊥  C←⊥
nop / di / ei       -                      flags untouched

jp / jr / ret       (see invalidation)
djnz                (see invalidation)
rst                 (see invalidation)
call                a←⊥, bc←⊥, de←⊥, hl←⊥  flags ←⊥
                    (call fenter: only hl←⊥, iy←⊥)
```

## Invalidation

Anything that ends the straight-line run resets every register and the
carry to UNKNOWN, because control can arrive from elsewhere:

  * `jp` / `jr` / `ret` / `reti` / `retn` / `djnz` / `rst`
  * any label definition

`call` is not invalidation - control returns to the next instruction -
but it clobbers the caller-saved set and the flags, so those go to ⊥.

## Fold and recomputation

The state is a left fold over the line stream: `s[i] = next(s[i-1],
line[i])`.  Because `next` is pure, the part of the fold before the
window - the already-emitted prefix - is a carried value that a rule
fire never invalidates.  Only the <=16-line window needs re-folding
after a rule rewrites it, and only the suffix from the modified line:
`s[k-1]` is reused, `s[k..]` recomputed.

## Queries

What the rules ask, each a read of the state at `win[0]`:

  * `iszero(r)`  - r's value number maps to 0       (r_h0)
  * `isclear()`  - carry is CLEAR                   (r_orclr)

A rule fires when it can delete or rewrite a line the state says is
already right; after the rewrite it re-folds the affected suffix.
