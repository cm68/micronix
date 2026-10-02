# Attributions

The micronix tree is a composite.  The bulk of it came from four
places - the Whitesmith's C compiler and its libraries, the HiTech C
runtime, Bell Labs Unix (v6, v7 and the Programmer's Workbench), and
UCB (2.11BSD and its relatives) - with a fair amount written here from
scratch and some read back out of binaries for which no source
survived.

This file records the origin of each command in `cmd/`, so the
provenance of a file can be checked without guessing.  It says nothing
about `lib/` and `sys/`, whose own makefiles and headers already carry
their lineage.

## Bell Labs Unix

### v6
- `dcheck` - the v6 link-count checker
- `fsck` - the v6 filesystem checker, written from the disassembly of `/bin/fsck`
- `icheck` - the v6 block-use checker
- `ncheck` - the v6 connectivity checker
- `mkfs` - the v6 filesystem maker
- `mknod` - a rewrite of the broken original; the v6 syscall underneath

### v7
- `ar` - the v7 archive format (the same `__.SYMDEF` table `ld` reads)
- `echo`
- `fgrep`
- `find` - the tree walk rewritten for micronix
- `ls` - the v7 version, patched
- `make` - the v7 make, adopted as the native make
- `mkdir`
- `more`
- `rm`

### PWB (Programmer's Workbench)
- `expr`
- `fd2`
- `lex`
- `msh` - the PWB shell, with its login accounting
- `wc` - a Unix wc, probably the PWB one
- `yacc`

## UCB

### 2.11BSD
- `awk`
- `cat`
- `chgrp`
- `chmod`
- `cmp`
- `cp`
- `date`
- `dd`
- `diff`
- `du`
- `ed`
- `grep`
- `ln`
- `mv`
- `od`
- `sed`
- `tar`
- `tee`

### other Berkeley
- `pr` - 4.3BSD
- `strings` - 2BSD

## Whitesmith's

The object-file toolchain, for the Whitesmith's 16-byte-header object
format (the `wsobj` layer folded into `obj.h`):

- `ld` - the Whitesmith's object linker
- `nm` - the Whitesmith's object dump, which disassembles too
- `size` - the Whitesmith's object sizes

## Third-party

- `asz` - the z80 assembler, from Gavin Tersteeg's TRASM relocating
  assembler, rewritten beyond recognition
- `less` - Mark Nudelman's less
- `s` - Webb Miller's screen editor, from "A Software Tools Sampler"
- `vi` - STevie, Tim Thompson's vi subset

## Recovered from disassembly

No source survived for these; they were read back out of the shipped
binary:

- `form`
- `init` - from `/etc/init`
- `man` - from `/bin/man`
- `sh` - the micronix shell, reconstructed
- `upm` - the CP/M emulator, from `/bin/upm`

## Written for this tree

- `ccc` - the compiler, written from scratch
- `far` - the floppy archiver
- `pwd` - a rewrite of the broken original
- `setdev` - the root/swap device patcher
- `sync` - a one-line syscall wrapper
