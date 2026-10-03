; signal trampolines
;
; __signal is the system call; _jtab is fifteen six-byte trampolines,
; one per signal, each loading its handler out of _stab and falling
; into the common save-and-call tail.  signal.c owns _stab and hands
; the kernel the address of a trampoline rather than of the handler.
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
						; r_signal() in sys/reg.c.
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

_jtab:
		push 	hl
		ld 		hl,(_stab)
		jr 		sigcall

		push 	hl
		ld 		hl,(_stab+2)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+4)
		jr		sigcall

		push	hl
		ld		hl,(_stab+6)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+8)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+10)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+12)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+14)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+16)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+18)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+20)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+22)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+24)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+26)
		jr 		sigcall

		push	hl
		ld		hl,(_stab+28)
		jr 		sigcall

sigcall:
		push	de
		push	bc
		exx
		push	hl
		push	de
		push	bc
		exx
		ex		af,af'
		push	af
		ex		af,af'
		push	af
		push	ix
		push	iy
		call	sjmp
		pop	 	iy
		pop	 	ix
		pop	 	af
		ex	 	af,af'
		pop	 	af
		ex	 	af,af'
		pop	 	af
		exx
		pop	 	bc
		pop		de
		pop	 	hl
		exx
		pop	 	bc
		pop		de
		pop	 	hl
		ret

sjmp:
		jp		(hl)

; vim: tabstop=4 shiftwidth=4 noexpandtab:
