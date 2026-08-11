;
; sexit for the boot loader
;
; micronix/stand/sexit.s
;
; crt0 ends with "jp sexit", so something has to answer to that name.
; Left to the library it is exit(), and exit() flushes stdio - which
; brings in fflush, the FILE table, stdin/stdout/stderr and a 512 byte
; buffer, none of which the loader ever touches.  Defining it here means
; the linker is satisfied before it looks in the archive.
;
; main() does not return: it jumps to the kernel it loaded.  The only
; thing that arrives here is a fall through, and the answer then is the
; same one bail() gives - back to the rom.
;
	.global	sexit

	.text
sexit:
	jp	0

; vim: tabstop=4 shiftwidth=4 noexpandtab:
