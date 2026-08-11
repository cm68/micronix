;
; link system call
;
; link(old, new)
; char *old, *new;
;
; A link to "old" is created, with the name "new".
; Either name may be an arbitrary pathname. "New" must
; not already exist, its directory must be writable,
; and it must be on the same device as "old". "Old"
; must not be a directory (unless the user is the
; super-user), and must not have more than 254 links.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _link

	.text
_link:
	pop 	de		; ret addr
	pop 	hl		; old
	ld 	(old),hl
	pop 	hl		; new
	ld 	(new),hl

	ld	hl,-4
	add	hl,sp
	ld	sp,hl

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
	.db 	009h
old:	.dw 	0
new:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
