# netdaemon - analysis of the recovered 1.6 binary

The listing is `netdaemon.dis`; addresses below are its addresses.  The
binary is `netdaemon.bin` (Whitesmiths object, text 0x44a8 at 0x100, data
0x063c at 0x45a8).  `netdaemon.ctl` names `main`, `cent` and `cret`.

## Calls and frames

Every function opens `call cent` (0x454d) and leaves by `jp cret` (0x4554).
After `cent`, DE is the frame: the first argument is at DE+4, the second at
DE+6, and the return value is in BC.  Arguments are pushed right to left,
so DE+4 is the last value pushed.  The startup at 0x100 calls `main` and
then 0x2e68, the runtime's exit path.

## Program shape

    0100  startup
    02aa  main
    02e5  daemon: loop forever
    0304  scan the inbound spool
    0375  scan the outbound spool
    0162  file transfer (the large worker)
    1718  read /etc/ttys, find the net- ports
    18b7  read /etc/netmap
    1948  netmap lookup / route
    063b  accept()
    06e1  propose()
    0901  ngets()
    09f9  nputchar()
    0ade  nopen()
    107e  lrec()
    136b  read a spool entry
    2e68  exit/cleanup

## main (0x02aa)

    if (argc != 1)
        debug = 1;                   ; H47bf
    if (!read_netmap())              ; 0x18b7
        return 0;
    if (!read_ttys())                ; 0x1718
        return 0;
    daemon();                        ; 0x02e5
    return 0;

The single argument is a debug switch.  `/etc/netmap` is read before
`/etc/ttys`; a failure of either stops the daemon before it opens a line.

## daemon loop (0x02e5)

    for (;;) {
        scan_in();                   ; 0x0304
        scan_out();                  ; 0x0375
    }

The two scans are synchronous and back to back; there is no select.  Each
call works on a spool directory and returns when it has nothing to do.

## scan_in (0x0304)

    if (!accept())                   ; 0x063b
        return;
    if (!H088d())                    ; exists/authorised?
        return;
    path = spool_path("/usr/spool/net/in/", H47bc);   ; 0x14bb
    fp = open(path);                                  ; 0x0c21
    H45b2 = fp;
    if (!H0469(fp))                  ; header/sequence?
        goto done;
    if (!transfer(fp))               ; 0x0162
        goto done;
    H3fbb(fp);                       ; runtime cleanup
 done:
    remove(path);                    ; 0x2d0f
    return;

The inbound spool is `/usr/spool/net/in/` (0x02f1).  The transfer worker
is 0x0162, the largest function and the one that calls the protocol
layer.

## scan_out (0x0375)

    entry = read_spool("/usr/spool/net/out/", H47bc);  ; 0x136b
    H45b4 = entry;
    if (!entry)
        return;
    if (!propose())                  ; 0x06e1
        return;
    if (!H0822())
        return;
    if (!read_entry(entry))          ; 0x03c0
        return;
    H3fbb(entry);                    ; runtime cleanup

The outbound spool is `/usr/spool/net/out/` (0x0361).  A request there is
proposed and then read; 0x03c0 uses "read".

## The protocol layer

The diagnostics preserve the original function names, which is how the
link layer can be labelled:

    063b  "accept ()"  "lrec fails"
    06e1  "propose ()"  "received"  "accept code"  "seq. mismatch"
          "collision"
    0901  "ngets()"  "ngets - timeout"
    09f9  "nputchar ()"
    0ade  " nopen ()"  "read"  "write"
    107e  "lrec ()"  "ngets fails"  " received"  "lrec - checksum"

So the link has, in the original's own names, nopen, nputchar, ngets,
lrec, accept and propose.  The messages are a uucp-style handshake:

- `propose ()` sends a transfer proposal; the peer answers with `accept ()
  ` and an accept code.
- `seq. mismatch` and `collision` are the sequence-number checks.
- `lrec ()` receives a line record; a bad one reports `lrec - checksum`.
- `ngets()` reads a line and can time out (`ngets - timeout`).
- `nputchar()` writes one character, so flow control is at the character
  level, matching the 40-50 bytes/second of network(4).

The same six-bit printable encoding seen in netcp (values masked to 0x3f,
then 0x20 added) appears in the transfer path; the spool format and the
link format are the same line-record framing.

## Spool and routing

`netcp` never touches a serial port.  It leaves a request in
`/usr/spool/net/out/`; netdaemon scans that directory, proposes the
transfer, reads it, and removes the entry.  Inbound files arrive in
`/usr/spool/net/in/` and are handed to the transfer worker.  Routing comes
from `/etc/netmap` (0x18b7 and 0x1948); the line configuration comes from
`/etc/ttys`, whose `net-` lines name the remote machine on each port
(0x1718, strings `/etc/ttys`, `net-`, `/dev/`).

## Reconstructed C for the loop

    void daemon(void)
    {
        for (;;) {
            scan_in();
            scan_out();
        }
    }

    void scan_in(void)
    {
        char *path;
        FILE *fp;

        if (!accept())
            return;
        if (!H088d())
            return;
        path = spool_path("/usr/spool/net/in/", spool);
        fp = open(path);
        if (H0469(fp) && transfer(fp))
            H3fbb(fp);
        remove(path);
    }

    void scan_out(void)
    {
        struct entry *e;

        e = read_spool("/usr/spool/net/out/", spool);
        if (!e)
            return;
        if (!propose())
            return;
        if (!H0822())
            return;
        if (!read_entry(e))
            return;
        H3fbb(e);
    }

## Running two instances

The recovered daemon reads /etc/ttys at startup and, for each `net-`
line, opens the port and forks.  The child returns from the port setup
with success and enters the daemon loop; the parent continues with the
rest of the file and exits at EOF.  So the daemon loop runs in the
forked child, not in the process usersim started.

The child's first call is accept(), which arms a 20 second alarm and
reads the line.  Only when that read returns - a proposal, or the alarm
- does it go on to scan the outbound spool.  A pair of idle daemons
therefore exchanges nothing until one side's timeout or the other side
proposes.

Two usersim roots with their /dev/ttyD ptys bridged reproduce this:
the recovered netcp queues an entry, and after the timeout the two
daemons exchange the S/D/E record with the three-character checksum
prefixes and the `A!`/`Y` answers recorded in cmd/netcp/ANALYSIS.md.
The reconstruction speaks the same lines and checksums, with one
process and a one second alarm rather than a fork per port; it does not
implement the serving exchange for an R request, which this capture did
not produce.

## Open points

- 0x0162 is the transfer worker; it calls lrec/ngets/nputchar indirectly
  through 0x107e, 0x0901 and 0x09f9.  Its local layout has not been
  reduced to C here.
- 0x18b7 and 0x1948 both read `/etc/netmap`; 0x1948 is the lookup used by
  the transfer path, 0x18b7 loads the map for the daemon.
- 0x136b reads a spool entry and uses "read" and "/"; it is the path
  builder/scanner for the outbound directory.
