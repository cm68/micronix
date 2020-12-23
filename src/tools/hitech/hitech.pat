;
; grammar file for the hitech c library replacement
;
; there are 2 distinct hitech libraries that we endeavor to patch:
; the one you get from compiling with the supplied libraries, and
; the one that was used to generate the compiler itself.
;
; they differ in that the supplied library source has stdio built on top
; of open/close/read/write, etc.
;
; the compiler has no such layer, with fread, fopen, fflush, fwrite,
; built directly on top of bdos calls.
;

;
; zcrtcpm.obj - this is org'ed at 0x100, and we can extract a lot of symbols
; directly from it and cascade down from there
;
block crt
	match 0100
		2a 06 00 f9
		11 ANY ANY			; 0x104 bss start
		b7
		21 ANY ANY 			; 0x108 bss end
		ed 52 4d 44
		0b 6b 62 13
		36 00 ed b0
		21 ANY ANY 			; 0x117 nularg
		e5 21 80 00
		4e 23 06 00
		09 36 00
		21 81 00 e5
		CALL ANY ANY		; 0x129 startup
		c1 c1 e5
		2a ANY ANY			; 0x12f argc
		e5
		CALL ANY ANY		; 0x133 _main
		e5
		CALL ANY ANY		; 0x137 _exit
		JUMP 0000			; 0x13a warmboot
	end
	extract crt+5 _Lbss
	extract crt+0x9 _Hbss
	extract crt+0x30 _argc
	extract crt+0x2a startup
	extract crt+0x34 _main
	extract crt+0x38 _exit
	;
	; nop-out the setting of the stack pointer
	;
	replace crt+0
		00 00 00 00
	end
	;
	; micronix gives us argc, argv, right on the stack.
	; we don't need to grunge it out of the unparsed command tail at 0x80
	;
	patch crt+0x17
		pop		bc
		ld		h,b
		ld		l,c
		ld		(_argc),hl
		ex		de,hl
		ld		hl,0
		add		hl,sp
		push	hl
		push	de
		call	_main
		pop		bc
		pop		bc
		push	hl
		call	_exit
	end
end

;
; function prolog that establishes a stack frame pointer in IX and saves IY
;
block csv
	match
		e1
		fd e5
		dd e5
		dd 21 00 00
		dd 39
		e9
	end
end

;
; function prolog that saves IY, puts a frame pointer in IX, and bumps the
; stack pointer to allocate automatic variable space with a negative offset
; off of IX.
;
block ncsv
	match
		e1 
		fd e5 
		dd e5 
		dd 21 00 00 
		dd 39 
		5e 
		23 
		56 
		23 
		eb 
		39 
		f9 
		eb 
		e9 
	end
end

;
; function epilog that pops everything off the stack, restores IY and returns
; to the function's caller.
;
block cret
	match
		dd f9
		dd e1
		fd e1
		c9
	end
end

;
; bdos call, returning 8 bit value, sign extended to HL
;
block bdosa1
	match
		CALL csv
		dd 5e 08
		dd 56 09
		dd 4e 06
		dd e5
		CALL 0005
		dd e1
		6f
		17
		9f
		67
		JUMP cret
	end
end

;
; variation of bdosa1, that saves and restores IY.  redundantly,
; since this is done by csv already.
;
block bdosa
	match
		CALL csv
		dd 5e 08
		dd 56 09
		dd 4e 06
		dd e5
		fd e5
		CALL 0005
		fd e1
		dd e1
		6f
		17
		9f
		67
		JUMP cret
	end
end

;
; bdos call, returning 16 bits
;
block bdoshl
	match
		CALL csv
		dd 5e 08
		dd 56 09
		dd 4e 06
		dd e5
		CALL 0005
		dd e1
		JUMP cret
	end
end

;
; one variation of exit.  calls cpm_clean
; and puts the return address into 0x80, presumably
; where the shell (NOT CCP) can test the exit status
;
block __exit
	match
		CALL ANY ANY
		e1
		e1
		22 0080
		JUMP 0000
	end
	;
	; micronix exit simply uses the value in hl.
	;
	patch __exit
		pop hl
		pop hl
		db 0xcf, 01
	end
end

;
; another variation on exit.  just calls __cpm_clean after
; storing hl into 0x80.  nothing on the stack, apparently
;
block exit
	match
		22 0080
		cd ANY ANY
		c3 0000
	end
	patch exit
		db 0xcf, 01
	end
end

;
; finally, the c version.  this calls _cleanup, which calls fclose on
; all the stdio FILES.  we'll do this too.
; this isn't the one found in the compiler binaries
;
block _exit
	match
		CALL csv
		CALL ANY ANY
		dd 6e 06
		dd 66 07
		e5
		CALL __exit
		JUMP cret			
	end
end

;
; the stdio version found in the compiler
;
block _exit
	match _exit
		CALL csv
		e5
		dd 36 ff 05
		fd 21 ANY ANY
		fd e5
		e1
		01 0008
		09
		e5
		fd e1
		ed 42
		e5
		CALL ANY ANY
		c1
		dd 7e ff
		c6 ff
		dd 77 ff
		b7
		20 e4
		dd 6e 06
		dd 66 07
		22 0080
		CALL 0000
		JUMP cret
	end
	extract _exit+0xa __iob
	extract _exit+0x1a _fclose
end

;
; stdio
;
block _fclose
	match _fclose
		CALL csv
		e5
		dd 6e 06
		dd 66 07
		e5
		fd e1
		11 __iob
	end
	extract _fclose+0x3d _fflush
	patch _fclose+0x48
		ld		l,(IY+7)
		ld		h,0
		db		0xcf, 06
		ld		hl,0
		jp		cret		
		ds		48, 0
	end
end

block _fflush
	match _fflush
		CALL csv
		e5
		dd 6e 06
		dd 66 07
		e5
		fd e1
		11 __iob
		fd e5
		e1
		b7
		ed 52
		11 0008
		cd ANY ANY
		11 0029
		cd ANY ANY
		11 ANY ANY
		19
		dd 75 fe
		dd 74 ff
		fd cb 06 4e
		20 06
		21 ff ff
		JUMP cret
	end
	patch _fflush
		call csv
		ld l,(ix+6)	; get FILE ptr
		ld h,(ix+7)
		push hl
		pop iy
		bit 1,(iy+6) ; if IOWRT
		jr z,retffl
	retffl:
		ld hl,0
		jp cret	
	end
end

block close
	match
		CALL csv
		e5
		06 08
		dd 7e 06
		cd ANY ANY
		38 06
		21 ff ff
		JUMP cret
		11 2a 00
		dd 6e 06
		26 00
		cd ANY ANY
		11 ANY ANY
		19
		e5
		fd e1
	end
end

block write
	match
		CALL ANY ANY
		79 ff
		06 08
		dd 7e 06
		cd ANY ANY
		38 06
		21 ff ff
		JUMP cret
		11 2a 00
		dd 6e 06
		26 00
		CALL ANY ANY
	end
	; write system call:  
	; file descriptor in hl,
	; RST8 04 buf.l buf.h bytes.l bytes.h
	; returns carry clear on good, byte count in hl
	; errno in hl.
	;
	patch write
		ld		hl,7
		add 	hl,sp
		ld		d,(hl)
		dec		hl
		ld		e,(hl)
		dec		hl
		ld		(sys_wr+4),de
		ld		d,(hl)
		dec		hl
		ld		e,(hl)
		dec		hl
		ld		(sys_wr+2),de
		ld		d,(hl)
		dec		hl
		ld		l,(hl)
		ld		h,d
sys_wr:	db		0xcf, 04, 0, 0, 0, 0
		ret		nc
		ld		hl,0xffff
		ret
	end
end

block read
	match
		CALL ANY ANY
		79 ff
		dd 36 fb 00
		dd 36 fc 00
		06 08
		dd 7e 06
		CALL ANY ANY
		38 06
		21 ff ff
		JUMP cret
		11 2a 00
		dd 6e 06
		26 00
		CALL ANY ANY
		11 ANY ANY
		19
		e5
		fd e1
	end
	patch read
		ld		hl, 7
		add		hl, sp
		ld		d,(hl)
		dec		hl
		ld		e,(hl)
		dec		hl	
		ld		(sys_rd+4),de
		ld		d,(hl)
		dec		hl
		ld		e,(hl)
		dec		hl
		ld		(sys_rd+2),de
		ld		d,(hl)
		dec		hl
		ld		l,(hl)
		ld		h,d
sys_rd:	db		0xcf, 03, 00, 00, 00, 00
		ret		nc
		ld		hl,0xffff
		ret
	end
end

block open
	match
		CALL csv
		e5
		dd 5e 08
		dd 56 09
		13
		dd 73 08
		dd 72 09
		21 03 00
		CALL ANY ANY
		f2 ANY ANY
		dd 36 08 03
		dd 36 09 00
		CALL ANY ANY
		e5
		fd e1
	end
	replace open
		21 05 00			; 00: ld hl, 5
		39					; 03: add hl, sp
		56 2b				; 04: ld d,(hl) ; dec hl
		5e 2b				; 06: ld e,(hl) ; dec hl
		ed 53 open+0x18		; 08: ld (open+),de
		56 2b				; 0c: ld d,(hl) ; dec hl
		5e 2b				; 0e: ld e,(hl) ; dec hl
		ed 53 open+0x16		; 10: ld (open+),de
		cf 05 00 00 00 00	; 14: open name, mode
		d0					
		21 ff ff
		c9
	end
end

block frm
	CALL csv
	e5 e5
	dd
end

block unlink
	match
	CALL ncsv
	d3 ff
	dd 6e 06
	dd 66 07
	e5
	dd e5
	d1
	21 d6 ff
	19
	e5
	CALL ANY ANY
	c1
	c1
	7d
	b7
	28 06
	21 00 00
	JUMP cret
	end
end

block creat
	match
	end
	patch creat
		ld hl, 5
		add hl, sp
		ld d,(hl)
		dec hl
		ld e,(hl)
		dec hl
		ld (crsys+4),de
		ld d,(hl)
		dec hl
		ld e,(hl)
		dec hl
		ld (crsys+2),de
	crsys: db cf 05 00 00 00 00
		ret nc
		ld hl, 0xffff
		ret
	end
end

;
; the compiler system interface uses 
; fopen, fclose, fwrite, fread, fflush
;
; iob { char *ptr, int cnt, char *base, char flag, char fd }
; flags (bits 0-7): rd wr nbuf mybuf eof err string binary
;

block filbuf
	match
	end
	; f->cnt = 0
	; if (f->flag & IORD) return EOF
	; if (f->base == 0) {
	;     f->ptr = &tmp
	; count = 1;
	; } else {
	;     f->ptr = f->base
	;	  count = BUFSIZE
	; } 
	; f->cnt = read(f->fd, f->ptr, count);
	; if (f->cnt > 0) {
	;   f->cnt--;
	;   return(*f->fptr++);
	; }
    ; if (f->cnt == 0) f->flag |= EOF
	; if (f->cnt < 0) f->flag |= ERR
	; return eof
	;
	patch
		pop hl				; get f into de
		pop de
		push de
		push hl

		ld hl,6				; if ! f->flag & IORD
		add hl,de
		bit 0,(hl)
		jr z,feof
	
		dec hl				; get base
		ld b,(hl)
		dec hl
		ld c,(hl)
		ld a,b
		or c
		jr nz, fbuf			; if base, use it
		ld bc,fend+1		; else use fend+1
	fbuf:
		ld (m_read+2),bc

		ld bc,0x200			; count = BUFSIZE
		jz nz,fcnt
		ld bc,1				; if !base count = 1
	fcnt:
		ld (m_read+4),bc
		dec hl
		ld bc,-1		; assume eof 
		ld (fend+1),bc
		ld bc,0

	m_read:			; read(fd, base, 0x200 - cnt)
		defb 0xcf, 5, 0, 0, 0, 0

		ld a,0x10	; ERR bit
		jr c,ferr
		ld a,h
		or l

	feof:
		ld a,0x08	; EOF bit
		jr z,fret

		ld b,h		; save returned byte count for f->cnt
		ld c,l

	ferr:
		ld (_errno), hl

	fret:			; a = bits for flags, bc = count
		ld hl,6
		add hl,de
		or (hl)	
		ld (hl),a
		or a
		jr z,fr
		ld bc,0		; if err ret, f->cnt = 0
	fr:
		dec hl		; f->cnt = bc
		dec hl
		dec hl
		ld (hl),b
		dec hl
		ld (hl),c
		ld a,c
		or b
		jr nz,fneof

	fend:
		ld hl,-1	; store into this instruction data field
		ret		
	end
end

block fflush
	match
	end
	;
	; if(!(f->_flag & _IOWRT) || 
	;    f->_base == (char *)NULL || 
	; (cnt = BUFSIZ - f->_cnt) == 0)
	;		return 0;
	; if (write(fileno(f), f->_base, cnt) != cnt)
	;	f->_flag |= _IOERR;
	;	f->_cnt = BUFSIZ;
	;	f->_ptr = f->_base;
	;	if(f->_flag & _IOERR)
	;		return(EOF);
	;	return 0;
	;
	patch fflush
		pop hl			; de = file *
		pop de
		push de
		push hl
		ld hl,6			; get flags
		add hl,de
		bit 1,(hl)		; not write
		jr z, ferr

		ld hl,2
		add hl,de
		ld c,(hl)		; bc = cnt
		inc hl
		ld b,(hl)
		ld hl,0x200		; BUFSIZE - cnt
		or a
		sbc hl,bc
		ld a,h
		or l
		jr z,fend
		ld b,h			; bc = to_write
		ld c,l
	
		ld hl,4			; if base == 0, no buffer
		add hl,de
		ld a,(hl)
		inc hl
		or (hl)
		jr z,fend

		push de			; save our file *
		push bc			; save our count

		ld (m_write+4),bc
		ld b,(hl)
		dec hl
		ld c,(hl)
		ld (m_write+2),bc
		inc hl
		inc hl
		inc hl
		ld l,(hl)
		ld h,0

	m_write:			; write(fd, base, 0x200 - cnt)
		defb 0xcf, 5, 0, 0, 0, 0

		pop bc			; restore file *, cnt
		pop de

		ld a,h			; if ret != cnt
		cp b
		jr nz,fioer
		ld a,l
		cp c
		jr z,fok
	fioer:
		ld hl,6
		add hl,de
		ld a,(hl)		; set error
		or 0x20
		ld (hl),a
	fok:	
		dec hl
		ld b,(hl)
		dec hl
		ld c,(hl)
		dec hl
		ld (hl),2		; f->cnt = 0x200
		dec hl
		ld (hl),0
		dec hl
		ld (hl),b		; f->ptr = f->base
		dec hl
		ld (hl),c	
		and 0x20
		jr nz,ferr
	fend:
		ld hl, 0
		ret	
	ferr:
		ld hl, 0xffff
		ret
	end	
end
