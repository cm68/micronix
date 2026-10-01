# /bin commands still without source

/bin is filled from the 1.3/1.4 distribution userland, and the tree
replaces it one command at a time.  These are what is still the
original distribution binary, with no source anywhere in the tree.

## to port

    anat      clean     cp1       cp2       cptree    cxr
    ddt       down      edit      hex       lib       lpr
    lprm      lprq      newuser   obj       pilot     print
    rp        td

## ported since this list was written

cal, chars, comm, e, group, help, lines, sum, tail, tty, unique,
update, wall, who, words, write.

`cal`, `comm`, `sum`, `tail`, `unique` and `tty` came from the v7
sources in `extra/`, `who`, `wall`, `write` and `update` likewise.
`chars`, `lines` and `words` turned out not to be `wc` with a flag
after all: they are three binaries of their own, but they count the
same things, so they are `cmd/wc` under three more names.  `e` is
`ed` and `help` is `man`, installed as aliases.

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

## relegate to /oldbin, then delete

Not wanted at all, replaced or dropped:

- `as`, `cc`, `cpp`, `link` - the Whitesmiths toolchain (assembler,
  driver, preprocessor, linker); the tree has asz, ccc, pass0 and ld.
- `lord` - a library lister; this tree's ar builds an index, so lord
  is not needed.
- `ptc` - a Pascal-to-C translator; this tree does no Pascal.

None of these is built by the tree; they are carried only because the
distribution disks put them in /bin.  Move them to /oldbin for eventual
deletion - losing them loses nothing but the original binary.
