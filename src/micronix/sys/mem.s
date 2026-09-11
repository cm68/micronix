;
; =====================================================================
; sys/mem.s  --  Z80 translation of sys/mem.anat (asz dialect)
; =====================================================================
;
; ------- A-NATURAL SOURCE: declarations -------
; /*
;  * low level memory access
;  *
;  * sys/mem.s
;  * Changed: <2022-01-04 10:36:15 curt>
;  */
;
; public	_getbyte
; public	_getword
; public	_putbyte
; public	_putword
; public	_copyin
; public	_copyout
; public	_memrw
; public	_segcopy
; public	_meminit
;
; MAP0	:= &0x0600		/task zero window registers
; IMAGE0	:= &0x0200		/readable version of MAP0
;
; MAP1	:= &0x0620		/task ones map registers
; IMAGE1	:= &0x0220		/task ones readable map registers
;
; LDIR	:= &0xB0ED		/z80 do *de++ = *hl++ while --bc != 0
;
	.globl	_getbyte, _getword, _putbyte, _putword, _copyin, _copyout
	.globl	_memrw, _segcopy, _zerouser
	.extern	_di, _ei, _meminit, _zpage
MAP0=0x0600
IMAGE0=0x0200
MAP1=0x0620
IMAGE1=0x0220

; ------- A-NATURAL SOURCE: _getbyte -------
; / getbyte(addr) -- Get a byte from the active _task
;
; _getbyte:
; 	hl = 2 + sp
; 	hl =a^ hl		/hl = addr
;
; 	call getw		/bc = word at address hl
;
; 	b = 0
; 	ret
;
_getbyte:
	call	getw		; hl = word at addr (arg0 arrives in hl)
	ld	h,0		; hl = byte, zero-extended
	ret

; ------- A-NATURAL SOURCE: _getword -------
; / getword(addr) -- Get a word from the active _task
;
; _getword:
; 	hl = 2 + sp
; 	hl =a^ hl		/hl = addr
; 				/drop thru to getw
;
_getword:			; addr (arg0) already in hl; fall through to getw

; ------- A-NATURAL SOURCE: getw -------
; / getw	  bc = word at address hl
; / Copy the appropriate user map registers to the windows,
; / replace the high nibble of addr with the window nibble,
; / get the data, and restore the _windows
;
; getw:
; 	a = h & 0xF0		/a = high nibble of addr (times 16)
; 	a <*> -1		/rotate a right 3 times w/o carry
; 	a <*> -1
; 	a <*> -1		/now a = 2 * high nibble of addr
;
; 	bc = IMAGE1		/task ones map
; 	a + c -> c		/bc points to map reg for desired word
;
; 	a = h & 0x0F		/remove high nibble from addr
; 	a | 0x10 -> h		/and replace it with window nibble
;
; 	call _di
; 	a = *bc -> *MAP0[2]	/now task zero can see desired word
; 	bc +1 +1		/Set up MAP0[4] in case the word
; 	a = *bc -> *MAP0[4]	/crosses a segment boundary
;
; 	bc =^ hl		/get word
; 	a = *IMAGE0[2]		/restore task 0s windows
; 	a -> *MAP0[2]
; 	a = *IMAGE0[4]
; 	a -> *MAP0[4]
;
; 	jmp _ei
;
getw:
	push	bc		; save bc (callee-saved)
	ld	a,h
	and	0xF0
	rrca
	rrca
	rrca			/now a = 2 * high nibble of addr
	ld	bc,IMAGE1
	add	a,c
	ld	c,a
	ld	a,h
	and	0x0F
	or	0x10
	ld	h,a
	call	_di
	ld	a,(bc)
	ld	(MAP0+2),a
	inc	bc
	inc	bc
	ld	a,(bc)
	ld	(MAP0+4),a
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	ld	l,c
	ld	h,b		; hl = word (return value)
	ld	a,(IMAGE0+2)
	ld	(MAP0+2),a
	ld	a,(IMAGE0+4)
	ld	(MAP0+4),a
	pop	bc		; restore bc
	jp	_ei

; ------- A-NATURAL SOURCE: _putbyte -------
; / putbyte(data, addr) -- Put a byte to the active _task
;
; _putbyte:
; 	hl = 2 + sp
; 	bc =^ hl		/bc = data
; 	hl =a^ (hl+1)		/hl = addr
; 				/drop thru to putb
;
_putbyte:
	push	bc		; save bc (callee-saved)
	ld	c,l		; c = data low byte (arg0 in hl)
	ld	hl,4
	add	hl,sp		; hl = &addr (arg1, now at sp+4)
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a		; hl = addr
	call	putb		; put c at [hl]
	pop	bc		; restore bc
	ret

; ------- A-NATURAL SOURCE: putb -------
; / putb	  put byte c to address hl
; / Copy the appropriate user map register to the window,
; / replace the high nibble of addr with the window nibble,
; / put the data, and restore the _window
;
; putb:
; 	sp <= de		/save de
;
; 	a = h & 0xF0		/a = high nibble of addr (times 16)
; 	a <*> -1		/rotate a right 3 times w/o carry
; 	a <*> -1
; 	a <*> -1		/now a = 2 * high nibble of addr
;
; 	de = IMAGE1		/task ones map
; 	a + e -> e		/de points to map reg for desired byte
;
; 	a = h & 0x0F		/remove high nibble from addr
; 	a | 0x10 -> h		/and replace it with window nibble
;
; 	call _di
; 	a = *de -> *MAP0[2]	/now task zero can see desired address
; 	c -> *hl		/put data
; 	a = *IMAGE0[2] -> *MAP0[2]	/restore task zeros window
;
; 	sp => de
; 	jmp _ei
;
putb:
	push	de
	ld	a,h
	and	0xF0
	rrca
	rrca
	rrca			/now a = 2 * high nibble of addr
	ld	de,IMAGE1
	add	a,e
	ld	e,a
	ld	a,h
	and	0x0F
	or	0x10
	ld	h,a
	call	_di
	ld	a,(de)
	ld	(MAP0+2),a
	ld	(hl),c
	ld	a,(IMAGE0+2)
	ld	(MAP0+2),a
	pop	de
	jp	_ei

; ------- A-NATURAL SOURCE: _putword -------
; / putword(data, addr) -- Put a word to the active _task
;
; _putword:
;
; 	hl = 2 + sp		/get arguments from stack
; 	bc =^ hl		/bc = data
; 	hl =a^ (hl+1)		/hl = addr
;
; 	sp <= bc <= hl
; 	call putb		/put c
;
; 	sp => hl => bc
; 	hl +1; c = b
; 	jmp putb		/put b
;
_putword:
	push	bc		; save bc (callee-saved)
	ld	a,h		; a = data high byte (arg0 in hl)
	ld	c,l		; c = data low byte
	ld	hl,4
	add	hl,sp		; hl = &addr (arg1, now at sp+4)
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		; de = addr
	push	de
	pop	hl		; hl = addr
	call	putb		; put c (low byte) at addr
	inc	de		; de = addr+1
	ld	c,a		; c = high byte
	push	de
	pop	hl		; hl = addr+1
	call	putb		; put c (high byte) at addr+1
	pop	bc		; restore bc
	ret

; ------- A-NATURAL SOURCE: _copyin -------
; / copyin(from, to, count)
; / Copy count bytes from from in the active task
; / to to in the kernel. Limited to 4K bytes per crack.
;
; _copyin:
; 	de => sp			/save C frame pointer
; 	a = *IMAGE0[2]; af => sp	/save images so interrupt code
; 	a = *IMAGE0[4]; af => sp	/ can use the windows
;
; 	hl = 13 + sp		/get arguments from stack
; 	b = *hl
; 	c = *(hl-1)		/bc = count
; 	d = *(hl-1)
; 	e = *(hl-1)		/de = to
; 	a = *(hl-1)
; 	l = *(hl-1)
; 	h = a			/hl = from
;
; 	a = h & 0xF0		/a = high nibble of from (times 16)
; 	a <*> -1		/rotate a right 3 times w/o carry
; 	a <*> -1
; 	a <*> -1		/now a = 2 * high nibble of from
;
; 	bc => sp		/save count
;
; 	bc = IMAGE1		/task ones map
; 	a + c -> c		/bc points to map reg for source
;
; 	call _di
; 	a = *bc
; 	a -> *IMAGE0[2] -> *MAP0[2]	/now task zero can see user space
; 	a = *(bc +1 +1)			/Set up MAP0[4] in case copy crosses
; 	a -> *IMAGE0[4] -> *MAP0[4]	/ a segment boundary
;
; 	bc <= sp			/restore count to bc
;
; 	a = h & 0x0F			/remove high nibble from from
; 	a | 0x10 -> h			/and replace it with window nibble
;
; 	a = b | c			/test for zero count
; 	jz .4
;
; 	LDIR				/z80 do *de++ = *hl++ while --bc != 0
;
; .4:
; 	af <= sp			/restore windows
; 	a -> *IMAGE0[4] -> *MAP0[4]
; 	af <= sp
; 	a -> *IMAGE0[2] -> *MAP0[2]
;
; 	de <= sp		/recover C frame pointer
; 	jmp _ei
;
_copyin:
	ld	a,(IMAGE0+2)
	push	af
	ld	a,(IMAGE0+4)
	push	af
	push	ix
	ld	ix,0
	add	ix,sp		; ix = sp
	push	bc		; save bc (callee-saved)
	ld	e,(ix+8)
	ld	d,(ix+9)	; de = to (arg1)
	ld	c,(ix+10)
	ld	b,(ix+11)	; bc = count (arg2)
	ld	a,h		; hl = from (arg0)
	and	0xF0
	rrca
	rrca
	rrca			/now a = 2 * high nibble of from
	push	bc		; save count
	ld	bc,IMAGE1
	add	a,c
	ld	c,a
	call	_di
	ld	a,(bc)
	ld	(IMAGE0+2),a
	ld	(MAP0+2),a
	inc	bc
	inc	bc
	ld	a,(bc)
	ld	(IMAGE0+4),a
	ld	(MAP0+4),a
	pop	bc		; restore count
	ld	a,h
	and	0x0F
	or	0x10
	ld	h,a		; hl = windowed from
	ld	a,b
	or	c
	jp	z,4f
	ldir
4:
	pop	bc		; restore caller's bc
	pop	ix
	pop	af
	ld	(IMAGE0+4),a
	ld	(MAP0+4),a
	pop	af
	ld	(IMAGE0+2),a
	ld	(MAP0+2),a
	jp	_ei

; ------- A-NATURAL SOURCE: _copyout -------
; / copyout(from, to, count)
; / Copy count bytes from from in the kernel
; / to to in the active task. Limited to 4K bytes per crack.
;
; _copyout:
; 	de => sp		/save C frame pointer
; 	a = *IMAGE0[2]; af => sp	/save images so interrupt code
; 	a = *IMAGE0[4]; af => sp	/ can use the windows
;
; 	hl = 13 + sp		/get arguments from stack
; 	b = *hl
; 	c = *(hl-1)		/bc = count
; 	d = *(hl-1)
; 	e = *(hl-1)		/de = to
; 	a = *(hl-1)
; 	l = *(hl-1)
; 	h = a			/hl = from
;
; 	a = d & 0xF0		/a = high nibble of to (times 16)
; 	a <*> -1		/rotate a right 3 times w/o carry
; 	a <*> -1
; 	a <*> -1		/now a = 2 * high nibble of to
;
; 	bc => sp		/save count
;
; 	bc = IMAGE1		/task ones map
; 	a + c -> c		/bc points to map reg for source
;
; 	call _di
; 	a = *bc
; 	a -> *IMAGE0[2] -> *MAP0[2]	/now task zero can see user space
; 	a = *(bc +1 +1)
; 	a -> *IMAGE0[4] -> *MAP0[4]
;
; 	bc <= sp		/restore count to bc
;
; 	a = d & 0x0F		/remove high nibble from to
; 	a | 0x10 -> d		/and replace it with window nibble
;
; 	a = b | c		/test for zero count
; 	jz .5
;
; 	LDIR			/z80 do *de++ = *hl++ while --bc != 0
;
; .5:
; 	af <= sp		/restore windows
; 	a -> *IMAGE0[4] -> *MAP0[4]
; 	af <= sp
; 	a -> *IMAGE0[2] -> *MAP0[2]
;
; 	de <= sp		/recover C frame pointer
; 	jmp _ei
;
_copyout:
	ld	a,(IMAGE0+2)
	push	af
	ld	a,(IMAGE0+4)
	push	af
	push	ix
	ld	ix,0
	add	ix,sp		; ix = sp
	push	bc		; save bc (callee-saved)
	ld	e,(ix+8)
	ld	d,(ix+9)	; de = to (arg1)
	ld	c,(ix+10)
	ld	b,(ix+11)	; bc = count (arg2)
	ld	a,d		; de = to
	and	0xF0
	rrca
	rrca
	rrca			/now a = 2 * high nibble of to
	push	bc		; save count
	ld	bc,IMAGE1
	add	a,c
	ld	c,a
	call	_di
	ld	a,(bc)
	ld	(IMAGE0+2),a
	ld	(MAP0+2),a
	inc	bc
	inc	bc
	ld	a,(bc)
	ld	(IMAGE0+4),a
	ld	(MAP0+4),a
	pop	bc		; restore count
	ld	a,d
	and	0x0F
	or	0x10
	ld	d,a		; de = windowed to
	ld	a,b
	or	c
	jp	z,5f
	ldir			; hl = from (arg0), de = windowed to, bc = count
5:
	pop	bc		; restore caller's bc
	pop	ix
	pop	af
	ld	(IMAGE0+4),a
	ld	(MAP0+4),a
	pop	af
	ld	(IMAGE0+2),a
	ld	(MAP0+2),a
	jp	_ei

; ------- _zerouser -------
; zerouser(addr, count) -- zero count bytes of the active task at addr.
; addr in hl (arg0), count on the stack (arg1).  Maps the user's page and
; the next into the window and ldir's from the zero page, 512 bytes at a
; time.  count must fit the two-page window.
_zerouser:
	ld	a,(IMAGE0+2)
	push	af
	ld	a,(IMAGE0+4)
	push	af
	push	ix
	ld	ix,0
	add	ix,sp
	push	bc		; save bc (callee-saved)
	ld	c,(ix+8)
	ld	b,(ix+9)	; bc = count (arg1)
	ld	a,h
	and	0xF0
	rrca
	rrca
	rrca		; a = 2 * high nibble of addr
	push	bc		; save count
	ld	bc,IMAGE1
	add	a,c
	ld	c,a		; bc = &image1[2*page]
	call	_di
	ld	a,(bc)
	ld	(IMAGE0+2),a
	ld	(MAP0+2),a	; map page N
	inc	bc
	inc	bc
	ld	a,(bc)
	ld	(IMAGE0+4),a
	ld	(MAP0+4),a	; map page N+1
	pop	bc		; restore count
	ld	a,h
	and	0x0F
	or	0x10
	ld	h,a		; hl = windowed addr
	ex	de,hl		; de = windowed addr
	ld	hl,_zpage	; hl = zero source
1:	ld	a,b
	or	c
	jp	z,2f		; count == 0
	ld	a,b
	cp	2
	jr	c,3f		; count < 512, last chunk
	push	bc
	ld	bc,512
	ldir		; zero 512 bytes
	pop	bc
	dec	b
	dec	b		; count -= 512
	ld	hl,_zpage	; reset source
	jr	1b
3:	ldir		; zero the last (< 512) bytes
2:	pop	bc
	pop	ix
	pop	af
	ld	(IMAGE0+4),a
	ld	(MAP0+4),a
	pop	af
	ld	(IMAGE0+2),a
	ld	(MAP0+2),a
	jp	_ei

; ------- A-NATURAL SOURCE: _memrw -------

; / memrw(user, space, direction, count)
; / Copy count bytes in the specified direction between
; / user space and outer space (an absolute long address).
; / Neither source nor destination should cross a 4K boudary.
;
; _memrw:
; 	de => sp			/save C frame pointer
; 	a = *IMAGE0[2]; af => sp	/save images so interrupt code
; 	a = *IMAGE0[4]; af => sp	/ can use the windows
;
; 	hl = 17 + sp		/get arguments from stack
; 	b = *hl
; 	c = *(hl-1)		/bc = count
; 	bc => sp		/save it
;
; 	     (hl-1)
; 	c = *(hl-1)		/c = direction
; 	bc => sp		/save it
;
; 	d = *(hl-1)
; 	e = *(hl-1)		/de = low word of space address
; 	     (hl-1)		/skip high byte of long
; 	b = *(hl-1)		/bde = 24 bit space address
;
; 	a = *(hl-1)
; 	l = *(hl-1)
; 	h = a			/hl = user address
;
; 	call _di		/set up space window
; 	a = b & 0x0F -> b	/keep low nibble of b
; 	a = d & 0xF0 | b
; 	a <*> -1		/rotate a right 4 times w/o carry
; 	a <*> -1
; 	a <*> -1
; 	a <*> -1		/now a = high 8 bits of 20-bit space addr
; 	a -> *IMAGE0[4] -> *MAP0[4]	/task0s window to space
; 	a = d & 0x0F		/remove high nibble from space addr
; 	a | 0x20 -> d		/replace it with window nibble
;
; 				/set up user window
; 	a = h & 0xF0		/a = high nibble of user addr (times 16)
; 	a <*> -1		/rotate a right 3 times w/o carry
; 	a <*> -1
; 	a <*> -1		/now a = 2 * high nibble of user addr
; 	bc = IMAGE1		/task ones map
; 	a + c -> c		/bc points to map reg for user addr
; 	a = *bc
; 	a -> *IMAGE0[2] -> *MAP0[2]	/task0s window to user
; 	a = h & 0x0F		/remove high nibble from user addr
; 	a | 0x10 -> h		/replace it with window nibble
;
; 	bc <= sp		/get direction
; 				/now hl -> user, de -> space
; 				/want hl -> source, de -> destination
; 	a = c | c		/non-0 means WRITE from user to space
; 	jnz .6			/ and we are set up for a write
; 	hl <> de
;
; .6:				/now hl -> source, de -> destination
; 	bc <= sp		/get count
; 	a = b | c		/test for zero count
; 	jz .7
;
; 	call _ei
; 	LDIR			/z80 do *de++ = *hl++ while --bc != 0
; 	call _di
;
; .7:
; 	af <= sp		/restore windows
; 	a -> *IMAGE0[4] -> *MAP0[4]
; 	af <= sp
; 	a -> *IMAGE0[2] -> *MAP0[2]
;
; 	de <= sp		/recover C frame pointer
; 	jmp _ei
;
_memrw:
	push	ix
	ld	ix,0
	add	ix,sp		; ix = sp
	ld	a,(IMAGE0+2)
	push	af
	ld	a,(IMAGE0+4)
	push	af
	push	bc		; save bc (callee-saved)
	ld	c,(ix+10)
	ld	b,(ix+11)	; bc = count (arg3)
	push	bc		; save count
	ld	a,(ix+8)	; a = direction (arg2 low byte)
	push	af		; save direction
	ld	d,(ix+7)	; d = space middle byte (arg1)
	ld	e,(ix+6)	; e = space low byte
	ld	b,(ix+4)	; b = space high byte
	call	_di
	ld	a,b
	and	0x0F
	ld	b,a
	ld	a,d
	and	0xF0
	or	b
	rrca
	rrca
	rrca
	rrca			/now a = high 8 bits of 20-bit space addr
	ld	(IMAGE0+4),a
	ld	(MAP0+4),a
	ld	a,d
	and	0x0F
	or	0x20
	ld	d,a		; de = windowed space addr
	ld	a,h		; hl = user (arg0)
	and	0xF0
	rrca
	rrca
	rrca			/now a = 2 * high nibble of user addr
	ld	bc,IMAGE1
	add	a,c
	ld	c,a
	ld	a,(bc)
	ld	(IMAGE0+2),a
	ld	(MAP0+2),a
	ld	a,h
	and	0x0F
	or	0x10
	ld	h,a		; hl = windowed user addr
	pop	af		; a = direction
	or	a
	jp	nz,6f
	ex	de,hl		; direction == 0: space -> user
6:
	pop	bc		; bc = count
	ld	a,b
	or	c
	jp	z,7f
	call	_ei
	ldir
	call	_di
7:
	pop	bc		; restore caller's bc
	pop	af
	ld	(IMAGE0+4),a
	ld	(MAP0+4),a
	pop	af
	ld	(IMAGE0+2),a
	ld	(MAP0+2),a
	pop	ix		; restore ix
	jp	_ei

; ------- A-NATURAL SOURCE: _segcopy -------
; /segcopy(from, to) -- Copy one 4K segment to another
;
; _segcopy:
;
; 	sp <= de		/save C frame pointer
; 	hl = 4 + sp		/point to arguments
; 	a = *IMAGE0[2]		/save current map
; 	sp <= af
; 	a = *IMAGE0[4]
; 	sp <= af
;
; 	call _di
; 	a = *hl			/from segment
; 	a -> *IMAGE0[2]
; 	a -> *MAP0[2]		/task 0 can see source segment
;
; 	a = *(hl +1 +1)		/to segment
; 	a -> *IMAGE0[4]
; 	a -> *MAP0[4]		/task 0 can see destination segment
; 	call _ei
;
; 	hl = 0x1000		/hl looks thru MAP[2]
; 	de = 0x2000		/de looks thru MAP[4]
; 	bc = 4096		/segment size
;
; 	LDIR			/z80 do *de++ = *hl++ while --bc != 0
;
; 	call _di
; 	sp => af		/restore task 0s map
; 	a -> *MAP0[4]
; 	a -> *IMAGE0[4]
; 	sp => af
; 	a -> *MAP0[2]
; 	a -> *IMAGE0[2]
;
; 	sp => de	/restore C frame pointer
; 	jmp _ei
;
_segcopy:
	push	ix
	ld	ix,0
	add	ix,sp		; ix = sp
	ld	a,(IMAGE0+2)
	push	af
	ld	a,(IMAGE0+4)
	push	af
	push	bc		; save bc (callee-saved, for ldir count)
	call	_di
	ld	a,l		; a = from (arg0, low byte)
	ld	(IMAGE0+2),a
	ld	(MAP0+2),a
	ld	a,(ix+4)	; a = to (arg1, low byte)
	ld	(IMAGE0+4),a
	ld	(MAP0+4),a
	call	_ei
	ld	hl,0x1000
	ld	de,0x2000
	ld	bc,4096
	ldir
	call	_di
	pop	bc		; restore bc
	pop	af
	ld	(MAP0+4),a
	ld	(IMAGE0+4),a
	pop	af
	ld	(MAP0+2),a
	ld	(IMAGE0+2),a
	pop	ix		; restore ix
	jp	_ei
