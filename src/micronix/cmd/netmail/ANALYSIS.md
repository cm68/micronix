# netmail - analysis of the recovered 1.6 binary

The listing is `netmail.dis`; addresses below are its addresses.  The
binary is `netmail16.bin` (Whitesmiths object, text 0x3b61 at 0x100,
data 0x0601 at 0x3c61).  `netmail.ctl` names `main`, `cent` and `cret`.

## Calls and frames

Every function opens `call cent` (0x3c06) and leaves by `jp cret`
(0x3c0d).  After `cent`, DE is the frame: the first argument is at DE+4,
the second at DE+6, and the return value is in BC.  The startup at 0x100
calls main and then 0x2666, the runtime exit.

## Program shape

    010e  main
    015f  usage
    01a0  send one message
    0418  build the M header
    0468  build a spool name
    0586  take the user part of an address
    0601  take the host part of an address
    0a73  read the local name from /etc/netmap
    0b04  look a route up in /etc/netmap
    0b94  read the local login name from /etc/passwd

0x1000 and up is the linked library: stdio, the six-bit encoder, the
time and pid calls, and the error strings.  The program's own code is
the block below about 0x0e00.

## main (0x010e)

    if (argc != 2)
        usage();                        ; 0x015f
    localname();                        ; 0x0a73, /etc/netmap line 1
    ownername();                        ; 0x0b94, /etc/passwd by uid
    send(argv[1]);                      ; 0x01a0
    return;

The single argument is the address `machine%user`.  `usage` prints
`usage: netmail "machine%user"` and leaves through the exit path.

## The M header (0x0418)

The literals are at 0x040d..0x0416: `"\n"`, three `" "`, and `"M "`.
The builder takes the address and the output buffer and produces

    M <rhost> <ruser> <lhost> <luser>\n

rhost and ruser are split out of the address by 0x0601 (up to the
percent) and 0x0586 (after it).  lhost is the pointer 0x3e9e, set by the
netmap reader, and luser is 0x3e9c, set by the passwd reader.  One run
on machine a as root, sending to b%root, produces exactly

    M b root a root

## The spool name (0x0468)

The builder takes the directory and a base name and makes

    <dir><base>%<c0><c1><c2>

Each character is `(value & 0x3f) + 0x20`: c0 from getpid (0x342e), c1
from the time (0x369c) reduced by the library's 32-bit arithmetic, c2
from a counter in the data segment.  A `/` produced by a code is
changed to `0` so that the name cannot leave the directory.

netmail calls it once with the base `TEMP`, writes the record, closes
it, then calls it again with the route as the base and makes the link.
The daemon scans for names whose prefix is the route, so the TEMP file
is invisible.  A captured pair is

    TEMP%"=!        while the record is being written
    b%"="           after publication

## Sending (0x01a0)

    name = spoolname("/usr/spool/net/out/", "TEMP");
    fp = fopen(name, "w");
    parse the address;
    route = netlookup(host);
    fprintf(fp, "M ...\n");             ; 0x0418
    read stdin;                         ; 0x091b
    write D records, 45 bytes at a time;
    write E<next>;
    fclose(fp);
    final = spoolname("/usr/spool/net/out/", route);
    link(name, final);
    unlink(name);

The record is text.  A D line is `D` + sequence + length + data, both
numbers six-bit; the data is three bytes to four characters, least
significant bit first, and a final group of one or two bytes is padded
to three with 0xff.  The sequence starts at one and the E carries the
next number.

## Netmap (0x0a73, 0x0b04)

0x0a73 opens /etc/netmap, reads the first line, chops it at the first
blank, and keeps that word as the local machine name.  The error path
prints `/etc/netmap`.

0x0b04 opens the same file and reads line by line.  The first word of a
line is a directly connected machine; further words are machines
reachable through it.  A name found anywhere on a line yields that
line's first word as the route.  This is the same lookup as in netcp and
netdaemon.

## Passwd (0x0b94)

0x0b94 calls getuid, opens /etc/passwd, and takes the name whose uid
field matches, the same owner name netcp puts in its header.

## Verified behaviour

Run under usersim on a machine whose netmap is `a`, `b`, netmail
`b%root` with a two line message wrote:

    M b root a root
    D!:H5&;L]&(N5&=M%6:LIP<E-V;N1&(LEF;EIP_
    E"

The D line is sequence 1, 26 bytes; the E is 2.  A 152 byte message
made four records of 45, 45, 45 and 17 bytes, with E 5.  The message
reached the destination through netdaemon and netact in both the
two-machine and the three-machine relay tests (`nettestmail.py`).

The reconstruction `netmail.c` writes the same 26 byte record byte for
byte, and mail running with it in place of the stock netmail delivered
through the stock daemons.  The only difference is the three unique
characters in the spool name.
