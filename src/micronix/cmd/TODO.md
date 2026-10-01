# /bin commands still without source

/bin is filled from the 1.3/1.4 distribution userland, and the tree
replaces it one command at a time.  These are what is still the
original distribution binary, with no source anywhere in the tree.

## to port

    anat      cal       chars     clean     comm      cp1
    cp2       cptree    crypt     cu        cxr       dc
    ddt       down      e         edit      file      group
    help      hex       lib       lines     lpr       lprm
    lprq      mail      newuser   obj       passwd    pilot
    print     rp        sort      stty      su        sum
    tail      td        tty       unique    update    wall
    who       words     write

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
