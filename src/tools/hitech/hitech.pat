;
; grammar file for the hitech c library replacement
;
;
; crt0
;
block crt
	match 0100
		2a 06 00
		f9
		11 ANY ANY			; 0x104 bss start
		b7
		21 ANY ANY 			; 0x108 bss end
		ed 52
		4d 
		44
		0b
		6b
		62
		13
		36 00
		ed b0
		21 ANY ANY 			; 0x117 nularg
		e5
		21 80 00
		4e
		23
		06 00
		09
		36 00
		21 81 00
		e5
		CALL ANY ANY		; 0x129 startup
		c1
		c1
		e5
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
	replace crt+0
		00 00 00 00
	end
	replace crt+0x17
		c1 						; pop bc
		60 69					; ld hl,bc
		22 _argc				; ld (argc), hl
		eb						; ex de,hl
		21 00 00				; ld hl,0
		39						; add hl,sp
		e5						; push hl
		d5						; push de
		CALL _main				; call main	
		c1						; pop bc
		c1						; pop bc
		e5						; push hl
		JUMP _exit				; call exit
		2a _main+5
		00 00 00 00 
		00 00 00 00
		00 00 00 00
	end
end

block getuser
	match
		CALL ANY ANY
		0e 20
		1e ff
		dd e5
		CALL 0005
		dd e1
		6f
		26 00
		JUMP ANY ANY
	end
	extract getuser+0x1 csv
	extract getuser+0x12 cret
end

block setuser
	match
		CALL ANY ANY
		dd 5e 06
		0e 20
		dd e5
		CALL 0005
		dd e1
		JUMP ANY ANY
	end
end

block bdosa
	match
		CALL ANY ANY
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
		JUMP ANY ANY
	end
end

block bdoshl
	match
		CALL ANY ANY
		dd 5e 08
		dd 56 09
		dd 4e 06
		dd e5
		CALL 0005
		dd e1
		JUMP ANY ANY
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
		CALL getuser
	end
end

block write
	match
	end
end


