;
; micronix hi-tech-c crt0.as
;
	psect	text,global,pure
	psect	data,global
	psect	bss,global

	psect	text
	defs	100h

	global	start,_main,_exit,__Hbss, __Lbss

start:
	ld	de,__Lbss	;Start of BSS segment
	or	a			;clear carry
	ld	hl,__Hbss
	sbc	hl,de		;size of uninitialized data area
	ld	c,l
	ld	b,h
	dec	bc	
	ld	l,e
	ld	h,d
	inc	de
	ld	(hl),0
	ldir			;clear memory
	pop	hl			;unjunk stack
	ex  de,hl
	ld  hl,0
	add hl,sp
	push hl
	push de
	call _main
	pop  bc
	pop  bc
	push hl
	call _exit
	jp	0

	end	start
