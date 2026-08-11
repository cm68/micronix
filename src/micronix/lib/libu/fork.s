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
	jp 	child
	ret 	nc		; parent: pid in hl
	ld 	(_errno),hl
	ld 	hl,-1
	ret
child:
	ld 	hl,0
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
