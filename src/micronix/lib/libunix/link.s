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
	ld 	(old),hl	; the first argument arrives in hl
	pop 	de		; ret addr
	pop 	hl		; new
	ld 	(new),hl

;
; Two pops moved sp up by four, so four is what comes back off it and
; the return address is on top again for the ret at the end.
;
; An older body said -4 where it owed -6.  sp came back two bytes
; high, so the ret took the saved "old" pointer for a return address
; and jumped into it: link returned to a filename.  Nothing said link
; - what it looked like was the compiler driver, which uses link and
; unlink for the rename v6 does not have, restarting itself in the
; middle of a -O compile and announcing "no input files specified"
; with the assembler temporary as its program name.  The count and
; the pops move together; mind them both.
;
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
