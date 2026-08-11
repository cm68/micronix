;
; nice system call
;
; nice(arg)
;
; The "nice" of a process is the opposite of its intuitive
; "priority" - the "nicer" it is, the less cpu time it hogs.
; Sets the nice of the calling process to the given argument.
; Nice values range from -128 to 127; the normal value is 0.
; Only the super-user can set a negative nice (high priority).
;
; A process' nice is passed to its children via fork.
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _nice

	.text
_nice:
	pop 	de		; ret addr
	pop 	hl		; nice value
	push 	hl
	push 	de

	rst 	08h
	.db 	022h
	ex 	de,hl
	ld 	hl,0
	ret 	nc
	ld 	(_errno),de
	dec 	hl
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
