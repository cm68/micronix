# /bin commands still without source

/bin is filled from the 1.3/1.4 distribution userland, and the tree
replaces it one command at a time.  These are what is still the
original distribution binary, with no source anywhere in the tree.

## to port

    clean     cpp       cxr       ddt       down      edit
    lpr       lprm      lprq      newuser   pilot     print
    rp        td

## ported since this list was written

cal, chars, comm, cptree, e, group, help, lines, sum, tail, tty,
unique, update, wall, who, words, write.

`cal`, `comm`, `sum`, `tail`, `unique` and `tty` came from the v7
sources in `extra/`, `who`, `wall`, `write` and `update` likewise.
`chars`, `lines` and `words` turned out not to be `wc` with a flag
after all: they are three binaries of their own, but they count the
same things, so they are `cmd/wc` under three more names.  `e` is
`ed` and `help` is `man`, installed as aliases.  `cptree` is `cp`
under another name: main() sees the name and forces `-r`.

`chown` and `group` are new and were never on this list: micronix
shipped `owner` and `group`, and `chown` is v7's name for the first
of them.  All four of the ownership commands, and `login`, now read
`/etc/passwd` and `/etc/group` through libc - `getpwent` and
`getgrent` - rather than each carrying their own reader.

Then, from v7: `sort`, `file`, `stty`, `cu`, `dc`, `mail`, `passwd`
and `su`, and `crypt` with `makekey` beside it in `/usr/lib` where
crypt looks for it.  That batch also put `crypt` and `getpass` in
libc - the password hash had been login's own code - so that passwd
and su could share it.

BEWARE, FOR WHOEVER TAKES THE REST: several of these commands do not
do what the 1982 binaries in the image do.  `cal` ignores a year
argument there, `comm` is a different program, `sum` has a different
checksum and `dc` is a different calculator altogether; each port
follows the manual and the v7 source and says so in its page.  Check
the shipped binary before assuming a port preserved its behaviour.

## moved to /old

Not wanted, replaced or dropped; moved out of /bin into /old for
eventual deletion - losing them loses nothing but the original binary:

- `as`, `cc`, `link`, `lib`, `obj`, `hex`, `lord`, `anat`, `cp1`,
  `cp2` - the Whitesmiths toolchain (assembler, driver, linker,
  librarian, object lister, and the two passes of the C compiler); the
  tree has asz, ccc, c0, c1 and ld.
- `ptc` - the Pascal compiler; this tree does no Pascal.

`cpp` is not among them: it is a standalone preprocessor with no
replacement in the tree, so it stays in /bin as a gap rather than a
duplicate.
