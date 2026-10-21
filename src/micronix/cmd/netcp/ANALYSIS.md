# netcp - analysis of the recovered 1.6 binary

The listing is `netcp.dis`; addresses below are its addresses.  The binary
is `netcp.bin` (Whitesmiths object, text 0x4454 at 0x100, data 0x0657 at
0x4554).  `netcp.ctl` names `main`, `cent` and `cret`.

## Calls and frames

Every function opens `call cent` (0x44f9) and leaves by `jp cret` (0x4500).
After `cent`, DE points at the saved-DE slot on the stack, so the first
argument passed by the caller is at DE+4, the second at DE+6, and a return
value comes back in BC.  A frame is reserved by subtracting from SP; locals
live at DE-2, DE-4, ....

## Program shape

    0100  startup: pop bc; frame; call main; call exit (0x2e7f)
    010e  main
    01a5  usage - prints "usage: netcp from to"
    01b3  copy dispatch
    071d  local copy: fork and exec /bin/cp
    0283  send: destination is machine%path
    0832  receive: source is machine%path
    1151  skip_host
    11b0  wrapper for skip_host
    11c2  islocal
    146b  line copy/encode helper
    2e7f  exit

The compiler pushes arguments right to left, so the first parameter of the
callee is the last one pushed and is at DE+4; the second is at DE+6.  In
the calls below the parameters are named source, destination.

## main

    010e  cent
    0111  call 0x15c3            ; startup: environment/stdio
    0114  call 0x16e4            ; startup: argument/flag setup
    0117  if (argc < 3) usage();
    0128  i = 1
    0132  while (i < argc - 1) {
              copy(argv[i], argv[argc-1]);   ; source, destination
              i++;
          }
    0189  return 1

So the command line contract is `netcp from to` or `netcp from ... dir`,
and every source is copied to the last name.

## copy dispatch (0x01b3)

DE+4 is the source, DE+6 the destination.  The source is tested first: if
it contains `%` the copy is a receive, if only the destination does it is
a send, and if neither does it is an ordinary local copy.

    if (!islocal(DE+4))          ; source is machine%path
        receive(DE+4, DE+6);     ; 0x0832
    else if (!islocal(DE+6))     ; destination is machine%path
        send(DE+4, DE+6);        ; 0x0283
    else
        local(DE+4, DE+6);       ; 0x071d

## islocal (0x11c2)

    BC = strchr(s, '%');         ; call 0x32d7
    return BC == 0;              ; 1 when there is no '%'

`netcp` checks the source first, so a `machine%path` source goes straight
to the receive path.

## skip_host (0x1151)

    if (strchr(s, '%') == 0)
        return s;                ; no host part
    p = s;
    while (*p && *p != '%')
        p++;
    return p + 1;                ; the path part

## receive (0x0832)

The source is parsed and checked against the network map, then a
temporary file is created under the outbound spool and opened for
writing.  When the destination is itself a network name it is parsed and
checked too, so a network-to-network copy is forwarded through the same
spool.  The daemon does the actual link transfer; netcp leaves a request.

    0832  cent; frame
    083a  host = netaddr(source);          ; 0x10d6
    0858  if (!netmap_lookup(host))        ; 0x1654
              error ": No such network address\n";
    086a  if (!islocal(dest)) {            ; destination also network
              dhost = netaddr(dest);
              if (!netmap_lookup(dhost)) goto spool;
          }
    08bc  path = join("/usr/spool/net/out/", "TEMP");  ; 0x0fb8
    08cf  tmp = mktemp(path);                          ; 0x0b5b
    08de  fp = open(tmp, "write");                     ; 0x1b01
    0911  write the request for the daemon
    0986  write 'E' (0x45), terminate with '\n'
    09a1  v &= 0x3f; v += 0x20             ; 6 bits to printable

The last two pieces are the wire encoding: six bits per printable byte,
with `E` and newline markers.  The exact line framing is in 0x0832 and
0x146b.

## send (0x0283)

The source is local, so it is opened for reading; the destination is
checked against the network map and its data is written to a spool
temporary.  The error strings distinguish a device from a regular file.

    0283  cent; frame (-10)
    0297  if (open(source) < 0)            ; 0x3c2d
              perror(source), return 0;
    02b3  if (special(source))             ; 0x0e8c
              error ": Special file\n";
    02f2  host = netaddr(dest);            ; 0x10d6
    0304  if (!netmap_lookup(host))        ; 0x1654
              error ": No such network address\n";
    0361  fp = open(source, "read");       ; 0x1b01
    0397  path = join("/usr/spool/net/out/", "TEMP");
    03af  open(path, "write");
    0403  write header                     ; 0x05ac
    0425  copy/encode                      ; 0x146b
    0555  close both;  remove temporary    ; 0x1dca, 0x0d26

## local copy (0x071d)

When neither name has a host part, netcp does not copy the file itself:
it forks and execs `/bin/cp` with the two names.

    072a  pid = fork();                    ; 0x3cd2
    0748  if (pid < 0) { perror(0); exit(0); }
    075e  parent: wait                     ; 0x3fa9
    078e  child:  execl("/bin/cp", "cp", ...);
    07b6  on exec failure: perror("cp"); exit(0)

## String table

    018f  "usage: netcp from to"
    0220  "/usr/spool/net/out/"
    0234  "write"
    023a  "TEMP"
    023f  "/usr/spool/net/out/"
    0253  "read"
    0258  ": No such network address"
    0273  ": Special file"
    0709  "cp"
    070c  "cp"
    070f  "/bin/cp"
    0717  "netcp"
    07e4  "/usr/spool/net/out/"
    07f8  "write"
    07fe  "TEMP"
    0803  "/usr/spool/net/out/"
    0817  ": No such network address"

## The real spool format

The recovered 1.6 netcp runs under usersim, and its own spool entry is
not the reconstruction's format.  For

    netcp /tmp/hello sales%/tmp/copy

it writes `/usr/spool/net/out/sales%"N"` (the quoted character varies
per request):

    S sales /tmp/copy doc /tmp/hello root 600
    D!/H5&;L]&+@X69T=W;RMF"
    E"

A receive request is shorter:

    R sales /etc/motd doc /tmp/fetched root
    E!

So the real record is line based:

- the first line is `S` or `R`, then the remote host and path, the local
  host and path, the owner; `S` adds the mode.
- a `D` line is `D`, a sequence character, the byte count encoded as
  `count + 0x20`, then the data.  `!` is 1 and `/` is 15 for the first
  example; `!>` carries 30 in the second.
- an `E` line ends the record; its one character differs between a data
  record (`"`) and a request (`!`).

The data encoding is six bits per character with `0x20` added, but the
bit order is the reverse of the obvious one: the input byte is
bit-reversed, packed six bits at a time MSB first, and each six-bit
value is bit-reversed before the `0x20` is added.  That is why the real
body for "hello, network\n" is

    H5&;L]&+@X69T=W;RMF"

and not the reconstruction's `:&5L;&\L(&YE='=O<FL*E`.

## The real wire format

Two usersim machines with the recovered binaries, their /dev/ttyD ptys
bridged, and a transfer queued with the recovered netcp.  After the
daemon's accept timeout, doc sent and sales answered:

    doc -> sales
    (blank)
    8, P!
    L$ F
    O# S sales /tmp/copy doc /tmp/hello root 600
    L3'D!/H5&;L]&+@X69T=W;RMF"
    $+ E"

    sales -> doc
    (blank)
    @* A!
    X% Y
    X% Y
    X% Y

The S, D and E payloads are the spool record's own lines.  Every line
carries a three-character checksum prefix before the payload: the sum
adds each byte, including the newline, and rotates the 16-bit value
left one place; the three characters are its bits 0-5, 6-11 and 12-17
with 0x20 added.  (`O# ` is the S line's checksum, `L3'` the D line's.)
The peer answers `A!` and `Y`; the sender waits for a Y after `F`,
after the S or R header, and after every D record, but not after E.

The reconstruction sends these same lines and checksums: two
reconstructed daemons reproduce the capture above byte for byte, and
the receiving one writes the file named by the header's remote path.

## Open points

- 0x05ac writes the request header: kind, remote host and path, local
  host and path, owner, and for S the mode.
- 0x146b is the line/bit encoder: six bits per character, least
  significant bit first, in 45-byte D records; 0x2661 is the writer.
- The source-first test is certain from 0x01b3; the original parameter
  names are not in the binary, so `(source, destination)` above is the
  reading that fits the calls.
