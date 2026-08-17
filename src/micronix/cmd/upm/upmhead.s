;
; upm - the hand-written bits
;
; cmd/upm/upmhead.s
;
; The parts of upm that are not C: the crt (start), the CP/M entry
; bridge (entry), the BIOS jump table (bios), the interrupt handler
; (tint) and the CP/M stack.  They are read out of the .dis and
; reproduced here; see README.
;
; The crt is the ordinary one - clear bss, build argc and argv, call
; main - with upm's own exit (cexit) at the end instead of sexit.  The
; layout is the odd thing: the text is linked at d645, so upm's code
; sits above the CP/M TPA a .com is loaded into.
;
	.extern _main
	.extern _cexit
	.extern _cpm
	.extern _wboot
	.extern _badbios
	.extern _recavai
	.global start
	.global _entry
	.global _bios
	.global _tint

	.text
start:
;
; Clear bss before anything else runs.  Nothing else does it, and the
; small handlers read the console counters without ever setting them.
;
	ld	hl,__Lbss
	ld	de,__Hbss
bssclr:
	ld	a,l
	cp	e
	jr	nz,bsszap
	ld	a,h
	cp	d
	jr	z,bssdone
bsszap:
	ld	(hl),0
	inc	hl
	jr	bssclr
bssdone:

	pop	de	; this is argc
	ld	hl,0
	add	hl,sp	; argv
	push	hl	; argv, the second argument
	ex	de,hl	; argc, the first, rides in hl
	call	_main
	call	_cexit

;
; entry - the door a CP/M program comes through.  The function number
; is in c and the argument in de.  Save the micronix stack, switch to
; the CP/M stack, call cpm, and switch back, leaving the result where
; CP/M expects it - the word in hl and the low byte in a.
;
_entry:
	ld	hl,0
	add	hl,sp
	ld	(_endentr),hl	; the micronix stack pointer
	ld	sp,_stack	; the CP/M stack

	ld	h,0
	ld	l,c		; the function number, into hl
	push	de		; the argument, onto the stack
	call	_cpm		; cpm(func, arg); the result in hl
	pop	af		; drop the argument

	ld	c,l		; keep the result through the switch
	ld	b,h
	ld	hl,(_endentr)
	ld	sp,hl		; back to the micronix stack

	ld	l,c		; the result, in hl
	ld	h,b
	ld	a,c		; and its low byte, in a
	ret

_endentr:
	.ds	2		; the saved micronix stack pointer

;
; The CP/M stack.  The label is the top; the stack grows down into the
; space reserved here.  It sits between entry and the BIOS table, not
; after them, so that bios lands on a page boundary above entry: patch
; copies the 51 bytes to (bios & 0xFF00), and with bios right after
; entry that copy lands on entry itself - the BDOS bridge the .com
; reaches through 0x0005 - and clobbers it.
;
	.ds	512
_stack:

;
; bios - the CP/M BIOS jump table, seventeen entries of three bytes.
; entry 0 is the warm boot (cexit), entry 1 the return from one
; (wboot), and the rest are not implemented - badbios.  patch copies
; these 51 bytes to a page boundary and points 0000h at entry 1.
;
_bios:
	jp	_cexit
	jp	_wboot
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios
	jp	_badbios

;
; tint - the interrupt handler.  All it does is set recavai, which is
; how the console layer learns a character is waiting.
;
_tint:
	push	af
	ld	a,1
	ld	(_recavai),a
	pop	af
	ret

	.text
__Htext::	.dw	0
__Ltext::	.dw	0
__Hdata::	.dw	0
__Ldata::	.dw	0
__Hbss::	.dw	0
__Lbss::	.dw	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
