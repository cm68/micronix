;
; dup system call
;
; dup(fd)
;
; Takes a file descriptor previously returned by open,
; creat, or pipe and allocates a new descriptor synonymous
; with the original. Subsequent reads or writes with the
; new descriptor will have exactly the same effect as the
; same call with the old descriptor.
;
; Since the algorithm returns the lowest available value,
; combinations of dup and close can be used to move file
; descriptors. This is used mostly for manipulating stdin
; (fd 0) and stdout (fd 1).
;
; returns -1 on error, else new file descriptor
;
	.extern _errno
	.extern fdcpy
	.global _dup

	.text
_dup:
	ld 	a,l
	ld 	(fd),a		; save old fd for fdcpy

	ld 	h,0		; fd in hl
	rst 	08h
	.db 	029h
	jr 	c,err		; new fd in hl
	ld 	a,(fd)
	jp 	fdcpy		; _fdpos[new] = _fdpos[old], new fd in hl
err:	ld 	(_errno),hl
	ld 	hl,-1
	ret

	.data
fd:	.db 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
