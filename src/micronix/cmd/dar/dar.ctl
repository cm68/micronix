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
define _link 0xa2c5		; 9
define _mknod 0xa2cd		; 14
define _open 0xa2d1		; 5
define _read 0xa2d9		; 3
define _seek 0xa2df		; 19
define _stat 0xa2e5		; 18
define _unlink 0xa2eb		; 10
define _write 0xa2ef		; 4

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

; ---------------------------------------------------------------------
; dar's data segment (0x85fb..0xa30b), still to be read apart
