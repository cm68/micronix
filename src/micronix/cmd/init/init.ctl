; names for /etc/init off the Micronix 1.6 standalone, for src/tools/disas
;
;	disas -f init.ctl init.dist
;
; There is no source for init.  The names below are not guesses either:
; they come from watching it run.  The simulator's syscall trace prints
; the address of every call it makes, so a run of the standalone names
; every library stub in the program by what it did.  The rest were read
; out of the code - see src/micronix/cmd/init/init.c, which is a
; reconstruction from this disassembly and uses the same names.
;
; It is Whitesmith's C - c.ent at 3f87, c.ret at 3f8e - and the syscall
; stubs live in the data segment, which starts at 3fe2, not in the text.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a
; 0 or ends in h, so every address here is written 0x....  An address
; written bare, as an earlier version of this file had them, is parsed as
; decimal and the name lands somewhere else entirely - which is why
; _fgets used to appear in the middle of opentty.
;
start 0x0100

define c.ent 0x3f87
define c.ret 0x3f8e

; the register-saving prologue and epilogue.  Whitesmith's keeps three
; register variables in the fixed cells r1/r2/r3; c.ent2 saves them into
; the frame and c.ret2 puts them back.  c.switch walks a table of
; (target, value) pairs terminated by a zero target and then the default.
define c.ent2 0x3f92
define c.ret2 0x3fa8
define c.switch 0x3f62

define r1 0x441d
define r2 0x441f
define r3 0x4421
define errno 0x440b

; the syscall stubs, named by the call each one was seen to make
define _close 0x37e8
define _fork 0x3852
define _sync 0x3b78
define _time 0x3b7f
define _wait 0x3bce
define _sbrk 0x437d
define _creat 0x4391
define _exec 0x4397
define _gtty 0x439d
define _open 0x43bb
define _read 0x43c1
define _seek 0x43c7
define _signal 0x43cf
define _stat 0x43f3
define _write 0x4405

; stdio, from the shape of the calls and what the syscall trace showed
; underneath them - fopen then fgets is an open, two seeks and a 512
; byte read
define _fopen 0x2127
define _fgets 0x2545

; /etc/ttys and the FILE it is read through
define ttysname 0x05d5
define ttysmode 0x05d0
define ttysfp 0x4118

; ---------------------------------------------------------------------
; init's own text runs 0100..2126; everything above that is library.

define main 0x0156
define multi 0x01aa
define spawn 0x01b9
define startlogin 0x0259
define consmsg 0x0313		; never called
define console 0x0424
define readttys 0x05df
define opentty 0x09a8
define savestr 0x0a45
define exists 0x0a90
define reload 0x0abb		; also the SIGHUP handler
define runrc 0x0af8
define mkempty 0x0b78
define ignoresigs 0x0bab
define catchsigs 0x0bcb
define spawnall 0x0beb
define closeall 0x0c2a
define mainloop 0x0c63
define ttynumber 0x0ca2		; never called
define logout 0x0e12
define bootrecord 0x1001
define findpid 0x1064
define shutdown 0x1102		; the SIGTERM handler
define minishell 0x12ea
define lookup 0x1566
define docmd 0x1600
define mkdirf 0x178a
define dirf 0x18b6
define mknodf 0x199e
define isnumber 0x1af0
define print 0x1b3b
define typef 0x1b63
define cpf 0x1be3
define getword 0x1d05
define speedcode 0x1dbc
define catfile 0x1e2e
define closefds 0x1ea9
define defaultsigs 0x1ed3
define rootpasswd 0x1f13
define resetttys 0x2003

; library, named from the calls and the syscall stubs they reach
define access 0x374f
define chdir 0x3786
define chmod 0x379e
define chown 0x37c1
define close 0x37e4
define dup 0x3817
define execv 0x382b
define _exit 0x384c
define fork 0x3852
define getpid 0x3865
define gtty 0x386a
define kill 0x388a
define link 0x38aa
define mknod 0x38cd
define mount 0x38fb
define open 0x3929
define read 0x394d
define seek 0x3979
define signal 0x39a4
define sleep 0x3b27
define stat 0x3b35
define stty 0x3b58
define sync 0x3b78
define time 0x3b7e
define umount 0x3b9d
define unlink 0x3bb5
define wait 0x3bcd
define write 0x3bea
define fclose 0x23f0
define malloc 0x2b52
define free 0x2c5e
define exit 0x2db7
define setmem 0x2ea0
define atoi 0x2eff
define sequal 0x2f80
define movblock 0x2fcb
define concat 0x302b
define any 0x3090
define equal 0x30dc
define perror 0x335b
define putstr 0x3422
define strlen 0x3464
define creat 0x3497
define exec 0x353c

; init's data segment
define downmsg 0x3fe2
define rootfd 0x3fe4
define swapfd 0x3fe5
define nttys 0x3fe6
define ttys 0x3fe8
define builtins 0x4198

; the two dead functions and the SIGTERM handler are only ever reached
; through a signal or not at all, so nothing branches to them and disas
; cannot find them by tracing.  Force them.
;
; The switch arms used to be here too - thirty more addresses, in
; docmd, mknodf and speedcode - because they are reached only through
; c.switch's tables.  disas recognises that helper by what it is now
; rather than by finding "c.switch" in a symbol table, so it walks the
; tables itself and the arms come out without being named.
code 0x0313
code 0x0ca2
code 0x1102
