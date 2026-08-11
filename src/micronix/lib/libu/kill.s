;
; kill system call
;
; kill(pid, sig)
;
; Sends the signal sig to the process with the given ID.
; The usual effect is to kill the process - see signal(2)
; for a discussion and a list of signals.
;
; The sending and receiving processes must have the same
; effective user IDs, or the sender must be the super-user.
; If the given process ID is 0, then the signal is sent to
; all other processes with the same controlling tty.
;
; A process can never kill itself.
;
; passes pid in hl
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _kill

	.text
_kill:
	pop 	hl		; ret addr
	pop 	de		; pid
	pop 	hl		; sig
	ld 	(sig),hl

	ld	hl,-4		; restore stack
	add	hl,sp
	ld	sp,hl
	
	ex	de,hl		; get pid into hl

	rst 	08h
	.db 	000h
	.dw 	scall

	ex 	de,hl		; save errno
	ld 	hl,0
	ret 	nc
	ld 	(_errno),de
	dec 	hl
	ret

	.data
scall:	.db 	0cfh
	.db 	025h
sig:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
