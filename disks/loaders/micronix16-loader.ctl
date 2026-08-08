;
; names for the 1.6 second level boot, for src/tools/disas
;
;	disas -f micronix16-loader.ctl -a 0x100 micronix16-loader.bin
;
; Whitesmith's C.  Every function opens "call cent", which makes DE the
; frame pointer and stacks the old one, and leaves by "jp cret", which
; puts SP back from DE; arguments are at DE+4 upward and the return value
; comes back in BC.  Reading it is much easier once you know that DE is
; never a value, it is always the frame.
;
; Names below are what the code shows it does.  Where the evidence is a
; string it prints or a port it touches, the name is certain; where it is
; only the shape of the code, the name says addr and you should read it
; before believing it.
;
start 0100

; the runtime
define crt0 0100		; ld sp,0100 / call main / jp 0
define exit 0106		; jp 0, back to the rom monitor
define cent 0a35		; c.ent - de becomes the frame pointer
define jphl 0a3b		; jp (hl), the tail of cent, also called directly
define cret 0a3c		; c.ret - sp back from de, pop de, ret
define arith0a02 0a02		; called with three stacked args by readblk
define shift08b4 08b4		; 16 iterations of add a,a - printf's converter

; talking to the hardware.  inp and outp assemble three bytes onto the
; stack - nop / in a,(port) / ret, nop / out (port),a / ret - and call
; them there, which is how a C program reaches an I/O port with no asm.
define inp 088b
define outp 08a1
define djpulse 0109		; outp(0ef, 0), which starts the channel
define djcmd 011c		; build a channel program and pulse it
define djwait 0228		; poll the status byte down a 16 bit timeout
define djinit 0285		; banner, then check the drive answers
define dj02f2 02f2
define djsense 043f
define fmterror 0483		; prints "Format?" and exits

; the console
define putchar 07cb
define printf 07fd		; the biggest routine here; everything prints through it
define getchar 0caa
define gets 0cd7		; line input, echoes backspace-space-backspace

; the filesystem, and what this loader exists to do
define main 0a67
define fs0a7c 0a7c		; main calls it before listing anything
define lsfiles 0ab0		; prints "Files:" and the names
define pickfile 0b5a		; "File to boot: ", "File not found", "No bootable files"
define bootkern 06b5		; "Loading", then "Entering", then into the kernel
define readblk 0be4		; block 0 reads as nothing; addresses on blk-1
define cmp0c5a 0c5a		; no calls, used by pickfile - a name compare

; low memory the controller and this code share
define djmbox 0050		; command byte the boot block also writes
define djchan 0051		; channel program address
define djstat 0053
define timeout 0d9e		; djwait counts this down
define dj0db1 0db1		; readblk hands this to arith0a02 and then to djcmd

; the block buffer, 0494 to 0693, exactly 512 bytes.  fs0a7c reads the
; superblock into it and lsfiles walks it from dirbuf to dirend, which is
; how you can tell it is a directory being listed and not a table.
define dirbuf 0494
define dirend 0694
define dirptr 06a0
define ptr069e 069e
