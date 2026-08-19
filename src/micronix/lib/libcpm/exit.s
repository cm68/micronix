;
;	exit(status)
;
;	CP/M 3 keeps a program return code for whoever ran us, and bdos
;	function 108 is how a program sets it: DE holds the value, and
;	0FFFFH in DE would ask for it instead.
;
;	This used to write the status to 80H and warm boot.  80H is the
;	default DMA address and the command tail arrives there, so the
;	status shared a buffer with both the arguments we were started
;	with and every sector read through the default DMA - it survived
;	only because nothing happened to land on it between the store and
;	the boot.  The return code is a register the system keeps for
;	this, and nothing else writes it.
;
	global	_exit, __cpm_clean, __cleanup

RETCODE	equ	108		;bdos: get/set program return code

	psect	text
_exit:
	push	hl		;the cleanups will not bring hl back - the
	call	__cleanup	;  old code stored the status before calling
	call	__cpm_clean	;  them for this reason
	pop	de		;status
	ld	c,RETCODE
	call	5
	jp	0		;warm boot
