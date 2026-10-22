# mail - analysis of the recovered 1.6 binary

The listing is `mail.dis`; addresses below are its addresses.  The
binary is `mail16.bin` (Whitesmiths object, text 0x4423 at 0x100, data
0x06ae at 0x4523).  `mail.ctl` names `main`, `cent` and `cret`.

## Calls and frames

Every function opens `call cent` (0x44c8) and leaves by `jp cret`
(0x44cf).  DE is the frame; the first argument is at DE+4, the second at
DE+6, and the return value is in BC.  The startup at 0x100 calls main
and then 0x2ca7, the runtime exit.

## Program shape

    0159  main
    0309  the command loop
    0734  build /usr/spool/mail/<user>
    084c  deliver one letter to a local user
    0a04  the send loop over the recipients
    0b5e  a network recipient
    0d71  duplicate a string
    12a2  read /etc/netmap
    13e9  is this a known machine
    151d  the option/flag table
    1817  build /usr/spool/mail/...
    1861  read /etc/passwd
    1a33 .. 268b  the reader, letter display and mbox code

The program is the v7 mail of usr/src/cmd/mail.c, ported to Micronix and
extended with a network path.  The literals name the v7 messages
("Mail saved in \"mbox\"", "Can't save mail.", "Do you wish to keep
your mail? ", "You have no mail.", "From ") and the Micronix additions
("/usr/spool/mail/", "/etc/netmap", "/etc/passwd", "netmail",
"/bin/netmail", ": No such network address", ": No such user",
"/bin/msgs").

## main (0x0159)

    init();                             ; 0x151d, flags and signals
    sending = 1;
    loop over the arguments;            ; 0x0309
    ...

The v7 structure is followed: mail with arguments sends, mail without
arguments reads the user's spool.  The mode flag is the global at
0x4791.

## The send path (0x0a04)

The send loop walks the recipient list one word at a time (the pointer
is the global at 0x4bcb).  For each recipient:

- a local name is looked up in /etc/passwd; an unknown one prints
  `<name>: No such user` (0x07ff);
- a name containing a percent sign (0x07de "%") is a network address.
  0x13e9 compares its machine part with the names collected from
  /etc/netmap (0x12a2 reads the file, 0x1861 reads /etc/passwd).  A
  name not in the map prints `<name>: No such network address` (0x0831);
- a known network address is handed to netmail:

      0x0c4d  execl("/bin/netmail", "netmail", <address>, 0)
      0x0c63  perror("netmail") if the exec fails

The letter is on standard input of netmail already, because it was the
input of mail; this is the same hand-off the v7 source calls
`sendrmt`, but with netmail in place of uux/rmail.

A local letter is appended to /usr/spool/mail/<user> by 0x084c; the
spool path is built by 0x0734.  The "From " line and the blank line
before it are the v7 format, and a saving mail prints
`Mail saved in "mbox"`.

## The network record

mail itself never writes a network record.  It execs netmail, which
builds the M record in /usr/spool/net/out (see cmd/netmail):

    M <rhost> <ruser> <lhost> <luser>
    D<seq><len><data>
    E<next>

At the destination netact runs mail again, which appends the message
to the recipient's spool.  The captured letter begins

    From a%root  Wed Oct 21 17:36:41 2026

so the sender shown is the network address, `host%user`, and the date
follows in the v7 form.

## What differs from the v7 port in this directory

cmd/mail/mail.c is a port of the v7 source.  Its `sendrmt` writes a
temporary file and runs `mail user < temp`, and for a remote address it
runs `uux - sys!rmail user`, which fails here because there is no uucp.
The recovered 1.6 binary does not use uux: a network name is checked
against /etc/netmap and passed to /bin/netmail.  The port predates the
recovery of netmail and netdaemon; the network branch of the recovered
program is the one the tree should follow.

## Verified behaviour

The stock mail and netmail, run under usersim, write the M record shown
above and deliver it through netdaemon and netact, on one link and
across a relay (see src/usersim/nettestmail.py and
cmd/netdaemon/README).  The same binaries were used as the front end in
the three-machine test whose wire capture is in cmd/netdaemon.
