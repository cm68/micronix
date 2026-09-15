	.data
_resched::
	.db 0
; top FUNC
; FUNC _wakeup:s
; params=1 locals=1 frame=0 framefree=0
; param event:s r=0 o=4
; local p:s r=4
	.bss
_next.1:
	.ds 24
; stmt BLOCK
; BLOCK n=11
; stmt EXPR
; EXPR
	.text
; (REGVAR:short IX#1)
; stmt LABEL
str0:
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (LT:ubyte (REGVAR:ushort IX) (ADD:ushort $_plist 1960:ushort)))
	.db "sched umode=%x", 0x0a, 0x00
str1:
	.db "sched nmode=%x", 0x0a, 0x00
	.data
_sched.4:
	.dw _plist
	.bss
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
_sched.5:
	.ds 1
; stmt BLOCK
; BLOCK n=1
; stmt IF
; IF nlbl=0
; (EQ:ubyte (DEREF:ushort (ADD:short (REGVAR:short IX) 10:short)) (DEREF:short (LOCALVAR:short IY+4)))
	.text
_wakeup::
	call	fentxw
	.dw	0  
	ld ix,_plist
@1__F1T:
	ld hl,_plist+1960
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	push ix
	pop de
	ex de,hl
; (HL:short)
	or a
; stmt LABEL
	sbc hl,de
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	jp nc,@1__F1B
no0_1:
	ld l,(ix+10)
	ld h,(ix+11)
	ld e,(iy+4)
; (REGVAR:short IX#1)
; stmt GOTO
	ld d,(iy+5)
; stmt LABEL
	or a
; stmt EXPR
; SEMI
	sbc hl,de
	jp nz,no2_1
	push ix
; top FUNC
; FUNC _run:s
; params=1 locals=0 frame=0 framefree=0
; param p:s r=4 o=4
	pop hl
	call _run
no2_1:
@1__F1C:
	push ix
	pop hl
	ld de,98
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	add hl,de
; (A:ubyte)
; stmt EXPR
; EXPR
	push hl
	pop ix
; (HL:ushort)
; stmt IF
; IF nlbl=0
; (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short))) 8:short)
	jp @1__F1T
@1__F1B:
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
Xwakeup:
	call	fexxw
; (A:byte)
	.dw	-2
_run::
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:byte $_swapping))
	push	ix    
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
        
	push	hl
; (HL:short)
	pop	ix
	set 2,(ix+8)
	ld (ix+10),0
	ld (ix+11),0
	bit 3,(ix+8)
; top FUNC
; FUNC _sleep:s
; params=2 locals=0 frame=0 framefree=0
; param event:s r=0 o=4
; param pri:s r=0 o=6
	jp z,no0_2
	ld a,1
; stmt BLOCK
; BLOCK n=7
; stmt EXPR
; EXPR
	ld (_resched),a
	jp no1_2
no0_2:
	ld a,(_swapping)
	or a
	jp nz,no2_2
	ld hl,(_swapproc)
	call _run
no2_2:
no1_2:
Xrun:
; (HL:ushort)
; stmt EXPR
; EXPR
	pop	ix
	ret
_sleep::
	call	fenterw
	ld hl,(_u+545)
	ld de,10
	add hl,de
; (A:ubyte)
; stmt EXPR
; EXPR
	push hl
	ld l,(iy+4)
	ld h,(iy+5)
	pop de
; (A:ubyte)
; stmt IF
; IF nlbl=1
; (LAND:ubyte (DEREF:byte $_memwant) (LNOT:short (AND:short (WIDEN:short (DEREF:ubyte (ADD:short (DEREF:short (ADD:short $_u 545:short)) 8:short))) 32:short)))
	ex de,hl
	ld (hl),e
	inc hl
	ld (hl),d
	ld hl,(_u+545)
	ld de,92
	add hl,de
	push hl
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld a,(iy+6)
	pop hl
; (HL:short)
	ld (hl),a
; stmt EXPR
; EXPR
	ld hl,(_u+545)
	ld de,8
; (HL:short)
; stmt EXPR
; EXPR
	add hl,de
; (HL:short)
; stmt EXPR
; EXPR
	res 2,(hl)
; (HL:short)
	ld a,(_memwant)
	or a
; top FUNC
; FUNC _next:s
; params=1 locals=3 frame=2 framefree=0
; param dummy:s r=0 o=4
; local force_frame:s r=0
; local _next.0:s r=0
; local n:s r=4
	jp z,no0_3
	ld hl,(_u+545)
	ld de,8
; stmt BLOCK
; BLOCK n=4
; stmt IF
; IF nlbl=0
; (NE:ubyte (ASSIGN:short (REGVAR:short IX) (CALL:short/0 $_sched)) (DEREF:short (ADD:short $_u 545:short)))
	add hl,de
	bit 5,(hl)
	jp nz,no0_3
	ld hl,(_swapproc)
	call _run
no0_3:
	ld hl,0
	call _next
	call _abort
	call _enable
Xsleep:
	jp	fexitw
; stmt BLOCK
; BLOCK n=5
; stmt EXPR
; EXPR
_next::
	call	fentxw
	.dw	-2  
	call _sched
	push hl
	pop ix
	push ix
; (HL:short)
; stmt EXPR
; EXPR
	ld hl,(_u+545)
; (HL:short)
; stmt EXPR
; EXPR
	pop de
	ex de,hl
	or a
	sbc hl,de
	jp z,no0_4
; (HL:short)
; stmt EXPR
; EXPR
	ld hl,(_u+545)
	ld de,16
	add hl,de
; (HL:short)
; stmt EXPR
; EXPR
	push hl
	ld hl,(_u+545)
	ld de,14
	add hl,de
	call _saveframe
	pop af
	call _di
	ld hl,_next.1+24
	push hl
	ld hl,_next.1+24
	call _setframe
	pop af
	push ix
	pop hl
	call _newmap
	ld hl,(_u+545)
	ld de,16
; (HL:short)
	add hl,de
; stmt EXPR
; EXPR
	ld a,(hl)
	inc hl
; (A:byte)
; stmt EXPR
; EXPR
	ld h,(hl)
; (HL:short)
; stmt RETURN
; RETURN hasval=1
	ld l,a
; (HL:short/v)
	push hl
	ld hl,(_u+545)
	ld de,14
	add hl,de
; top FUNC
; FUNC _sched:s
; params=0 locals=4 frame=0 framefree=1
; local _sched.3:B r=0
; local _sched.2:s r=0
; local p:s r=4
; local L0:s r=3
	ld a,(hl)
	inc hl
	ld h,(hl)
; stmt BLOCK
; BLOCK n=13
; stmt EXPR
; EXPR
	ld l,a
; (HL:short)
; stmt EXPR
; EXPR
	call _setframe
; (BC:short)
; stmt LABEL
	pop af
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (EQ:ubyte (REGVAR:short BC) 0:short))
no0_4:
	ld a,0
	ld (_resched),a
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	call _enable
	ld hl,0
; stmt BLOCK
; BLOCK n=11
; stmt EXPR
; EXPR
Xnext:
	call	fexxw
	.dw	-4
_sched::
	push	bc
	push	ix
; (HL:short)
; stmt LABEL
	call _enable
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (LT:ubyte (DEREF:ubyte $_sched.5) 20:ubyte))
	ld bc,0
@5__W2T:
	ld a,c
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	or b
; stmt BLOCK
; BLOCK n=3
; stmt IF
; IF nlbl=0
; (LE:ubyte (ADD:ushort $_plist 1960:short) (REGVAR:ushort IX))
	jp nz,@5__W2B
no0_5:
	ld hl,(_sched.4)
	push hl
	pop ix
	ld a,0
	ld (_sched.5),a
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld l,a
; (REGVAR:short IX#1)
	ld h,0
; stmt IF
; IF nlbl=0
; (NE:ubyte (AND:ubyte (DEREF:ubyte (ADD:short (REGVAR:short IX) 8:short)) 14:ubyte) 14:ubyte)
@5__F3T:
	ld a,(_sched.5)
	cp 20
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	jp nc,@5__F3B
no2_5:
; stmt IF
; IF nlbl=1
; (LOR:ubyte (EQ:ubyte (REGVAR:short BC) 0:short) (LT:ubyte (DEREF:ushort (ADD:short (REGVAR:short BC) 91:short)) (DEREF:ushort (ADD:short (REGVAR:short IX) 91:short))))
	ld hl,_plist+1960
	push ix
	pop de
	ex de,hl
	or a
	sbc hl,de
	jp c,no4_5
	ld ix,_plist
no4_5:
	ld a,(ix+8)
	and 14
	cp 14
	jp nz,@5__F3C
no6_5:
	ld a,c
	or b
	jp z,_C0
	ld l,c
	ld h,b
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld de,91
	add hl,de
; (BC:short)
	ld a,(hl)
; stmt LABEL
	inc hl
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	ld h,(hl)
	ld l,a
	push hl
	ld l,(ix+91)
	ld h,(ix+92)
	pop de
	ex de,hl
	or a
	sbc hl,de
	jp nc,no8_5
_C0:
; (HL:short)
; stmt GOTO
	ld c,ixl
; stmt LABEL
	ld b,ixh
; stmt EXPR
; SEMI
; stmt GOTO
no8_5:
; stmt LABEL
@5__F3C:
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	push ix
	pop hl
	ld de,98
	add hl,de
	push hl
; (HL:short)
; stmt EXPR
; EXPR
	pop ix
	ld hl,_sched.5
	ld a,(hl)
	inc (hl)
	ld l,a
	ld h,0
	jp @5__F3T
@5__F3B:
	jp @5__W2T
@5__W2B:
; (HL:short)
; stmt EXPR
; EXPR
	ld l,c
	ld h,b
	ld de,98
	add hl,de
	ld (_sched.4),hl
	ld hl,(_u+545)
	ld de,8
	add hl,de
	ld l,(hl)
	ld h,0
; (HL:short)
; stmt RETURN
; RETURN hasval=1
	push hl
	ld hl,str0
; (HL:short/v)
	call _pr
	pop af
	ld l,c
	ld h,b
	ld de,8
; EOF
	add hl,de
	ld l,(hl)
	ld h,0
	push hl
	ld hl,str1
	call _pr
	pop af
	ld l,c
	ld h,b
Xsched:
	pop	ix
	pop	bc
	ret
