;
; select system call
;
; select(mask)
;
; mask is a 16-bit set of open file descriptor numbers, in HL.  The
; call returns the subset that can be read, in HL, or -1 with errno
; set.  NOPEN is 16, so one word is the whole open-file table; there is
; no timeout and no write set (use alarm(2) for a deadline).
;
; The kernel takes the mask from u.hl and returns the answer the same
; way, so this stub only has to say which call it is and to turn the
; carry into errno, exactly as jobctl.s does.
;
	.extern _errno
	.global _select

	.text
_select:
	rst 	08h
	.db 	000h
	.dw 	scall
	ret 	nc		; success: HL is the readable mask
	ld 	(_errno),hl	; failure: HL is the reason
	ld 	hl,-1
	ret

	.data
scall:	.db 	0cfh
	.db 	03ah		; 58 = select

; vim: tabstop=8 shiftwidth=8 noexpandtab:
