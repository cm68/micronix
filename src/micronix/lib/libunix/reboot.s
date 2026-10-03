;
; reboot system call
;
; reboot(howto)
; int howto;
;
; Halts the machine (RB_HALT), or would reboot it (RB_AUTOBOOT) if
; there were a bootstrap to return to.  The halt drops the cpu in
; kernel mode: the simulators end the run on it, and real hardware
; traps to the monitor.
;
; Restricted to the super-user.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _reboot

	.text
_reboot:
	ld 	(howto),hl		; the argument arrives in hl

	rst 	08h
	.db 	000h
	.dw 	scall
	ex 	de,hl
	ld 	hl,0
	ret 	nc
	ld 	(_errno),de
	dec 	hl
	ret

	.data
scall:	.db 	0cfh
	.db 	037h
howto:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
