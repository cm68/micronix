# Media changer protocol

Out-of-band control of removable media in the simulator.  A guest
process — or a shell, through `/dev/mediachg` — asks the sim to bind a
drive to a medium, position that medium, and inquire what is there.
There is no eject system call and no in-band way for the guest to say
"done with disk 5, load disk 6", so the change is made from outside the
machine.  The guest sees it only as the drive going *not ready* and then
*ready* again, which is what a real install drove.

## The abstraction

The changer is a function of the *simulator*, not of a controller.  What
it manipulates is a **media set** — an ordered list of volumes — which
the sim mounts on a drive.  Usually the set holds one volume and the
whole thing is "which image file is in the drive", but the set is what
makes `next` mean anything: the changer remembers where in a set a drive
is, not merely which file it opened.

A drive is named by the card that offers it and a logical unit on that
card: `djdma:0` .. `djdma:7` (the DJ-DMA has eight), `hdca:0` ..,
`disk1:0` .. (four per instance).  A card offers its drives to the
changer through `hwsim/media.h`; the changer decides what each one holds.

The card owns the image — it is what reads it — so the changer never
opens a file behind the card's back: it says "unit N now holds this" and
the card opens it.

## The two halves

- **Guest-visible.** An unbound drive reports *not ready* through the
  controller's `sense drive status` (SB1_NORDY / SB3_RDY).  Nothing here
  needs a new syscall: the guest reads, gets "not ready", prompts, and
  retries until the next disk appears.
- **Out-of-band.** A one-line text command on S-100 port `0xd2`.  The sim
  buffers the line, runs it against the media sets, and answers on the
  same port.

## Transport

Port `0xd2`, in the 0xd0 page the simulator keeps for its own ports: 0xd0
is the kernel's control port and 0xd1 its program name (hwsim/d1/mpz80.c),
and this is the third.  A write emits the command a byte at a time
(`out 0xd2`); a read takes the response a byte at a time (`in 0xd2`).  The
device node is

    c 3 210  mediachg

major 3 is the `io` device (`iorw`, `sys/memdev.c`); a nonzero minor is a
fixed port — 210 is 0xd2 — so `/dev/mediachg` needs no seek: writing goes
straight to port 0xd2.

## Framing

    frame := '@' command '\n'

`@` (0x40) is the leadin, `\n` (0x0a) the terminator, and a command is at
most 80 bytes.  `@` never appears in a command (see the grammar), so it is
the resync point for both directions.  A response is one `\n`-terminated
line, at most 512 bytes — a drive list is not 80.

## Grammar

    command  := bind | query | next | sets | drives | dir | ls
    bind     := drive source
    query    := drive
    next     := "next"  drive
    sets     := "sets"
    drives   := "drives"
    dir      := "dir" [ path ]        ; report the media directory, or set it
    ls       := "ls"  [ path ]        ; the names in it

    source   := path | "blank:" name | set ":" vol | "empty"
    drive    := card ":" number       ; 0-based logical unit on that card
    set      := name
    vol      := number                ; 0-based volume within the set
    card     := name
    name     := [A-Za-z0-9._-]+
    number   := [0-9]+

A drive always contains `:` and no verb does, so the two forms never
collide: `djdma:0` alone is *what is in it*, and `djdma:0 <source>` is
*put this in it*.  A `blank:` name and a `set:vol` both contain `:`, which
is why they are told apart by their prefix and not by position.

The old spellings still work, each being one of the forms above said the
long way: `load S onto D`, `unload D`, `eject D`, `query D`, `query set`.

## Semantics

- `drive source` — what the drive holds from now on, ejecting whatever is
  there.  The change is applied at once; the guest is expected to do it
  between I/O, not during a transfer.
  - `path` — an image that must already be there.  A path without a
    leading `/` is taken relative to the media directory, and so is each
    volume of a set when `set:vol` names one.
  - `blank:name` — a **fresh** image at that name: the file is unlinked
    first, so a blank that turns out to hold last week's volume is not a
    trap.  This is the one source that does not have to exist yet, and it
    is what makes "create a distribution set" a loop of bind-blank,
    format, mount, copy, umount.
  - `set:vol` — one volume of a named media set.
  - `empty` — eject; the drive reports not ready.  Emptying an
    already-empty drive is `ok`.
- `drive` — what is loaded there now: `ok set:vol`, `ok path`, or `ok
  empty`.
- `next drive` — advance to the following volume of the set the drive
  currently has loaded (the "I'm done with disk N" case, without the
  guest knowing the count).
- `sets` — the media sets and their volume counts.
- `drives` — every drive the simulator can address, and what is in it.
  The spelling is the command's own, so the answer reads back as
  commands.
- `dir [path]` — report the media directory, or set it.  It is where a
  relative path is resolved from, and it answers `ok (none)` when unset.
- `ls [path]` — the names in the media directory, or in `path`.

## Responses

    response   := ok-line | error-line
    ok-line    := "ok" [ ' ' data ] '\n'
    error-line := "error" ' ' code '\n'
    data       := number | set ":" vol | path | "empty" | name list

| command | response |
|---|---|
| bind / `next` (success) | `ok` |
| `sets` | `ok dist8:9 dist5:25`, or `ok none` |
| `drives` | `ok djdma:0=dist8:0 djdma:1=empty ...` |
| `dir` | `ok /home/curt/src/micronix/disks` or `ok (none)` |
| `ls` | `ok d8ss.img d5ds.img ...` |
| `drive` | `ok dist8:2`, `ok d8ss.img` or `ok empty` |
| failure | `error <code>` |

Codes: `syntax` (malformed, unknown verb, too long), `noset`, `novol`
(out of range, or `next` past the last volume), `nodrive`, `nomedia`
(no set bound to the drive `next` names), `nodir` (the directory will not
open), `ioerror` (the image won't open).

## State machine

`@` is mandatory; a command without it is discarded.

    reset = '@', eol = '\n', max = 80
    buf[80], n, staged response

    state := idle | collecting

    idle:       b == '@'  -> collecting (n = 0, staged = none)
                else      -> discard

    collecting: b == '@'  -> n = 0, staged = none      ; abort + restart
                b == '\n' -> execute buf; stage response; -> idle
                n == max  -> stage "error syntax"; -> idle
                else      -> buf[n++] = b

The leadin is the single sync point: it discards a half-sent command and
any unread staged response, returning the parser to idle.  A guest that
segfaults mid-command is healed by the next `@`.  Overflow (the 81st byte
before a terminator) answers `error syntax` and returns to idle rather
than growing without bound.

## Media sets

Top-level in the machine file, a library shared by every profile:

    "mediasets": {
        "dist8": { "volumes": [ "dist/mx2.0-8-1.IMD", ... ] },
        "dist5": { "volumes": [ "dist/mx2.0-5-1.IMD", ... ] }
    }

Volume 0 is the first path; `dist8:0` is `mx2.0-8-1.IMD`.

The simulator does not read the machine file — the provisioning tool turns
the `mediasets` block into a manifest and hands it to the sim with `-e`,
one line per set, name then image paths:

    dist8 /home/curt/src/micronix/dist/mx2.0-8-1.IMD ... mx2.0-8-9.IMD
    dist5 /home/curt/src/micronix/dist/mx2.0-5-1.IMD ... mx2.0-5-25.IMD

`dist/distsets` prints this manifest from the images `create_dist` built,
so a rebuilt set needs nothing edited.

## The guest side

- **Write, by hand or in a script** — `echo` supplies the terminator:

      echo "@djdma:2 empty" >/dev/mediachg
      echo "@djdma:0 dist8:0" >/dev/mediachg
      echo "@next djdma:0" >/dev/mediachg

  Fire-and-forget is enough for a bind: a wrong disk fails loudly in-band
  on the next guest read.
- **Read** — the inquiries are the ones whose answers matter.  A small
  `chg` command writes the line and reads the response to `\n`, printing
  it and exiting 0/1:

      chg djdma:0                 ok dist8:0
      chg drives                  ok djdma:0=dist8:0 djdma:1=empty ...
      chg dir /usr/dist           ok

  `chg` is a door onto the port, not a parser of its own: the command it
  sends is the changer's grammar, passed through as typed.
- **In the sim's log** — every command and its response is also written
  to the sim's own output, so a `chg`-less guest using the fire-and-forget
  spelling is not silent about a refusal, and the log reads as a
  transcript:

      media: djdma:2 dist:1 -> ok
      media: next djdma:2 -> ok
      media: djdma:3 dist:9 -> error novol

## Gotchas

- A command without its `@` is silently ignored (that is what makes
  crashes self-healing).
- `echo -n` leaves the command buffered, never run — it needs the `\n`.
- Volumes and drives are 0-based.
- Reading the node with `cp` never ends: `iorw` loops `while (u.count)`
  and has no end-of-file, so a `cp /dev/mediachg` fills the whole request
  with zeroes once the response runs out.  Use `chg`, or `dd` a byte at a
  time.

## Example

    for v in 0 1 2 3 4 5 6 7 8; do
        chg djdma:0 dist2.0:$v
        install-from djdma:0
    done
    chg djdma:0 empty

Building a distribution set is the same loop with a blank at the front:

    for i in 1 2 3 4; do
        chg djdma:0 blank:dist4-$i.img
        fdj format djdma:0
        mount /dev/fdj0 /mnt
        cptree /usr/src /mnt/src
        umount /mnt
    done
