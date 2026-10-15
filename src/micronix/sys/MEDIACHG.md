# Media changer protocol

Out-of-band control of removable media in the simulator.  A guest
process — or a shell, through `/dev/mediachg` — asks the sim to load,
unload and query floppy images on the DJ-DMA controller.  There is no
eject system call and no in-band way for the guest to say "done with
disk 5, load disk 6", so the change is made from outside the machine.
The guest sees it only as the drive going *not ready* and then *ready*
again, which is what a real install drove.

## The two halves

- **Guest-visible.** An ejected drive reports *not ready* through the
  controller's `sense drive status` (SB1_NORDY / SB3_RDY).  Nothing here
  needs a new syscall: the guest reads, gets "not ready", prompts, and
  retries until the next disk appears.
- **Out-of-band.** A one-line text command on S-100 port `0xd2`.  The sim
  buffers the line, runs it against the media sets in `sim.json`, and
  answers on the same port.

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
line.

## Grammar

    command  := load | unload | query | next
    load     := "load"  set ":" vol "onto" drive
    unload   := "unload" drive
    query    := "query" ( set | drive )
    next     := "next"  drive

    set      := name
    vol      := number               ; 0-based volume within the set
    drive    := "djdma" ":" number   ; 0-based logical drive, 0-7
    name     := [A-Za-z0-9._-]+
    number   := [0-9]+

`eject` is accepted as a synonym for `unload`.

## Semantics

- `load set:vol onto drive` — put volume `vol` of set `set` into `drive`,
  ejecting whatever is there.  The change is applied at once; the guest
  is expected to do it between I/O, not during a transfer.
- `unload drive` / `eject drive` — eject; the drive reports not ready.
  Unloading an already-empty drive is `ok`.
- `query set` — how many volumes the set has.
- `query drive` — what is loaded there now.
- `next drive` — advance to the following volume of the set the drive
  currently has loaded (the "I'm done with disk N" case, without the
  guest knowing the count).

## Responses

    response   := ok-line | error-line
    ok-line    := "ok" [ ' ' data ] '\n'
    error-line := "error" ' ' code '\n'
    data       := number | set ":" vol | "empty"

| command | response |
|---|---|
| `load` / `unload` / `next` (success) | `ok` |
| `query set` | `ok 9` |
| `query drive` | `ok dist2.0:2` or `ok empty` |
| failure | `error <code>` |

Codes: `syntax` (malformed, unknown verb, too long), `noset`, `novol`
(out of range, or `next` past the last volume), `nodrive`, `nomedia`
(no set loaded on the drive `next` names), `ioerror` (the image won't
open).

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

## Media sets (`sim.json`)

Top-level, a library shared by every sim:

    "mediasets": {
        "dist8": { "volumes": [ "dist/mx2.0-8-1.IMD", ... ] },
        "dist5": { "volumes": [ "dist/mx2.0-5-1.IMD", ... ] }
    }

Volume 0 is the first path; `dist8:0` is `mx2.0-8-1.IMD`.

The simulator does not read `sim.json` — the provisioning tool turns the
`mediasets` block into a manifest and hands it to the sim with `-e`, one
line per set, name then image paths:

    dist8 /home/curt/src/micronix/dist/mx2.0-8-1.IMD ... mx2.0-8-9.IMD
    dist5 /home/curt/src/micronix/dist/mx2.0-5-1.IMD ... mx2.0-5-25.IMD

`dist/distsets` prints this manifest from the images `create_dist` built,
so a rebuilt set needs nothing edited.

## The guest side

- **Write, by hand or in a script** — `echo` supplies the terminator:

      echo "@eject djdma:2" >/dev/mediachg
      echo "@load dist2.0:0 onto djdma:0" >/dev/mediachg

  Fire-and-forget is enough for `load`/`eject`/`unload`: a wrong disk
  fails loudly in-band on the next guest read.
- **Read** — `query` is the one verb whose answer matters.  A small `chg`
  command writes the line and reads the response to `\n`, printing it and
  exiting 0/1; the sim also tees each response into its log so a `-L`
  tail shows `ok 9` without reading the node.

## Gotchas

- A command without its `@` is silently ignored (that is what makes
  crashes self-healing).
- `echo -n` leaves the command buffered, never run — it needs the `\n`.
- Volumes are 0-based; drives are 0-based.

## Example

    for v in 0 1 2 3 4 5 6 7 8; do
        chg load dist2.0:$v onto djdma:0
        install-from djdma:0
    done
    chg eject djdma:0
