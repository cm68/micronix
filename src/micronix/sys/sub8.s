;
; =====================================================================
; sys/sub8.s  --  Z80 translation of sys/sub8.anat (asz dialect)
; =====================================================================
;
; ------- COMPLETE ORIGINAL A-NATURAL SOURCE (every line preserved) -------
;
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
; 
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
; 
; /setframe(frmptr, stkptr);
; /Set the frame pointer and stack pointer so that on
; /return, after C pops the arguments, sp == _stkptr
; _setframe:
; 	sp => bc => de => hl	/de = frmptr
; 	sp = hl			/sp = stkptr
; 	sp <= bc <= bc <= bc	/get ready for C pops
; 	ret
; 
; 
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
; 
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
; 
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
; 
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
; 
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
; 
; /enable()	Unconditionally enable interrupts
; _enable:
; 	a ^ a -> dicount       /zero the di count
; 	0xED; 0x46		/z80 interrupt mode 0
; 	ei
; 	ret
; 
; /disable()	Initialize di counter
; _disable:
; 	di
; 	a = 1
; 	a -> dicount
; 	ret
; 
; /disable count
; dicount:	0
;
; ------- END A-NATURAL SOURCE -------
;
; ------- Z80 TRANSLATION -------
;
	.globl	_saveframe, _setframe, _zero, _copy, _x3to4, _x4to3
	.globl	_di, _ei, _enable, _disable, dicount
	.extern	_pr
_saveframe:
	pop	bc		; bc = return address
	pop	hl		; hl = &u->frmptr
	push	hl
	push	bc
	ld	(hl),e
	inc	hl
	ld	(hl),d
	ld	hl,6
	add	hl,sp
	ld	c,l
	ld	b,h
	dec	hl
	dec	hl
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	ld	(hl),c
	inc	hl
	ld	(hl),b
	ret
_setframe:
	pop	bc		; bc = return address
	pop	de		; de = frmptr
	pop	hl		; hl = stkptr
	ld	sp,hl
	push	bc
	push	bc
	push	bc
	ret
_zero:
	push	de
	ld	hl,4
	add	hl,sp
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	ex	de,hl
	ld	a,b
	or	c
	jp	z,2f
	ld	e,0
1:
	ld	(hl),e
	inc	hl
	dec	bc
	ld	a,b
	or	c
	jp	nz,1b
2:
	pop	de
	ret
_copy:
	push	de
	ld	hl,9
	add	hl,sp
	ld	b,(hl)
	dec	hl
	ld	c,(hl)
	dec	hl
	ld	d,(hl)
	dec	hl
	ld	e,(hl)
	dec	hl
	ld	a,(hl)
	dec	hl
	ld	l,(hl)
	ld	h,a
	ld	a,b
	or	c
	jp	z,3f
	ldir
3:
	pop	de
	ret
_x3to4:
	ld	hl,2
	add	hl,sp
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	inc	hl
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	ld	a,(bc)
	ld	(hl),a
	xor	a
	inc	hl
	ld	(hl),a
	inc	bc
	ld	a,(bc)
	inc	hl
	ld	(hl),a
	inc	bc
	ld	a,(bc)
	inc	hl
	ld	(hl),a
	ret
_x4to3:
	ld	hl,2
	add	hl,sp
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	inc	hl
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	ld	a,(bc)
	ld	(hl),a
	inc	bc
	inc	bc
	ld	a,(bc)
	inc	hl
	ld	(hl),a
	inc	bc
	ld	a,(bc)
	inc	hl
	ld	(hl),a
	ret
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
	ld	hl,badmsg
	push	hl
	call	_pr
	pop	hl
	pop	hl
	ret
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
_enable:
	xor	a
	ld	(dicount),a
	im	0
	ei
	ret
_disable:
	di
	ld	a,1
	ld	(dicount),a
	ret
dicount:
	.defb	0
badmsg:
	.defb	"di < 0", 0
