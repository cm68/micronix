;
; grammar file for the hitech c library replacement
;
; blocks with no patch are used as pattern verifiers for called
; functions and to extract labels
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
; the intention is to have the patch completely fill the space occupied
; by the replaced function, padding it out with NOPs
;
; zcrtcpm.obj - this is org'ed at 0x100, and we can extract a lot of symbols
; directly from it and cascade down from there
;
; the stdio from hitech has an array of fcb's corresponding to the file entries.
; the fcb has an 32 bit file offset after the usual cp/m fcb.
;
; symbols starting with _ are c-visible, either generated by the compiler
; or explicitly in assembly code to be seen by C code.
;

;
; c startup code found at 0x100
; CRTCPM.OBJ
;
block crt
	match 0100
		2a 06 00 f9
		11 __Lbss
		b7
		21 __Hbss
		ed 52 4d 44
		0b 6b 62 13
		36 00 ed b0
		21 ANY ANY
		e5 21 80 00
		4e 23 06 00
		09 36 00
		21 81 00 e5
		CALL startup c1 c1 e5
		2a _argc
		e5
		CALL _main e5
		CALL _exit
		JUMP 0000
	end

	;
	; micronix gives us argc, argv, right on the stack.
	; we don't need to grunge it out of the unparsed command tail at 0x80

	patch crt 61
		ld		de,__Lbss
		xor		a
		ld		hl,__Hbss	
		sbc		hl,de
		ld		c,l
		ld		b,h
		dec		bc
		ld		l,e
		ld		h,d
		inc		de
		ld		(hl),a
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
		jp		_exit
	end
	;
	; make a dummy function that is guaranteed to breakpoint
	;
	define lose crt+40
	patch lose
		call 0005
	end
end

;
; a whole lot of patterns for recognizing symbols for other blocks
;

;
; function prolog that establishes a stack frame pointer in IX and saves IY
; csv.obj
;
block csv
	match
		e1 fd e5 dd e5 dd 21 00 00 
		dd 39 e9
	end
end

;
; function prolog that saves IY, puts a frame pointer in IX, and bumps the
; stack pointer to allocate automatic variable space with a negative offset
; off of IX.
; csv.obj
;
block ncsv
	match
		e1 fd e5 dd e5 dd 21 00 00 
		dd 39 5e 23 56 23 eb 39 f9 
		eb e9 
	end
end

;
; function epilog that pops everything off the stack, restores IY and returns
; to the function's caller.
; csv.obj
;
block cret
	match
		dd f9 dd e1 fd e1
		RET
	end
end

;
; function that compares bytes, used as anchor
; brelop.obj
;
block brelop
	match
		d5 5f a8 
		fa ANY ANY
		7b 98 d1 RET
		7b e6 80 57 7b 98 7a 3c d1 RET
	end
end

;
; bdos call, returning 8 bit value, sign extended to HL
;
block _bdosa1
	match
		CALL csv
		dd 5e 08 dd 56 09
		dd 4e 06 
		dd e5 CALL 0005 dd e1
		6f 17 9f 67
		JUMP cret
	end
end

;
; variation of bdosa1, that saves and restores IY.  redundantly,
; since this is done by csv already.
; bdos.obj
;
block _bdosa
	match
		CALL csv
		dd 5e 08 dd 56 09 dd 4e 06
		dd e5 fd e5 CALL 0005 fd e1 dd e1 
		6f 17 9f 67
		JUMP cret
	end
end

;
; bdos call, returning 16 bits
; bdoshl.obj
;
block _bdoshl
	match
		CALL csv 
		dd 5e 08 dd 56 09 dd 4e 06 
		dd e5 CALL 0005 dd e1
		JUMP cret
	end
end

;
; one variation of exit.  calls cpm_clean
; and puts the return address into 0x80, presumably
; where the shell (NOT CCP) can test the exit status
; _exit.obj
;
block __exit
	match
		CALL cpm_clean e1 e1 
		22 0080
		JUMP 0000
	end
	;
	; micronix exit simply uses the value in hl.
	;
	patch __exit 12
		pop hl			; return address
		pop hl			; exit code
		db 0xcf, 01		; exit(hl)
	end
end

;
; another variation on exit.  just calls __cpm_clean after
; storing hl into 0x80.  nothing on the stack, apparently
;
block exit
	match
		22 0080
		CALL __cpm_clean
		JUMP 0000
	end
	patch exit
		db 0xcf, 01		; exit(hl)
	end
end

;
; finally, the c version.  this calls _cleanup, which calls fclose on
; all the stdio FILES.  we'll do this too.
; this isn't the one found in the compiler binaries
; exit.obj
;
block _exit
	match
		CALL csv
		CALL __cleanup
		dd 6e 06 dd 66 07 
		e5 CALL __exit
		JUMP cret			
	end
end

;
; the stdio version found in the compiler
; - note: no source code available.
;
block _exit
	match
		CALL csv
		e5				
		dd 36 ff 05	
		fd 21 __iob
		fd e5 e1
		01 0008 09 e5 fd e1 ed 42
		e5 CALL _fclose c1
		dd 7e ff c6 ff
		dd 77 ff b7 20 e4
		dd 6e 06 dd 66 07
		22 0080
		CALL 0000
		JUMP cret
	end
end

;
; open the file, the cp/m version builds an fcb and so on
; open.obj
;
block _open
	match
		CALL csv 
		e5 
		dd 5e 08 dd 56 09 13 dd 73 08 dd 72 09
		21 03 00 CALL wrelop
		f2 ANY ANY 
		dd 36 08 03 dd 36 09 00
		CALL _getfcb e5 
		fd e1 7d b4 20 06 21 ff ff
		JUMP cret 
		dd 6e 06 dd 66 07 
		e5 fd e5 CALL _setfcb c1 c1 
		7d b7 20 68 
		11 01 00
		dd 6e 08 dd 66 09 b7 ed 52 20 19 
		21 0c 00 e5 CALL _bdos c1 
		7d 06 30 CALL brelop fa ANY ANY
		fd 7e 06 f6 80 fd 77 06
		CALL _getuid 
		dd 75 ff fd 6e 29 26 00 
		e5 CALL _setuid c1 
		fd e5 21 0f 00 e5 CALL _bdos c1 c1 
		7d fe ff 20 11 
		fd e5 CALL _putfcb dd 6e ff 26 00 e3
		CALL _setuid c1 18 90 
		dd 6e ff 26 00 
		e5 CALL _setuid c1 
		dd 7e 08 fd 77 28 11 __fcb
		fd e5 e1 b7 ed 52 11 2a 00 CALL adiv
		JUMP cret 
	end
	;
	; the micronix version is truly simple.
	; this is a boon, since we use the space after the new open code
	; to contain some worker subroutines.
	;
	; while we are here, let's set the file position.  
	; this is stored in the FCB.
	;
	define fcbcalc _open+51
	patch _open 190
		ld		hl, 5
		add 	hl, sp
		ld		d,(hl)
		dec		hl
		ld		e,(hl)
		dec		hl
		ld		(sys_open+4),de
		ld		d,(hl)
		dec		hl
		ld		e,(hl)
		dec		hl
		ld		(sys_open+2),de
sys_open:
		db		0xcf, 05, 00, 00, 00, 00
		jr		c,openfail
		push	hl
		ld		a,l

		call	fcbcalc
		ld		de,36
		add		hl,de
		xor		a
		ld		(hl),a
		inc		hl
		ld		(hl),a
		inc		hl
		ld		(hl),a
		inc		hl
		ld		(hl),a
		pop		hl	
		ret
openfail:
		ld		hl,0xffff
		ret
	end
	;
	; given the file number in a, return the fcb in hl
	;
	patch fcbcalc
		ld		de,__fcb
		ld		l,a
		ld		h,0
		add		hl,hl
		ex		de,hl
		add		hl,de
		ex		de,hl
		add		hl,hl
		add		hl,hl
		ex		de,hl
		add		hl,de
		ex		de,hl
		add		hl,hl
		add		hl,hl
		add		hl,de
		ret
	end
end

;
; this is the fclose found in the compiler passes
;
block _fclose
	match _fclose
		CALL csv
		e5 
		dd 6e 06 dd 66 07 e5 fd e1 
		11 __iob fd e5 e1 b7 ed 52
		11 08 00 CALL adiv
		11 29 00 CALL lmul
		11 __fcb 19
		dd 75 fe dd 74 ff
		fd 7e 06 e6 03 b7 20 06
		21 ff ff JUMP cret
		fd e5 CALL _fflush c1 
		fd 7e 06 e6 f8
		fd 77 06 
		dd 5e fe dd 56 ff
		21 24 00 19 7e fe 02 28 0c
		21 0c 00 e5 CALL _bdoshl c1 
		cb 44 28 10
		dd 6e fe dd 66 ff 
		e5 21 10 00 e5 CALL _bdosa c1 c1
		dd 5e fe dd 56 ff
		21 24 00 19 36 00
		21 00 00 JUMP cret
	end
	patch _fclose 133
		call	csv
		ld		l,(ix+6)
		ld		h,(ix+7)
		push	hl
		pop		iy
		ld		a,(iy+6)
		and		0x3
		jr		nz,noflush
		push	iy
		call	_fflush
		pop		bc
		ld		a,(iy+6)
		and		0xf8
		ld		(iy+6),a
	noflush:
		ld		l,(iy+7)
		ld		h,0
		db		0xcf, 06
		ld		hl,0
		jp		cret
		ret
	end
end

;
; this is the version of fclose that is in cpp, etc
; fclose.o bj
;
block _fclose
	match
		CALL csv
		dd 6e 06 dd 66 07 e5 fd e1
		fd 7e 06 e6 03 b7 20 06
		21 ff ff
		JUMP cret
		fd e5 CALL _fflush c1 
		fd 7e 06 e6 f8 fd 77 06
		fd 7e 04 fd b6 05 28 19 
		fd cb 06 5e 20 13 
		fd 6e 04 fd 66 05 e5 CALL __buffree c1 
		fd 36 04 00 fd 36 05 00
		fd 6e 07 26 00 e5 CALL _close c1 
		11 ff ff b7 ed 52 28 b9
		fd cb 06 6e 20 b3 21 00 00 
		JUMP cret 
	end
	patch
		call lose
	end
end

;
; we find 2 versions of fflush.  this one, which is a simple C implementation
; found in fflush.obj
;
block _fflush
	; guaranteed to fail
	match _fflush
		 CALL csv 
		 e5 dd 6e 06 dd 66 07 e5 
		 fd e1 fd cb 06 4e 28 1e 
		 fd 7e 04 fd b6 05 28 16 
		 fd 5e 02 fd 56 03 21 00 02 b7 ed 52 
		 dd 75 fe dd 74 ff 7d b4 20 06 
		 21 00 00 JUMP cret
		 dd 6e fe dd 66 ff e5 
		 fd 6e 04 fd 66 05 e5 
		 fd 6e 07 26 00 e5 CALL write c1 c1 c1
		 dd 5e fe dd 56 ff b7 ed 52 28 04 
		 fd cb 06 ee 
		 fd 36 02 00 fd 36 03 02 
		 fd 6e 04 fd 66 05 
		 fd 75 00 fd 74 01 
		 fd cb 06 6e 28 b7 21 ff ff JUMP cret
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
	patch _fflush
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
		defb 0xcf, 4, 0, 0, 0, 0

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

;
; and this one, which knows about fcb's, etc.
; no source code
;
block _fflush
	match _fflush
		CALL csv
		e5 dd 6e 06 dd 66 07 
		e5 fd e1 11 __iob
		fd e5 e1 b7 ed 52
		11 0008 CALL adiv
		11 0029 CALL lmul
		11 __fcb 19
		dd 75 fe dd 74 ff
		fd cb 06 4e 20 06
		21 ff ff
		JUMP cret
		fd 7e 02 e6 7f
		6f af 67 7d b4 28 27
		06 04 
		dd 5e fe dd 56 ff
		21 24 00
		19 7e CALL brelop 30 15
		fd 6e 00 fd 66 01
		36 1a fd 6e 02 fd 66 03
		2b fd 75 02 fd 74 03
		fd 6e 04 fd 66 05 
		fd 75 00 fd 74 01
		7d b4 20 06
		21 00 00 JUMP cret
		fd 5e 02 fd 56 03
		21 00 02 b7 ed 52
		fd 75 02 fd 74 03
		dd 5e fe dd 56 ff
		21 24 00 19 7e fe 02 28 52
		fe 04 28 3a
		fd 36 02 00 fd 36 03 00
		fd 6e 04 fd 66 05
		fd 75 00 fd 74 01
		fd 36 02 00 fd 36 03 02 18 b7
		fd 6e 00 fd 66 01 
		7e 23 fd 75 00 fd 74 01 
		6f 17 9f 67 
		e5 21 02 00 e5 CALL bdosa c1 c1 
		fd 6e 02 fd 66 03 2b
		fd 75 02 fd 74 03 23 7d b4 20 d2 18 b2
		fd 5e 02 fd 56 03 
		21 7f 00 19 11 80 00 cd adiv 
		fd 75 02 fd 74 03 18 34 
		fd 6e 00 fd 66 01 
		e5 21 1a 00 e5 CALL bdosa c1
		dd 6e fe dd 66 ff e3 
		21 15 00 e5 CALL bdosa c1 c1 
		7d b7 c2 ANY ANY
		11 80 00 fd 6e 00 fd 66 01 19 
		fd 75 00 fd 74 01 fd 6e 02 fd 66 03 2b
		fd 75 02 fd 74 03 23 7d b4 20 ba 
		c3 ANY ANY
	end
;	patch _fflush
;		call csv
;		ld l,(ix+6)	; get FILE ptr
;		ld h,(ix+7)
;		push hl
;		pop iy
;		bit 1,(iy+6) ; if IOWRT
;		jr z,retffl
;	retffl:
;		ld hl,0
;		jp cret	
;	end
end

;
; close finds the fcb and calls cp/m close on it.
; close.obj
block _close
	match
		CALL csv 
		e5 06 08 dd 7e 06
		CALL brelop 38 06
		21 ff ff JUMP cret
		11 2a 00 dd 6e 06 26 00
		CALL amul 
		11 __fcb 19 e5 fd e1
		CALL _getuid 
		dd 75 ff 
		fd 6e 29 26 00 
		e5 CALL _setuid c1
		fd 7e 28 fe 02 28 1d 
		fe 03 28 19 
		21 0c 00 e5 CALL _bdoshl c1 
		af 6f 7c e6 05
		67 7d b4 28 12 
		fd 7e 28 
		fe 01 20 0b 
		fd e5 21 10 00 e5 CALL _bdos c1 c1
		fd 36 28 00 
		dd 6e ff 
		26 00 e5 CALL _setuid c1 
		21 00 00
		JUMP cret 
	end
	; we simply call the micronix close with the fd in hl
	patch _close 121
		call	csv
		ld		l,(IX+6)
		ld		h,0
		db		0xcf, 06
		ld		hl,0
		jp		cret
	end
end

;
; the hitech write system call has to do a lot of work with
; setting the dma buffer, getting the userid, sector copy,
; knowing about character devices, etc.
; write.obj
;
block _write
	match
		CALL ncsv 79 ff 
		06 08 dd 7e 06 CALL brelop 38 06 
		21 ff ff JUMP cret
		11 2a 00 dd 6e 06 26 00 CALL amul 
		11 __fcb 19 e5 fd e1
		dd 36 fe 02 
		dd 6e 0a dd 66 0b dd 75 f9 dd 74 fa fd 
		7e 28 fe 02 ca ANY ANY
		fe 03 ca ANY ANY 
		fe 04 28 71 
		fe 06 28 25 
		fe 07 28 3c 
		18 bd 
		CALL __sigchk 
		dd 6e 08 dd 66 09 7e 23 
		dd 75 08 dd 74 09 
		6f 17 9f 67 
		e5 21 04 00 e5 CALL _bdos c1 c1
		dd 6e 0a dd 66 0b 2b 
		dd 75 0a dd 74 0b 23 7d b4 20 cf
		dd 6e f9 dd 66 fa JUMP cret 
		dd 36 fe 05 18 27 CALL __sigchk 
		dd 6e 08 dd 66 09 7e 23 
		dd 75 08 dd 74 09 6f 17 9f 67 
		dd 75 fb dd 74 fc e5 
		dd 6e fe 26 00 e5 CALL _bdos c1 c1 
		dd 6e 0a dd 66 0b 2b
		dd 75 0a dd 74 0b 23 7d b4 
		20 c7 18 b6 CALL _getuid 
		5d dd 73 fd c3 ANY ANY CALL __sigchk 
		fd 6e 29 26 00 e5 CALL _setuid c1 
		fd 7e 24 e6 7f dd 77 fe
		5f 16 00 21 80 00 b7 ed 52 
		dd 75 ff 5d dd 6e 0a dd 66 0b 
		CALL wrelop 30 06
		dd 7e 0a dd 77 ff 
		11 80 00 21 00 00 e5 d5 
		fd 5e 24 fd 56 25 fd 6e 26 fd 66 27 
		CALL aldiv
		e5 d5 fd e5 d1 21 21 00 19 e5 CALL __putrno c1 c1 c1 
		dd 7e ff fe 80 20 12 
		dd 6e 08 dd 66 09 e5 
		21 1a 00 e5 CALL _bdos c1 c1 18 5e 
		dd e5 d1 21 79 ff 19 e5 21 1a 00 e5 CALL _bdos c1
		dd e5 d1 21 79 ff 19 36
		1a 21 7f 00 e3 dd e5 d1 
		21 7a ff 19 e5 dd e5 d1 
		21 79 ff 19 e5 CALL _bmove c1 c1 c1 
		fd e5 21 21 00 e5 CALL _bdos c1 
		dd 6e ff 26 00 e3 dd e5
		d1 dd 6e fe 26 00 19 11 79 
		ff 19 e5 dd 6e 08 dd 66 09 
		e5 CALL _bmove c1 c1 c1 
		fd e5 21 22 00 e5 CALL _bdos c1 c1 
		7d b7 20 49 
		dd 5e ff 16 00 
		dd 6e 08 dd 66 09 19
		dd 75 08 dd 74 09 7b 21 00 00 
		55 e5 d5 fd e5 d1 21 24 00 
		19 CALL asaladd
		dd 5e ff 16 00 dd 6e 0a dd 
		66 0b b7 ed 52 dd 75 0a dd 
		74 0b dd 6e fd 62 e5 CALL _setuid c1 
		dd 7e 0a dd b6 0b c2 ANY ANY 
		dd 6e fd 26 00 e5 CALL _setuid c1 
		dd 5e 0a dd 56 0b dd 6e f9 
		dd 66 fa b7 ed 52 JUMP cret
	end
	;
	; micronix is much simpler; we do have to maintain the file
	; offset, which we keep in the fcb.
	;
	patch _write
		call	csv
		ld		a,(ix+6)
		call	fcbcalc
		push	hl
		pop		iy

		ld		d,(ix+9)
		ld		e,(ix+8)
		ld		b,(ix+11)
		ld		c,(ix+10)
		ld		(m_write+2),de
		ld		(m_write+4),bc
		ld		l,(ix+6)
		ld		h,0

	m_write:			; write(fd, base, 0x200 - cnt)
		defb 0xcf, 4, 0, 0, 0, 0

		jmp		cret
	end
end

block _read
	match
		CALL ncsv 79 ff 
		dd 36 fb 00 dd 36 fc 00 
		06 08 dd 7e 06 CALL brelop 38 06
		21 ff ff JUMP cret
		11 2a 00 dd 6e 06 26 00 
		CALL amul 
		11 __fcb 
		19 e5 fd e1 fd 7e 28 
		fe 01 ca ANY ANY 
		fe 03 ca ANY ANY fe 
		04 28 57 
		fe 05 20 d3 
		dd 6e 0a dd 66 0b 
		dd 75 fb dd 74 fc 
		dd 7e 0a dd b6 0b 
		20 12 
		dd 5e 0a dd 56 0b 
		dd 6e fb dd 66 fc b7 ed 52 
		JUMP cret 
		dd 6e 0a dd 66 0b 2b 
		dd 75 0a dd 74 0b 
		21 03 00 e5 CALL _bdos c1 
		7d e6 7f
		dd 6e 08 dd 66 09 23 
		dd 75 08 dd 74 09 2b 
		77 fe 0a 20 bb 18 c1 
		dd 5e 0a dd 56 0b 21 80 00 
		CALL wrelop 30 08 
		dd 36 0a 80 dd 36 0b 00 
		dd 7e 0a dd e5 d1 21 79 ff 
		19 77 dd e5 d1 21 79 ff
		19 e5 21 0a 00 e5 CALL _bdos c1 c1 
		dd e5 d1 21 7a ff 19
		6e 26 00 dd 75 fb dd 74 fc 
		dd 5e 0a dd 56 0b dd 66 fc 
		CALL wrelop 30 2c 
		21 0a 00 e5 
		21 02 00 e5 CALL _bdos c1 c1
		dd e5 d1 dd 6e fb dd
		66 fc 23 23 19 11 79 ff 19 
		36 0a dd 6e fb dd 66 fc 23 
		dd 75 fb dd 74 fc dd 6e fb 
		dd 66 fc e5 dd 6e 08 dd 66 
		09 e5 dd e5 d1 21 7b ff 19 
		e5 CALL _bmove c1 c1 c1 
		dd 6e fb dd 66 fc JUMP cret
		CALL _getuid 
		5d dd 73 fd dd 6e 0a dd 66 
		0b dd 75 fb dd 74 fc 
		c3 ANY ANY 
		CALL __sigchk 
		fd 6e 29 26 00 e5 CALL _setuid c1 
		fd 7e 24 e6 7f dd 77 fe
		5f 16 00 21 80 00 b7 ed 52 
		dd 75 ff 5d dd 6e 0a dd 66 0b
		CALL wrelop 30 06
		dd 7e 0a dd 77 ff 
		11 80 00 21 00 00 e5 d5 
		fd 5e 24 fd 56 25 
		fd 6e 26 fd 66 27 
		CALL aldiv 
		e5 d5 fd e5 d1 21 21 00 19 
		e5 CALL __putrno c1 c1 c1 
		dd 7e ff fe 80 20
		22 dd 6e 08 dd 66 09 e5 
		21 1a 00 e5 CALL _bdos c1 c1
		fd e5 21 21 00 e5 CALL _bdos c1 c1 
		7d b7 28 44
		c3 ANY ANY 
		dd e5 d1 21 79 ff 19 e5 21 
		1a 00 e5 CALL _bdos c1 c1 
		fd e5 21 21 00 e5 CALL _bdos c1 c1 
		7d b7 20 6a dd 6e ff
		26 00 e5 dd 6e 08 dd 66 09 
		e5 dd e5 d1 dd 6e fe 26 00 
		19 11 79 ff 19 e5 CALL _bmove c1 c1 c1 
		dd 5e ff 16 00 dd
		6e 08 dd 66 09 19 dd 75 08 
		dd 74 09 7b 21 00 00 55 e5 
		d5 fd e5 d1 21 24 00 19 
		CALL asaladd 
		dd 5e ff 16 00 dd 6e 0a dd 
		66 0b b7 ed 52 dd 75 0a dd 
		74 0b dd 6e fd 62 e5 
		CALL _setuid 
		c1 dd 7e 0a dd b6 0b c2 ANY 
		ANY dd 6e fd 26 00 e5 CALL _setuid c1
		c3 ANY ANY 
	end

	patch _read
		call	csv
		ld		a,(ix+6)
		call	fcbcalc
		push	hl
		pop		iy

		ld		d,(ix+9)
		ld		e,(ix+8)
		ld		b,(ix+11)
		ld		c,(ix+10)
		ld		(m_read+2),de
		ld		(m_read+4),bc
		ld		l,(ix+6)
		ld		h,0

	m_read:			; read(fd, base, cnt)
		defb 0xcf, 3, 0, 0, 0, 0

		jmp		cret
	end
end

;
; remove a file
; unlink.obj
;
block _unlink
	match
		CALL ncsv d3 ff 
		dd 6e 06 dd 66 07 e5
		dd e5 d1 21 d6 ff 19 e5 CALL _setfcb c1 c1 
		7d b7 28 06 21 00 00 JUMP cret
		CALL _getuid 
		dd 75 d5 dd 6e ff 26 00 
		e5 CALL _setuid dd e5 d1 21 d6 ff 19 e3 
		21 13 00 e5 CALL _bdos c1 
		7d 17 9f 67 
		dd 75 d3 dd 74 d4 dd 6e d5 26 00 
		e3 CALL _setuid c1 
		dd 6e d3 dd 66 d4
		JUMP cret 
	end
end

;
; make a new file
; creat.obj
;
block _creat
	match
		CALL csv 
		e5 CALL _getfcb 
		e5 fd e1 7d b4 20 06 
		21 ff ff JUMP cret
		CALL _getuid 
		dd 75 ff dd 6e 06 dd 66 07 
		e5 fd e5 CALL _setfcb c1 c1 
		7d b7 20 3c dd 6e 06
		dd 66 07 e5 CALL _unlink
		fd 6e 29 26 00 e3 
		CALL _setuid c1 
		fd e5 21 16 00 e5 CALL _bdos c1 c1 
		7d fe ff dd 6e ff 26
		00 e5 20 0a CALL _setuid c1 
		fd 36 28 00 18 ae
		CALL _setuid c1 
		fd 36 28 02 11 __fcb
		fd e5 e1 b7 ed 52 11 2a 00 
		CALL adiv 
		JUMP cret 

	end
	patch _creat
		ld hl,0x5		; point at mode
		add hl,sp
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
	crsys: defb 0xcf, 08, 00, 00, 00, 00
		ret nc
		ld hl, 0xffff
		ret
		ds 91,0
	end
end

;
; used by the compiler passes, it does all the work for fopen
; the fopen itself does not need any patching
; FILE *reopen(char *name, char *mode, FILE *
block _freopen
	match
	CALL csv 
	e5 e5 dd 6e 0a dd 66 0b e5 
	fd e1 e5 CALL _fclose c1 
	11 __iob fd e5 e1 b7 ed 52
	11 08 00 CALL adiv 
	7d fd 77 07 6f 17 9f 67 
	11 29 00 CALL lmul
	11 __fcb 19 
	dd 75 fc dd 74 fd 
	dd 36 ff 00 dd 36 fe 0f 
	fd 7e 06 e6 4f fd 77 06 
	dd 6e 08 dd 66 09 7e fe 72 28 0b 
	fe 77 20 17 
	dd 34 ff dd 36 fe 16 dd 6e 08 dd 66 09 
	23 7e fe 62 20 04 
	fd 36 06 80 
	dd 6e 06 dd 66 07 e5 
	dd 6e fc dd 66 fd e5 CALL _setfcb c1 c1
	dd 7e fe fe 16 dd 6e fc dd 66 fd e5 20 0f 
	21 13 00 e5 CALL bdosa c1 
	dd 6e fc dd 66 fd e3 dd 6e fe 26 00 
	e5 CALL bdosa c1 c1 
	7d fe ff 20 06 
	21 00 00 JUMP cret 
	dd 7e ff b7 20 05 
	21 01 00 18 03 
	21 02 00 7d dd 5e fc dd 56 fd 
	21 24 00 19 77 
	dd 7e ff 3c 5f fd 7e 06 b3 fd 77 06 e6 0c b7 20 16 
	fd 7e 04 fd b6 05 20 0e 
	21 00 02 e5 CALL _sbrk c1 
	fd 75 04 fd 74 05 
	fd 6e 04 fd 66 05 fd 75 00 fd 74 01 
	7d fd b6 05 28 10 
	dd 7e ff b7 fd 36 02 00 28 0a 
	fd 36 03 02 18 08 
	fd 36 02 00 fd 36 03 00 fd e5 e1 JUMP cret
	end

	;
	; we're not going to try for full library compatibility
	; just enough to make the compiler work. 
	; the specific hair that we are not going to split is a
	; stream open for both read and write.  what's the file
	; pointer? how does this interact with flushing?
	; screw that.
	;
	patch _freopen 295
		call	csv

		ld		h,(ix+11)	; close the file
		ld		l,(ix+10)
		call	_fclose

		ld		h,(ix+9)	; parse mode: rwa = 012
		ld		l,(ix+8)
		ld		b,0
		ld		a,(hl)
		cp		'r'
		jr		z,modeset
		inc		b
		cp		'w'
		jr		z,modeset
		set		1,b
	modeset:
		ld		a,5			; do an open
		ld		(sys+1),a
		ld		a,b			; mode r,w = 0,1
		and		1
		ld		(sys+4),a
		push	hl
		ld		h,(ix+7)	; get the name
		ld		l,(ix+6)
		ld		(sys+2),hl

	sys:
		db		0xcf, 0x5, 0x0, 0x0, 0x0, 0x0
		jr		nc, opened

		dec		b			; mode 0 must succeed
		jr		c, openfail
		ld		a,8			; change it to a creat	
		ld		(sys+1),a
		ld		hl,666O		; permissive mode
		ld		(sys+4),hl
		db		0xcf, 0x0, sys
		jr		c,openfail

	opened:
		bit		1,b
		jr		z,noseek
		push	hl
		db		0xcf, 19, 0, 0, 2, 0
		pop		hl
	noseek:

	openfail:
		ld		hl,0
		jp		cret
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
		01 02 03 04 05 06 07 99 99 99
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

;
; the ugly work in the lseek logic happens here,
; where the cp/m implementation does a 'get file size'
; seek.obj
;
block _fsize
	match
		CALL ncsv fb ff 
		06 08 dd 7e 06 CALL brelop 38 08
		11 ff ff 6b 62 JUMP cret
		11 2a 00 dd 6e 06 26 00 CALL amul 
		11 __fcb 
		19 e5 fd e1 CALL _getuid 
		dd 75 fb fd 6e 29 26 00 e5 CALL _setuid c1 
		fd e5 21 23 00 e5 CALL _bdos c1
		dd 6e fb 26 00 e3 CALL _setuid c1 
		06 10 fd 7e 23 21 00 00 55 5f CALL allsh
		e5 d5 06 08 fd 7e 22 21 00 00 55 5f CALL allsh 
		e5 d5 fd 7e 21 21 00 00 55 5f CALL aladd 
		CALL aladd 
		dd 73 fc dd 72 fd dd 75 fe 
		dd 74 ff 06 07 dd e5 e1 2b 
		2b 2b 2b CALL asallsh 
		dd 5e fc dd 56 fd dd 6e fe 
		dd 66 ff e5 d5 fd 5e 24 fd 
		56 25 fd 6e 26 fd 66 27 CALL arelop f2 ANY ANY 
		dd 5e fc dd 56 fd dd 6e fe dd 66 ff 
		JUMP cret
		fd 5e 24 fd 56 25 fd 6e 26 fd 66 27 
		JUMP cret
	end	
	;
	; fstat the file the file and return the file size as a long
	; in hlde
	;
	patch _fsize
		pop		de
		pop		hl
		push	hl
		push	de
		ld		a,l
		call	fcbcalc
		push	hl
		ld		(fs_call+2),hl
		ld		h,0
		ld		l,a
	fs_call:
		db		0xcf, 28, 0x00, 0x00
		ld		de,9
		pop		hl
		add		hl,de				; stat size
		ld		c,(hl)				; high byte
		inc		hl
		ld		e,(hl)				; low
		inc		hl
		ld		d,(hl)				; middle
		ld		h,0
		ld		l,c
		ret
	end
end

;
; micronix has no lseek, being v6.  however, we do
; have seek, which is a little more work
; seek.obj
;
block _lseek
	;
	; lseek(fd, offset, whence)
	;	    +2,     +4,     +8  on stack
	; offset is a long, and whence = (0 = abs,1 = rel,2 = end)
	;
	match
		CALL csv 
		e5 e5 06 08 dd 7e 06 CALL brelop 38 08 
		11 ff ff 6b 62 JUMP cret
		11 2a 00 dd 6e 06 26 00 CALL amul 
		11 __fcb 
		19 e5 fd e1 dd 7e 0c fe 01 
		28 49 fe 02 
		dd 5e 08 dd 56 09 
		dd 6e 0a dd 66 0b 28 64 
		dd 73 fc dd 72 fd 
		dd 75 fe dd 74 ff 
		dd cb ff 7e 20 bd 
		dd 5e fc dd 56 fd 
		dd 6e fe dd 66 ff 
		fd 73 24 fd 72 25 
		fd 75 26 fd 74 27 
		fd 5e 24 fd 56 25 
		fd 6e 26 fd 66 27 
		JUMP cret 
		dd 5e 08 dd 56 09 
		dd 6e 0a dd 66 0b e5 d5 
		fd 5e 24 fd 56 25 
		fd 6e 26 fd 66 27 CALL aladd 
		dd 73 fc dd 72 fd 
		dd 75 fe dd 74 ff 18 a8 
		e5 d5 dd 6e 06 26 00 e5 
		CALL _fsize c1 
		18 e1 
	end
	;
	; we need to do 2 seek calls, with the high byte multiplied by 2.
	; the gnarly part is that we then need to return the seek position
	; which isn't returned.
	patch _lseek 178
		ld		hl,8
		add		hl,sp
		ld		a,(hl)
		ld		(seek_lo+4),a
		add		3
		ld		(seek_hi+4),a
		dec		hl
		dec		hl
		ld		a,(hl)
		add		a
		ld		(seek_hi+2),a
		xor		a
		ld		(seek_hi+3),a
		dec		hl
		ld		a,(hl)
		ld		(seek_lo+3),a
		dec		hl
		ld		a,(hl)
		ld		(seek_lo+2),a
		dec		hl
		ld		d,(hl)
		dec		hl
		ld		l,(hl)
		ld		h,d
		push	hl
		push	hl
	seek_lo:
		db		0xcf, 19, 0x00, 0x00, 0x00
		pop		hl
	seek_hi:
		db		0xcf, 19, 0x00, 0x00, 0x00
		pop		hl

		ld		a,l
		call	fcbcalc				; file descriptor in a preserved
		push	hl					; save fcb
		ld		de,36				; rwp offset
		add		hl,de
		push	hl					; save fcb+rwp on stack
		push	af

		ld		a,(seek_lo+4)		; get 'whence'
		ld		de,seek_zero		; assume absolute
		dec		a
		jr		c,addit				; if it was zero, go with absolute
		ex		de,hl
		jr		z,addit				; relative
		pop		af
			
	seek_zero:
		db		0, 0, 0, 0
	; 
	; add the long pointed at by de to the input offset and
	; put the result in the rwp, which is pointed at by bc.
	; this will either be zero, the fcb->rwp, or the file size
	; this is a signed add, so eof-5 will do the right thing
	;
	addit:
		pop		bc
		ld		hl,4				; point at file offset long low
		add		hl,sp

		ld		a,(de)				; bits 0-7
		add		(hl)
		ld		(bc),a
		inc		hl
		inc		de
		inc		bc

		ld		a,(de)				; bits 8-15
		add		(hl)
		ld		(bc),a
		inc		hl
		inc		de
		inc		bc
			
		ld		a,(de)				; bits 16 - 23
		add		(hl)
		ld		(bc),a
		inc		hl
		inc		de
		inc		bc

		ld		a,(de)				; bits 24 - 31
		add		(hl)
		ld		(bc),a

	;
	; we're done now, and need to get the result into hlde
	; luckily, we're pointing at it with bc
	;	
		ld		h,b
		ld		l,c
		ld		b,(hl)
		dec		hl
		ld		c,(hl)
		dec		hl
		ld		d,(hl)
		dec		hl
		ld		e,(hl)
		ld		h,b
		ld		l,c
	end
end

