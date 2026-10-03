;
; exit system call
;
; exit(status)
;
; Closes all open files, terminates the calling process,
; and notifies the parent process (if it is executing a
; wait). The low byte of status is available to the parent
; via wait.
;
; This call never returns.
;
	.global __exit

	.text
__exit:
				; status arrives in hl, where the kernel wants it
	rst 	08h
	.db 	001h

; vim: tabstop=8 shiftwidth=8 noexpandtab:
