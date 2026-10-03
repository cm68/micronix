;
; raw seek system call
;
; seekraw(fd, offset, whence)
;
; this is the bare kernel call.  lseek and the seek()
; wrapper in seek.c are the front doors; user code should
; go through one of them, since they know how a 32-bit
; position is split across the two argument forms.
;
; Moves the read/write pointer of an open file:
;   whence 0: set to offset (from beginning)
;   whence 1: set to current + offset
;   whence 2: set to end + offset
;   whence 3,4,5: same as 0,1,2 but offset * 512
;
; Seeks are not allowed on pipes, but are allowed on
; character devices (though most ignore them). Seeking
; past end of file and writing creates a hole.
;
; passes fd in hl
;
; returns the new position, or -1 on failure
;
	.extern _errno
	.global _seekraw

	.text
_seekraw:
	ld 	a,l		; fd arrives in hl
	pop 	hl		; discard ret addr
	pop 	hl		; offset
	ld 	(offset),hl
	pop 	hl		; whence
	ld 	(whence),hl

	ld 	hl,-6		; restore stack
	add 	hl,sp
	ld 	sp,hl

	ld 	l,a		; fd in hl
	ld 	h,0
	rst 	08h
	.db 	000h
	.dw 	scall
	ret 	nc		; kernel: hl' = high word, hl = low word,
				; which is ccc's long already - and DE it
				; never touched, so there is nothing to
				; cross and nothing to give back
	ld 	(_errno),hl
	;
	; A failed long is -1 in both halves.  Setting hl' means swapping
	; the bank in, and the set has to be done on both sides of the swap
	; so that each bank keeps the other registers it came in with.
	;
	ld 	hl,-1
	exx
	ld 	hl,-1
	exx
	ret

	.data
scall:	.db 	0cfh
	.db 	013h
offset:	.dw 	0
whence:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
