; top FUNC
; FUNC _parsePtrPfx:s
; params=1 locals=0 frame=0 framefree=0
; param @41:s r=3 o=4
; stmt BLOCK
; BLOCK n=8
; stmt LABEL
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (EQ:ubyte (DEREF:ubyte $_cur) 36:ubyte))
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
; (HL:void)
; stmt EXPR
; EXPR
; (BC:short)
; stmt GOTO
; stmt LABEL
; stmt EXPR
; SEMI
; stmt RETURN
; RETURN hasval=1
; (HL:short/v)
; top FUNC
; FUNC _createPrmEnt:s
; params=2 locals=1 frame=0 framefree=0
; param @113:S r=0 o=4
; param @33:s r=0 o=6
; local @236:s r=4
; stmt BLOCK
; BLOCK n=7
; stmt EXPR
; EXPR
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
; (HL:ushort)
; stmt EXPR
; EXPR
; (HL:short)
; stmt EXPR
; EXPR
; (A:ubyte)
; stmt EXPR
; EXPR
; (HL:ubyte)
; stmt EXPR
; EXPR
; (HL:ubyte)
; stmt RETURN
; RETURN hasval=1
; (HL:short/v)
; top FUNC
; FUNC _slimFnArgs:v
; params=1 locals=4 frame=6 framefree=0
; param @41:s r=4 o=4
; local @240:s r=0
; local @239:s r=0
; local @238:s r=0
; local @237:s r=3
	.bss
_redeclOld::
	.ds 2
	.text
_parsePtrPfx::
; stmt BLOCK
; BLOCK n=20
; stmt LABEL
	push	bc    
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=1
; (LNOT:short (LAND:ubyte (REGVAR:short IX) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 8:short))))
        
	ld c,l
	ld b,h
@1__W1T:	;@dead a,f
	ld a,(_cur)
	cp 36
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	jp nz,@1__W1B
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=0
; (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 48:short)
no0_1:	;@dead a,f
	call _gettoken
	ld hl,0
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	push hl
	push bc
; (REGVAR:short IX#1)
	ld hl,16
	call _getType
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=0
	pop af
	pop af
; stmt GOTO
	ld c,l
; stmt LABEL
	ld b,h
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=1
; (LOR:ubyte (LNOT:short (REGVAR:short IX)) (DEREF:short (ADD:short (REGVAR:short IX) 2:short)))
	jp @1__W1T	;@dead a,f
@1__W1B:	;@dead a,f
	ld l,c
	ld h,b
XparsePtrPfx:
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=0
	pop	bc
	ret
_createPrmEnt::
; stmt EXPR
; EXPR
	call	fentxw
	.dw	0  
; (HL:short)
; stmt EXPR
; EXPR
	ld hl,23
	call _galloc
	push hl
	pop ix
; (BC:short)
; stmt EXPR
; EXPR
	ld l,(iy+4)
	ld h,(iy+5)
; (HL:short)
; stmt EXPR
; EXPR
	ld (ix+0),l
	ld (ix+1),h
; (HL:short)
; stmt LABEL
	ld l,(iy+6)
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (REGVAR:short BC))
	ld h,(iy+7)
	ld (ix+2),l
	ld (ix+3),h
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld a,(_lexlevel)
	inc a
; stmt BLOCK
; BLOCK n=9
; stmt EXPR
; EXPR
	ld (ix+7),a
	ld (ix+8),0
	ld (ix+6),9
	push ix
	pop hl
XcreatePrmEnt:
	call	fexxw
; (HL:short)
; stmt EXPR
; EXPR
	.dw	-2
_slimFnArgs::
	call	fentbxw
	.dw	-6  
	push	hl
; (HL:void)
; stmt EXPR
; EXPR
	pop	ix
@3__W2T:	;@dead a,f
	ld a,ixl
	or ixh
; (HL:short)
; stmt EXPR
; EXPR
	jp z,_C0
	bit 3,(ix+8)
	jp z,no0_3
_C0:
	jp @3__W2B	;@dead a,f
no0_3:	;@dead a,f
	ld a,(ix+8)
	and 48
	jp z,no3_3
	ld l,(ix+6)
	ld h,(ix+7)
	push hl
; (HL:short)
; stmt EXPR
; EXPR
	pop ix
	jp no4_3	;@dead a,f
no3_3:	;@dead a,f
	jp	XslimFnArgs	;@dead a,f
no4_3:	;@dead a,f
	jp @3__W2T	;@dead a,f
@3__W2B:	;@dead a,f
; (HL:short)
; stmt IF
; IF nlbl=0
; (DEREF:short (LOCALVAR:short IY-2))
	ld a,ixl
	or ixh
	jp z,_C1
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld a,(ix+2)
	or (ix+3)
	jp z,no5_3
_C1:
	jp	XslimFnArgs	;@dead a,f
no5_3:	;@dead a,f
	ld (ix+2),1
	ld (ix+3),0
	ld l,(ix+4)
	ld h,(ix+5)
	ld c,l
	ld b,h
; (HL:short)
	xor a
	ld (ix+4),a
	ld (ix+5),a
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld (iy-2),a
	ld (iy-1),a
@3__W3T:	;@dead a,f
	ld a,c
; (HL:short)
	or b
; stmt EXPR
; EXPR
	jp z,@3__W3B
no8_3:	;@dead a,f
	ld hl,12
; (HL:short)
; stmt EXPR
; EXPR
	add hl,bc
	ld a,(hl)
	inc hl
; (HL:void)
; stmt EXPR
; EXPR
	ld h,(hl)
	ld l,a
	ld (iy-6),l
	ld (iy-5),h
; (BC:short)
; stmt GOTO
	ld hl,2
; stmt LABEL
	add hl,bc
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	ld a,(hl)
	inc hl
	ld h,(hl)
; (HL:void)
	ld l,a
	call _slimFnArgs
	ld hl,4
; top FUNC
; FUNC _symDecl:s
; params=2 locals=2 frame=0 framefree=0
; param @246:s r=0 o=4
; param @151:B r=0 o=6
; local @247:s r=4
; local L0:s r=3
	call _permalloc
	ld (iy-4),l
	ld (iy-3),h
; stmt BLOCK
; BLOCK n=6
; stmt EXPR
; EXPR
	ld hl,2
	add hl,bc
; (HL:short)
; stmt IF
; IF nlbl=0
; (DEREF:ubyte (LOCALVAR:ubyte IY+6))
	ld a,(hl)
	inc hl
	ld h,(hl)
; stmt BLOCK
; BLOCK n=11
; stmt EXPR
; EXPR
	ld l,a
	ld e,(iy-4)
	ld d,(iy-3)
	ex de,hl
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	ld (hl),e
	inc hl
	ld (hl),d
; (HL:ushort)
; stmt EXPR
; EXPR
	ld l,(iy-4)
	ld h,(iy-3)
	inc hl
	inc hl
; (HL:short)
; stmt EXPR
; EXPR
	ld (hl),0
	inc hl
; (A:ubyte)
; stmt EXPR
; EXPR
	ld (hl),0
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld a,(iy-2)
; (HL:ubyte)
; stmt EXPR
; EXPR
	or (iy-1)
	jp z,no10_3
; (HL:ushort)
; stmt EXPR
; EXPR
	ld l,(iy-2)
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld h,(iy-1)
; (HL:ubyte)
; stmt EXPR
; EXPR
	inc hl
	inc hl
; (HL:short)
; stmt EXPR
; EXPR
	push hl
	ld l,(iy-4)
; (HL:short)
	ld h,(iy-3)
	pop de
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ex de,hl
	ld (hl),e
	inc hl
	ld (hl),d
	jp no11_3	;@dead a,f
no10_3:	;@dead a,f
	ld l,(iy-4)
; (BC:short)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (REGVAR:short BC) (EQ:ubyte (DEREF:ubyte (ADD:short (REGVAR:short BC) 7:short)) (DEREF:ubyte $_lexlevel)))
	ld h,(iy-3)
	ld (ix+4),l
	ld (ix+5),h
no11_3:	;@dead a,f
	ld l,(iy-4)
	ld h,(iy-3)
	ld (iy-2),l
	ld (iy-1),h
	ld l,c
	ld h,b
	call _free
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=2
; (LAND:ubyte (LAND:ubyte (DEREF:short (ADD:short (REGVAR:short BC) 2:short)) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short BC) 2:short)) 8:short))) 8:short)) (LNOT:short (DEREF:short (ADD:short (REGVAR:short BC) 14:short))))
	ld l,(iy-6)
	ld h,(iy-5)
	ld c,l
	ld b,h
	jp @3__W3T	;@dead a,f
@3__W3B:	;@dead a,f
	ld l,(ix+6)
	ld h,(ix+7)
	call _slimFnArgs
XslimFnArgs:
	call	fexbxw
	.dw	-10
_symDecl::
	call	fentbxw
	.dw	0  
	ld hl,0
	ld (_redeclOld),hl
	ld a,(iy+6)
	or a
	jp z,no0_4
	ld hl,23
	call _galloc
	push hl
	pop ix
	ld hl,(_cur+1)
	ld (ix+0),l
	ld (ix+1),h
	ld l,(iy+4)
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld h,(iy+5)
	ld (ix+2),l
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	ld (ix+3),h
	ld a,(_lexlevel)
	ld (ix+7),a
	ld (ix+8),0
	ld (ix+6),5
	xor a
	ld (ix+16),a
	ld (ix+17),a
; (HL:short)
	ld (ix+19),a
	ld (ix+18),a
; stmt IF
; IF nlbl=3
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (EQ:ubyte (DEREF:ubyte $_lexlevel) 1:ubyte) (DEREF:short (ADD:short (REGVAR:short BC) 2:short))) (LNOT:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short)))) (EQ:ubyte (DEREF:ubyte (ADD:short (REGVAR:short BC) 6:short)) 4:ubyte))
	ld (ix+12),a
	ld (ix+13),a
	ld (ix+14),a
	ld (ix+15),a
	jp no1_4	;@dead a,f
no0_4:	;@dead a,f
	ld hl,0
	push hl
	ld hl,(_cur+1)
	call _findName
	pop af
	ld c,l
	ld b,h
	ld a,c
	or b
	jp z,no2_4
	ld hl,7
	add hl,bc
	ld a,(hl)
	push af
	ld de,(_lexlevel)
	pop af
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	cp e
	jp nz,no2_4
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	ld hl,2
	add hl,bc
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
; (HL:short)
	or h
	jp z,no5_4
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld hl,2
	add hl,bc
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld de,8
	add hl,de
	bit 3,(hl)
	jp z,no5_4
	ld hl,14
	add hl,bc
	ld a,(hl)
	inc hl
; (REGVAR:short IX#1)
	ld h,(hl)
	ld l,a
	or h
; stmt IF
; IF nlbl=1
; (LAND:ubyte (REGVAR:short BC) (LT:ubyte (DEREF:ubyte (ADD:short (REGVAR:short BC) 7:short)) (DEREF:ubyte $_lexlevel)))
	jp nz,no5_4
	push bc
	pop ix
	ld hl,2
	add hl,bc
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld (_redeclOld),hl
	jp no6_4	;@dead a,f
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
no5_4:	;@dead a,f
	ld a,(_lexlevel)
	cp 1
	jp nz,no9_4
	ld hl,2
	add hl,bc
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp z,no9_4
	ld hl,8
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	add hl,bc
	ld a,(hl)
	or a
	jp nz,no9_4
; (A:ubyte)
	ld hl,6
	add hl,bc
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld a,(hl)
	cp 4
	jp nz,no9_4
	push bc
	pop ix
	ld hl,2
	add hl,bc
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld (_redeclOld),hl
	jp no10_4	;@dead a,f
no9_4:	;@dead a,f
; (REGVAR:short IX#1)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (LT:ubyte 2:ubyte (DEREF:ubyte $_lexlevel)) (LNOT:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))))
	ld hl,0
	push hl
	ld l,(iy+4)
	ld h,(iy+5)
	push hl
	ld hl,4
	push hl
	ld hl,(_cur+1)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	call _newName
	pop af
	pop af
	pop af
; (A:ubyte)
	push hl
	pop ix
no10_4:	;@dead a,f
no6_4:	;@dead a,f
; stmt IF
; IF nlbl=1
; (LAND:ubyte (REGVAR:short IX) (LE:ubyte 2:ubyte (DEREF:ubyte $_lexlevel)))
	jp no3_4	;@dead a,f
no2_4:	;@dead a,f
	ld a,c
	or b
	jp z,no14_4
	ld hl,7
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	add hl,bc
	ld a,(hl)
; (HL:ubyte)
	push af
; stmt EXPR
; EXPR
	ld de,(_lexlevel)
; (HL:void)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 8:ubyte)
	pop af
	cp e
	jp nc,no14_4
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	ld hl,0
; (HL:void)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (NE:ubyte (DEREF:ubyte $_cur) 21:ubyte) (NE:ubyte (DEREF:ubyte $_cur) 25:ubyte))
	push hl
	ld l,(iy+4)
	ld h,(iy+5)
	push hl
	ld hl,4
	push hl
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld hl,(_cur+1)
	call _newName
; (HL:void)
	pop af
	pop af
; stmt IF
; IF nlbl=0
; (LT:ubyte 32:long (DEREF:long (ADD:short $_cur 1:short)))
	pop af
	push hl
	pop ix
	ld hl,_shadowCtr
	inc (hl)
	ld a,(hl)
	ld (ix+11),a
	jp no15_4	;@dead a,f
no14_4:	;@dead a,f
	ld hl,0
	push hl
	ld l,(iy+4)
	ld h,(iy+5)
	push hl
	ld hl,4
	push hl
	ld hl,(_cur+1)
	call _newName
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	pop af
	pop af
; (HL:void)
	pop af
	push hl
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	pop ix
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld a,(_lexlevel)
	cp 2
	jr nz,$+3
	scf
	jp c,no17_4
; (HL:ubyte)
	ld a,(ix+8)
	or a
; stmt EXPR
; EXPR
	jp nz,no17_4
; (HL:void)
	ld hl,_shadowCtr
; stmt RETURN
; RETURN hasval=1
	inc (hl)
	ld a,(hl)
; (HL:short/v)
	ld (ix+11),a
no17_4:	;@dead a,f
no15_4:	;@dead a,f
no3_4:	;@dead a,f
; top FUNC
; FUNC _prmFnPtr:s
; params=2 locals=5 frame=8 framefree=0
; param @250:s r=3 o=4
; param @251:s r=0 o=6
; local L2:s r=0
; local L1:s r=0
; local L0:s r=4
; local L4:s r=0
; local L3:s r=0
no1_4:	;@dead a,f
	ld a,ixl
	or ixh
	jp z,no20_4
	ld a,(_lexlevel)
; stmt BLOCK
; BLOCK n=2
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 36:ubyte)
	cp 2
	jp c,no20_4
	call _curblk
; stmt BLOCK
; BLOCK n=4
; stmt EXPR
; EXPR
	ld (ix+20),l
; (HL:void)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 20:ubyte)
no20_4:	;@dead a,f
	call _gettoken
	ld a,(_cur)
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	cp 8
	jp nz,no23_4
	call _gettoken
	ld a,(_cur)
	cp 21
	jp z,no25_4
	ld a,(_cur)
; (HL:ushort)
; stmt EXPR
; EXPR
	cp 25
; (HL:void)
	jp z,no25_4
; stmt EXPR
; EXPR
	ld hl,17
	call _gripe
	jp no26_4	;@dead a,f
no25_4:	;@dead a,f
	exx
; (HL:void)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 6:ubyte)
	ld hl,(_cur+1)
	exx
	ld hl,(_cur+3)
; stmt BLOCK
; BLOCK n=14
; stmt EXPR
; EXPR
	push hl
	exx
	push hl
	exx
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	ld hl,32
; (HL:ubyte)
; stmt EXPR
; EXPR
	exx
	ld hl,0
; (BC:short)
; stmt EXPR
; EXPR
; (HL:short)
; stmt EXPR
; EXPR
	pop de
; (HL:void)
; stmt LABEL
	exx
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=1
; (LNOT:short (LAND:ubyte (NE:ubyte (DEREF:ubyte $_cur) 7:ubyte) (NE:ubyte (DEREF:ubyte $_cur) 0:ubyte)))
	pop de
	call qcmp
	jp p,no28_4
	ld hl,18
	call _gripe
	jp no29_4	;@dead a,f
no28_4:	;@dead a,f
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld (ix+6),8
	exx
; stmt BLOCK
; BLOCK n=10
; stmt EXPR
; EXPR
	ld hl,(_cur+1)
	exx
	ld hl,(_cur+3)
; (HL:short)
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:short (LOCALVAR:short IY-8)))
	ld (ix+19),l
no29_4:	;@dead a,f
no26_4:	;@dead a,f
; stmt BLOCK
; BLOCK n=2
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 9:ubyte)
	call _gettoken
no23_4:	;@dead a,f
	push ix
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	pop hl
; (HL:void)
; stmt GOTO
XsymDecl:
; stmt GOTO
	call	fexbxw
	.dw	-4
; stmt EXPR
; EXPR
_prmFnPtr::
	call	fentbxw
	.dw	-8  
	ld c,l
	ld b,h
; (HL:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 20:ubyte)
	ld a,(_cur)
	cp 36
	jp nz,no0_5
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	call _gettoken
; (HL:void)
	ld a,(_cur)
; stmt EXPR
; EXPR
	cp 20
	jp nz,no2_5
	ld hl,(_cur+1)
	ld e,(iy+6)
	ld d,(iy+7)
	ex de,hl
	ld (hl),e
	inc hl
; (HL:short)
; stmt EXPR
; EXPR
	ld (hl),d
	call _gettoken
no2_5:	;@dead a,f
	ld hl,20
	push hl
	ld hl,7
	call _expect
; (HL:short)
; stmt IF
; IF nlbl=0
; (DEREF:short (LOCALVAR:short IY-2))
	pop af
	ld a,(_cur)
	cp 6
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp nz,no4_5
	ld hl,11
	call _permalloc
	push hl
	pop ix
	ld (ix+8),8
	ld (ix+6),c
	ld (ix+7),b
	xor a
	ld (iy-2),a
	ld (iy-1),a
	call _gettoken
@5__W4T:	;@dead a,f
; (HL:short)
	ld a,(_cur)
	cp 7
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp z,_C2
	ld a,(_cur)
	cp 0
	jp nz,no6_5
; (HL:short)
_C2:
; stmt EXPR
; EXPR
	jp @5__W4B	;@dead a,f
no6_5:	;@dead a,f
	call _getbasetype
	ld (iy-8),l
; (HL:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 9:ubyte)
	ld (iy-7),h
	ld a,(iy-8)
	or (iy-7)
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	jp nz,no9_5
; (HL:void)
; stmt GOTO
	ld a,(_cur)
	cp 9
; stmt GOTO
	jp nz,no11_5
; stmt GOTO
	call _gettoken
; stmt LABEL
	jp @5__W4T	;@dead a,f
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
no11_5:	;@dead a,f
	jp @5__W4B	;@dead a,f
no9_5:	;@dead a,f
	ld l,(iy-8)
	ld h,(iy-7)
; (HL:void)
; stmt EXPR
; EXPR
	call _parsePtrPfx
	ld (iy-6),l
	ld (iy-5),h
	ld a,(_cur)
	cp 20
	call z,_gettoken
no13_5:	;@dead a,f
	ld l,(iy-6)
	ld h,(iy-5)
	push hl
; (BC:short)
	ld hl,0
	call _createPrmEnt
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	pop af
	ld (iy-4),l
	ld (iy-3),h
	ld l,(iy-4)
	ld h,(iy-3)
	ld de,12
	add hl,de
	ld (hl),0
	inc hl
	ld (hl),0
	ld a,(iy-2)
; (BC:short)
	or (iy-1)
	jp z,no15_5
	ld l,(iy-2)
; stmt BLOCK
; BLOCK n=9
; stmt EXPR
; EXPR
	ld h,(iy-1)
	ld de,12
; (HL:void)
; stmt LABEL
	add hl,de
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=1
; (LNOT:short (LAND:ubyte (NE:ubyte (DEREF:ubyte $_cur) 7:ubyte) (NE:ubyte (DEREF:ubyte $_cur) 0:ubyte)))
	push hl
	ld l,(iy-4)
	ld h,(iy-3)
	pop de
	ex de,hl
	ld (hl),e
	inc hl
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld (hl),d
	jp no16_5	;@dead a,f
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
no15_5:	;@dead a,f
; (HL:void)
; stmt GOTO
	ld l,(iy-4)
; stmt LABEL
	ld h,(iy-3)
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 7:ubyte)
	ld (ix+4),l
	ld (ix+5),h
no16_5:	;@dead a,f
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld l,(iy-4)
; (HL:void)
	ld h,(iy-3)
	ld (iy-2),l
; stmt RETURN
; RETURN hasval=1
	ld (iy-1),h
	ld a,(_cur)
; (HL:short/v)
	cp 9
	jp nz,no17_5
	call _gettoken
	jp @5__W4T	;@dead a,f
; top FUNC
; FUNC _prmDecl:s
; params=0 locals=6 frame=7 framefree=0
; local @236:s r=4
; local @264:B r=0
; local @250:s r=3
; local @263:s r=0
; local @262:S r=0
; local L0:s r=0
no17_5:	;@dead a,f
	jp @5__W4B	;@dead a,f
	jp @5__W4T	;@dead a,f
; stmt BLOCK
; BLOCK n=12
; stmt EXPR
; EXPR
@5__W4B:	;@dead a,f
	ld hl,20
; (HL:ushort)
; stmt EXPR
; EXPR
	push hl
	ld hl,7
; (HL:ubyte)
; stmt EXPR
; EXPR
	call _expect
	pop af
; (HL:short)
; stmt IF
; IF nlbl=3
; (LAND:ubyte (DEREF:ubyte (LOCALVAR:ubyte IY-1)) (LOR:ubyte (LOR:ubyte (EQ:ubyte (DEREF:ubyte $_cur) 20:ubyte) (EQ:ubyte (DEREF:ubyte $_cur) 36:ubyte)) (EQ:ubyte (DEREF:ubyte $_cur) 6:ubyte)))
	ld hl,0
	push hl
	push ix
	ld hl,16
	call _getType
	pop af
	pop af
	ld c,l
	ld b,h
	jp no5_5	;@dead a,f
no4_5:	;@dead a,f
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld hl,0
	push hl
; (HL:short)
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:short (LOCALVAR:short IY-3)))
	push bc
	ld hl,16
	call _getType
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	pop af
	pop af
	ld c,l
; (HL:short)
	ld b,h
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:short (LOCALVAR:short IY-3)))
no5_5:	;@dead a,f
	jp no1_5	;@dead a,f
no0_5:	;@dead a,f
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld hl,20
	call _gripe
; (HL:void)
; stmt RETURN
; RETURN hasval=1
@5__W5T:	;@dead a,f
; (HL:short/v)
	ld a,(_cur)
	cp 7
; stmt EXPR
; EXPR
	jp z,_C3
	ld a,(_cur)
	cp 0
	jp nz,no19_5
_C3:
; (BC:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 6:ubyte)
	jp @5__W5B	;@dead a,f
no19_5:	;@dead a,f
	call _gettoken
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	jp @5__W5T	;@dead a,f
; (HL:void)
; stmt EXPR
; EXPR
@5__W5B:	;@dead a,f
	ld a,(_cur)
	cp 7
	call z,_gettoken
no22_5:	;@dead a,f
no1_5:	;@dead a,f
	ld l,c
	ld h,b
XprmFnPtr:
; (BC:short)
	call	fexbxw
	.dw	-12
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 20:ubyte)
_prmDecl::
	call	fentbx
	.dw	-7  
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	xor a
	ld (iy-5),a
	ld (iy-4),a
	call _parseSclass
; (HL:ushort)
; stmt EXPR
; EXPR
	ld (iy-1),l
; (HL:void)
	xor a
	ld (iy-3),a
	ld (iy-2),a
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 4:ubyte)
	ld a,(iy-1)
	or a
	jp z,no0_6
; stmt BLOCK
; BLOCK n=4
; stmt EXPR
; EXPR
	ld a,(_cur)
; (HL:void)
; stmt IF
; IF nlbl=0
; (NE:ubyte (DEREF:ubyte $_cur) 5:ubyte)
	cp 20
	jp z,_C4
	ld a,(_cur)
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	cp 36
	jp z,_C4
	ld a,(_cur)
	cp 6
; (HL:short)
; stmt IF
; IF nlbl=0
; (DEREF:short (LOCALVAR:short IY-7))
	jp nz,no0_6
_C4:
	ld hl,(_inttype)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld (iy-3),l
	ld (iy-2),h
no0_6:	;@dead a,f
; (HL:void)
	ld a,(iy-3)
	or (iy-2)
; stmt EXPR
; EXPR
	jp nz,no5_6
	call _getbasetype
	ld (iy-3),l
	ld (iy-2),h
no5_6:	;@dead a,f
; (HL:void)
; stmt EXPR
; EXPR
	ld a,(iy-3)
	or (iy-2)
	jp nz,no7_6
	ld hl,20
	call _gripe
	ld hl,0
	jp	XprmDecl	;@dead a,f
no7_6:	;@dead a,f
	ld l,(iy-3)
	ld h,(iy-2)
	call _parsePtrPfx
; (BC:short)
	ld c,l
; stmt EXPR
; EXPR
	ld b,h
	ld a,(_cur)
	cp 6
	jp nz,no9_6
	call _gettoken
	push iy
	pop hl
	ld de,-5
	add hl,de
; (REGVAR:short IX#1)
; stmt IF
; IF nlbl=0
; (AND:short (WIDEN:short (DEREF:ubyte (LOCALVAR:ubyte IY-1))) 16:short)
	push hl
	ld l,c
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld h,b
; (HL:ubyte)
	call _prmFnPtr
; stmt RETURN
; RETURN hasval=1
	pop af
	ld c,l
; (HL:short/v)
	ld b,h
	jp no10_6	;@dead a,f
no9_6:	;@dead a,f
	ld a,(_cur)
; top FUNC
; FUNC _fnParams:s
; params=1 locals=3 frame=2 framefree=0
; param @246:s r=0 o=4
; local @267:s r=0
; local @236:s r=3
; local @88:s r=4
	cp 20
	jp nz,no11_6
	ld hl,(_cur+1)
; stmt BLOCK
; BLOCK n=13
; stmt EXPR
; EXPR
	ld (iy-5),l
	ld (iy-4),h
	call _gettoken
no11_6:	;@dead a,f
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
no10_6:	;@dead a,f
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld a,(_cur)
	cp 4
	jp nz,no13_6
	call _gettoken
	ld a,(_cur)
	cp 5
	jp z,no15_6
	ld hl,0
	call _parseExpr
	ld (iy-7),l
	ld (iy-6),h
; (HL:short)
; stmt EXPR
; EXPR
	ld a,(iy-7)
	or (iy-6)
; (HL:short)
; stmt LABEL
	jp z,no17_6
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=1
; (LNOT:short (LAND:ubyte (NE:ubyte (DEREF:ubyte $_cur) 7:ubyte) (NE:ubyte (DEREF:ubyte $_cur) 0:ubyte)))
	ld l,(iy-7)
	ld h,(iy-6)
	call _FreeExpr
no17_6:	;@dead a,f
no15_6:	;@dead a,f
	ld hl,20
	push hl
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld hl,5
	call _expect
; stmt BLOCK
; BLOCK n=8
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 92:ubyte)
	pop af
	ld hl,0
	push hl
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
; (HL:void)
; stmt EXPR
; EXPR
; (A:ubyte)
; stmt GOTO
	push bc
	ld hl,16
; stmt EXPR
; EXPR
	call _getType
	pop af
	pop af
; (BC:short)
; stmt IF
; IF nlbl=0
; (LNOT:short (REGVAR:short BC))
	ld c,l
	ld b,h
no13_6:	;@dead a,f
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
; stmt EXPR
; EXPR
	push bc
	ld l,(iy-5)
	ld h,(iy-4)
	call _createPrmEnt
	pop af
; (HL:short)
; stmt IF
; IF nlbl=0
; (DEREF:short (LOCALVAR:short IY-2))
	push hl
	pop ix
	bit 4,(iy-1)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp z,no19_6
	ld (ix+9),16
no19_6:	;@dead a,f
	push ix
	pop hl
XprmDecl:
; (HL:short)
	call	fexbx
	.dw	-11
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
_fnParams::
	call	fentbxw
; (BC:short)
	.dw	-2  
; stmt EXPR
; EXPR
	ld hl,11
	call _permalloc
; (BC:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 9:ubyte)
	push hl
	pop ix
	ld (ix+8),8
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld a,(iy+4)
; (HL:void)
; stmt GOTO
	or (iy+5)
	jp z,_T5
; stmt IF
; IF nlbl=0
; (NE:ubyte (DEREF:ubyte $_cur) 7:ubyte)
	ld l,(iy+4)
	ld h,(iy+5)
	jp _E5
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
_T5:
	ld hl,(_inttype)
; (HL:void)
; stmt GOTO
_E5:
	ld (ix+6),l
; stmt GOTO
	ld (ix+7),h
; stmt LABEL
	xor a
	ld (iy-2),a
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	ld (iy-1),a
@7__W6T:	;@dead a,f
	ld a,(_cur)
	cp 7
	jp z,_C6
; (HL:void)
; stmt RETURN
; RETURN hasval=1
	ld a,(_cur)
	cp 0
; (HL:short/v)
	jp nz,no0_7
_C6:
	jp @7__W6B	;@dead a,f
no0_7:	;@dead a,f
; top FUNC
; FUNC _needsFuncSfx:s
; params=1 locals=1 frame=0 framefree=0
; param @247:s r=3 o=4
; local @271:s r=4
	ld a,(_cur)
	cp 92
	jp nz,no3_7
	call _gettoken
	set 7,(ix+8)
; stmt BLOCK
; BLOCK n=6
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:short (ADD:short (REGVAR:short BC) 2:short)))
	jp @7__W6B	;@dead a,f
no3_7:	;@dead a,f
	call _prmDecl
	ld c,l
	ld b,h
	ld a,c
	or b
	jp z,@7__W6B
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
no5_7:	;@dead a,f
; (HL:short/v)
	ld hl,12
	add hl,bc
; stmt EXPR
; EXPR
	ld (hl),0
	inc hl
	ld (hl),0
	ld a,(iy-2)
	or (iy-1)
	jp z,no7_7
	ld l,(iy-2)
	ld h,(iy-1)
; (REGVAR:short IX#1)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 16:short) (LNOT:short (DEREF:short (ADD:short (REGVAR:short IX) 6:short))))
	ld de,12
	add hl,de
	ld (hl),c
	inc hl
	ld (hl),b
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	jp no8_7	;@dead a,f
; (HL:short/v)
no7_7:	;@dead a,f
	ld (ix+4),c
; stmt IF
; IF nlbl=3
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 32:short) (DEREF:short (ADD:short (REGVAR:short IX) 6:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 6:short)) 8:short))) 16:short)) (LNOT:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 6:short)) 6:short))))
	ld (ix+5),b
no8_7:	;@dead a,f
	ld (iy-2),c
	ld (iy-1),b
	ld a,(_cur)
	cp 9
	jp nz,no9_7
	call _gettoken
	jp @7__W6T	;@dead a,f
no9_7:	;@dead a,f
	ld a,(_cur)
	cp 7
	jp z,no11_7
	ld hl,20
	call _gripe
	jp @7__W6B	;@dead a,f
no11_7:	;@dead a,f
	jp @7__W6T	;@dead a,f
@7__W6B:	;@dead a,f
	ld hl,20
	push hl
	ld hl,7
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	call _expect
; (HL:short/v)
	pop af
	push ix
; stmt IF
; IF nlbl=2
; (LAND:ubyte (LOR:ubyte (LT:ubyte 1:ubyte (DEREF:ubyte $_lexlevel)) (CALL:short/1 $_idOnce (DEREF:ushort/s (REGVAR:short BC)))) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 40:short)))
	pop hl
XfnParams:
	call	fexbxw
	.dw	-6
_needsFuncSfx:
	push	bc     
	push	ix
	ld c,l
	ld b,h
	ld hl,2
	add hl,bc
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp nz,no0_8
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	ld hl,0
; (HL:short/v)
	jp	XneedsFuncSfx	;@dead a,f
no0_8:	;@dead a,f
; stmt RETURN
; RETURN hasval=1
	ld hl,2
; (HL:short/v)
	add hl,bc
	ld a,(hl)
	inc hl
	ld h,(hl)
; top FUNC
; FUNC _skipParams:s
; params=2 locals=2 frame=1 framefree=0
; param @247:s r=0 o=4
; param @246:s r=3 o=6
; local @273:B r=0
; local @88:s r=4
	ld l,a
	push hl
	pop ix
	bit 4,(ix+8)
	jp z,no2_8
; stmt BLOCK
; BLOCK n=12
; stmt EXPR
; EXPR
	ld a,(ix+6)
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	or (ix+7)
; (HL:ubyte)
; stmt IF
; IF nlbl=0
; (CALL:short/1 $_needsFuncSfx (DEREF:short/s (LOCALVAR:short IY+4)))
	jp nz,no2_8
	ld hl,1
	jp	XneedsFuncSfx	;@dead a,f
no2_8:	;@dead a,f
	bit 5,(ix+8)
	jp z,no5_8
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	ld a,(ix+6)
	or (ix+7)
	jp z,no5_8
	ld l,(ix+6)
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	ld h,(ix+7)
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld de,8
	add hl,de
	bit 4,(hl)
	jp z,no5_8
	ld l,(ix+6)
	ld h,(ix+7)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
; (HL:short)
	ld l,a
; stmt LABEL
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=1
; (LNOT:short (LAND:ubyte (DEREF:ubyte (LOCALVAR:ubyte IY-1)) (NE:ubyte (DEREF:ubyte $_cur) 0:ubyte)))
	or h
	jp nz,no5_8
	ld hl,1
	jp	XneedsFuncSfx	;@dead a,f
no5_8:	;@dead a,f
	ld a,(_lexlevel)
	cp 1
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	jr nz,$+3
	scf
; stmt BLOCK
; BLOCK n=2
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 6:ubyte)
	jp nc,_C7
	ld l,c
	ld h,b
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld a,(hl)
; (HL:ubyte)
	inc hl
	ld h,(hl)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 7:ubyte)
	ld l,a
	call _idOnce
	ld a,l
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	or h
; (HL:ubyte)
	jp z,no10_8
_C7:
; stmt IF
; IF nlbl=0
; (DEREF:ubyte (LOCALVAR:ubyte IY-1))
	ld a,(ix+8)
	and 40
	jp nz,no10_8
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld hl,1
; (HL:void)
	jp	XneedsFuncSfx	;@dead a,f
; stmt GOTO
no10_8:	;@dead a,f
; stmt LABEL
	ld hl,0
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
XneedsFuncSfx:
	pop	ix
	pop	bc
	ret
_skipParams::
; (HL:void)
; stmt RETURN
; RETURN hasval=1
	call	fentbxw
	.dw	-1  
; (HL:short/v)
	ld	c,(iy+6)
	ld	b,(iy+7)
	ld ix,0
	ld (iy-1),1
; top FUNC
; FUNC _suffixApplies:s
; params=2 locals=1 frame=0 framefree=0
; param @88:s r=3 o=4
; param @247:s r=0 o=6
; local @271:s r=4
	ld l,(iy+4)
	ld h,(iy+5)
	call _needsFuncSfx
	ld a,l
	or h
; stmt BLOCK
; BLOCK n=8
; stmt IF
; IF nlbl=0
; (LNOT:short (REGVAR:short BC))
	jp z,no0_9
	ld hl,11
	call _permalloc
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	push hl
; (HL:short/v)
	pop ix
	ld (ix+8),8
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_phase) 1:ubyte)
	ld a,c
	or b
	jp z,_T8
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	ld l,c
; (HL:short/v)
	ld h,b
	jp _E8
; stmt EXPR
; EXPR
_T8:
	ld hl,(_inttype)
_E8:
	ld (ix+6),l
	ld (ix+7),h
no0_9:	;@dead a,f
@9__W7T:	;@dead a,f
	ld a,(iy-1)
	or a
	jp z,_C9
; (REGVAR:short IX#1)
; stmt IF
; IF nlbl=2
; (LAND:ubyte (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 32:short) (LNOT:short (LAND:ubyte (REGVAR:short IX) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 32:short))))
	ld a,(_cur)
	cp 0
	jp nz,no2_9
_C9:
	jp @9__W7B	;@dead a,f
no2_9:	;@dead a,f
	ld a,(_cur)
	cp 6
	jp nz,no5_9
	inc (iy-1)
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	jp no6_9	;@dead a,f
; (HL:short/v)
no5_9:	;@dead a,f
	ld a,(_cur)
; stmt IF
; IF nlbl=3
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 8:short) (REGVAR:short IX)) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 16:short)) (LNOT:short (DEREF:short (ADD:short (REGVAR:short IX) 6:short))))
	cp 7
	jp nz,no7_9
	dec (iy-1)
no7_9:	;@dead a,f
no6_9:	;@dead a,f
	ld a,(iy-1)
	or a
	call nz,_gettoken
no9_9:	;@dead a,f
	jp @9__W7T	;@dead a,f
@9__W7B:	;@dead a,f
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	ld hl,20
; (HL:short/v)
	push hl
	ld hl,7
; stmt IF
; IF nlbl=5
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 8:short) (REGVAR:short IX)) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 32:short)) (DEREF:short (ADD:short (REGVAR:short IX) 6:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 6:short)) 8:short))) 16:short)) (LNOT:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 6:short)) 6:short))))
	call _expect
	pop af
	push ix
	pop hl
XskipParams:
	call	fexbxw
	.dw	-5
_suffixApplies:
	call	fentbxw
	.dw	0  
	ld c,l
	ld b,h
	ld a,c
	or b
	jp nz,no0_10
	ld hl,0
	jp	XsuffixApplies	;@dead a,f
no0_10:	;@dead a,f
	ld a,(_phase)
	cp 1
	jp nz,no2_10
	ld hl,1
	jp	XsuffixApplies	;@dead a,f
no2_10:	;@dead a,f
	ld l,(iy+6)
	ld h,(iy+7)
	inc hl
	inc hl
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	ld a,(hl)
; (HL:short/v)
	inc hl
	ld h,(hl)
; stmt IF
; IF nlbl=5
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 8:short) (REGVAR:short IX)) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 40:short))) (LNOT:short (LAND:ubyte (LAND:ubyte (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 16:short) (DEREF:short (ADD:short (REGVAR:short IX) 6:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 6:short)) 8:short))) 8:short))))
	ld l,a
	push hl
	pop ix
	ld hl,8
	add hl,bc
	bit 5,(hl)
	jp z,no4_10
	ld a,ixl
	or ixh
	jp z,_C10
	bit 5,(ix+8)
	jp nz,no4_10
_C10:
	ld hl,1
	jp	XsuffixApplies	;@dead a,f
no4_10:	;@dead a,f
	ld hl,8
	add hl,bc
	bit 3,(hl)
	jp z,no8_10
	ld a,ixl
	or ixh
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	jp z,no8_10
; (HL:short/v)
	bit 4,(ix+8)
	jp z,no8_10
; stmt RETURN
; RETURN hasval=1
	ld a,(ix+6)
; (HL:short/v)
	or (ix+7)
	jp nz,no8_10
	ld hl,1
	jp	XsuffixApplies	;@dead a,f
; top FUNC
; FUNC _declare:s
; params=2 locals=9 frame=40 framefree=0
; param @150:s r=0 o=4
; param @151:B r=0 o=6
; local @279:B r=0
; local @278:B r=0
; local @277:s r=0
; local @104:L r=0
; local @62:s r=0
; local @88:s r=3
; local @246:s r=0
; local @41:s r=0
; local @247:s r=4
no8_10:	;@dead a,f
	ld hl,8
	add hl,bc
	bit 3,(hl)
	jp z,no13_10
	ld a,ixl
; stmt BLOCK
; BLOCK n=32
; stmt EXPR
; EXPR
	or ixh
; (HL:ubyte)
; stmt EXPR
; EXPR
	jp z,no13_10
; (BC:short)
; stmt EXPR
; EXPR
	bit 5,(ix+8)
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	jp z,no13_10
	ld a,(ix+6)
	or (ix+7)
; (HL:short)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (DEREF:short (LOCALVAR:short IY-12)) (DEREF:short (DEREF:short (LOCALVAR:short IY+4))))
	jp z,no13_10
	ld l,(ix+6)
	ld h,(ix+7)
	ld de,8
	add hl,de
	bit 4,(hl)
	jp z,no13_10
	ld l,(ix+6)
	ld h,(ix+7)
	ld de,6
	add hl,de
	ld a,(hl)
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	inc hl
	ld h,(hl)
; (HL:void)
; stmt EXPR
; EXPR
	ld l,a
; (HL:short)
	or h
; stmt IF
; IF nlbl=0
; (DEREF:short (LOCALVAR:short IY-12))
	jp nz,no13_10
	ld hl,1
	jp	XsuffixApplies	;@dead a,f
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
no13_10:	;@dead a,f
	ld hl,8
	add hl,bc
	bit 3,(hl)
	jp z,no20_10
	ld a,ixl
	or ixh
	jp z,no20_10
; (HL:short)
	ld a,(ix+8)
; stmt EXPR
; EXPR
	and 40
	jp nz,no20_10
	bit 4,(ix+8)
	jp z,_C11
	ld a,(ix+6)
	or (ix+7)
	jp z,_C11
	ld l,(ix+6)
; (HL:short)
; stmt EXPR
; EXPR
	ld h,(ix+7)
	ld de,8
	add hl,de
	bit 3,(hl)
	jp nz,no20_10
; (HL:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 6:ubyte)
_C11:
	ld hl,1
	jp	XsuffixApplies	;@dead a,f
; stmt BLOCK
; BLOCK n=7
; stmt EXPR
; EXPR
no20_10:	;@dead a,f
; (HL:ubyte)
; stmt EXPR
; EXPR
	ld hl,0
; (HL:void)
; stmt EXPR
; EXPR
XsuffixApplies:
; (HL:short)
; stmt EXPR
; EXPR
	call	fexbxw
	.dw	-4
_declare::
	call	fentbxw
	.dw	-12  
	ld	hl,-24
	add	hl,sp
	ld	sp,hl
	ld (iy-1),0
	ld bc,0
	ld ix,0
	call _getbasetype
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	ld (iy-12),l
	ld (iy-11),h
	ld a,(iy-12)
	or (iy-11)
	jp z,no0_11
; (HL:void)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (DEREF:short (DEREF:short (LOCALVAR:short IY+4))) (DEREF:short (LOCALVAR:short IY-8)))
	ld l,(iy+4)
	ld h,(iy+5)
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp z,no0_11
	ld hl,11
	call _gripe
	xor a
	ld (iy-12),a
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld (iy-11),a
no0_11:	;@dead a,f
; (HL:void)
; stmt EXPR
; EXPR
	ld a,(iy-12)
	or (iy-11)
; (HL:short)
	jp z,no3_11
; stmt IF
; IF nlbl=1
; (LAND:ubyte (DEREF:short (LOCALVAR:short IY-8)) (LNOT:short (REGVAR:short IX)))
	ld l,(iy-12)
	ld h,(iy-11)
	ld e,(iy+4)
	ld d,(iy+5)
	ex de,hl
	ld (hl),e
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	inc hl
	ld (hl),d
no3_11:	;@dead a,f
	ld l,(iy+4)
	ld h,(iy+5)
	ld a,(hl)
	inc hl
	ld h,(hl)
; (HL:short)
	ld l,a
	ld (iy-10),l
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 7:ubyte)
	ld (iy-9),h
	ld l,(iy-10)
	ld h,(iy-9)
; stmt BLOCK
; BLOCK n=2
; stmt IF
; IF nlbl=0
; (LNOT:short (REGVAR:short IX))
	call _parsePtrPfx
	ld (iy-10),l
	ld (iy-9),h
; stmt BLOCK
; BLOCK n=11
; stmt EXPR
; EXPR
	ld a,(_cur)
	cp 6
	jp nz,no5_11
	ld (iy-1),1
; (HL:short)
; stmt LABEL
	call _gettoken
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:short (ADD:short (DEREF:short (LOCALVAR:short IY-12)) 6:short)))
	xor a
	ld (iy-8),a
	ld (iy-7),a
	ld l,(iy+6)
	ld h,0
	push hl
	push iy
	pop hl
	ld de,-8
	add hl,de
	call _declare
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	pop af
	push hl
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	pop ix
	ld hl,15
	push hl
	ld hl,7
	call _expect
	pop af
	ld l,(iy+4)
	ld h,(iy+5)
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp z,no7_11
	ld a,(iy-8)
; (HL:short)
; stmt EXPR
; EXPR
	or (iy-7)
	jp z,no7_11
	ld hl,11
	call _gripe
	xor a
	ld (iy-8),a
	ld (iy-7),a
no7_11:	;@dead a,f
	ld a,(iy-8)
; (HL:short)
; stmt LABEL
	or (iy-7)
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	jp z,no10_11
	ld a,ixl
	or ixh
	jp nz,no10_11
	ld l,(iy-8)
	ld h,(iy-7)
	ld e,(iy+4)
	ld d,(iy+5)
	ex de,hl
	ld (hl),e
; (HL:short)
; stmt GOTO
	inc hl
; stmt LABEL
	ld (hl),d
; stmt EXPR
; SEMI
no10_11:	;@dead a,f
; stmt RETURN
; RETURN hasval=1
no5_11:	;@dead a,f
	ld a,(_cur)
; (HL:short/v)
	cp 7
	jp nz,no13_11
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 20:ubyte)
	ld a,ixl
	or ixh
	jp nz,no15_11
; stmt BLOCK
; BLOCK n=2
; stmt IF
; IF nlbl=0
; (REGVAR:short IX)
	ld l,(iy-10)
	ld h,(iy-9)
	ld (iy-12),l
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld (iy-11),h
@11__F8T:	;@dead a,f
; (HL:void)
	ld l,(iy-12)
; stmt EXPR
; EXPR
	ld h,(iy-11)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
; (REGVAR:short IX#1)
	jp z,@11__F8B
; stmt EXPR
; EXPR
no17_11:	;@dead a,f
; (HL:ubyte)
; stmt LABEL
	ld l,(iy-12)
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (EQ:ubyte (DEREF:ubyte $_cur) 4:ubyte))
	ld h,(iy-11)
	ld de,6
	add hl,de
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	push hl
	ld l,(iy+4)
; stmt BLOCK
; BLOCK n=4
; stmt EXPR
; EXPR
	ld h,(iy+5)
; (HL:void)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 5:ubyte)
	ld a,(hl)
	inc hl
	ld h,(hl)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld l,a
	pop de
	ex de,hl
	ld (hl),e
; (HL:ulong)
	inc hl
	ld (hl),d
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld l,(iy-10)
	ld h,(iy-9)
; (HL:void)
; stmt EXPR
; EXPR
	ld e,(iy+4)
	ld d,(iy+5)
	ex de,hl
	ld (hl),e
	inc hl
	ld (hl),d
; (HL:ulong)
@11__F8C:	;@dead a,f
; stmt IF
; IF nlbl=0
; (LT:ubyte (DEREF:ubyte (LOCALVAR:ubyte IY-2)) 6:ubyte)
	ld l,(iy-12)
	ld h,(iy-11)
	ld de,6
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld (iy-12),l
	ld (iy-11),h
	jp @11__F8T	;@dead a,f
@11__F8B:	;@dead a,f
no15_11:	;@dead a,f
	push ix
	pop hl
	jp	Xdeclare	;@dead a,f
no13_11:	;@dead a,f
	ld a,(_cur)
	cp 20
; (HL:ulong)
	jp nz,no19_11
	ld a,ixl
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	or ixh
	jp z,no21_11
; (HL:void)
	ld hl,16
; stmt EXPR
; EXPR
	call _gripe
no21_11:	;@dead a,f
	ld l,(iy+6)
	ld h,0
; (HL:void)
; stmt GOTO
	push hl
; stmt LABEL
	ld l,(iy-10)
; stmt EXPR
; SEMI
; stmt LABEL
	ld h,(iy-9)
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:ubyte (LOCALVAR:ubyte IY-2)))
	call _symDecl
	pop af
	push hl
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	pop ix
no19_11:	;@dead a,f
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	ld (iy-2),0
@11__W9T:	;@dead a,f
	ld a,(_cur)
	cp 4
	jp nz,@11__W9B
no23_11:	;@dead a,f
	call _gettoken
	ld a,(_cur)
	cp 5
	jp nz,no25_11
	ld (iy-6),255
	ld (iy-5),255
	ld (iy-4),255
	ld (iy-3),255
	jp no26_11	;@dead a,f
no25_11:	;@dead a,f
	ld hl,5
	call _parseConst
	exx
	ld hl,(_constVal)
	exx
	ld hl,(_constVal+2)
	call qstiy
; (HL:short)
; stmt EXPR
; EXPR
	.db -6
no26_11:	;@dead a,f
	ld a,(iy-2)
	cp 6
; (BC:short)
; stmt GOTO
	jp nc,no27_11
; stmt LABEL
	ld a,(iy-2)
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 6:ubyte)
	inc (iy-2)
	ld l,a
	ld h,0
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	add hl,hl
; (HL:void)
; stmt IF
; IF nlbl=0
; (REGVAR:short BC)
	add hl,hl
	ex de,hl
	push iy
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	pop hl
	add hl,de
; (HL:void)
; stmt EXPR
; EXPR
	ld de,-40
; (BC:short)
	add hl,de
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_phase) 2:ubyte)
	push hl
	call qldiy
	.db -6
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	call qst
	jp no28_11	;@dead a,f
no27_11:	;@dead a,f
	ld hl,19
	call _gripe
no28_11:	;@dead a,f
	ld hl,19
	push hl
	ld hl,5
; (BC:short)
	call _expect
	pop af
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp @11__W9T	;@dead a,f
@11__W9B:	;@dead a,f
@11__W10T:	;@dead a,f
	ld a,(iy-2)
	or a
; (BC:short)
	jp z,@11__W10B
; stmt IF
; IF nlbl=4
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (NE:ubyte (DEREF:ubyte $_cur) 80:ubyte) (NE:ubyte (DEREF:ubyte $_cur) 2:ubyte)) (NE:ubyte (DEREF:ubyte $_cur) 9:ubyte)) (NE:ubyte (DEREF:ubyte $_cur) 1:ubyte)) (NE:ubyte (DEREF:ubyte $_cur) 7:ubyte))
no29_11:	;@dead a,f
	ld a,(iy-2)
	dec a
	ld (iy-2),a
	ld l,a
	ld h,0
	add hl,hl
	add hl,hl
	ex de,hl
	push iy
	pop hl
	add hl,de
	ld de,-40
	add hl,de
	call qld
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
	push hl
	ld l,(iy-10)
; (HL:void)
; stmt EXPR
; EXPR
	ld h,(iy-9)
; (REGVAR:short IX#1)
	push hl
; stmt IF
; IF nlbl=0
; (LNOT:short (REGVAR:short IX))
	ld hl,48
	call _getType
	pop af
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	pop af
; (HL:short/v)
	ld (iy-10),l
	ld (iy-9),h
; stmt IF
; IF nlbl=0
; (CALL:short/2 $_suffixApplies (REGVAR:short/s IX) (REGVAR:short/s BC))
	ld l,(iy-10)
	ld h,(iy-9)
	ld c,l
	ld b,h
	jp @11__W10T	;@dead a,f
@11__W10B:	;@dead a,f
	ld a,(_cur)
	cp 6
	jp nz,no31_11
	call _gettoken
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=5
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 16:short)) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 32:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 8:short)) (LNOT:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)))) (NE:ubyte (DEREF:short (ADD:short (REGVAR:short BC) 6:short)) (DEREF:short (ADD:short (REGVAR:short IX) 2:short))))
	ld a,c
	or b
	jp z,no33_11
	ld hl,20
	call _gripe
	ld bc,0
no33_11:	;@dead a,f
	ld a,(_phase)
	cp 2
	jp nz,no35_11
	ld l,(iy-10)
	ld h,(iy-9)
	push hl
	push ix
	pop hl
	call _skipParams
	pop af
	ld c,l
	ld b,h
	jp no36_11	;@dead a,f
no35_11:	;@dead a,f
	ld l,(iy-10)
	ld h,(iy-9)
	call _fnParams
	ld c,l
	ld b,h
no36_11:	;@dead a,f
no31_11:	;@dead a,f
	ld a,(_cur)
	cp 80
	jp z,no37_11
	ld a,(_cur)
	cp 2
	jp z,no37_11
	ld a,(_cur)
	cp 9
	jp z,no37_11
	ld a,(_cur)
	cp 1
	jp z,no37_11
	ld a,(_cur)
	cp 7
	jp z,no37_11
	ld hl,23
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	call _gripe
	ld ix,0
no37_11:	;@dead a,f
	ld a,ixl
	or ixh
	jp nz,no43_11
	ld hl,0
	jp	Xdeclare	;@dead a,f
no43_11:	;@dead a,f
; (HL:short)
	push ix
	ld l,c
; stmt IF
; IF nlbl=5
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (DEREF:ubyte (LOCALVAR:ubyte IY-1)) (DEREF:short (ADD:short (REGVAR:short IX) 2:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 16:short)) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 32:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 32:short)) (LNOT:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short))))
	ld h,b
	call _suffixApplies
	pop af
	ld a,l
	or h
	jp z,no45_11
	ld a,(ix+2)
	or (ix+3)
	jp z,no47_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 4,(hl)
	jp z,no47_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 5,(hl)
	jp nz,no47_11
	ld hl,8
	add hl,bc
	bit 3,(hl)
	jp z,no47_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	or h
	jp nz,no47_11
	ld hl,6
	add hl,bc
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	push hl
	ld l,(ix+2)
; (HL:short)
	ld h,(ix+3)
	pop de
; stmt IF
; IF nlbl=6
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 32:short)) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 8:short)) (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) 8:short))) 16:short)) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) 8:short))) 32:short))) (LNOT:short (DEREF:short (ADD:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) 6:short))))
	ex de,hl
	or a
	sbc hl,de
	jp z,no47_11
	ld hl,0
	push hl
	push bc
	ld hl,16
	call _getType
	pop af
	pop af
	ld (ix+2),l
	ld (ix+3),h
	jp no48_11	;@dead a,f
no47_11:	;@dead a,f
	ld a,(iy-1)
	or a
	jp z,no54_11
	ld a,(ix+2)
	or (ix+3)
	jp z,no54_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 4,(hl)
	jp z,no54_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 5,(hl)
	jp nz,no54_11
	ld hl,8
	add hl,bc
	bit 5,(hl)
	jp z,no54_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp nz,no54_11
	ld hl,0
	push hl
	push bc
	ld hl,16
	call _getType
	pop af
	pop af
	ld (ix+2),l
	ld (ix+3),h
	jp no55_11	;@dead a,f
no54_11:	;@dead a,f
	ld a,(ix+2)
	or (ix+3)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp z,no61_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 5,(hl)
	jp z,no61_11
	ld hl,8
	add hl,bc
	bit 3,(hl)
	jp z,no61_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp z,no61_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
; (HL:short)
	add hl,de
	ld a,(hl)
; stmt IF
; IF nlbl=6
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (DEREF:ubyte (LOCALVAR:ubyte IY-1)) (DEREF:short (ADD:short (REGVAR:short IX) 2:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 16:short)) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 32:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 32:short)) (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) 8:short))) 32:short))
	inc hl
	ld h,(hl)
	ld l,a
	ld de,8
	add hl,de
	bit 4,(hl)
	jp z,no61_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld de,8
	add hl,de
	bit 5,(hl)
	jp nz,no61_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp nz,no61_11
	ld l,(ix+2)
	ld h,(ix+3)
	inc hl
	inc hl
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	push hl
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; SEMI
	ld hl,0
	push hl
; stmt IF
; IF nlbl=6
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (DEREF:ubyte (LOCALVAR:ubyte IY-1)) (DEREF:short (ADD:short (REGVAR:short IX) 2:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 16:short)) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 32:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 8:short)) (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) 8:short))) 8:short))
	push bc
	ld hl,16
	call _getType
	pop af
	pop af
	push hl
	ld hl,48
	call _getType
	pop af
	pop af
	ld (ix+2),l
	ld (ix+3),h
	jp no62_11	;@dead a,f
no61_11:	;@dead a,f
	ld a,(iy-1)
	or a
	jp z,no69_11
	ld a,(ix+2)
	or (ix+3)
	jp z,no69_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 4,(hl)
	jp z,no69_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 5,(hl)
	jp nz,no69_11
	ld hl,8
	add hl,bc
	bit 5,(hl)
	jp z,no69_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; SEMI
	ld l,a
; stmt IF
; IF nlbl=6
; (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (LAND:ubyte (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 8:short)) (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) 8:short))) 16:short)) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) 8:short))) 32:short))) (LNOT:short (DEREF:short (ADD:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) 6:short)))) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short BC) 8:short))) 8:short))
	or h
	jp z,no69_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld de,8
	add hl,de
	bit 5,(hl)
	jp nz,no70_11
no69_11:	;@dead a,f
	ld a,(iy-1)
	or a
	jp z,no77_11
	ld a,(ix+2)
	or (ix+3)
	jp z,no77_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 4,(hl)
	jp z,no77_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 5,(hl)
	jp nz,no77_11
	ld hl,8
	add hl,bc
	bit 3,(hl)
	jp z,no77_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp z,no77_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld de,8
	add hl,de
	bit 3,(hl)
	jp nz,no78_11
no77_11:	;@dead a,f
	ld a,(ix+2)
	or (ix+3)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp z,no85_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 3,(hl)
	jp z,no85_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp z,no85_11
	ld l,(ix+2)
; (HL:short)
	ld h,(ix+3)
	ld de,6
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	add hl,de
	ld a,(hl)
; (BC:short)
	inc hl
	ld h,(hl)
	ld l,a
	ld de,8
	add hl,de
	bit 4,(hl)
	jp z,no85_11
; stmt IF
; IF nlbl=1
; (LAND:ubyte (DEREF:short $_redeclOld) (DEREF:short (ADD:short (REGVAR:short IX) 2:short)))
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=1
; (LAND:ubyte (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 8:short) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short $_redeclOld) 8:short))) 8:short))
	ld l,a
	ld de,8
	add hl,de
	bit 5,(hl)
	jp nz,no85_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=0
; (LNOT:short (CALL:byte/2 $_sameRet (DEREF:short/s (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) (DEREF:short/s (ADD:short (DEREF:short $_redeclOld) 6:short))))
	ld h,(hl)
	ld l,a
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	or h
	jp nz,no85_11
	ld hl,8
	add hl,bc
	bit 3,(hl)
	jp z,no85_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	push hl
	ld hl,0
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	push hl
; (HL:void)
	push bc
	ld hl,16
; stmt IF
; IF nlbl=1
; (LAND:ubyte (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 8:short))) 32:short) (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short $_redeclOld) 8:short))) 32:short))
	call _getType
	pop af
	pop af
	pop de
	ex de,hl
	ld (hl),e
	inc hl
	ld (hl),d
	jp no86_11	;@dead a,f
no85_11:	;@dead a,f
	ld (ix+2),c
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=0
; (LNOT:short (CALL:byte/2 $_sameType (DEREF:short/s (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 6:short)) (DEREF:short/s (ADD:short (DEREF:short $_redeclOld) 6:short))))
	ld (ix+3),b
no86_11:	;@dead a,f
no78_11:	;@dead a,f
no70_11:	;@dead a,f
no62_11:	;@dead a,f
no55_11:	;@dead a,f
no48_11:	;@dead a,f
no45_11:	;@dead a,f
	ld hl,(_redeclOld)
	ld a,l
	or h
	jp z,no93_11
	ld a,(ix+2)
	or (ix+3)
	jp z,no93_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
	bit 3,(hl)
	jp z,no96_11
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld hl,(_redeclOld)
	ld de,8
; (HL:void)
	add hl,de
	bit 3,(hl)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (LE:ubyte (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 2:short)) 0:short) (LT:ubyte 0:short (DEREF:short (ADD:short (DEREF:short $_redeclOld) 2:short))))
	jp z,no96_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	push hl
	ld hl,(_redeclOld)
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	call _sameRet
	pop af
	ld a,l
	or h
	jp nz,no99_11
	ld hl,29
	call _gripe
no99_11:	;@dead a,f
	jp no97_11	;@dead a,f
no96_11:	;@dead a,f
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,8
	add hl,de
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	bit 5,(hl)
	jp z,no101_11
	ld hl,(_redeclOld)
; (HL:short)
	ld de,8
	add hl,de
; stmt IF
; IF nlbl=2
; (LAND:ubyte (LAND:ubyte (LT:ubyte 0:short (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 2:short))) (LT:ubyte 0:short (DEREF:short (ADD:short (DEREF:short $_redeclOld) 2:short)))) (NE:ubyte (DEREF:short (ADD:short (DEREF:short (ADD:short (REGVAR:short IX) 2:short)) 2:short)) (DEREF:short (ADD:short (DEREF:short $_redeclOld) 2:short))))
	bit 5,(hl)
	jp z,no101_11
	ld l,(ix+2)
	ld h,(ix+3)
	ld de,6
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	push hl
	ld hl,(_redeclOld)
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	call _sameType
	pop af
	ld a,l
	or h
	jp nz,no104_11
	ld hl,29
	call _gripe
	jp no105_11	;@dead a,f
no104_11:	;@dead a,f
	ld l,(ix+2)
	ld h,(ix+3)
	inc hl
	inc hl
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld a,h
	or a
	jp m,$+7
	ld a,h
	or l
	jr $+3
	xor a
	jp nz,no106_11
	ld hl,(_redeclOld)
	inc hl
	inc hl
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld a,h
	or a
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp m,$+7
	ld a,h
; (HL:void)
	or l
	jr $+3
	xor a
	jp z,no106_11
	ld hl,(_redeclOld)
; stmt IF
; IF nlbl=0
; (LNOT:short (CALL:byte/2 $_sameType (DEREF:short/s (ADD:short (REGVAR:short IX) 2:short)) (DEREF:short/s $_redeclOld)))
	ld (ix+2),l
	ld (ix+3),h
	jp no107_11	;@dead a,f
no106_11:	;@dead a,f
	ld l,(ix+2)
	ld h,(ix+3)
	inc hl
	inc hl
	ld a,(hl)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	inc hl
	ld h,(hl)
; (HL:void)
	ld l,a
	ld a,h
	or a
	jp m,$+7
; stmt RETURN
; RETURN hasval=1
	ld a,h
	or l
; (HL:short/v)
	jr $+3
	xor a
	jp z,no109_11
	ld hl,(_redeclOld)
; top FUNC
; FUNC _isCastStart:b
; params=0 locals=0 frame=0 framefree=1
	inc hl
; stmt BLOCK
; BLOCK n=2
; stmt IF
; IF nlbl=0
; (CALL:short/1 $_isTypeToken (DEREF:ubyte/s $_cur))
	inc hl
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld a,h
	or a
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
	jp m,$+7
; (HL:short/v)
	ld a,h
	or l
; stmt RETURN
; RETURN hasval=1
	jr $+3
; (HL:short/v)
	xor a
	jp z,no109_11
	ld l,(ix+2)
; top FUNC
; FUNC _parseTypeName:s
; params=0 locals=7 frame=36 framefree=0
; local @288:s r=3
; local @287:s r=0
; local L3:L r=0
; local L2:B r=0
; local L1:s r=0
; local L0:s r=4
; local L4:B r=0
	ld h,(ix+3)
	inc hl
	inc hl
	ld a,(hl)
	inc hl
	ld h,(hl)
; stmt BLOCK
; BLOCK n=5
; stmt EXPR
; EXPR
	ld l,a
	push hl
	ld hl,(_redeclOld)
; (HL:short)
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:short (LOCALVAR:short IY-2)))
	inc hl
	inc hl
	ld a,(hl)
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	inc hl
	ld h,(hl)
	ld l,a
; (HL:short)
	pop de
; stmt EXPR
; EXPR
	ex de,hl
	or a
	sbc hl,de
	jp z,no109_11
	ld hl,29
; (BC:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 6:ubyte)
	call _gripe
no109_11:	;@dead a,f
no107_11:	;@dead a,f
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
no105_11:	;@dead a,f
; (HL:void)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 36:ubyte)
	jp no102_11	;@dead a,f
no101_11:	;@dead a,f
	ld l,(ix+2)
; stmt BLOCK
; BLOCK n=15
; stmt EXPR
; EXPR
	ld h,(ix+3)
; (HL:void)
; stmt LABEL
	push hl
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (EQ:ubyte (DEREF:ubyte $_cur) 36:ubyte))
	ld hl,(_redeclOld)
	call _sameType
	pop af
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld a,l
	or h
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	jp nz,no113_11
; (HL:void)
; stmt GOTO
	ld hl,29
; stmt LABEL
	call _gripe
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
no113_11:	;@dead a,f
no102_11:	;@dead a,f
no97_11:	;@dead a,f
no93_11:	;@dead a,f
	push ix
; (HL:void)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 4:ubyte)
	pop hl
Xdeclare:
; stmt BLOCK
; BLOCK n=16
; stmt EXPR
; EXPR
	call	fexbxw
; (HL:ubyte)
; stmt LABEL
	.dw	-16
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (EQ:ubyte (DEREF:ubyte $_cur) 4:ubyte))
_isCastStart::
	ld a,(_cur)
	ld l,a
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld h,0
	call _isTypeToken
; stmt BLOCK
; BLOCK n=4
; stmt EXPR
; EXPR
	ld a,l
; (HL:void)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 5:ubyte)
	or h
	jp z,no0_12
	ld hl,1
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ret
no0_12:	;@dead a,f
	ld hl,0
	ret
; (HL:ulong)
XisCastStart:
	ret
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
_parseTypeName::
	call	fentbx
; (HL:void)
; stmt EXPR
; EXPR
	.dw	-8  
	ld	hl,-24
	add	hl,sp
	ld	sp,hl
	call _getbasetype
	ld (iy-2),l
; (HL:ulong)
	ld (iy-1),h
; stmt IF
; IF nlbl=0
; (LT:ubyte (DEREF:ubyte (LOCALVAR:ubyte IY-7)) 6:ubyte)
	ld a,(iy-2)
	or (iy-1)
	jp nz,no0_13
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld hl,(_inttype)
	ld (iy-2),l
	ld (iy-1),h
no0_13:	;@dead a,f
	ld l,(iy-2)
	ld h,(iy-1)
	call _parsePtrPfx
	ld c,l
	ld b,h
	ld a,(_cur)
	cp 6
	jp nz,no2_13
	call _gettoken
	ld a,(_cur)
	cp 36
	jp nz,no4_13
; (HL:ulong)
	call _gettoken
@13__W11T:	;@dead a,f
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld a,(_cur)
	cp 36
; (HL:void)
; stmt EXPR
; EXPR
	jp nz,@13__W11B
no6_13:	;@dead a,f
	call _gettoken
	jp @13__W11T	;@dead a,f
@13__W11B:	;@dead a,f
; (HL:void)
; stmt GOTO
	ld hl,15
; stmt LABEL
	push hl
; stmt EXPR
; SEMI
; stmt LABEL
	ld hl,7
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:ubyte (LOCALVAR:ubyte IY-7)))
	call _expect
	pop af
	ld a,(_cur)
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	cp 4
	jp nz,no8_13
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld (iy-7),0
@13__W12T:	;@dead a,f
	ld a,(_cur)
	cp 4
	jp nz,@13__W12B
no10_13:	;@dead a,f
	call _gettoken
	ld a,(_cur)
	cp 5
	jp nz,no12_13
	ld (iy-6),255
	ld (iy-5),255
	ld (iy-4),255
	ld (iy-3),255
	jp no13_13	;@dead a,f
no12_13:	;@dead a,f
	ld hl,5
	call _parseConst
	exx
	ld hl,(_constVal)
	exx
	ld hl,(_constVal+2)
	call qstiy
; (BC:short)
; stmt GOTO
	.db -6
; stmt LABEL
no13_13:	;@dead a,f
; stmt EXPR
; SEMI
; stmt RETURN
; RETURN hasval=1
	ld a,(iy-7)
	cp 6
	jp nc,no14_13
	ld a,(iy-7)
	inc (iy-7)
	ld l,a
	ld h,0
	add hl,hl
	add hl,hl
; (HL:short/v)
	ex de,hl
	push iy
; stmt EXPR
; EXPR
	pop hl
	add hl,de
	ld de,-36
	add hl,de
; (REGVAR:short IX#1)
; stmt EXPR
; EXPR
	push hl
; (HL:ubyte)
; stmt EXPR
; EXPR
	call qldiy
	.db -6
; (BC:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 6:ubyte)
	call qst
	jp no15_13	;@dead a,f
no14_13:	;@dead a,f
; stmt BLOCK
; BLOCK n=10
; stmt EXPR
; EXPR
	ld hl,19
; (HL:ubyte)
; stmt EXPR
; EXPR
	call _gripe
; (HL:void)
; stmt LABEL
no15_13:	;@dead a,f
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=1
; (LNOT:short (LAND:ubyte (DEREF:ubyte (LOCALVAR:ubyte IY-8)) (NE:ubyte (DEREF:ubyte $_cur) 0:ubyte)))
	ld hl,19
	push hl
	ld hl,5
	call _expect
	pop af
	jp @13__W12T	;@dead a,f
@13__W12B:	;@dead a,f
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
@13__W13T:	;@dead a,f
	ld a,(iy-7)
; stmt BLOCK
; BLOCK n=2
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 6:ubyte)
	or a
	jp z,@13__W13B
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
no16_13:	;@dead a,f
; (HL:ubyte)
	ld a,(iy-7)
	dec a
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ubyte $_cur) 7:ubyte)
	ld (iy-7),a
	ld l,a
	ld h,0
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	add hl,hl
; (HL:ubyte)
	add hl,hl
	ex de,hl
; stmt IF
; IF nlbl=0
; (DEREF:ubyte (LOCALVAR:ubyte IY-8))
	push iy
	pop hl
	add hl,de
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld de,-36
; (HL:void)
	add hl,de
; stmt GOTO
	call qld
; stmt LABEL
	push hl
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	push bc
	ld hl,48
	call _getType
; (HL:void)
	pop af
; stmt EXPR
; EXPR
	pop af
	ld c,l
	ld b,h
	jp @13__W13T	;@dead a,f
@13__W13B:	;@dead a,f
	ld hl,0
	push hl
	push bc
	ld hl,16
; (BC:short)
	call _getType
	pop af
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	pop af
	jp	XparseTypeName	;@dead a,f
; (HL:void)
no8_13:	;@dead a,f
	ld hl,11
; stmt RETURN
; RETURN hasval=1
	call _permalloc
	push hl
; (HL:short/v)
	pop ix
	ld (ix+8),8
	ld (ix+6),c
	ld (ix+7),b
; EOF
	ld a,(_cur)
	cp 6
	jp nz,no18_13
	ld (iy-8),1
	call _gettoken
@13__W14T:	;@dead a,f
	ld a,(iy-8)
	or a
	jp z,_C12
	ld a,(_cur)
	cp 0
	jp nz,no20_13
_C12:
	jp @13__W14B	;@dead a,f
no20_13:	;@dead a,f
	ld a,(_cur)
	cp 6
	jp nz,no23_13
	inc (iy-8)
	jp no24_13	;@dead a,f
no23_13:	;@dead a,f
	ld a,(_cur)
	cp 7
	jp nz,no25_13
	dec (iy-8)
no25_13:	;@dead a,f
no24_13:	;@dead a,f
	ld a,(iy-8)
	or a
	call nz,_gettoken
no27_13:	;@dead a,f
	jp @13__W14T	;@dead a,f
@13__W14B:	;@dead a,f
	ld hl,15
	push hl
	ld hl,7
	call _expect
	pop af
no18_13:	;@dead a,f
	ld hl,0
	push hl
	push ix
	ld hl,16
	call _getType
	pop af
	pop af
	ld c,l
	ld b,h
	jp no5_13	;@dead a,f
no4_13:	;@dead a,f
	ld hl,15
	call _gripe
no5_13:	;@dead a,f
no2_13:	;@dead a,f
	ld l,c
	ld h,b
XparseTypeName:
	call	fexbx
	.dw	-12
