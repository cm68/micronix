;
; cp/m startup code
;
	.extern _exit
	.extern _main

	.text
start:
	ld		hl,(0006h)		; get the bdos address
	ld		sp,hl			; and put the stack right below it

;
; Startup is version-agnostic.  The one CP/M 3 feature it wants is the
; error mode, and that is skipped on 2.2, which has no such call.  A
; .com must not refuse to load for want of CP/M 3: the calls in libcpm
; fail soft where 2.2 has no equivalent, and the passes that genuinely
; need the TPA of a banked CP/M 3 are the exception, not the rule.
;
; Neither of the calls below touches the DMA buffer at 0080h, so the
; command tail is still there for the argv code further down.  A disk
; call here would have eaten it.
;
	ld		c,12			; get version
	call	0005h
	ld		a,l
	cp		030h
	jr		c,noerrmode		; 2.2 has no error mode to set
;
; Hand disk errors back to the program.  The default mode prints the
; BDOS's own message over whatever we were saying and terminates us
; where we stand - no diagnostic of ours, no cleanup, no status.  A
; compiler that cannot open its input should be able to name the file.
;
	ld		c,45			; set error mode
	ld		e,0ffh			; return, do not display or terminate
	call	0005h

noerrmode:

	ld		de,__Lbss		; clear bss
	or		a
	ld		hl,__Hbss
	sbc		hl,de
	jr		z,nobss
	ld		c,l
	ld		b,h
	dec		bc
	ld		l,e
	ld		h,d
	inc		de
	ld		(hl),00h
	ldir

nobss:
;
;	The command tail length, at 0080h.  This read (80), which this
;	assembler takes for decimal - address 0050h, in the middle of
;	page zero, which is zero.  So the length came back zero, no
;	room was made for the strings and the loop below copied
;	nothing: every program under CP/M saw argc 1 and an empty
;	argv[0].  The very next instruction to name the same buffer
;	spells it 80h.
;
	ld		a,(80h)			; command tail length
	inc		a
	neg
	ld		l,a
	ld		h,-1
	add		hl,sp			; sp -= (cmdlen + 1)
	ld		sp,hl

	ld		bc,0			; flag end of args
	push	bc

	ld		hl,80h			; address of argument buffer
	ld		c,(hl)			; c = cmd len
	ld		b,0
	add		hl,bc			; point at last char in cmd
	ld		b,c				; b = cmd len
	ex		de,hl			; save end in de

	ld		hl,(0006)		; get bdos address
	ld		c,1				; argc
	dec		hl				; hl is argv string pointer
	ld		(hl),0			; string terminator
	inc		b
	jr		3f				; enter loop

2:	ld		a,(de)			; get end character
	cp		' '			; is it space?
	dec		de				; bump source
	jr		nz,1f			; not space

	push	hl				; is space, next arg
	inc		c				; argc++

4:	ld		a,(de)			; remove extra spaces
	cp		' '
	jr		nz,5f
	dec		de
	djnz	4b
	jr		6f

5:
	xor		a				; change space to null

1:	dec		hl				; bump
	ld		(hl),a			; store argv char
3:
	djnz	2b				; loop for cmd bytes

6:
	push	hl

	ld		hl,0
	add		hl,sp
	push	hl
	push	bc

	call	_main			; exit(main(argc, argv))
	push	hl
	call	_exit
	jp		0000h

	.data

;
; these symbols are filled out by the linker
;
__Htext::	.dw	0
__Ltext::	.dw	0
__Hdata::	.dw	0
__Ldata::	.dw	0
__Hbss::	.dw	0
__Lbss::	.dw	0

; vim: tabstop=4 shiftwidth=4 noexpandtab:
