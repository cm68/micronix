;
; =====================================================================
; sys/sub8.s  --  Z80 translation of sys/sub8.anat (asz dialect)
; =====================================================================
;
; ------- A-NATURAL SOURCE: declarations -------
; /*
;  * Miscellaneous subroutines needed by kernel
;  *
;  * sys/sub8.s
;  * Changed: <>
;  */
;
; public	_saveframe
; public	_setframe
; public	_zero
; public	_copy
; public	_x3to4
; public	_x4to3
; public	_di
; public	_ei
; public	_enable
; public	_disable
; public	dicount
;
	.globl	_saveframe, _setframe, _zero, _copy, _x3to4, _x4to3
	.globl	_di, _ei, _enable, _disable, dicount
	.extern	_pr

; ------- A-NATURAL SOURCE: _saveframe -------
; /saveframe(&u->frmptr, &u->stkptr);
; /Save the C frame pointer and stack pointer (before the call)
; _saveframe:
; 	sp => bc => hl	/hl = &u->frmptr
; 	sp <= hl <= bc	/restore stack
; 	e -> *hl	/store the C frame pointer
; 	d -> *(hl +1)
; 	hl = 6 + sp	/hl = sp before this call
; 	hl -> bc	/bc = sp before this call
; 	hl -1 -1	/hl points to &u->stkptr
; 	hl =a^ hl	/hl = &u->stkptr
; 	c -> *hl	/store the stack pointer
; 	b -> *(hl +1)
; 	ret
;
_saveframe:
	push	ix		; save ix (callee-saved under ccc)
	ld	ix,0
	add	ix,sp		; ix = sp, so arg1 is at (ix+4)
	push	iy
	pop	de		; de = iy, the ccc frame pointer
	ld	(hl),e
	inc	hl
	ld	(hl),d		; *frmptr = frame pointer
	ld	hl,6
	add	hl,sp		; hl = pre-call sp (entry_sp + 4)
	ex	de,hl		; de = pre-call sp
	ld	l,(ix+4)
	ld	h,(ix+5)	; hl = &stkptr (arg1)
	ld	(hl),e
	inc	hl
	ld	(hl),d		; *stkptr = pre-call sp
	pop	ix		; restore ix
	ret

; ------- A-NATURAL SOURCE: _setframe -------
; /setframe(frmptr, stkptr);
; /Set the frame pointer and stack pointer so that on
; /return, after C pops the arguments, sp == _stkptr
; _setframe:
; 	sp => bc => de => hl	/de = frmptr
; 	sp = hl			/sp = stkptr
; 	sp <= bc <= bc <= bc	/get ready for C pops
; 	ret
;
_setframe:
	push	hl
	pop	iy		; iy = frmptr (arg0)
	pop	de		; de = return address
	pop	hl		; hl = stkptr (arg1)
	ld	sp,hl		; sp = stkptr
	push	de
	push	de		; return address x2 (1 for ret, 1 for caller's arg1 drop)
	ret

; ------- A-NATURAL SOURCE: _zero -------
; /zero(buf, count)
; /Zero count bytes beginning at buf
; _zero:
; 	sp <= de
;
; 	hl = 4 + sp
; 	de =^ hl	/de = buf
; 	bc =^(hl +1)	/bc = count
; 	hl <> de	/hl = buf
;
; 	a = b | c
; 	jz .2
;
; 	e = 0
; .1:
; 	e -> *hl
; 	hl +1
; 	bc -1
; 	a = b | c
; 	jnz .1
;
; .2:
; 	sp => de
; 	ret
;
_zero:
	push	ix
	ld	ix,0
	add	ix,sp		; ix = sp
	push	bc		; save bc (callee-saved)
	ld	c,(ix+4)
	ld	b,(ix+5)	; bc = count (arg1)
	ld	a,b
	or	c
	jp	z,2f
	ld	e,0		; e = 0 (the zero byte)
1:
	ld	(hl),e		; hl = buf (arg0)
	inc	hl
	dec	bc
	ld	a,b
	or	c
	jp	nz,1b
2:
	pop	bc
	pop	ix
	ret

; ------- A-NATURAL SOURCE: _copy -------
; /copy(source, dest, count)
; /Copy count bytes from source to dest (all in kernel space)
; 	LDIR	:= &0xB0ED	/z80 do *de++ = *hl++ while --bc != 0
; _copy:
; 	sp <= de
;
; 	hl = 9 + sp
; 	b = *hl
; 	c = *(hl-1)	/bc = count
; 	d = *(hl-1)
; 	e = *(hl-1)	/de = dest
; 	a = *(hl-1)
; 	l = *(hl-1)
; 	h = a		/hl = source
;
; 	a = b | c
; 	jz .3
;
; 	LDIR		/z80 do *de++ = *hl++ while --bc != 0
;
; .3:
; 	sp => de
; 	ret
;
_copy:
	push	ix
	ld	ix,0
	add	ix,sp		; ix = sp
	push	bc		; save bc (callee-saved)
	ld	e,(ix+4)
	ld	d,(ix+5)	; de = dest (arg1)
	ld	c,(ix+6)
	ld	b,(ix+7)	; bc = count (arg2)
	ld	a,b
	or	c
	jp	z,3f
	ldir			; hl = source (arg0), de = dest, bc = count
3:
	pop	bc
	pop	ix
	ret

; ------- A-NATURAL SOURCE: _x3to4 -------
; /x3to4(&three, &long)
; /Translate a 3-byte integer (inode size) into a _long
; /3-byte stored as   2 0 1 (in order of significance)
; /4-byte stored as 2 3 0 _1
; _x3to4:
; 	hl = 2 + sp
; 	bc =^ hl	/bc = &three
; 	hl =a^ (hl +1)	/hl = &long
; 	a = *bc -> *hl
; 	a ^ a -> *(hl +1)	/zero msb of long
; 	a = *(bc +1) -> *(hl +1)
; 	a = *(bc +1) -> *(hl +1)
; 	ret
;
_x3to4:
	push	ix
	ld	ix,0
	add	ix,sp		; ix = sp
	ex	de,hl		; de = &three (arg0)
	ld	l,(ix+4)
	ld	h,(ix+5)	; hl = &long (arg1)
	ld	a,(de)
	ld	(hl),a
	xor	a
	inc	hl
	ld	(hl),a
	inc	de
	ld	a,(de)
	inc	hl
	ld	(hl),a
	inc	de
	ld	a,(de)
	inc	hl
	ld	(hl),a
	pop	ix
	ret

; ------- A-NATURAL SOURCE: _x4to3 -------
; /x4to3(&long, &three)
; /vice-versa
; _x4to3:
; 	hl = 2 + sp
; 	bc =^ hl	/bc = &long
; 	hl =a^ (hl +1)	/hl = &three
; 	a = *bc -> *hl
; 	a = *(bc +1 +1) -> *(hl +1)
; 	a = *(bc +1) -> *(hl +1)
; 	ret
;
_x4to3:
	push	ix
	ld	ix,0
	add	ix,sp		; ix = sp
	ex	de,hl		; de = &long (arg0)
	ld	l,(ix+4)
	ld	h,(ix+5)	; hl = &three (arg1)
	ld	a,(de)
	ld	(hl),a
	inc	de
	inc	de
	ld	a,(de)
	inc	hl
	ld	(hl),a
	inc	de
	ld	a,(de)
	inc	hl
	ld	(hl),a
	pop	ix
	ret

; ------- A-NATURAL SOURCE: _di -------
; /di()	Disable interrupts and increment the level count
; _di:
; 	di
; 	sp <= hl
; 	hl = &dicount
; 	*hl +1
; 	sp => hl
; 	rp
; 	sp <= hl
; 	hl = &dicount
; 	*hl = 1
; 	hl = &="di < 0\0" => sp
; 	call _pr
; 	sp => hl => hl
; 	ret
;
_di:
	di
	push	hl
	ld	hl,dicount
	inc	(hl)
	pop	hl
	ret	p
	push	hl
	ld	hl,dicount
	ld	(hl),1
	ld	hl,badmsg	; hl = badmsg (arg0 for _pr)
	call	_pr
	pop	hl
	ret

; ------- A-NATURAL SOURCE: _ei -------
; /ei()	Conditionally enable interrupts (at level zero)
; _ei:
; 	sp <= hl
; 	hl = &dicount
; 	*hl -1
; 	jp ok
; 	*hl = 0
; 	ei
; ok:
; 	sp => hl
; 	rnz
; 	ei
; 	ret
;
_ei:
	push	hl
	ld	hl,dicount
	dec	(hl)
	jp	p,ok
	ld	(hl),0
	ei
ok:
	pop	hl
	ret	nz
	ei
	ret

; ------- A-NATURAL SOURCE: _enable -------
; /enable()	Unconditionally enable interrupts
; _enable:
; 	a ^ a -> dicount       /zero the di count
; 	0xED; 0x46		/z80 interrupt mode 0
; 	ei
; 	ret
;
_enable:
	xor	a
	ld	(dicount),a
	im	0
	ei
	ret

; ------- A-NATURAL SOURCE: _disable -------
; /disable()	Initialize di counter
; _disable:
; 	di
; 	a = 1
; 	a -> dicount
; 	ret
;
_disable:
	di
	ld	a,1
	ld	(dicount),a
	ret

; ------- A-NATURAL SOURCE: dicount -------
; /disable count
; dicount:	0
;
dicount:
	.defb	0
badmsg:
	.defb	"di < 0", 0
