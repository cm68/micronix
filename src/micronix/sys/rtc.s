; top FUNC
; FUNC _clkcmd:v
; params=1 locals=0 frame=0 framefree=0
; param cmd:s r=3 o=4
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
; (HL:short)
; stmt EXPR
; EXPR
	.data
_clkcmd:
	push	bc    
        
	ld c,l
	ld b,h
	ld l,c
	ld h,b
	push hl
; (HL:short)
; stmt EXPR
; EXPR
	call oarg
	.dw 74
	pop af
	ld l,c
	ld h,b
	ld a,l
; (HL:short)
	or 32
	ld l,a
	ld a,h
; top FUNC
; FUNC _clkshift:v
; params=0 locals=0 frame=0 framefree=1
	or 0
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld h,a
	push hl
	call oarg
	.dw 74
	pop af
; (HL:short)
; stmt EXPR
; EXPR
	ld l,c
	ld h,b
	push hl
	call oarg
	.dw 74
; (HL:short)
	pop af
Xclkcmd:
; top FUNC
; FUNC _rtcget:v
; params=1 locals=1 frame=0 framefree=0
; param r:s r=0 o=4
; local b:s r=3
	pop	bc
	ret
_clkshift:
; stmt BLOCK
; BLOCK n=24
; stmt EXPR
; EXPR
	ld hl,2
	push hl
; (HL:void)
; stmt EXPR
; EXPR
	call oarg
	.dw 74
; (HL:void)
; stmt EXPR
; EXPR
	pop af
; (BC:short)
; stmt LABEL
	ld hl,0
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (LT:ubyte (REGVAR:short BC) 5:short))
	push hl
	call oarg
	.dw 74
	pop af
Xclkshift:
	ret
_rtcget:
	call	fentbw
	.dw	0  
	ld hl,4
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	call _clkcmd
	ld hl,12
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	call _clkcmd
	ld bc,0
@3__F1T:
	ld l,c
; (HL:ubyte)
; stmt LABEL
	ld h,b
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	ld de,5
; (HL:short)
; stmt GOTO
	or a
; stmt LABEL
	sbc hl,de
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	ld a,h
; (BC:short)
; stmt LABEL
	jp po,$+5
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (LT:ubyte (REGVAR:short BC) 40:short))
	xor 80h
	or a
	jp p,@3__F1B
no0_3:
	ld l,(iy+4)
	ld h,(iy+5)
	add hl,bc
	ld (hl),0
@3__F1C:
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	inc bc
	jp @3__F1T
; stmt BLOCK
; BLOCK n=2
; stmt IF
; IF nlbl=0
; (AND:short (CALL:short/1 $_in 74:ubyte/s) 1:short)
@3__F1B:
	ld bc,0
@3__F2T:
	ld l,c
	ld h,b
	ld de,40
	or a
	sbc hl,de
	ld a,h
	jp po,$+5
	xor 80h
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	or a
	jp p,@3__F2B
no2_3:
	ld hl,74
	call _in
	ld a,l
	and 1
	ld l,a
	xor a
	ld h,a
	ld a,l
	or h
	jp z,no4_3
	ld l,c
	ld h,b
	ld de,8
	call adiv
	ex de,hl
	ld l,(iy+4)
	ld h,(iy+5)
	add hl,de
	push hl
	ld a,(hl)
	push af
	ld l,c
	ld h,b
	ld de,8
; (A:ubyte)
	call amod
; stmt EXPR
; EXPR
	ex de,hl
; (HL:void)
; stmt LABEL
	push bc
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	ld b,e
; (HL:short)
; stmt GOTO
	ld a,1
; stmt LABEL
	inc b
; stmt EXPR
; SEMI
	jr $+4
	sla a
	djnz $-2
; top FUNC
; FUNC _rtcset:v
; params=1 locals=2 frame=2 framefree=0
; param r:s r=0 o=4
; local b:s r=3
; local L0:s r=0
	pop bc
	ld e,a
	pop af
; stmt BLOCK
; BLOCK n=13
; stmt EXPR
; EXPR
	or e
	pop hl
; (HL:void)
; stmt EXPR
; EXPR
	ld (hl),a
; (BC:short)
; stmt LABEL
no4_3:
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (LT:ubyte (REGVAR:short BC) 40:short))
	call _clkshift
@3__F2C:
	inc bc
	jp @3__F2T
@3__F2B:
Xrtcget:
	call	fexbw
	.dw	-2
_rtcset:
	call	fentbw
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	.dw	-2  
	ld hl,4
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	call _clkcmd
	ld bc,0
@4__F3T:
	ld l,c
	ld h,b
	ld de,40
	or a
	sbc hl,de
	ld a,h
	jp po,$+5
	xor 80h
	or a
	jp p,@4__F3B
no0_4:
	ld l,c
	ld h,b
	ld de,8
	call adiv
	ex de,hl
	ld l,(iy+4)
	ld h,(iy+5)
	add hl,de
	ld a,(hl)
	ld l,a
	ld h,0
	push hl
	ld l,c
	ld h,b
	ld de,8
	call amod
	pop de
	ex de,hl
; (HL:short)
; stmt EXPR
; EXPR
	ld a,e
	or a
	jr z,$+9
	sra h
	rr l
	dec a
	jr nz,$-5
	ld a,l
	and 1
	ld l,a
	xor a
	ld h,a
	ld (iy-2),l
	ld (iy-1),h
	ld a,(iy-2)
	or (iy-1)
	jp z,_T0
	ld hl,1
; (HL:short)
; stmt EXPR
; EXPR
	jp _E0
_T0:
	ld hl,0
_E0:
	ld h,0
	ld a,l
	or 2
	ld l,a
	ld a,h
	or 0
	ld h,a
	push hl
; (HL:short)
; stmt LABEL
	call oarg
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	.dw 74
; (HL:short)
; stmt GOTO
	pop af
; stmt LABEL
	ld a,(iy-2)
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	or (iy-1)
	jp z,_T1
; (HL:void)
	ld hl,1
	jp _E1
_T1:
; top FUNC
; FUNC _rtcinit:v
; params=1 locals=5 frame=21 framefree=0
; param base:l r=0 o=4
; local m:s r=0
; local y:s r=3
; local t:l r=0
; local b:s r=0
; local r:s r=0
	ld hl,0
_E1:
	push hl
	call oarg
	.dw 74
	pop af
; stmt BLOCK
; BLOCK n=15
; stmt EXPR
; EXPR
@4__F3C:
	inc bc
	jp @4__F3T
@4__F3B:
	ld hl,8
; (HL:void)
; stmt EXPR
; EXPR
	call _clkcmd
Xrtcset:
	call	fexbw
	.dw	-4
_rtcinit::
	call	fentbq
	.dw	-6  
	ld	hl,-13
	add	hl,sp
; (HL:void)
; stmt EXPR
; EXPR
	ld	sp,hl
	push iy
	pop hl
	ld de,-21
	add hl,de
; (BC:short)
; stmt EXPR
; EXPR
	call _rtcget
	push iy
	pop hl
	ld de,-16
	add hl,de
	push hl
	call qldiy
	.db 4
	call _unixtobcd
	pop af
	ld l,(iy-16)
	ld h,0
; (HL:short)
; stmt LABEL
	ld c,l
; stmt EXPR
; SEMI
; stmt BLOCK
; BLOCK n=9
; stmt EXPR
; EXPR
	ld b,h
; (HL:ubyte)
; stmt IF
; IF nlbl=0
; (LE:ubyte 9:short (DEREF:short (LOCALVAR:short IY-2)))
	ld a,(iy-17)
	ld l,a
	ld h,0
	sra h
	rr l
	sra h
	rr l
	sra h
	rr l
	sra h
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	rr l
	ld (iy-2),l
	ld (iy-1),h
; (A:ubyte)
@5__F4T:
	ld (iy-16),c
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld l,(iy-2)
	ld h,(iy-1)
	ld de,9
; (A:ubyte)
	or a
; stmt EXPR
; EXPR
	sbc hl,de
	ld a,h
; (A:ubyte)
; stmt EXPR
; EXPR
	jp po,$+5
	xor 80h
; (A:ubyte)
; stmt EXPR
; EXPR
	or a
	jp m,no0_5
; (A:ubyte)
; stmt EXPR
; EXPR
	ld a,(iy-2)
	add a,7
; (A:ubyte)
; stmt EXPR
; EXPR
	ld (iy-15),a
	jp no1_5
no0_5:
	ld a,(iy-2)
	inc a
	ld (iy-15),a
no1_5:
; (HL:long)
; stmt IF
; IF nlbl=0
; (LE:ubyte (DEREF:long (LOCALVAR:long IY+4)) (DEREF:long (LOCALVAR:long IY-6)))
	ld a,(iy-18)
	ld (iy-14),a
	ld a,(iy-19)
	ld (iy-13),a
	ld a,(iy-20)
	ld (iy-12),a
	ld a,(iy-21)
	ld (iy-11),a
	push iy
	pop hl
	ld de,-16
	add hl,de
	call _bcd2unix
	call qstiy
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	.db -6
	call qldiy
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:short BC) 153:short)
	.db 4
	push hl
	exx
	push hl
	exx
	call qldiy
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	.db -6
; (BC:short)
	exx
	pop de
; stmt IF
; IF nlbl=0
; (EQ:ubyte (AND:short (REGVAR:short BC) 15:short) 9:ubyte)
	exx
	pop de
	call qcmp
	jp p,@5__F4B
no2_5:
	ld l,c
	ld h,b
	ld de,153
	or a
	sbc hl,de
	jp nz,no4_5
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld bc,0
	jp no5_5
no4_5:
	ld l,c
	ld h,b
	ld a,l
; (BC:short)
	and 15
	ld l,a
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	xor a
	ld h,a
	ld de,9
	or a
	sbc hl,de
; (BC:short)
	jp nz,no6_5
	ld l,c
; stmt LABEL
	ld h,b
; stmt EXPR
; SEMI
; stmt GOTO
	ld de,7
; stmt LABEL
	add hl,de
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	ld c,l
; (HL:short)
; stmt EXPR
; EXPR
	ld b,h
	jp no7_5
no6_5:
	ld l,c
	ld h,b
	ld de,1
; (HL:long)
; stmt EXPR
; EXPR
	add hl,de
; (HL:short)
	ld c,l
	ld b,h
no7_5:
; top FUNC
; FUNC _rtcwrite:v
; params=1 locals=3 frame=15 framefree=0
; param t:l r=0 o=4
; local m:s r=3
; local b:s r=0
; local r:s r=0
no5_5:
@5__F4C:
	jp @5__F4T
@5__F4B:
	call _di
	call qldiy
; stmt BLOCK
; BLOCK n=8
; stmt EXPR
; EXPR
	.db -6
	exx
	ld (_seconds),hl
	exx
	ld (_seconds+2),hl
	call _ei
Xrtcinit:
	call	fexbq
	.dw	-8
; (HL:void)
; stmt EXPR
; EXPR
_rtcwrite::
	call	fentbq
	.dw	0  
	ld	hl,-13
	add	hl,sp
	ld	sp,hl
	push iy
	pop hl
	ld de,-10
	add hl,de
	push hl
	call qldiy
	.db 4
	call _unixtobcd
	pop af
	ld l,(iy-9)
	ld h,0
	ld a,l
	and 15
	ld l,a
	xor a
	ld h,a
; (BC:short)
; stmt EXPR
; EXPR
	push hl
	ld a,(iy-9)
; (A:ubyte)
; stmt EXPR
; EXPR
	cp 16
	jp c,_T2
; (A:ubyte)
; stmt EXPR
; EXPR
	ld hl,10
	jp _E2
; (A:ubyte)
; stmt EXPR
; EXPR
_T2:
	ld hl,0
; (A:ubyte)
; stmt EXPR
; EXPR
_E2:
	ld h,0
	pop de
	ex de,hl
	add hl,de
	ld c,l
	ld b,h
	ld a,(iy-5)
	ld (iy-15),a
	ld a,(iy-6)
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld (iy-14),a
	ld a,(iy-7)
	ld (iy-13),a
	ld a,(iy-8)
	ld (iy-12),a
; (HL:void)
	ld l,c
	ld h,b
	ld de,1
; EOF
	or a
	sbc hl,de
	add hl,hl
	add hl,hl
	add hl,hl
	add hl,hl
	ld (iy-11),l
	push iy
	pop hl
	ld de,-15
	add hl,de
	call _rtcset
Xrtcwrite:
	call	fexbq
	.dw	-2
