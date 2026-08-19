;
; =====================================================================
; sys/inout.s  --  Z80 translation of sys/inout.anat (asz dialect)
; =====================================================================
;
; ------- A-NATURAL SOURCE: declarations -------
; /*
;  * input and output subroutines
;  *
;  * sys/inout.s
;  * Changed: <2021-12-24 06:06:31 curt>
;  */
;
; INTOC	:= &0x48ED		/Z80 input port (c) to c
; OUTA	:= &0x79ED		/Z80 output a to port (c)
;
	.globl	_in, _out

; ------- A-NATURAL SOURCE: _in -------
; /in(port)
; _in:
; 	/
; 	sp => hl => bc <= bc <= hl;
; 	INTOC			/Z80 input port (c) to c
; 	b = 0
; 	ret;
; 	/
;
_in:
	pop	hl		; hl = return address
	pop	bc		; bc = port
	push	bc
	push	hl
	in	c,(c)
	ld	b,0
	ret

; ------- A-NATURAL SOURCE: _out -------
; /out(port, data)
; _out:
; 	/
; 	c = *(hl = 2 + sp);
; 	a = *(hl +1+1)		/a = data
; 	OUTA			/Z80 output a to port (c)
; 	ret;
; 	/
;
_out:
	ld	hl,2
	add	hl,sp
	ld	c,(hl)		; c = port
	inc	hl
	inc	hl
	ld	a,(hl)		; a = data
	out	(c),a
	ret
