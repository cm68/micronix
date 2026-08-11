;
; pipe system call
;
; pipe(fds)
; int fds[2];
;
; Returns two file descriptors that can be used to
; communicate between processes created by subsequent fork
; calls. When the pipe is written using fds[1], up to 4096
; bytes of data will be buffered before the writing process
; is suspended. A read using fds[0] will pick up the data.
;
; Read calls on an empty pipe with no writers will return
; an end-of-file (0 bytes read). Write calls under similar
; conditions will generate a signal.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _pipe

	.text
_pipe:
	pop 	hl		; discard ret addr
	pop 	de		; fds pointer

	ld 	hl,-4		; restore stack
	add 	hl,sp
	ld 	sp,hl

	push 	de		; save fds pointer
	rst 	08h
	.db 	02ah

	jr 	c,error

	; returns: hl = read fd, de = write fd
	ex	de,hl		; read fd in de
	ex	(sp),hl		; pointer in hl, write fd to stack

	ld 	(hl),e
	inc 	hl
	ld 	(hl),d
	inc 	hl

	ex	(sp),hl		; pointer to stack, write fd to hl
	ex	de,hl		; write fd to de
	pop	hl		; get pointer

	ld 	(hl),e
	inc 	hl
	ld 	(hl),d

	ld 	hl,0
	ret
error:
	ld 	(_errno),hl
	ld 	hl,-1
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
