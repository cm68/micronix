;
; sys/expandbuf.s  --  buffer-list expansion, a leaf in the u page
;
; expand_bufs(arena, nheaders) mints nheaders struct buf headers out of
; the init-text arena and appends them to the blist after the 8 boot
; headers binit() seeded.  It runs last - after every init-only
; function has returned - so it may overwrite the init text that was
; there.  This file is .data and lives in the u page (see user.c), so
; the minting loop itself is never inside the arena it writes: that is
; the chicken-and-egg fix.
;
; struct buf is 19 bytes (see sys/buf.h):
;   flags(1) dev(2) blk(2) count(2) data(2) xmem(1) forw(2) back(2)
;   cyl(2) error(1) time(2)
; binit() zeroes the header and sets data (offset 7) and xmem (offset
; 9); we do the same.  The minted headers take buffer[8..], so the data
; pointer starts at _buffer + 8*512 and walks up by 512 per header.
;
;   arg0 (hl)       = arena base
;   arg1 (stack)    = nheaders
;
	.globl	_expand_bufs
	.extern	_buffer
	.data

_expand_bufs:
	push	ix
	ld	ix,0
	add	ix,sp		; ix = frame
	push	bc		; callee-save
	push	iy

	ex	de,hl		; de = arena (arg0)
	ld	c,(ix+6)
	ld	b,(ix+7)	; bc = nheaders (arg1)
	ld	hl,(_buffer)
	push	de		; save arena
	ld	de,4096		; 8 boot blocks * 512
	add	hl,de		; hl = buffer[8] (data for the first header)
	pop	de		; restore arena

loop:
	ld	a,b
	or	c
	jr	z,done		; nheaders == 0

	; zero 19 bytes at [de]
	push	hl		; save data
	push	bc		; save count
	push	de
	pop	hl		; hl = de
	xor	a
	ld	b,19
zloop:	ld	(hl),a
	inc	hl
	djnz	zloop
	pop	bc		; restore count
	pop	hl		; restore data

	; data (offset 7) = hl, xmem (offset 9) = 0
	push	de
	pop	iy		; iy = header base
	ld	a,l
	ld	(iy+7),a
	ld	a,h
	ld	(iy+8),a
	xor	a
	ld	(iy+9),a

	; de += 19
	push	hl		; save data
	push	bc		; save count
	ld	hl,19
	add	hl,de
	ex	de,hl		; de = next header
	pop	bc		; restore count
	pop	hl		; restore data

	; hl += 512
	push	bc
	ld	bc,512
	add	hl,bc		; hl = next buffer block
	pop	bc

	dec	bc		; one header minted
	jr	loop

done:
	pop	iy
	pop	bc
	pop	ix
	ret
