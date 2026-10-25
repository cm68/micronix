;
; jobctl system call
;
; jobctl(pid, op)
;
; Sets or clears the kernel's two job-control switches for a process.
; The shell owns what a job is; this call is only how it says that a
; process is stopped (JOBSTOP), may run again (JOBCONT), still owns
; terminal input (JOBTTYIN), or has had it taken away (JOBTTYOUT).
;
; The pid is the guest's own.  The sender must own the process (or be
; the super-user) and may not name itself, exactly as for kill.
;
; returns 0 on success, -1 with errno set on failure
;
	.extern _errno
	.global _jobctl

	.text
_jobctl:
	ex 	de,hl		; pid arrives in hl; parked while op comes off
	pop 	hl		; discard return address
	pop 	hl		; op
	ld 	(op),hl

	ld 	hl,-4		; restore the stack: ret + one word
	add 	hl,sp
	ld 	sp,hl

	ex 	de,hl		; get pid into hl
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
	.db 	039h		; 57 = jobctl
op:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
