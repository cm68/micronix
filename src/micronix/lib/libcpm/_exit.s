;
;	_exit(status) - exit without flushing
;
;	The status reaches CP/M 3 through bdos function 108, the return
;	code the system keeps for whoever ran us.  See exit.s for why
;	that replaced a store to 80H.
;
;	The argument is read off the stack rather than out of hl because
;	this is called as an ordinary C function and never returns.
;
	global	__exit, __cpm_clean

RETCODE	equ	108		;bdos: get/set program return code

	psect	text
__exit:
	call	__cpm_clean
	pop	hl		;return address
	pop	de		;exit status, straight into de for bdos
	ld	c,RETCODE
	call	5
	jp	0		;warm boot
