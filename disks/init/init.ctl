; names for /etc/init off the Micronix 1.6 standalone, for src/tools/disas
;
;	disas -f init.ctl init
;
; There is no source for init.  The names below are not guesses either:
; they come from watching it run.  The simulator's syscall trace prints
; the address of every call it makes, so a run of the standalone names
; every library stub in the program by what it did.
;
; It is Whitesmith's C - c.ent at 3f87, c.ret at 3f8e - and the syscall
; stubs live in the data segment, which starts at 3fe2, not in the text.
;
start 0100

define c.ent 3f87
define c.ret 3f8e

; the syscall stubs, named by the call each one was seen to make
define _close 37e8
define _fork 3852
define _sync 3b78
define _time 3b7f
define _wait 3bce
define _sbrk 437d
define _creat 4391
define _exec 4397
define _gtty 439d
define _open 43bb
define _read 43c1
define _seek 43c7
define _signal 43cf
define _stat 43f3
define _write 4405

; stdio, from the shape of the calls and what the syscall trace showed
; underneath them - fopen then fgets is an open, two seeks and a 512
; byte read
define _fopen 2127
define _fgets 2545

; /etc/ttys and the FILE it is read through
define ttysname 05d5
define ttysmode 05d0
define ttysfp 4118
