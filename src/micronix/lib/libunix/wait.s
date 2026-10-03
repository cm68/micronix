;
; wait system call
;
; wait(pstat)
; int *pstat;
;
; Waits for the termination of any of the caller's children.
; If any child has died since the last wait, return is
; immediate. If there are no children, an error is returned.
; If there are several children, several wait calls are
; necessary to learn of all the deaths.
;
; Returns the process ID of the terminated child, and fills
; in the user-supplied integer with the termination status.
; In case of normal termination via exit, the low byte of
; the status is 0, and the high byte is the low byte of the
; child's exit argument.
;
; returns -1 on error, else child pid
;
	.extern _errno
	.global _wait

	.text
_wait:
	push	bc		; the caller's register variable: bc is a
				; home, and pstat lands in it below
	push 	hl		; save pstat, which arrives in hl
	rst 	08h
	.db 	007h
	; returns: hl = pid, de = status
	pop 	bc		; bc = pstat
	jr 	c,error
	ld 	a,b
	or 	c
	jr 	z,9f		; pstat is NULL
	ld 	a,e
	ld 	(bc),a
	inc 	bc
	ld 	a,d
	ld 	(bc),a
9:	pop	bc
	ret
error:
	ld 	(_errno),hl
	ld 	hl,-1
	pop	bc
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
