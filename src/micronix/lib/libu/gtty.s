;
; gtty system call
;
; gtty(fd, buf)
; struct sgtty *buf;
;
; Gets the status of the terminal associated with the file
; descriptor, and writes the status into the 6-byte structure
; pointed at by buf.
;
; struct sgtty {
;   char ispeed;   /* input speed */
;   char ospeed;   /* output speed */
;   char erase;    /* erase character */
;   char kill;     /* kill character */
;   int  mode;     /* terminal mode */
; };
;
; passes fd in hl
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _gtty

	.text
_gtty:
	pop 	hl		; ret addr
	pop 	hl		; fd in l (byte arg: high byte is junk)
	ld 	a,l
	pop 	hl		; buf
	ld 	(buf),hl

	ld	hl,-6
	add	hl,sp
	ld	sp,hl

	ld 	l,a		; fd in hl
	ld 	h,0
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
	.db 	020h
buf:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
