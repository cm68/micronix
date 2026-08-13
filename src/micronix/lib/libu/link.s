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

;
; Three pops moved sp up by six, so six is what comes back off it and
; the return address is on top again for the ret at the end.
;
; This said -4.  sp came back two bytes high, so the ret took the
; saved "old" pointer for a return address and jumped into it: link
; returned to a filename.  Nothing said link - what it looked like was
; the compiler driver, which uses link and unlink for the rename v6
; does not have, restarting itself in the middle of a -O compile and
; announcing "no input files specified" with the assembler temporary
; as its program name.  The peephole had run and its output was
; already linked into place; only the unlink after it was missing.
;
	ld	hl,-6
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
