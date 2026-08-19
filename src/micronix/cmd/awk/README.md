# awk — 2.11BSD port

**Status: works.** awk runs in micronix's flat 64 K address space as
integer arithmetic: `AWKFLOAT` is a signed 32-bit `long`, the lexer is a
hand-written recognizer instead of lex-generated tables, and four ccc
codegen bugs are worked around in source.  The image is ~54 KB
(text 41,407 + data 12,885) and it is in DIRS, so it builds with the
rest of the tree.

## What this directory is

A port of 2.11BSD awk (`extra/2.11/pdp11/usr/src/bin/awk`) to micronix.
Two things ccc cannot do shaped it:

- **Floating point.** ccc has no `float`/`double`, so awk's numeric model
  (`AWKFLOAT`) is a signed 32-bit `long`.  Ordinary awk arithmetic - field
  counts, running totals, string lengths - is exact; only the fractional
  part of a division and the transcendental builtins are gone, and
  `log`/`exp`/`sqrt` collapse to integer approximations.  `intmath.c`
  supplies `isqrt`/`ilog`/`iexp`, `ftoa` (a number as `%ld`), and `fcmp`
  (the three-way compare).  No soft-float library is linked.

- **Struct-by-value.** awk passes its 4-byte `obj` around by value, which
  ccc rejects, so `obj` is packed into a `long` - cell pointer in the low
  word, otype in the next byte, osub on top - and every `x.optr` /
  `x.otype` / `x.osub` access went through the `objptr` / `objtype` /
  `objsub` / `objmk` macros in `awk.def`.

## The lexer

`yylex.c` is hand-written.  awk's lexical grammar is just keywords,
operators, numbers, quoted strings and inline `/regex/`, so a brute-force
scan with a one-character pushback is a fraction of the DFA tables' size
(`yylex.o` 4.1 K vs the generated `lex.yy.o` 10.1 K).  `lex.yy.c` is kept
for reference but is not built.

## ccc codegen bugs worked around

Getting awk to run surfaced four more codegen bugs, all places c1 loses
one half of a two-word `long` (CODEGENGAPS entry 23):

- `(long)(x) & 0xffffL` returns the **high** word, not the low.  This is
  why `print` dumped the text segment: in `nodetoobj`, `(cell *)a->nobj`
  lowered to a four-byte load of the struct field - `nobj` *and* the
  neighbouring `narg[0]` - and the mask returned `narg[0]`.  `objptr`
  narrows through `int`, `objmk` widens through `unsigned`.
- a `long` global initialised from a shift folds to 16 bits, so `true`
  and `false` came out two bytes and zero; they are literals now.
- the `fcmp` nested ternary `(a)>(b)?1:(a)<(b)?-1:0` returns 255 for the
  -1 when the operands are 32-bit struct fields, so `1 > 3` was true;
  `fcmp` is now a real `if`/`else` function.

The parser's `++yyps > &yys[YYMAXDEPTH]` overflow check also needed the
entry-22 workaround (a pre-computed `yyslim` and a split increment) in
`y.tab.c`, the same fix as v7 make's parser.

## How it builds

The generated files are committed so the native build needs only ccc:

- `y.tab.c` from `awk.g.y` by yacc (on the host),
- `proctab.c` from `proc.c` + `token.c` (a host program),
- `yylex.c` is hand-written (replaces `lex.yy.c`).

`GNUmakefile` cross builds it; the lowercase `makefile` builds it inside
micronix.  `popen.c` is a stub (`popen`/`pclose` return NULL/0): micronix
has no shell to pipe through, so `print | cmd` and `cmd | getline` are
dead.

## Size

| segment | bytes |
|---|---|
| text | 41,407 |
| data | 12,885 |
| **total** | **54,292** |

The 32-bit `long` arithmetic is the price of awk's numeric range: the
`q*`/`l*` helper routines the Z80 calls for every 32-bit operation are
~690 bytes of the text, shared across all the call sites.  Using 16-bit
`int` would drop them but cap numbers at ±32767, which is not a usable
awk.

## Not ported (faithfully)

The relational-expression grammar is the original 2.11BSD one: a
comparison is an expression only inside `if (...)` and patterns, so
`x = (a == b)` and `print (a == b)` are syntax errors, and `print a > b`
parses as redirection.  This is the stock grammar, not a port bug.
