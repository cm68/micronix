;
; assembly source for errno data space
;
; /usr/src/lib/libu/errno.s
;
; Changed: <2023-07-07 00:36:28 curt>
;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;
	.globl	_errno

	.data
	.dw	0
