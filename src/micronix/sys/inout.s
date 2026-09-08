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
	push	bc		; save bc (callee-saved under ccc)
	ld	b,h
	ld	c,l		; bc = port (arg0 arrives in hl)
	in	l,(c)		; l = result
	ld	h,0		; hl = result, zero-extended
	pop	bc		; restore bc
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
	push	bc		; save bc (callee-saved under ccc)
	ld	b,h
	ld	c,l		; bc = port (arg0 arrives in hl)
	ld	hl,4
	add	hl,sp		; hl = &data (arg1, now at sp+4 after the push)
	ld	a,(hl)		; a = data
	out	(c),a
	pop	bc		; restore bc
	ret
