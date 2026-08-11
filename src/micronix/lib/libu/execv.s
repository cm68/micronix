;
; execv and execl - wrappers for exec
;
; execv(name, argv)
; char *name;
; char *argv[];
;
; execl(name, arg0, arg1, ..., argn, 0)
; char *name, *arg0, *arg1, ..., *argn;
;
; Execv is useful when the number of arguments is not known
; in advance. Pointers to the argument strings are collected
; into a list, a null pointer is appended to mark the end,
; and execv is called with the address of the list.
;
; Execl is useful when a known file is being executed with
; known arguments. Any number of arguments may be given,
; but the last must be a 0.
;
	.extern _exec
	.global _execv
	.global _execl

	.text
; bc is a register-variable home and the shuffle below pops an
; argument into it, so the caller's copy is saved before anything
; touches it and the arguments are read where they lie.

_execv:
	push	bc		; the caller's register variable
	ld 	hl,4
	add 	hl,sp		; past the save and the return address
	ld 	e,(hl)
	inc 	hl
	ld 	d,(hl)		; de = name
	inc 	hl
	ld 	c,(hl)
	inc 	hl
	ld 	b,(hl)		; bc = argv

	push 	bc		; push argv
	push 	de		; push name
	call 	_exec
	pop 	af
	pop 	af
	pop	bc
	ret

_execl:
	push	bc		; the caller's register variable
	ld 	hl,4
	add 	hl,sp		; past the save and the return address
	ld 	e,(hl)
	inc 	hl
	ld 	d,(hl)		; de = name
	inc 	hl		; hl = &arg0, the rest of the list

	push 	hl		; push &arg0 as argv
	push 	de		; push name
	call 	_exec
	pop 	af
	pop 	af
	pop	bc
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
