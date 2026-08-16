;
; exit - run cleanup handlers, then terminate with status.
;
; Two entries:
;   _exit  - normal C callers; status arrives in hl.
;   sexit  - startup (crt0) jumps here with [status] on the stack,
;            keeping its no-return jp style without a fake
;            return-address slot.
;
; Status is parked in BC across __cleanup (callee-saved in both
; hitech and ccc), then handed to __exit in HL the way any first
; argument travels.
;
	.extern	__cleanup, __exit
	.global	_exit, sexit

	.text
_exit:
	push	hl		; status, into the shape sexit expects
sexit:
	pop	bc		; status
	call	__cleanup
	ld	l,c
	ld	h,b		; status, in hl for __exit
	call	__exit		; terminates; never returns

; vim: tabstop=8 shiftwidth=8 noexpandtab:
