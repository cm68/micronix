;
; assembly source for alarm system call
;
; /usr/src/lib/libu/alarm.s
;
; Changed: <2023-07-07 00:50:13 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;
	.globl	_alarm
	.extern _errno

	.text
_alarm:
	ld	hl, 0x2
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	.db	0xcf, 0x00
	.dw	sys
	ld	bc, 0x0
	ret

	.data
sys:	.db	0xcf, 0x1b
