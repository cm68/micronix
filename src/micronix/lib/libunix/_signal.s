; signal trampolines
;
; __signal is the system call; _jtab is fifteen trampolines, one per
; signal, each loading its handler out of _stab and falling into the
; common save-and-call tail.  signal.c owns _stab and hands the kernel
; the address of a trampoline rather than of the handler.
;
; These declarations were missing.  Without them the file still
; assembles, but every symbol in it is local - so the object defines
; nothing, __signal and _jtab resolve to nobody, and signal() cannot
; be linked at all.  Every other file here declares what it exports.
;
	.global __signal
	.global _jtab

	.extern _errno
	.extern _stab

.text:
__signal:
		ld		(sig),hl	; the signal number is the first
						; argument, and the first argument
						; arrives in hl.  it belongs in the
						; sig slot, which is the word the
						; kernel reads as arg[0] - see
						; signal() in sys/sig.c.
		pop		de		; the return address
		pop		hl		; the handler, the second argument
		ld		(func),hl
		push	hl
		push	de
		rst		08h
		.db		0
		.dw		sys
		ret		nc
		ld		(_errno),hl
		ld		hl,-1
		ret

.data:
sys:	.db	0xcf
		.db 0x30
sig:	.dw	0
func:	.dw	0

.text:

; A handler is C and takes the signal number as its one argument,
; which under ccc travels in hl.  So each trampoline saves the
; interrupted hl and de, parks the handler in de and the number in
; hl, and sigcall jumps through de - push de; ret is the only
; indirect jump the machine has besides jp (hl), and hl is taken.
; A handler is C and takes the signal number as its one argument,
; which under ccc travels in hl.  So each trampoline saves the
; interrupted hl and de, parks the handler in de and the number in
; hl, and sigcall jumps through de - push de; ret is the only
; indirect jump the machine has besides jp (hl), and hl is taken.
;
; The trampolines are not all the same length (the assembler turns a
; jp into a jr when it can reach), so they cannot be indexed by a
; stride.  signal.c reads the addresses out of jtaddr instead.
_jtab:
jt1:		push	hl
		push	de
		ld		de,(_stab)
		ld		hl,1
		jp		sigcall

jt2:		push	hl
		push	de
		ld		de,(_stab+2)
		ld		hl,2
		jp		sigcall

jt3:		push	hl
		push	de
		ld		de,(_stab+4)
		ld		hl,3
		jp		sigcall

jt4:		push	hl
		push	de
		ld		de,(_stab+6)
		ld		hl,4
		jp		sigcall

jt5:		push	hl
		push	de
		ld		de,(_stab+8)
		ld		hl,5
		jp		sigcall

jt6:		push	hl
		push	de
		ld		de,(_stab+10)
		ld		hl,6
		jp		sigcall

jt7:		push	hl
		push	de
		ld		de,(_stab+12)
		ld		hl,7
		jp		sigcall

jt8:		push	hl
		push	de
		ld		de,(_stab+14)
		ld		hl,8
		jp		sigcall

jt9:		push	hl
		push	de
		ld		de,(_stab+16)
		ld		hl,9
		jp		sigcall

jt10:		push	hl
		push	de
		ld		de,(_stab+18)
		ld		hl,10
		jp		sigcall

jt11:		push	hl
		push	de
		ld		de,(_stab+20)
		ld		hl,11
		jp		sigcall

jt12:		push	hl
		push	de
		ld		de,(_stab+22)
		ld		hl,12
		jp		sigcall

jt13:		push	hl
		push	de
		ld		de,(_stab+24)
		ld		hl,13
		jp		sigcall

jt14:		push	hl
		push	de
		ld		de,(_stab+26)
		ld		hl,14
		jp		sigcall

jt15:		push	hl
		push	de
		ld		de,(_stab+28)
		ld		hl,15
		jp		sigcall

; The handler is C and may use any register, so save the rest of the
; machine here.  The pops mirror the pushes exactly: the shadow set
; goes back under exx, the accumulator pair by exchanging twice.  The
; interrupted hl and de were saved by the trampoline; they come back
; after the handler, leaving only the return pc for ret.
sigcall:
		push	af
		push	bc
		exx
		push	hl
		push	de
		push	bc
		exx
		ex		af,af'
		push	af
		ex		af,af'
		push	ix
		push	iy
		ld		bc,sigret
		push	bc
		push	de
		ret

sigret:
		pop		iy
		pop		ix
		ex		af,af'
		pop		af
		ex		af,af'
		exx
		pop		bc
		pop		de
		pop		hl
		exx
		pop		bc
		pop		af
		pop		de
		pop		hl
		ret

.data:
; signal.c indexes this by sig-1
	.global	_jtaddr
_jtaddr:
		.dw		jt1, jt2, jt3, jt4, jt5
		.dw		jt6, jt7, jt8, jt9, jt10
		.dw		jt11, jt12, jt13, jt14, jt15

; vim: tabstop=4 shiftwidth=4 noexpandtab:
