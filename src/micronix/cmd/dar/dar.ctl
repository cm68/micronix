; names for /bin/dar off the Micronix 1.67 patch, for src/tools/disas
;
;	disas -f dar.ctl dar.dist
;
; There is no source for dar.  The names below are read out of the code
; itself - see cmd/dar/README and, as it takes shape, dar.c, which is the
; reconstruction this file is written against.
;
; It is Whitesmith's C, laid out like the other Micronix binaries: c.ent
; and c.ret at the top of the library, the register cells r1/r2/r3 in
; fixed data cells, and the syscall stubs in the data segment.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a 0
; or ends in h, so every address here is written 0x....
;
start 0x0100

; ---------------------------------------------------------------------
; Whitesmith's runtime, at the top of the library (text is 0x0100..0x85fb).
; c.ent2 saves r1/r2/r3 into the frame and c.ret2 puts them back; the
; three cells are in the data segment.  c.switch is not named yet.

define c.ent 0x8442
define c.ret 0x8449
define c.ent2 0x844d
define c.ret2 0x8463

define r1 0xa305
define r2 0xa307
define r3 0xa309

; ---------------------------------------------------------------------
; dar's own text.  main is what the startup at 0x0100 calls; everything
; below it in this file is still to be named.

define main 0x0226

; The option scan.  main hands argc/argv to doargs, which walks the
; operation letters through c.switch and the table at 0x0261 - each
; letter dispatches to its handler:
;	c -> doCopy		d -> doDelete	p -> doPut
;	r -> doRename		t -> doTouch	x -> doExtract
;	v -> doVerbose		'-' -> optDash	'?' -> usage
define doargs 0x0110
define optscan 0x0289
define doCopy 0x02e8		; c - copy FROM the DOS diskette
define doDelete 0x02ff		; d
define doPut 0x030c		; p - copy TO the DOS diskette
define doRename 0x0319		; r
define doTouch 0x0326		; t
define doExtract 0x0333		; x
define doVerbose 0x0340		; v
define optDash 0x034d		; -
define usage 0x0356		; ?
define optDefault 0x0350

define exit 0x5e3b		; the C exit, called with 1 after main's work

; The driver.  main runs doargs, then execute, then exit.  execute opens
; the device (H1106, taking the devname) and runs the chosen operation
; (H1214), which reads the option-mode byte H8bdb and does the copy,
; delete or display.  Still unnamed: H10a2, H3a65, H38d2, H47ee and the
; body of each handler.
define execute 0x03ed
define devname 0x8bd9		; the device argument (a pointer)
define badopt 0x3a65		; optDefault falls here; bad option or no args
define mkdevname 0x1106		; builds "/dev/<name>" out of the 0x10f6 literal

; ---------------------------------------------------------------------
; The message printer.  say(s) is how every prompt and diagnostic is
; emitted (35 call sites); atmost is the "at most one of d,r,t,x" error.

define atmost 0x0390		; prints s.atmost and exits
define say 0x03c1		; print one string (delegates to H561d)

; ---------------------------------------------------------------------
; The DOS-side routines, each found by the string it emits:
;   getdiskname - "Enter disk name:"        format - ": Unknown format"
;   mkfile      - ": Can't allocate directory entry" and ": Volume full"
;   space       - "K free space left" / "% full," (the -v report)

define getdiskname 0x16aa
define mkfile 0x21d9		; the big one - allocate dir entry + clusters
define space 0x355f
define format 0x38d2

; The sector I/O.  seeksec(fd, sector) multiplies the sector by 512
; (H8142, a shift-left-by-N) and seeks; readboot seek+reads the boot
; sector to learn the geometry (the 4 bytes at H87b1..H87b4).

define seeksec 0x1330
define readboot 0x13bf

; The sector read/write primitives.  readsector does read(fd, buf, 0x200)
; and writesector write(fd, buf, 0x200); the buffers are in the data
; segment at 0x9ff3 (write) and 0xa018/0xa01e (read).

define readsector 0x56d1
define writesector 0x588a

; The string layer above them.  getstr pulls bytes out of the 0xa018
; sector buffer (refilling with readsector when the 512-byte counter
; runs dry) and copies until 0xff or 0x0a, then null-terminates; putstr
; is its inverse.  These are file-content access, not sector access.

define getstr 0x53f3
define putstr 0x55b1

; The menu and the FAT/directory layer.  menu (entered from doargs when
; no operation flag was given) gets the disk name, reads the directory,
; and runs operations in a loop.  readfat/writefat sit on the string and
; sector layers and are the sector-number -> FAT/directory access
; (tentative: which of FAT vs directory is theirs is still to be read).

define menu 0x0ab8
define readfat 0x0cb4
define writefat 0x47ee

; The output layer.  say (above) is the message printer; printf is the
; formatted writer it and the prompts run through (calls write twice).

define printf 0x744b

; The two copy operations (tentative - the names come from the call
; pattern, not the strings).  doput is the copy-TO-DOS operation: it
; calls mkfile to make the DOS file, then read + close in a loop.
; docopy is the copy-FROM-DOS operation: read + close, no mkfile.

define doput 0x27b8
define docopy 0x2b74

; ---------------------------------------------------------------------
; The option flags, one 16-bit cell per operation.  Each handler sets
; its cell to 1 (the ADD A,A / SBC A,A pairs the high byte to 0).  The
; "at most one of d, r, t, x" check is the mutual-exclusion of four of
; these; doVerbose is the v flag, optDash sets nothing.

define copyflag 0x87bf		; c
define delflag 0x87c1		; d
define putflag 0x87c3		; p
define renflag 0x87c5		; r
define touchflag 0x87c7		; t
define xflag 0x87c9		; x
define verboseflag 0x87cb	; v

; ---------------------------------------------------------------------
; The syscall stubs, in the data segment, each the two bytes "cf <n>" -
; RST 8 and the call number - reached by the indirect trap form.  close,
; exit and the like trap directly and are named by disas itself.

define _access 0xa2ab		; 33
define _sbrk 0xa2b1		; 17
define _creat 0xa2b5		; 8
define _fstat 0xa2bb		; 28
define _gtty 0xa2bf		; 32
define _link 0xa2c3		; 9
define _mknod 0xa2c9		; 14
define _open 0xa2d1		; 5
define _read 0xa2d7		; 3
define _seek 0xa2dd		; 19
define _stat 0xa2e3		; 18
define _unlink 0xa2e9		; 10
define _write 0xa2ed		; 4

; ---------------------------------------------------------------------
; The syscall wrappers, in the library at 0x7667..0x7853 - each does the
; indirect trap "cf 00 <stub addr>" to the matching stub above.  The
; entry a CALL reaches is the prologue, not the SYS: the multi-argument
; ones move their args into fixed cells first, so read() is entered at
; 0x7776 (its SYS is 0x7794) and seek() at 0x77a2 (SYS 0x77c0).  Named
; here at the CALL address, which is what the reconstruction calls.

define access 0x7667
define sbrk 0x767b
define creat 0x76ad
define fstat 0x76d4
define gtty 0x76f4
define link 0x7717
define mknod 0x7745
define open 0x7768
define read 0x7776
define seek 0x77a2
define stat 0x77e3
define unlink 0x781a
define write 0x7827
define close 0x7688

; ---------------------------------------------------------------------
; dar's string literals, in the text segment (Whitesmith's puts them
; there).  Each marks the function that emits it - the next round of
; naming starts from these.

define s.atmost 0x0363		; "At most one of d, r, t, x may be specified."
define s.notdos 0x03d8		; "Not a DOS diskette!"
define s.dupfile 0x07ec		; ": duplicate file name"
define s.filedel 0x09da		; "File to delete: "
define s.filecopy 0x09ec	; "File to copy (or RETURN for all files): "
define s.menucopy 0x0a41	; "A) Copy from the DOS diskette"
define s.menuput 0x0a60		; "B) Copy to the DOS diskette"
define s.menudisp 0x0a7d	; "C) Display file on the DOS diskette"
define s.selection 0x0aac	; "Selection: "
define s.diskname 0x166e	; "Enter disk name: "
define s.insert 0x1680		; "Insert diskette and close the drive door"
define s.volfull 0x21a4		; ": Volume full"
define s.dirent 0x21b3		; ": Can't allocate directory entry"
define s.nospace 0x2775		; "Not enough space left on volume"
define s.kfree 0x3539		; "K free space left"
define s.pctfull 0x354c		; "% full, "
define s.diskette 0x3555	; "Diskette "
define s.unkfmt 0x38b3		; ": Unknown format"
define s.devsize 0x38c5		; "Device size "
define s.example 0x3a2a		; "Example: dar mfa xv"
define s.usage 0x3a3f		; "usage: dar device {cdprtx} files ..."
define s.notdos2 0x47d9		; "Not a DOS diskette."

; ---------------------------------------------------------------------
; library, to be named from what dar calls it for

define strcmp 0x6e72		; the char-by-char compare mkfile uses for "." and "/"

; ---------------------------------------------------------------------
; The data structures, read apart:
;   FAT12.  readfat (0x36b0) masks a cluster entry with AND 0x0f - the
;     FAT is the 12-bit one, entry n at byte n*3/2 in the FAT buffer.
;   0x8bdc - the FAT buffer (indexed by readfat).
;   0x9e44 - the formats table, 34-byte entries of known DOS diskette
;     geometries; format (0x38d2) matches one and leaves its pointer in
;     0x9e42.  Offset 12 of the matched entry is the total cluster count
;     that space (0x355f) walks.
;   0xa018 - the sector buffer getsec reads into.
;   0x8bd7 / 0x8609 - a table of 2-byte pointers into the directory,
;     and its index; the duplicate-name check (0x0803) walks it.
;   The directory entries are the standard DOS 32-byte layout: name 0-7,
;     ext 8-10, attribute 11, time 22-23, date 24-25, first cluster
;     26-27, size 28-31.

; ---------------------------------------------------------------------
; dar's data segment (0x85fb..0xa30b), still to be read apart
