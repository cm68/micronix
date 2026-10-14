# rcp — remote copy, over rsh

The guest's remote-copy client, over the NIC's socket face (sys/NET.md).
The command line and function clone the real rcp; the transport is rsh to
the remote's rshd, and `.rhosts` on that end does the auth.  The ESP32
does all of tcp/ip; this program speaks rsh and the rcp file stream.

## Command line

    rcp host:file1 host:file2 ... /local/dir     GET remote -> local dir
    rcp /local/file1 /local/file2 ... host:/dir  PUT local -> remote dir
    rcp host:/path/file  /local/name             one file, renamed

One side is remote and carries the `host:` prefix (an ip); the other is
local.  The last argument is the destination: a directory (each source
lands in it under its basename) when there is more than one source or it
ends in `/`; a single source into a non-`/` destination is renamed to it.

    rcp [-P port] [-r] host:file ... dir
    rcp [-P port] [-r] file ... host:dir

    -P port   the rshd port; default 514
    -r        copy directories recursively

No password: the server trusts this host via `.rhosts`.

## The rsh request

One connection to the rshd port.  The client sends, right after connect:

    "0" NUL <luser> NUL <ruser> NUL <command> NUL

`0` is the stderr port (no back-channel yet), the two users are the local
and remote account names (`RSH_USER`, "curt" by default), and `<command>`
is what rshd runs on the remote:

    PUT   rcp -t <remote-dest>          (rcp -r -t for -r)
    GET   rcp -f <src1> <src2> ...      (rcp -r -f for -r)

## The rcp file stream

Over that same connection, after the request.  This is the wire protocol
the real BSD rcp speaks, discovered by probing rshd, and it is a sequence
of pieces, each of which the receiver acknowledges with one NUL byte:

The source opens with a NUL, then a piece per directory entry:

    D<mode> <size> <name>\n         a directory (size is 0); ack it
    C<mode> <size> <name>\n         a file; ack the header, then it sends
        <size bytes>                the file's bytes and the end-of-file
        NUL                         NUL back to back, and waits once, so
                                    ack the pair
    E\n                             done with this directory; ack it

`mode` is four octal digits, `size` is decimal, `name` is the entry's
basename inline to the newline.  The source closes the connection after
the last piece, so EOF there is the end of the transfer.

For PUT the client is the sender (`rcp -t` on the far end writes each
file by basename); for GET the client is the receiver (`rcp -f` sends the
named files) and writes each one locally, using the stream's name for a
directory destination or the given name for a rename.

## Notes

- `size` is a `long`: files over 32767 bytes overflow a 16-bit `int`.
- The `C` piece takes *two* acks (header, then data+NUL), not three; a
  third ack lets the source run a piece ahead and close early.
- The `D`/`E` walk keeps the destination path in `curpath` and a depth
  count, mirroring the source's own recursion.

## Deferred

`-p` time preservation (the source's `T` nanoseconds, ignored here) and
the stderr back-channel (the request's first field, kept `0`).  Nothing
here sends a password, and nothing keeps the connection for anything but
the one copy.
