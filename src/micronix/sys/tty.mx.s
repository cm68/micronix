str0:
	.db "ttyread nbreak=%x", 0x0a, 0x00
str1:
	.db "ttyread cok=%x", 0x0a, 0x00
str2:
	.db "cookin sleep nbreak=%x", 0x0a, 0x00
str3:
; top FUNC
; FUNC _ttyopen:s
; params=1 locals=0 frame=0 framefree=0
; param tty:s r=4 o=4
	.db "cookin woke nbreak=%x", 0x0a, 0x00
	.bss
_cookin.3:
	.ds 200
_cookin.4:
	.ds 2
_cookin.5:
; stmt BLOCK
; BLOCK n=3
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ushort (ADD:short (DEREF:short (ADD:short $_u 545:short)) 20:short)) 0:ushort)
	.ds 2
	.text
str4:
	.db "--more--", 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, "        ", 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00
	.data
_cookout.7:
	.dw str4
	.text
_ttyopen::
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	push	ix    
        
	push	hl
	pop	ix
	ld hl,(_u+545)
	ld de,20
	add hl,de
; (DE:ushort)
	ld a,(hl)
; stmt EXPR
; EXPR
	inc hl
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld h,(hl)
; (A:ubyte)
	ld l,a
	ld a,l
	or h
; top FUNC
; FUNC _ttyclose:s
; params=1 locals=0 frame=0 framefree=0
; param tty:s r=4 o=4
	jp nz,no0_1
	ld hl,(_u+545)
	ld de,20
	add hl,de
	push ix
	pop de
	ld (hl),e
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=0
; (LNOT:short (PREDEC:ubyte/1 (ADD:short (REGVAR:short IX) 36:short)))
	inc hl
	ld (hl),d
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
no0_1:
; (A:ubyte)
	inc (ix+36)
	set 6,(ix+6)
Xttyopen:
; top FUNC
; FUNC _ttywrite:s
; params=1 locals=0 frame=0 framefree=0
; param tty:s r=4 o=4
	pop	ix
	ret
_ttyclose::
	push	ix    
        
	push	hl
; stmt BLOCK
; BLOCK n=8
; stmt LABEL
	pop	ix
; stmt EXPR
; SEMI
; stmt BLOCK
; BLOCK n=4
; stmt EXPR
; EXPR
	dec (ix+36)
	jp nz,no0_2
	res 6,(ix+6)
no0_2:
Xttyclose:
; (HL:short)
; stmt EXPR
; EXPR
	pop	ix
	ret
_ttywrite::
; (HL:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ushort (ADD:short $_u 599:short)) 0:ushort)
	push	ix    
        
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	push	hl
	pop	ix
; stmt EXPR
; EXPR
@3__F1T:
	push ix
	pop hl
	ld de,31
	add hl,de
	call _fillque
; (HL:short)
; stmt LABEL
	push ix
; stmt EXPR
; SEMI
; stmt GOTO
	pop hl
; stmt LABEL
	call _cstart
; stmt EXPR
; SEMI
	ld hl,(_u+599)
	ld a,l
	or h
; top FUNC
; FUNC _ttyread:s
; params=1 locals=1 frame=0 framefree=0
; param tty:s r=3 o=4
; local cok:s r=4
	jp z,@3__F1B
no0_3:
	ld hl,16
	push hl
; stmt BLOCK
; BLOCK n=7
; stmt EXPR
; EXPR
	push ix
	pop hl
	call _outwait
	pop af
@3__F1C:
	jp @3__F1T
@3__F1B:
Xttywrite:
	pop	ix
	ret
_ttyread::
; (HL:short)
; stmt EXPR
; EXPR
	push	bc     
	push	ix
	ld c,l
	ld b,h
	ld l,c
	ld h,b
	ld de,20
	add hl,de
	ld l,(hl)
	ld h,0
; (HL:short)
; stmt IF
; IF nlbl=0
; (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 6:short))) 128:short)
	push hl
	ld hl,str0
	call _pr
	pop af
	ld l,c
	ld h,b
; stmt BLOCK
; BLOCK n=4
; stmt EXPR
; EXPR
	ld de,26
	add hl,de
	ld l,(hl)
	ld h,0
; (A:ubyte)
; stmt EXPR
; EXPR
	push hl
	ld hl,str1
	call _pr
	pop af
	ld l,c
; (HL:short)
; stmt EXPR
; EXPR
	ld h,b
	ld de,6
; (A:ubyte)
; stmt RETURN
; RETURN hasval=0
	add hl,de
	bit 7,(hl)
; stmt EXPR
; EXPR
	jp z,no0_4
	push bc
	pop hl
	ld de,6
	add hl,de
	res 7,(hl)
; (REGVAR:short IX#1)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte (REGVAR:short IX)) 0:ubyte)
	ld l,c
	ld h,b
	ld de,21
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	add hl,de
	call _drain
	ld a,5
; (HL:short)
	ld (_u+547),a
; stmt EXPR
; EXPR
	jp	Xttyread
no0_4:
	ld l,c
; (HL:short)
; stmt IF
; IF nlbl=0
; (NE:ubyte (DEREF:ubyte (REGVAR:short IX)) 0:ubyte)
	ld h,b
	ld de,26
	add hl,de
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	push hl
	pop ix
	ld a,(ix+0)
	or a
	jp nz,no2_4
	ld l,c
; (HL:short)
	ld h,b
	call _cookin
no2_4:
	push ix
; top FUNC
; FUNC _cookin:s
; params=1 locals=6 frame=2 framefree=0
; param tty:s r=0 o=4
; local _cookin.2:s r=0
; local _cookin.1:s r=0
; local _cookin.0:s r=0
; local c:s r=3
; local raw:s r=4
; local cook:s r=0
	pop hl
	call _sendque
	ld a,(ix+0)
; stmt BLOCK
; BLOCK n=36
; stmt EXPR
; EXPR
	cp 0
	jp z,no4_4
	ld hl,7
	push hl
	ld l,c
	ld h,b
; (HL:short)
; stmt EXPR
; EXPR
	call _killall
	pop af
no4_4:
Xttyread:
	pop	ix
	pop	bc
	ret
; (REGVAR:short IX#1)
; stmt IF
; IF nlbl=0
; (AND:ushort (DEREF:ushort (ADD:short (DEREF:short (LOCALVAR:short IY+4)) 4:short)) 8224:short)
_cookin::
	call	fentbxw
	.dw	-2  
	ld de,26
	add hl,de
	ld (iy-2),l
	ld (iy-1),h
	ld l,(iy+4)
	ld h,(iy+5)
	ld de,21
	add hl,de
	push hl
	pop ix
	ld l,(iy+4)
	ld h,(iy+5)
	inc hl
	inc hl
; stmt BLOCK
; BLOCK n=10
; stmt EXPR
; EXPR
	inc hl
	inc hl
	ld a,(hl)
	inc hl
	ld h,(hl)
; (BC:short)
; stmt LABEL
	ld l,a
; stmt EXPR
; SEMI
; stmt BLOCK
; BLOCK n=4
; stmt EXPR
; EXPR
	ld a,l
; (HL:short)
; stmt IF
; IF nlbl=2
; (LAND:ubyte (DEREF:ubyte (ADD:short (DEREF:short (LOCALVAR:short IY+4)) 20:short)) (LOR:ubyte (EQ:ubyte (REGVAR:short BC) 10:short) (EQ:ubyte (REGVAR:short BC) 4:short)))
	and 32
	ld l,a
	ld a,h
	and 32
	ld h,a
	ld a,l
	or h
	jp z,no0_5
	ld l,(iy+4)
	ld h,(iy+5)
	call _cwait
	ld c,l
	ld b,h
@5__D2T:
	call _di
	ld l,(iy+4)
	ld h,(iy+5)
	ld de,20
	add hl,de
	ld a,(hl)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	or a
	jp z,no2_5
	ld l,c
	ld h,b
	ld de,10
	or a
	sbc hl,de
; (A:ubyte)
	jp z,_C0
; stmt EXPR
; EXPR
	ld l,c
; (HL:short)
; stmt EXPR
; EXPR
	ld h,b
	ld de,4
	or a
	sbc hl,de
	jp nz,no2_5
_C0:
	ld l,(iy+4)
; (HL:short)
; stmt LABEL
	ld h,(iy+5)
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (NE:ubyte (ASSIGN:short (REGVAR:short BC) (CALL:short/1 $_getc (REGVAR:short/s IX))) -1:short)
	ld de,20
	add hl,de
	ld a,(hl)
	dec a
	ld (hl),a
no2_5:
	call _ei
	ld l,c
	ld h,b
	push hl
	ld l,(iy-2)
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld h,(iy-1)
	call _putc
; stmt LABEL
	pop af
; stmt EXPR
; SEMI
; stmt RETURN
; RETURN hasval=0
@5__D2C:
	push ix
; stmt LABEL
	pop hl
; stmt LABEL
	call _getc
; stmt EXPR
; SEMI
; stmt BLOCK
; BLOCK n=6
; stmt EXPR
; EXPR
	ld c,l
; (HL:short)
; stmt IF
; IF nlbl=0
; (DEREF:ubyte (ADD:short (DEREF:short (LOCALVAR:short IY+4)) 20:short))
	ld b,h
	ld l,c
	ld h,b
	ld de,1
	add hl,de
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	jp nc,@5__D2T
no6_5:
; stmt EXPR
; EXPR
@5__D2B:
	jp	Xcookin
no0_5:
@5tryagain:
@5__F3T:
; (A:ubyte)
; stmt EXPR
; EXPR
	call _di
	ld l,(iy+4)
	ld h,(iy+5)
	ld de,20
	add hl,de
	ld a,(hl)
	or a
	jp nz,@5__F3B
no8_5:
	ld l,(iy+4)
; (HL:short)
; stmt EXPR
; EXPR
	ld h,(iy+5)
	ld de,6
	add hl,de
	set 2,(hl)
	ld l,(iy+4)
	ld h,(iy+5)
; (HL:short)
; stmt EXPR
; EXPR
	ld de,20
	add hl,de
	ld l,(hl)
	ld h,0
	push hl
	ld hl,str2
	call _pr
	pop af
	ld hl,40
	push hl
; (HL:short)
; stmt LABEL
	push ix
; stmt EXPR
; SEMI
; stmt GOTO
	pop hl
; stmt LABEL
	call _sleep
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	pop af
; (HL:short)
; stmt EXPR
; EXPR
	ld l,(iy+4)
	ld h,(iy+5)
; (HL:short)
; stmt LABEL
	ld de,20
; stmt EXPR
; SEMI
; stmt BLOCK
; BLOCK n=6
; stmt IF
; IF nlbl=0
; (LE:ubyte (ADD:ushort $_cookin.3 198:ushort) (DEREF:ushort $_cookin.4))
	add hl,de
	ld l,(hl)
	ld h,0
	push hl
	ld hl,str3
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	call _pr
	pop af
; (HL:short)
@5__F3C:
; stmt EXPR
; EXPR
	jp @5__F3T
; (HL:short)
; stmt EXPR
; EXPR
@5__F3B:
	call _ei
	ld hl,_cookin.3
	ld (_cookin.4),hl
@5__F4T:
; (BC:short)
; stmt IF
; IF nlbl=0
; (LT:ubyte (REGVAR:short BC) 0:short)
	ld de,(_cookin.4)
	ld hl,_cookin.3+198
	ex de,hl
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	or a
	sbc hl,de
	jp c,no10_5
	ld hl,_cookin.3
	ld (_cookin.4),hl
; (HL:ubyte)
; stmt EXPR
; EXPR
no10_5:
; (HL:short)
; stmt GOTO
	call _di
	push ix
; stmt EXPR
; EXPR
	pop hl
; (HL:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:short BC) 4:short)
	call _getc
	ld c,l
	ld b,h
	ld a,b
	or a
	jp p,no12_5
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld l,(iy+4)
	ld h,(iy+5)
	ld de,20
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:short BC) 10:short)
	add hl,de
	ld (hl),0
	call _ei
	jp @5tryagain
no12_5:
	call _ei
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld l,c
	ld h,b
	ld de,4
	or a
	sbc hl,de
	jp nz,no14_5
; (A:byte)
; stmt GOTO
	jp @5__F4B
	jp no15_5
no14_5:
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:short BC) (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (LOCALVAR:short IY+4)) 3:short))))
	ld l,c
	ld h,b
	ld de,10
	or a
	sbc hl,de
	jp nz,no16_5
	ld hl,(_cookin.4)
	inc hl
	ld (_cookin.4),hl
	dec hl
	ld a,c
	ld (hl),a
	jp @5__F4B
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp no17_5
no16_5:
; (HL:short)
	ld l,(iy+4)
	ld h,(iy+5)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:short BC) (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (LOCALVAR:short IY+4)) 2:short))))
	inc hl
	inc hl
	inc hl
	ld a,(hl)
	ld e,a
	ld d,0
	ld l,c
	ld h,b
	or a
	sbc hl,de
	jp nz,no18_5
	ld hl,_cookin.3
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=0
; (NE:ubyte (DEREF:short $_cookin.4) $_cookin.3)
	ld (_cookin.4),hl
	jp no19_5
no18_5:
	ld l,(iy+4)
	ld h,(iy+5)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	inc hl
	inc hl
	ld a,(hl)
; (HL:short)
	ld e,a
	ld d,0
	ld l,c
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:short BC) 92:short)
	ld h,b
	or a
	sbc hl,de
	jp nz,no20_5
	ld hl,(_cookin.4)
	ld de,_cookin.3
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	or a
	sbc hl,de
	jp z,no22_5
	ld hl,(_cookin.4)
	dec hl
; (BC:short)
; stmt IF
; IF nlbl=1
; (LOR:ubyte (EQ:ubyte (REGVAR:short BC) (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (LOCALVAR:short IY+4)) 2:short)))) (EQ:ubyte (REGVAR:short BC) (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (LOCALVAR:short IY+4)) 3:short)))))
	ld (_cookin.4),hl
no22_5:
	jp no21_5
no20_5:
	ld l,c
	ld h,b
	ld de,92
	or a
	sbc hl,de
	jp nz,no24_5
	push ix
	pop hl
	call _getc
	ld c,l
	ld b,h
	ld l,(iy+4)
	ld h,(iy+5)
	inc hl
	inc hl
	ld a,(hl)
	ld e,a
	ld d,0
	ld l,c
	ld h,b
	or a
	sbc hl,de
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp z,_C1
	ld l,(iy+4)
	ld h,(iy+5)
	inc hl
	inc hl
	inc hl
; (A:byte)
	ld a,(hl)
	ld e,a
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld d,0
	ld l,c
	ld h,b
	or a
	sbc hl,de
; (HL:byte)
; stmt EXPR
; EXPR
	jp nz,no26_5
_C1:
	ld hl,(_cookin.4)
	inc hl
	ld (_cookin.4),hl
	dec hl
; (A:byte)
	ld a,c
; stmt IF
; IF nlbl=1
; (LOR:ubyte (EQ:ubyte (REGVAR:short BC) 10:short) (EQ:ubyte (REGVAR:short BC) 4:short))
	ld (hl),a
	jp no27_5
no26_5:
	ld hl,(_cookin.4)
	inc hl
	ld (_cookin.4),hl
	dec hl
	ld (hl),92
	ld hl,(_cookin.4)
	inc hl
	ld (_cookin.4),hl
	dec hl
	ld a,c
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld (hl),a
no27_5:
	ld l,c
	ld h,b
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld de,10
	or a
	sbc hl,de
	jp z,_C2
	ld l,c
	ld h,b
; (A:byte)
	ld de,4
	or a
	sbc hl,de
	jp nz,no29_5
_C2:
; stmt LABEL
	jp @5__F4B
; stmt EXPR
; SEMI
; stmt GOTO
no29_5:
; stmt LABEL
	jp no25_5
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
no24_5:
; (HL:short)
; stmt IF
; IF nlbl=0
; (DEREF:ubyte (ADD:short (DEREF:short (LOCALVAR:short IY+4)) 20:short))
	ld hl,(_cookin.4)
	inc hl
	ld (_cookin.4),hl
	dec hl
	ld a,c
	ld (hl),a
no25_5:
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
no21_5:
no19_5:
no17_5:
no15_5:
@5__F4C:
	jp @5__F4T
@5__F4B:
; (A:ubyte)
	call _di
; stmt EXPR
; EXPR
	ld l,(iy+4)
; (HL:short)
; stmt EXPR
; EXPR
	ld h,(iy+5)
	ld de,20
; (HL:short)
; stmt LABEL
	add hl,de
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (LT:ubyte (DEREF:ushort $_cookin.5) (DEREF:ushort $_cookin.4)))
	ld a,(hl)
	or a
	jp z,no32_5
	ld l,(iy+4)
	ld h,(iy+5)
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld de,20
	add hl,de
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld a,(hl)
	dec a
	ld (hl),a
no32_5:
	call _ei
	ld hl,_cookin.3
	ld (_cookin.5),hl
@5__F5T:
	ld hl,(_cookin.5)
; (HL:short)
; stmt LABEL
	ld de,(_cookin.4)
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	or a
	sbc hl,de
; (HL:short)
; stmt GOTO
	jp nc,@5__F5B
; stmt LABEL
no34_5:
; stmt EXPR
; SEMI
	ld hl,(_cookin.5)
	ld l,(hl)
; top FUNC
; FUNC _cwait:s
; params=1 locals=1 frame=0 framefree=0
; param tty:s r=3 o=4
; local raw:s r=4
	ld h,0
	push hl
	ld l,(iy-2)
	ld h,(iy-1)
	call _putc
; stmt BLOCK
; BLOCK n=10
; stmt EXPR
; EXPR
	pop af
@5__F5C:
	ld hl,(_cookin.5)
	inc hl
	ld (_cookin.5),hl
	jp @5__F5T
; (REGVAR:short IX#1)
; stmt LABEL
@5__F5B:
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (COMMA:short (CALL:short/0 $_di) (WIDEN:short (EQ:ubyte (DEREF:ubyte (REGVAR:short IX)) 0:ubyte))))
Xcookin:
	call	fexbxw
	.dw	-6
_cwait::
	push	bc     
	push	ix
	ld c,l
	ld b,h
	ld l,c
	ld h,b
	ld de,21
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	add hl,de
	push hl
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	pop ix
@6__W6T:
	call _di
	ld a,(ix+0)
	cp 0
; (A:ubyte)
; stmt EXPR
; EXPR
	ld a,0
	jr nz,$+3
	inc a
	ld l,a
	ld h,0
	ld a,l
; (HL:short)
; stmt GOTO
	or h
; stmt LABEL
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	jp z,@6__W6B
; (HL:short)
; stmt RETURN
; RETURN hasval=1
no0_6:
	push bc
	pop hl
; (HL:short/v)
	ld de,6
	add hl,de
	set 2,(hl)
	ld hl,40
; top FUNC
; FUNC _ttymode:s
; params=2 locals=0 frame=0 framefree=0
; param tty:s r=4 o=4
; param flag:s r=3 o=6
	push hl
	push ix
	pop hl
	call _sleep
	pop af
	jp @6__W6T
@6__W6B:
	call _ei
	push ix
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	pop hl
	call _getc
Xcwait:
	pop	ix
	pop	bc
	ret
_ttymode::
	call	fentbxw
; (HL:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:short BC) 1:short)
	.dw	0  
	ld	c,(iy+6)
	ld	b,(iy+7)
	push	hl
	pop	ix
	ld hl,6
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	push hl
	push ix
	ld l,c
	ld h,b
	call _iomove
; (HL:short)
	pop af
	pop af
	ld l,c
	ld h,b
; top FUNC
; FUNC _ttyout:s
; params=1 locals=2 frame=1 framefree=0
; param tty:s r=3 o=4
; local cold:b r=0
; local state:s r=4
	ld de,1
	or a
	sbc hl,de
	jp nz,no0_7
	ld l,(ix+17)
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	ld h,(ix+18)
	push hl
	push ix
	pop hl
	pop de
	call trampde
; (REGVAR:short IX#1)
; stmt IF
; IF nlbl=2
; (LAND:ubyte (LNOT:short (AND:short (SEXT:short (DEREF:byte (REGVAR:short IX))) 1:short)) (LOR:ubyte (DEREF:ubyte (ADD:short (REGVAR:short BC) 31:short)) (DEREF:ubyte (ADD:short (REGVAR:short BC) 19:short))))
no0_7:
Xttymode:
	call	fexbxw
	.dw	-4
_ttyout::
	call	fentbxw
	.dw	-1  
	ld c,l
	ld b,h
	ld l,c
	ld h,b
	ld de,6
	add hl,de
	push hl
	pop ix
	ld a,(ix+0)
	and 1
	jp nz,no0_8
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=1
; (LOR:ubyte (AND:ushort (DEREF:ushort (ADD:short (REGVAR:short BC) 4:short)) 4114:short) (DEREF:ubyte (ADD:short (REGVAR:short BC) 19:short)))
	ld l,c
	ld h,b
	ld de,31
	add hl,de
	ld a,(hl)
	or a
	jp nz,_C3
	ld l,c
	ld h,b
	ld de,19
	add hl,de
	ld a,(hl)
	or a
	jp z,no0_8
_C3:
	ld l,c
	ld h,b
	ld de,4
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld a,l
	and 18
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld l,a
	ld a,h
	and 16
	ld h,a
	ld a,l
	or h
	jp nz,_C4
	ld l,c
; (HL:short)
	ld h,b
	ld de,19
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	add hl,de
	ld a,(hl)
	or a
	jp z,no4_8
_C4:
	ld l,c
	ld h,b
	call _cookout
	push hl
	ld l,c
; (HL:short)
	ld h,b
	call _mputc
	pop af
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	jp no5_8
no4_8:
	ld l,c
	ld h,b
	ld de,31
	add hl,de
	call _getc
	push hl
	ld l,c
	ld h,b
	call _mputc
	pop af
no5_8:
	jp no1_8
no0_8:
	ld a,(ix+0)
	ld l,a
	rla
	sbc a,a
	ld h,a
	ld a,l
	and 64
	ld l,a
	xor a
	ld h,a
	ld a,l
	or h
	jp nz,_L7
	ld l,c
	ld h,b
	ld de,31
	add hl,de
	ld a,(hl)
	or a
	jp nz,_L7
	xor a
	inc a
	jp _L8
_L7:
	xor a
_L8:
; (A:byte)
; stmt EXPR
; EXPR
	jp z,_L5
	ld l,c
	ld h,b
	ld de,19
	add hl,de
	ld a,(hl)
	or a
	jp nz,_L5
	xor a
	inc a
	jp _L6
_L5:
	xor a
_L6:
	ld (iy-1),a
	ld l,(iy-1)
	ld h,0
; (HL:short)
; stmt IF
; IF nlbl=0
; (DEREF:byte (LOCALVAR:byte IY-1))
	push hl
	ld l,c
	ld h,b
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld de,13
	add hl,de
	ld a,(hl)
; (HL:short)
; stmt EXPR
; EXPR
	inc hl
; (HL:byte)
	ld h,(hl)
	ld l,a
; stmt IF
; IF nlbl=1
; (LAND:ubyte (AND:short (SEXT:short (DEREF:byte (REGVAR:short IX))) 2:short) (LE:ubyte (DEREF:ubyte (ADD:short (REGVAR:short BC) 31:short)) 16:ubyte))
	push hl
	ld l,c
	ld h,b
	pop de
	call trampde
	pop af
	ld a,(iy-1)
	or a
	jp z,no7_8
	ld l,c
	ld h,b
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	call _drainques
; (A:byte)
; stmt EXPR
; EXPR
	ld (ix+0),0
no7_8:
no1_8:
	bit 1,(ix+0)
	jp z,no9_8
; (HL:short)
	ld l,c
	ld h,b
	ld de,31
	add hl,de
; top FUNC
; FUNC _cookout:s
; params=1 locals=5 frame=6 framefree=0
; param tty:s r=4 o=4
; local _cookout.6:s r=0
; local inc:s r=0
; local c:b r=1
; local mode:s r=0
; local que:s r=0
	ld a,(hl)
	cp 16
	jr nz,$+3
	scf
	jp nc,no9_8
	res 1,(ix+0)
	ld l,c
; stmt BLOCK
; BLOCK n=10
; stmt LABEL
	ld h,b
; stmt EXPR
; EXPR
	ld de,31
	add hl,de
	call _wakeup
no9_8:
Xttyout:
	call	fexbxw
; (HL:short)
; stmt EXPR
; EXPR
	.dw	-5
_cookout::
; (HL:short)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (AND:short (DEREF:short (LOCALVAR:short IY-4)) 4096:ushort) (LT:ubyte 21:ubyte (DEREF:ubyte (ADD:short (REGVAR:short IX) 10:short))))
	call	fentbxw
	.dw	-6  
	push	hl
	pop	ix
@9loop:
	push ix
	pop hl
	ld de,31
	add hl,de
	ld (iy-6),l
	ld (iy-5),h
	ld l,(ix+4)
	ld h,(ix+5)
	ld (iy-4),l
	ld (iy-3),h
	ld l,(iy-4)
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld h,(iy-3)
; (HL:ubyte)
; stmt EXPR
; EXPR
; (HL:ubyte)
	xor a
; stmt IF
; IF nlbl=0
; (DEREF:ubyte (ADD:short (REGVAR:short IX) 19:short))
	ld l,a
	ld a,h
	and 16
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld h,a
	ld a,l
; (A:byte)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:byte B) 10:ubyte)
	or h
	jp z,no0_9
	ld a,(ix+10)
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	cp 21
; (HL:ubyte)
; stmt EXPR
; EXPR
	jr nz,$+3
; (HL:ubyte)
; stmt RETURN
; RETURN hasval=1
	scf
; (HL:short/v)
	jp c,no0_9
	ld (ix+10),0
	ld (ix+19),32
; stmt IF
; IF nlbl=0
; (LT:ubyte (REGVAR:byte B) 8:ubyte)
no0_9:
	ld a,(ix+19)
	or a
	jp z,no3_9
	ld b,(ix+19)
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld a,b
; (HL:ubyte)
; stmt RETURN
; RETURN hasval=1
	cp 10
; (HL:short/v)
	jp nz,no5_9
	ld (ix+19),0
	inc (ix+10)
; stmt BLOCK
; BLOCK n=4
; stmt EXPR
; EXPR
	ld hl,10
	jp	Xcookout
	jp no6_9
; (A:byte)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:byte B) 7:ubyte)
no5_9:
	ld a,b
	sub 8
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp po,$+5
; (A:ubyte)
	xor 80h
; stmt IF
; IF nlbl=0
; (LT:ubyte (REGVAR:byte B) 31:ubyte)
	or a
	jp p,no7_9
	dec (ix+19)
	ld hl,32
	jp	Xcookout
	jp no8_9
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
no7_9:
; (HL:ubyte)
	ld a,b
	sub 32
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld b,a
; (HL:ubyte)
	ld a,b
; stmt RETURN
; RETURN hasval=1
	cp 7
	jp nz,no9_9
	set 0,(ix+6)
no9_9:
	ld a,b
	sub 31
	jp po,$+5
	xor 80h
	or a
	jp p,no11_9
	inc (ix+19)
	jp no12_9
; (HL:short/v)
no11_9:
	ld (ix+19),0
no12_9:
	ld a,b
; stmt EXPR
; EXPR
	ld e,a
	rla
	sbc a,a
	ld d,a
; (HL:byte)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (LE:ubyte 32:ubyte (REGVAR:byte B)) (LE:ubyte (REGVAR:byte B) 127:ubyte))
	ld hl,(_cookout.7)
	add hl,de
	ld a,(hl)
	ld l,a
	rla
	sbc a,a
	ld h,a
	jp	Xcookout
no8_9:
no6_9:
no3_9:
	ld l,(iy-6)
	ld h,(iy-5)
	call _getc
	ld b,l
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld a,b
; (HL:ubyte)
; stmt RETURN
; RETURN hasval=1
	sub 32
	jp po,$+5
	xor 80h
	or a
	jp m,no13_9
; (HL:short/v)
	ld a,b
	sub 127
; stmt SWITCH
; SWITCH n=4
	jp po,$+5
	xor 80h
	or a
	jp m,$+5
	jr $+3
; (HL:short/v)
	xor a
; stmt CASE
; CASE n=1
	jp nz,no13_9
; stmt IF
; IF nlbl=0
; (AND:short (DEREF:short (LOCALVAR:short IY-4)) 16:short)
	inc (ix+7)
	ld a,b
	ld l,a
	rla
	sbc a,a
	ld h,a
	jp	Xcookout
no13_9:
	ld a,b
	ld l,a
	rla
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	sbc a,a
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld h,a
; (HL:ubyte)
; stmt RETURN
; RETURN hasval=1
	jp _D16_9
; (HL:short/v)
_K16_9_0:
	ld l,(iy-4)
	ld h,(iy-3)
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld a,l
; (HL:ubyte)
; stmt RETURN
; RETURN hasval=1
	and 16
; (HL:short/v)
	ld l,a
; stmt CASE
; CASE n=3
	xor a
; stmt EXPR
; EXPR
	ld h,a
	ld a,l
	or h
	jp z,no17_9
	ld (ix+19),10
	ld (ix+7),0
	ld hl,13
	jp	Xcookout
	jp no18_9
no17_9:
	inc (ix+10)
	ld hl,10
	jp	Xcookout
no18_9:
_K16_9_1:
; (HL:short)
; stmt EXPR
; EXPR
	ld l,(ix+7)
	ld h,0
	ld a,l
	and 7
	ld l,a
; (A:ubyte)
; stmt IF
; IF nlbl=0
; (AND:short (DEREF:short (LOCALVAR:short IY-4)) 2:short)
	xor a
	ld h,a
	ex de,hl
	ld hl,8
	or a
	sbc hl,de
	ld (iy-2),l
	ld (iy-1),h
	ld a,(ix+7)
	push af
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld e,(iy-2)
	pop af
	add a,e
; (A:ubyte)
; stmt RETURN
; RETURN hasval=1
	ld (ix+7),a
; (HL:short/v)
	ld l,(iy-4)
	ld h,(iy-3)
	ld a,l
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	and 2
; (HL:short/v)
	ld l,a
; stmt CASE
; CASE n=2
	xor a
; stmt IF
; IF nlbl=0
; (NE:ubyte (DEREF:ubyte (ADD:short (REGVAR:short IX) 7:short)) 0:ubyte)
	ld h,a
	ld a,l
	or h
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp z,no19_9
; (HL:ubyte)
	ld a,(iy-2)
; stmt RETURN
; RETURN hasval=1
	dec a
; (HL:short/v)
	ld (ix+19),a
; stmt DEFAULT
; DEFAULT n=1
	ld hl,32
; stmt RETURN
; RETURN hasval=1
	jp	Xcookout
	jp no20_9
no19_9:
	ld hl,9
	jp	Xcookout
; (HL:short/v)
no20_9:
_K16_9_2:
	ld a,(ix+7)
	cp 0
	jp z,no21_9
	dec (ix+7)
no21_9:
	ld hl,8
	jp	Xcookout
_F16_9:
	ld a,b
	ld l,a
	rla
	sbc a,a
	ld h,a
	jp	Xcookout
; stmt LABEL
	jp _X16_9
; stmt EXPR
; SEMI
_D16_9:
	ld a,h
	or a
; top FUNC
; FUNC _ttyerror:s
; params=1 locals=0 frame=0 framefree=1
; param t:s r=4 o=4
	jp nz,_N16_9
	ld a,l
	call swidx
	.db 8
	.db 3
	.dw _K16_9_2
	.dw _K16_9_1
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	.dw _K16_9_0
; (A:ubyte)
_N16_9:
	jp _F16_9
_X16_9:
; top FUNC
; FUNC _ttyconnect:s
; params=1 locals=0 frame=0 framefree=0
; param t:s r=4 o=4
@9__S7B:
Xcookout:
	call	fexbxw
	.dw	-10
_ttyerror::
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	push	ix    
; (A:ubyte)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (AND:ubyte (DEREF:ubyte (ADD:short (REGVAR:short IX) 37:short)) 33:ubyte) 1:ubyte)
        
	push	hl
	pop	ix
	set 7,(ix+6)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
Xttyerror:
	pop	ix
	ret
_ttyconnect::
; (HL:short)
	push	ix    
        
	push	hl
; top FUNC
; FUNC _ttyhangup:s
; params=1 locals=0 frame=0 framefree=0
; param t:s r=4 o=4
	pop	ix
	set 1,(ix+37)
	ld a,(ix+37)
	and 33
	cp 1
	jp nz,no0_11
	push ix
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	pop hl
; (A:ubyte)
; stmt IF
; IF nlbl=0
; (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 37:short))) 32:short))
	ld de,37
	add hl,de
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	call _wakeup
no0_11:
Xttyconnect:
; (HL:short)
; stmt EXPR
; EXPR
	pop	ix
	ret
_ttyhangup::
	push	ix    
; (HL:short)
        
	push	hl
	pop	ix
	res 1,(ix+37)
; top FUNC
; FUNC _ttyin:s
; params=2 locals=4 frame=5 framefree=0
; param c:s r=0 o=4
; param tty:s r=3 o=6
; local mode:s r=0
; local state:b r=0
; local out:s r=4
; local raw:s r=0
	bit 5,(ix+37)
	jp nz,no0_12
	push ix
	pop hl
	call _drainques
; stmt BLOCK
; BLOCK n=14
; stmt EXPR
; EXPR
	ld hl,9
	push hl
	push ix
	pop hl
	call _killall
	pop af
; (HL:short)
; stmt EXPR
; EXPR
no0_12:
Xttyhangup:
	pop	ix
	ret
_ttyin::
	call	fentbxw
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	.dw	-5  
	ld	c,(iy+6)
	ld	b,(iy+7)
	ld l,c
	ld h,b
	ld de,21
	add hl,de
	ld (iy-5),l
	ld (iy-4),h
	ld l,c
; (HL:short)
; stmt EXPR
; EXPR
	ld h,b
	ld de,31
	add hl,de
	push hl
	pop ix
	ld l,c
; (A:byte)
; stmt IF
; IF nlbl=0
; (LE:ubyte 200:ubyte (DEREF:ubyte (DEREF:short (LOCALVAR:short IY-5))))
	ld h,b
	ld de,4
	add hl,de
	ld a,(hl)
	inc hl
; stmt BLOCK
; BLOCK n=5
; stmt EXPR
; EXPR
	ld h,(hl)
; (HL:short)
; stmt EXPR
; EXPR
	ld l,a
	ld (iy-2),l
	ld (iy-1),h
; (HL:short)
; stmt EXPR
; EXPR
	ld l,c
	ld h,b
	ld de,6
	add hl,de
	ld a,(hl)
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld (iy-3),a
; (HL:short)
; stmt RETURN
; RETURN hasval=0
	ld l,(iy-5)
	ld h,(iy-4)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (AND:short (DEREF:short (LOCALVAR:short IY-2)) 16384:ushort) 0:short)
	ld a,(hl)
	cp 200
	jp c,no0_13
	call _di
	ld l,(iy-5)
	ld h,(iy-4)
	call _drain
	ld l,c
	ld h,b
	ld de,20
	add hl,de
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld (hl),0
	call _ei
	jp	Xttyin
no0_13:
	ld l,(iy-2)
	ld h,(iy-1)
	xor a
	ld l,a
	ld a,h
; (HL:short)
	and 64
; stmt IF
; IF nlbl=1
; (LAND:ubyte (EQ:ubyte (DEREF:short (LOCALVAR:short IY+4)) 13:short) (AND:short (DEREF:short (LOCALVAR:short IY-2)) 16:short))
	ld h,a
	ld a,l
	or h
	jp nz,no2_13
	ld l,(iy+4)
	ld h,(iy+5)
	ld a,l
	and 127
	ld l,a
	xor a
	ld h,a
	ld (iy+4),l
	ld (iy+5),h
no2_13:
	ld l,(iy+4)
	ld h,(iy+5)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld de,13
	or a
; (HL:short)
	sbc hl,de
; stmt IF
; IF nlbl=0
; (EQ:ubyte (AND:ubyte (DEREF:ubyte (LOCALVAR:short IY-2)) 32:ubyte) 0:ubyte)
	jp nz,no4_13
	ld l,(iy-2)
	ld h,(iy-1)
	ld a,l
; stmt BLOCK
; BLOCK n=3
; stmt SWITCH
; SWITCH n=4
	and 16
	ld l,a
; (HL:short/v)
; stmt CASE
; CASE n=3
	xor a
; stmt EXPR
; EXPR
	ld h,a
	ld a,l
	or h
	jp z,no4_13
	ld (iy+4),10
	ld (iy+5),0
; (HL:short)
; stmt EXPR
; EXPR
no4_13:
	ld a,(iy-2)
	and 32
; (HL:short)
; stmt RETURN
; RETURN hasval=0
	or a
; stmt CASE
; CASE n=3
	jp nz,no7_13
; stmt EXPR
; EXPR
	ld l,(iy+4)
	ld h,(iy+5)
	jp _D9_13
_K9_13_0:
	ld hl,3
	push hl
; (HL:short)
; stmt EXPR
; EXPR
	ld l,c
	ld h,b
	call _killall
; (HL:short)
; stmt RETURN
; RETURN hasval=0
	pop af
; stmt CASE
; CASE n=3
	ld l,c
; stmt EXPR
; EXPR
	ld h,b
	call _ustart
	jp	Xttyin
_K9_13_1:
	ld hl,2
; (A:ubyte)
; stmt EXPR
; EXPR
	push hl
	ld l,c
	ld h,b
	call _killall
	pop af
	ld l,c
	ld h,b
	call _ustart
	jp	Xttyin
_K9_13_2:
	push bc
	pop hl
	ld de,6
	add hl,de
	set 0,(hl)
	ld hl,0
; (HL:short)
; stmt RETURN
; RETURN hasval=0
	push hl
; stmt CASE
; CASE n=2
	ld l,c
; stmt IF
; IF nlbl=0
; (AND:short (SEXT:short (DEREF:byte (LOCALVAR:byte IY-3))) 1:short)
	ld h,b
	ld de,13
	add hl,de
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld a,(hl)
	inc hl
	ld h,(hl)
; (HL:short)
	ld l,a
; stmt RETURN
; RETURN hasval=0
	push hl
	ld l,c
	ld h,b
	pop de
	call trampde
	pop af
	jp	Xttyin
_K9_13_3:
	ld a,(iy-3)
	and 1
	jp z,no10_13
	ld l,c
	ld h,b
	call _ustart
no10_13:
	jp	Xttyin
	jp _X9_13
_D9_13:
	ld a,h
; stmt LABEL
	or a
; stmt EXPR
; SEMI
	jp nz,_N9_13
; stmt EXPR
; EXPR
	ld a,l
; (HL:short)
; stmt IF
; IF nlbl=1
; (LOR:ubyte (EQ:ubyte (DEREF:short (LOCALVAR:short IY+4)) 4:short) (EQ:ubyte (DEREF:short (LOCALVAR:short IY+4)) 10:short))
	call swtab
	.db 4
	.db 28
	.db 127
	.db 19
	.db 17
	.dw _K9_13_3
	.dw _K9_13_2
	.dw _K9_13_1
	.dw _K9_13_0
_N9_13:
_X9_13:
@13__S8B:
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
no7_13:
	call _di
	ld l,(iy+4)
	ld h,(iy+5)
	ld de,4
	or a
	sbc hl,de
; (A:ubyte)
	jp z,_C9
; stmt EXPR
; EXPR
	ld l,(iy+4)
	ld h,(iy+5)
	ld de,10
	or a
	sbc hl,de
	jp nz,no12_13
_C9:
; (HL:short)
; stmt EXPR
; EXPR
	ld l,c
; (HL:short)
; stmt IF
; IF nlbl=0
; (AND:short (DEREF:short (LOCALVAR:short IY-2)) 8:short)
	ld h,b
	ld de,20
	add hl,de
	ld a,(hl)
	inc a
	ld (hl),a
no12_13:
	ld l,(iy+4)
	ld h,(iy+5)
	push hl
	ld l,(iy-5)
; stmt BLOCK
; BLOCK n=2
; stmt IF
; IF nlbl=0
; (AND:short (DEREF:short (LOCALVAR:short IY-2)) 32:short)
	ld h,(iy-4)
	call _putc
	pop af
	call _ei
	ld l,(iy-2)
	ld h,(iy-1)
	ld a,l
	and 8
	ld l,a
	xor a
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld h,a
	ld a,l
	or h
	jp z,no15_13
	ld l,(iy-2)
	ld h,(iy-1)
	ld a,l
; (HL:short)
	and 32
	ld l,a
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:short (LOCALVAR:short IY+4)) (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 3:short))))
	xor a
	ld h,a
	ld a,l
	or h
	jp z,no17_13
	ld l,(iy+4)
	ld h,(iy+5)
	push hl
	push ix
	pop hl
	call _putc
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	pop af
	jp no18_13
no17_13:
	ld l,c
	ld h,b
	ld de,3
; (HL:short)
	add hl,de
	ld a,(hl)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:short (LOCALVAR:short IY+4)) (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 2:short))))
	ld e,a
	ld d,0
	ld l,(iy+4)
	ld h,(iy+5)
	or a
	sbc hl,de
	jp nz,no19_13
	ld hl,10
	push hl
	push ix
	pop hl
	call _putc
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	pop af
	jp no20_13
no19_13:
	ld l,c
	ld h,b
	ld de,2
; (HL:short)
; stmt EXPR
; EXPR
	add hl,de
	ld a,(hl)
	ld e,a
	ld d,0
	ld l,(iy+4)
	ld h,(iy+5)
; (HL:short)
; stmt EXPR
; EXPR
	or a
	sbc hl,de
	jp nz,no21_13
	ld hl,8
	push hl
	push ix
; (HL:short)
	pop hl
	call _putc
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	pop af
	ld hl,32
	push hl
	push ix
	pop hl
	call _putc
	pop af
; (HL:short)
	ld hl,8
	push hl
	push ix
; stmt EXPR
; EXPR
	pop hl
	call _putc
	pop af
; (HL:short)
	jp no22_13
; stmt IF
; IF nlbl=3
; (LOR:ubyte (LOR:ubyte (LOR:ubyte (AND:short (DEREF:short (LOCALVAR:short IY-2)) 32:short) (AND:short (DEREF:short (LOCALVAR:short IY-2)) 8192:ushort)) (EQ:ubyte (DEREF:short (LOCALVAR:short IY+4)) 10:short)) (EQ:ubyte (DEREF:short (LOCALVAR:short IY+4)) 4:short))
no21_13:
	ld l,(iy+4)
	ld h,(iy+5)
	push hl
	push ix
	pop hl
	call _putc
	pop af
no22_13:
no20_13:
no18_13:
	ld l,c
	ld h,b
	call _cstart
no15_13:
	ld l,(iy-2)
	ld h,(iy-1)
	ld a,l
	and 32
	ld l,a
	xor a
	ld h,a
	ld a,l
	or h
	jp nz,_C10
	ld l,(iy-2)
	ld h,(iy-1)
	xor a
	ld l,a
	ld a,h
	and 32
	ld h,a
	ld a,l
; stmt BLOCK
; BLOCK n=3
; stmt IF
; IF nlbl=0
; (AND:short (SEXT:short (DEREF:byte (LOCALVAR:byte IY-3))) 4:short)
	or h
	jp nz,_C10
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld l,(iy+4)
	ld h,(iy+5)
	ld de,10
	or a
	sbc hl,de
; (A:ubyte)
; stmt EXPR
; EXPR
	jp z,_C10
	ld l,(iy+4)
	ld h,(iy+5)
; (HL:short)
	ld de,4
; stmt EXPR
; EXPR
	or a
	sbc hl,de
	jp nz,no23_13
_C10:
	bit 2,(iy-3)
	jp z,no28_13
; (HL:short)
; stmt EXPR
; EXPR
	push bc
	pop hl
	ld de,6
	add hl,de
	res 2,(hl)
; (HL:ubyte)
	ld l,(iy-5)
	ld h,(iy-4)
	call _wakeup
no28_13:
; top FUNC
; FUNC _cstart:s
; params=1 locals=0 frame=0 framefree=0
; param tty:s r=4 o=4
	ld hl,7
	push hl
	ld l,c
	ld h,b
	call _killall
	pop af
	ld l,c
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=0
; (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 6:short))) 1:short))
	ld h,b
	ld de,10
	add hl,de
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld (hl),0
no23_13:
Xttyin:
	call	fexbxw
	.dw	-9
_cstart::
; (HL:short)
	push	ix    
        
	push	hl
; top FUNC
; FUNC _ustart:s
; params=1 locals=0 frame=0 framefree=0
; param tty:s r=4 o=4
	pop	ix
	ld a,(ix+6)
	and 1
	jp nz,no0_14
	ld l,(ix+11)
	ld h,(ix+12)
	push hl
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	push ix
; (A:ubyte)
; stmt EXPR
; EXPR
	pop hl
	pop de
	call trampde
no0_14:
Xcstart:
	pop	ix
	ret
; (HL:short)
_ustart::
; top FUNC
; FUNC _outwait:s
; params=2 locals=1 frame=0 framefree=0
; param tty:s r=0 o=4
; param count:s r=3 o=6
; local out:s r=4
	push	ix    
        
	push	hl
	pop	ix
	res 0,(ix+6)
; stmt BLOCK
; BLOCK n=9
; stmt EXPR
; EXPR
	ld l,(ix+11)
	ld h,(ix+12)
	push hl
	push ix
	pop hl
	pop de
; (REGVAR:short IX#1)
; stmt LABEL
	call trampde
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (COMMA:short (CALL:short/0 $_di) (WIDEN:short (LT:ubyte (REGVAR:short BC) (WIDEN:short (DEREF:ubyte (REGVAR:short IX)))))))
Xustart:
	pop	ix
	ret
_outwait::
	call	fentbxw
	.dw	0  
	ld	c,(iy+6)
	ld	b,(iy+7)
	ld de,31
	add hl,de
	push hl
	pop ix
@16__W9T:
	call _di
	ld a,(ix+0)
	ld e,a
	ld d,0
	ld l,c
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld h,b
	or a
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	sbc hl,de
	ld a,h
	jp po,$+5
	xor 80h
	or a
; (A:ubyte)
; stmt EXPR
; EXPR
	ld a,0
	jp p,$+4
	inc a
	ld l,a
	ld h,0
	ld a,l
; (HL:short)
; stmt GOTO
	or h
; stmt LABEL
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	jp z,@16__W9B
; (HL:short)
no0_16:
	ld l,(iy+4)
	ld h,(iy+5)
; EOF
	ld de,6
	add hl,de
	set 1,(hl)
	ld hl,40
	push hl
	push ix
	pop hl
	call _sleep
	pop af
	jp @16__W9T
@16__W9B:
	call _ei
Xoutwait:
	call	fexbxw
	.dw	-4
