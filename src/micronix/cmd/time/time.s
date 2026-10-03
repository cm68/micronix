str0:
	.db "time: cannot fork", 0x0a, 0x00
str1:
	.db "%s: not found", 0x0a, 0x00
str2:
	.db "Command terminated abnormally.", 0x0a, 0x00
str3:
	.db 0x0a, "real %ldm%lds", 0x0a, 0x00
; top FUNC
; FUNC _main:s
; params=2 locals=4 frame=10 framefree=0
; param argc:s r=0 o=4
; param argv:s r=0 o=6
; local p:s r=3
; local status:s r=0
; local after:l r=0
; local before:l r=0
str4:
	.db ".", 0x00
str5:
; stmt BLOCK
; BLOCK n=18
; stmt IF
; IF nlbl=0
; (LE:ubyte (DEREF:short (LOCALVAR:short IY+4)) 1:short)
	.db "/bin", 0x00
str6:
	.db "/usr/bin", 0x00
str7:
	.db "/", 0x00
	.data
_execvp.1:
	.dw str4
	.dw str5
	.dw str6
	.dw 0
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	.text
_main::
; (HL:short)
; stmt EXPR
; EXPR
	call	fentbw
	.dw	-10  
	ld de,1
	ex de,hl
; (HL:long)
; stmt EXPR
; EXPR
	or a
	sbc hl,de
	ld a,h
; (BC:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:short BC) -1:short)
	jp po,$+5
	xor 80h
	or a
	jp m,no0_1
	ld hl,0
	call _exit
; stmt BLOCK
; BLOCK n=2
; stmt EXPR
; EXPR
no0_1:
	push iy
	pop hl
	ld de,-10
	add hl,de
; (HL:short)
; stmt EXPR
; EXPR
	call _time
	call _fork
; (HL:short)
	ld c,l
; stmt IF
; IF nlbl=0
; (EQ:ubyte (REGVAR:short BC) 0:short)
	ld b,h
	ld l,c
	ld h,b
; stmt BLOCK
; BLOCK n=3
; stmt EXPR
; EXPR
	ld de,1
	add hl,de
	jp nc,no2_1
	ld hl,str0
	push hl
	ld hl,__iob+16
	call _fprintf
	pop af
	ld hl,1
	call _exit
no2_1:
	ld a,c
	or b
	jp nz,no4_1
; (HL:short)
; stmt EXPR
; EXPR
	ld l,(iy+6)
	ld h,(iy+7)
	inc hl
	inc hl
	push hl
	ld l,(iy+6)
	ld h,(iy+7)
	inc hl
	inc hl
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	call _execvp
	pop af
; (HL:short)
; stmt EXPR
; EXPR
	ld l,(iy+6)
	ld h,(iy+7)
; (HL:short)
	inc hl
; stmt EXPR
; EXPR
	inc hl
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
; (HL:short)
; stmt EXPR
; EXPR
	push hl
	ld hl,str1
	push hl
	ld hl,__iob+16
	call _fprintf
; (HL:short)
; stmt LABEL
	pop af
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (NE:ubyte (CALL:short/1 $_wait (LOCALVAR:short/s IY-2)) (REGVAR:short BC)))
	pop af
	ld hl,1
	call _exit
no4_1:
	ld hl,1
	push hl
	ld hl,2
	call _signal
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	pop af
	ld hl,1
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; SEMI
; stmt GOTO
	push hl
; stmt LABEL
	ld hl,3
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	call _signal
	pop af
@1__W1T:
	push iy
	pop hl
; (HL:long)
; stmt IF
; IF nlbl=0
; (NE:ubyte (AND:ubyte (DEREF:ubyte (LOCALVAR:short IY-2)) 255:ubyte) 0:ubyte)
	ld de,-2
	add hl,de
	call _wait
	or a
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	sbc hl,bc
	jp z,@1__W1B
no6_1:
	jp @1__W1T
; (HL:short)
@1__W1B:
; stmt EXPR
; EXPR
	push iy
	pop hl
	ld de,-6
	add hl,de
	call _time
	ld a,(iy-2)
	and 255
	cp 0
	jp z,no8_1
	ld hl,str2
	push hl
	ld hl,__iob+16
	call _fprintf
	pop af
no8_1:
	call qldiy
	.db -10
	push hl
	exx
	push hl
	exx
	call qldiy
	.db -6
	exx
	pop de
	exx
	pop de
	call qsub
	ld de,60
	exx
	ld de,0
	exx
	call qmod
	push hl
	exx
	push hl
	exx
	call qldiy
	.db -10
	push hl
	exx
	push hl
	exx
	call qldiy
	.db -6
	exx
	pop de
	exx
	pop de
	call qsub
	ld de,60
	exx
	ld de,0
; (HL:short)
; stmt EXPR
; EXPR
	exx
	call qdiv
	push hl
	exx
	push hl
	exx
	ld hl,str3
	push hl
; (HL:short)
	ld hl,__iob+16
	call _fprintf
	pop af
; top FUNC
; FUNC _execvp:s
; params=2 locals=3 frame=132 framefree=0
; param name:s r=0 o=4
; param argv:s r=0 o=6
; local i:s r=3
; local buf:s r=0
; local _execvp.0:s r=0
	pop af
	pop af
	pop af
	pop af
	ld l,(iy-2)
	ld h,(iy-1)
; stmt BLOCK
; BLOCK n=13
; stmt IF
; IF nlbl=0
; (CALL:short/2 $_strchr 47:ubyte/s (DEREF:short/s (LOCALVAR:short IY+4)))
	ld l,h
	ld a,h
	rla
	sbc a,a
	ld h,a
	call _exit
Xmain:
	call	fexbw
	.dw	-12
; stmt BLOCK
; BLOCK n=1
; stmt RETURN
; RETURN hasval=1
_execvp::
	call	fentbw
	.dw	0  
	ld	hl,-130
	add	hl,sp
	ld	sp,hl
	ld hl,47
; (HL:short/v)
	push hl
	ld l,(iy+4)
; stmt EXPR
; EXPR
	ld h,(iy+5)
; (BC:short)
; stmt LABEL
	call _strchr
; stmt EXPR
; SEMI
; stmt IF
; IF nlbl=0
; (LNOT:short (DEREF:short (ADD:short $_execvp.1 (MUL:short (REGVAR:short BC) 2:short))))
	pop af
	ld a,l
	or h
	jp z,no0_2
	ld l,(iy+6)
	ld h,(iy+7)
	push hl
	ld l,(iy+4)
	ld h,(iy+5)
	call _execv
	pop af
	jp	Xexecvp
no0_2:
; stmt BLOCK
; BLOCK n=1
; stmt GOTO
	ld bc,0
@2__F2T:
; stmt BLOCK
; BLOCK n=4
; stmt EXPR
; EXPR
	ld l,c
	ld h,b
	add hl,hl
	ex de,hl
	ld hl,_execvp.1
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld a,l
	or h
	jp z,@2__F2B
no2_2:
	ld l,c
	ld h,b
; (HL:short)
; stmt EXPR
; EXPR
	add hl,hl
	ex de,hl
	ld hl,_execvp.1
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
; (HL:short)
; stmt EXPR
; EXPR
	push hl
	push iy
	pop hl
	ld de,-132
	add hl,de
	call _strcpy
	pop af
	ld hl,str7
	push hl
; (HL:short)
; stmt IF
; IF nlbl=0
; (EQ:ubyte (CALL:short/2 $_access 1:ubyte/s (LOCALVAR:short/s IY-132)) 0:short)
	push iy
	pop hl
	ld de,-132
	add hl,de
	call _strcat
	pop af
	ld l,(iy+4)
	ld h,(iy+5)
	push hl
	push iy
	pop hl
; stmt BLOCK
; BLOCK n=1
; stmt EXPR
; EXPR
	ld de,-132
	add hl,de
	call _strcat
	pop af
	ld hl,1
	push hl
	push iy
	pop hl
	ld de,-132
; (HL:short)
	add hl,de
; stmt LABEL
	call _access
; stmt EXPR
; SEMI
; stmt EXPR
; EXPR
	pop af
; (HL:short)
; stmt GOTO
	ld a,l
; stmt LABEL
	or h
; stmt EXPR
; SEMI
; stmt RETURN
; RETURN hasval=1
	jp nz,no4_2
; (HL:short/v)
	ld l,(iy+6)
	ld h,(iy+7)
	push hl
	push iy
; EOF
	pop hl
	ld de,-132
	add hl,de
	call _execv
	pop af
no4_2:
@2__F2C:
	inc bc
	jp @2__F2T
@2__F2B:
	ld hl,-1
Xexecvp:
	call	fexbw
	.dw	-2
