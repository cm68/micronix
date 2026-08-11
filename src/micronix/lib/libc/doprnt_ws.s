; Generated from doprnt_ws.o by wsnm -g
	.extern __ctype_
	.extern __pnum
	.extern _atoi
	.extern _fputc
	.extern _strlen
	.extern brelop
	.extern cret
	.extern csv
	.extern indir
	.extern ncsv
	.global __doprnt

	.text
T1:
	call csv
	ld hl,(B0)
	push hl
	ld a,(ix+6)
	ld l,a
	rla
	sbc a,a
	ld h,a
	push hl
	call _fputc
	jp cret
T2:
	call csv
	ld l,(ix+6)
	ld h,(ix+7)
	push hl
	pop iy
	push hl
	call _atoi
	pop bc
	ld a,l
	ld (B1),a
	jr R0
R1:
	inc iy
R0:
	ld e,(iy+0)
	ld d,00h
	ld hl,__ctype_+1
	add hl,de
	bit 2,(hl)
	jr nz,R1
	push iy
	pop hl
	jp cret
__doprnt:
	call ncsv
	rst 30h
	rst 38h
	ld l,(ix+8)
	ld h,(ix+9)
	push hl
	pop iy
	ld l,(ix+6)
	ld h,(ix+7)
	ld (B0),hl
	jp T0
T11:
	ld a,(ix-1)
	cp 25h
	jr z,R2
	ld l,a
	rla
	sbc a,a
	ld h,a
	push hl
	call T1
	pop bc
	jp T0
R2:
	ld (ix-5),00ah
	ld (ix-6),000h
	ld (ix-8),000h
	ld (ix-3),000h
	ld (ix-9),001h
	ld a,(iy+0)
	cp 2dh
	jr nz,R3
	inc iy
	inc (ix-3)
R3:
	ld a,(iy+0)
	cp 30h
	ld hl,0001h
	jr z,R4
	dec hl
R4:
	ld (ix-2),l
	ld e,(iy+0)
	ld d,00h
	ld hl,__ctype_+1
	add hl,de
	bit 2,(hl)
	jr z,R5
	push iy
	call T2
	pop bc
	push hl
	pop iy
	ld a,(B1)
	ld (ix-6),a
	jr R6
R5:
	ld a,(iy+0)
	cp 2ah
	jr nz,R6
	ld l,(ix+10)
	ld h,(ix+11)
	ld a,(hl)
	inc hl
	inc hl
	ld (ix+10),l
	ld (ix+11),h
	ld (ix-6),a
	inc iy
R6:
	ld a,(iy+0)
	cp 2eh
	jr nz,R7
	inc iy
	ld a,(iy+0)
	cp 2ah
	jr nz,R8
	ld l,(ix+10)
	ld h,(ix+11)
	ld a,(hl)
	inc hl
	inc hl
	ld (ix+10),l
	ld (ix+11),h
	ld (ix-7),a
	inc iy
	jr R9
R8:
	push iy
	call T2
	pop bc
	push hl
	pop iy
	ld a,(B1)
	ld (ix-7),a
	jr R9
R7:
	ld a,(ix-2)
	or a
	jr nz,R10
	ld hl,0000h
	jr R11
R10:
	ld l,(ix-6)
	ld h,00h
R11:
	ld (ix-7),l
R9:
	ld a,(iy+0)
	cp 6ch
	jr nz,R12
	inc iy
	ld (ix-9),002h
R12:
	ld a,(iy+0)
	inc iy
	ld (ix-1),a
	or a
	jp z,cret
	cp 44h
	jp z,T3
	cp 4fh
	jr z,R13
	cp 58h
	jp z,T4
	cp 63h
	jp z,T5
	cp 64h
	jr z,R14
	cp 6fh
	jr z,R13
	cp 73h
	jp z,T6
	cp 75h
	jr z,R15
	cp 78h
	jr z,R16
	jp T7
R13:
	ld (ix-5),008h
R15:
	ld a,(ix-3)
	or a
	jr z,R17
	ld a,(ix-6)
	ld (ix-3),a
	ld (ix-6),000h
R17:
	ld a,(ix-1)
	ld e,a
	rla
	sbc a,a
	ld d,a
	ld hl,__ctype_+1
	add hl,de
	bit 0,(hl)
	jr z,R18
	ld (ix-9),002h
R18:
	ld hl,T1
	push hl
	ld l,(ix-5)
	ld h,00h
	push hl
	ld l,(ix-8)
	push hl
	ld l,(ix-6)
	push hl
	ld l,(ix-7)
	push hl
	ld a,(ix-9)
	cp 01h
	jp z,T8
	ld l,(ix+10)
	ld h,(ix+11)
	ld e,(hl)
	inc hl
	ld d,(hl)
	inc hl
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	jp T9
T3:
R14:
	ld (ix-8),001h
	jr R15
T4:
R16:
	ld (ix-5),010h
	jr R15
T6:
	ld l,(ix+10)
	ld h,(ix+11)
	ld c,(hl)
	inc hl
	ld b,(hl)
	ld (B2),bc
	inc hl
	ld (ix+10),l
	ld (ix+11),h
	ld l,c
	ld h,b
	ld a,l
	or h
	jr nz,R19
	ld hl,D0
	ld (B2),hl
R19:
	ld hl,(B2)
	push hl
	call _strlen
	pop bc
	ld (ix-4),l
T10:
	ld a,(ix-7)
	or a
	jr z,R20
	ld b,(ix-4)
	call brelop
	jr nc,R20
	ld a,(ix-7)
	ld (ix-4),a
R20:
	ld b,(ix-6)
	ld a,(ix-4)
	call brelop
	jr nc,R21
	ld a,(ix-6)
	sub (ix-4)
	ld (ix-6),a
	jr R22
R21:
	ld (ix-6),000h
R22:
	ld a,(ix-3)
	or a
	jr nz,R23
	jr R24
R25:
	ld hl,0020h
	push hl
	call T1
	pop bc
R24:
	ld a,(ix-6)
	dec (ix-6)
	or a
	jr nz,R25
	jr R23
R26:
	ld hl,(B2)
	ld a,(hl)
	inc hl
	ld (B2),hl
	ld l,a
	rla
	sbc a,a
	ld h,a
	push hl
	call T1
	pop bc
R23:
	ld a,(ix-4)
	dec (ix-4)
	or a
	jr nz,R26
	ld a,(ix-3)
	or a
	jp z,T0
	jr R27
R28:
	ld hl,0020h
	push hl
	call T1
	pop bc
R27:
	ld a,(ix-6)
	dec (ix-6)
	or a
	jr nz,R28
	jp T0
T5:
	ld l,(ix+10)
	ld h,(ix+11)
	ld a,(hl)
	inc hl
	inc hl
	ld (ix+10),l
	ld (ix+11),h
	ld (ix-1),a
T7:
	push ix
	pop hl
	dec hl
	ld (B2),hl
	ld (ix-4),001h
	jp T10
T8:
	ld a,(ix-8)
	or a
	ld l,(ix+10)
	ld h,(ix+11)
	ld e,(hl)
	inc hl
	ld d,(hl)
	jr nz,R29
	ld hl,0000h
	jr R30
R29:
	ld a,d
	rla
	sbc a,a
	ld l,a
	ld h,a
T9:
R30:
	push hl
	push de
	call __pnum
	exx
	ld hl,000eh
	add hl,sp
	ld sp,hl
	exx
	ld (ix-6),l
	ld l,(ix-9)
	ld h,00h
	add hl,hl
	ex de,hl
	ld l,(ix+10)
	ld h,(ix+11)
	add hl,de
	ld (ix+10),l
	ld (ix+11),h
	jr R31
R32:
	ld hl,0020h
	push hl
	call T1
	pop bc
R31:
	ld b,(ix-3)
	dec (ix-3)
	ld a,(ix-6)
	call brelop
	jr c,R32
T0:
	ld a,(iy+0)
	inc iy
	ld (ix-1),a
	or a
	jp nz,T11
	jp cret

	.data
D0:
	.db	"(null)",000h

	.bss
_.bss:
B1:
	.ds 1
B2:
	.ds 2
B0:
	.ds 2

; vim: tabstop=4 shiftwidth=4 noexpandtab:
