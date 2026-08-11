;
; fork system call
;
; fork()
;
; Fork is the only way to create a new process. The calling
; process splits into a "parent" and a "child". The child's
; core image is a copy of the parent's, open files are
; shared, and signals remain unchanged.
;
; returns 0 to child, child pid to parent, -1 on error
;
	.extern _errno
	.global _fork

	.text
_fork:
	rst 	08h
	.db 	002h
;
; The child comes back here; the parent comes back three bytes later,
; which is the whole of how fork returns twice.  Whatever sits here
; therefore has to be exactly three bytes long.
;
; Written as "jp child" it was not: asz shortens a jump that is in
; reach to a two byte "jr", so the parent landed one byte early, on
; the "ld (_errno),hl" of the error path.  It stored its child's pid
; into errno and returned -1, and every fork in the system looked as
; though it had failed - the driver forked cpp, cpp ran and exited,
; and the driver reported "fork" and gave up.
;
; Spelling the jump out as bytes is what keeps it three of them.  Do
; not put a mnemonic back here.
;
	.db 	0c3h		; jp child
	.dw 	child
	ret 	nc		; parent: pid in hl
	ld 	(_errno),hl
	ld 	hl,-1
	ret
child:
	ld 	hl,0
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
