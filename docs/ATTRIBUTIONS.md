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
- `cal`
- `chown` - v7 chown, which micronix calls `owner`
- `comm`
- `cpp` - John Reiser's preprocessor, the standalone `/bin/cpp` (not the compiler front end)
- `crypt`
- `cu`
- `dc`
- `echo`
- `fgrep`
- `file`
- `find` - the tree walk rewritten for micronix
- `ls` - the v7 version, patched
- `m4`
- `mail`
- `make` - the v7 make, adopted as the native make
- `makekey`
- `mkdir`
- `more`
- `owner` - v7 chown, under micronix's name
- `passwd`
- `ratfor` - the Rational Fortran preprocessor
- `rm`
- `sort`
- `stty`
- `su`
- `sum`
- `tail`
- `tr`
- `tty`
- `unique` - v7 uniq, under micronix's name
- `update`
- `wall`
- `who`
- `write`

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
- `rmdir`
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
- `edit` - the Software Tools line editor (Kernighan & Plauger), the
  Ratfor source translated to C
- `less` - Mark Nudelman's less
- `s` - Webb Miller's screen editor, from "A Software Tools Sampler"
- `unrot` - the Software Tools kwic post-processor, reconstructed from
  its behaviour
- `vi` - STevie, Tim Thompson's vi subset

## Recovered from disassembly

No source survived for these; they were read back out of the shipped
binary:

- `form`
- `init` - from `/etc/init`
- `man` - from `/bin/man`
- `sh` - the micronix shell, reconstructed
- `upm` - the CP/M emulator, from `/bin/upm`

And these, read back out of the Micronix 1.6 distribution binaries
(each one's own README records the inode it came from):

- `change`
- `common`
- `compare`
- `compress`
- `concat`
- `copy`
- `detab`
- `entab`
- `expand`
- `include`
- `kwic`
- `login`
- `newuser`
- `overstrike`
- `print`
- `td`

## Written for this tree

- `ccc` - the compiler, written from scratch
- `df` - reads the kernel's mount table and the superblocks
- `djformat` - the Morrow DJ-DMA floppy formatter
- `far` - the floppy archiver
- `group` - the group-side twin of `owner`, micronix's own
- `halt` - the machine stopper
- `kill` - the v6/v7 interface, written here
- `kstat` - dumps the kernel's tables out of `/dev/mem`
- `label` - reads and writes a hard disk's label
- `lz4` - the LZ4 frame compressor
- `md` - a markdown reader for the docs/ tree
- `mount` - a reconstruction, written from the v7 source
- `mwformat` - the Morrow HD-DMA hard-disk formatter
- `packsrc` - packs a staged tree into right-sized tars
- `ps` - the 1982 original, recovered off a developer's floppy and ported
- `pwd` - a rewrite of the broken original
- `setdev` - the root/swap device patcher
- `sync` - a one-line syscall wrapper
- `umount` - a reconstruction, written from the v7 source
- `unlz4` - the LZ4 frame decompressor
- `xargs` - runs a command with arguments read from stdin
