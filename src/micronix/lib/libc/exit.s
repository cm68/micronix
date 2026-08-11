;
; exit - run cleanup handlers, then terminate with status.
;
; Two entries:
;   _exit  - normal C callers; stack = [ret][status].  The return
;            address is discarded since exit never returns.
;   sexit  - startup (crt0) jumps here with just [status] on the
;            stack, keeping its no-return jp style without a fake
;            return-address slot.
;
; Status is parked in BC across __cleanup (callee-saved in both
; hitech and ccc).  The final call gives __exit the [ret][status]
; stack shape its pop discipline expects.
;
	.extern	__cleanup, __exit
	.global	_exit, sexit

	.text
_exit:
	pop	hl		; discard return address - no return
sexit:
	pop	bc		; status
	call	__cleanup
	push	bc		; status
	call	__exit		; terminates; never returns

; vim: tabstop=8 shiftwidth=8 noexpandtab:
