_concheck	equ	0x8383
wrelop	equ	0x8773
_filbuf	equ	0x8299
_freopen	equ	0x759d
_setfcb	equ	0x7c86
_getuid	equ	0x869d
_setuid	equ	0x86b1
_bdos	equ	0x85d3
_unlink	equ	0x863e
exit	equ	0x83ac
_con_check	equ	0x8383
bdosa	equ	0x85d3
adiv	equ	0x8791
lmul	equ	0x8919
__fcb	equ	0x9be1
_fflush	equ	0x847e
__iob	equ	0x9bb9
_fclose	equ	0x83e3
freep	equ	0xa29e
_sbrk	equ	0x899f
_bufallo	equ	0x7c13
_bdoshl	equ	0x8468
_bdosa	equ	0x85d3
brelop	equ	0x875f
indir	equ	0x8a81
cret	equ	0x8a7a
ncsv	equ	0x8a82
csv	equ	0x8a6e
_lose	equ	0x0128
__Lbss	equ	0x9caf
__Hbss	equ	0xa4b0
startup	equ	0x85ed
_argc	equ	0xa4ac
_main	equ	0x367e
_exit	equ	0x83ac
crt	equ	0x0100

	.org	0x0000
	pop hl    ; get f into de
	pop de
	push de
	push hl
	; if (f->flag & IORD) return EOF
	ld hl,6
	add hl,de
	bit 0,(hl)
	jr z,feof
	
	; if (f->base) {
	;     bc = f->base ; count = 512 ;
	; } else {
	;   bc = fend + 1 ; count = 1;
	; }
	dec hl    ; get base
	ld b,(hl)
	dec hl
	ld c,(hl)
	ld a,b
	or c
	jr nz, fbuf   ; if base, use it
	ld bc,fend+1  ; else use fend+1
fbuf:
	ld (m_read+2),bc
	ld bc,0x200   ; count = BUFSIZE
	jr nz,fcnt
	ld bc,1    ; if !base count = 1
fcnt:
	ld (m_read+4),bc
	dec hl
	ld bc,-1  ; assume eof 
	ld (fend+1),bc
	ld bc,0
m_read:   ; read(fd, base, 0x200 - cnt)
	defb 0xcf, 5, 0, 0, 0, 0
	ld a,0x20 ; ERR bit
	jr c,fret
	ld a,h  ; is byte count 0?
	or l
feof:
	ld a,0x10 ; EOF bit
	jr z,fret
	ld b,h  ; save returned byte count for f->cnt
	ld c,l
fret:   ; a = bits for flags, bc = count
	ld hl,6
	add hl,de
	or (hl) 
	ld (hl),a
	or a
	jr z,fr
	ld bc,0  ; if err ret, f->cnt = 0
fr:
	dec hl  ; f->cnt = bc
	dec hl
	dec hl
	ld (hl),b
	dec hl
	ld (hl),c
fend:
	ld hl,-1 ; store into this instruction data field
	ret 
	end
