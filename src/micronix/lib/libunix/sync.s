;
; sync system call
;
; sync()
;
; Causes all information in core memory that should be on
; disk to be written out. This includes modified super-blocks,
; modified inodes, and delayed block I/O.
;
; always succeeds
;
	.global _sync

	.text
_sync:
	rst 	08h
	.db 	024h
	ld 	hl,0
	ret

; vim: tabstop=8 shiftwidth=8 noexpandtab:
