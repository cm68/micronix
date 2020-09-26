*****************************************************************
*								*
* Disk format program for Disk Jockey 2D controller.		*
* 11/16/79							*
*								*
*****************************************************************

	title	'*** Format Program for CP/M Ver. 2.0 ***'

	org	100h

revnum	equ	52		;Revision # times 10
bdos	equ	5		;CP/M entry point
wboot	equ	0

origin	equ	0F800H
DISKIO	EQU	ORIGIN+3f8h
DATREG	EQU	DISKIO+7
DRVSEL	EQU	DISKIO+1
CSTALL	EQU	DISKIO+3
CMDREG	EQU	DISKIO+4
DSIDE	EQU	10Q
UNLOADB	EQU	17Q
WTCMD	EQU	364Q
SICMD	EQU	131Q
IMMIRQ	EQU	320Q
UNLOADA	EQU	30Q
RESTOR	EQU	11Q
INDEX	EQU	20Q
TRKZRO	EQU	4
intrqa	equ	1
intrqb	equ	4
lhsdenb	equ	90dh
lhddenb	equ	80ch
lhsdena	equ	111h
lhddena	equ	10h
acr	equ	0dh
alf	equ	0ah

START	lxi	sp,stack
	LHLD	ORIGIN+7	;adjust
	INX	H		;   the
	MOV	A,M		;   calling
	LXI	H,STDVSL+1	;   routines
	LXI	D,4		;   for
	MOV	M,A		;   different
	XRI	3		;   versions
	DAD	D		;   of the
	MOV	M,A		;   disk
	DAD	D		;   jockey
	MOV	M,A		;   controller
	LXI	d,SMESSG	;print the
	CALL	pbuff		;   sign on message
CROK	LXI	d,DMESSG	;echo the CR and
	CALL	pbuff		;   print drive select
	CALL	rbuff		;wait for response
	CPI	'N'		;   new parameter
	JZ	CROK		;   request
	STA	LETTER		;save for exit
	SUI	'A'		;test for good
	JP	NOTLOW		;   drive select
INPUTB	LXI	d,BMESSG	;print the bad
	CALL	pbuff		;   input message
	JMP	CROK		;   and wait for input

NOTLOW	CPI	4		;test for drive
	JP	INPUTB		;   select too large
	STA	DRVNO		;save drive no.

fmtsiz	lxi	d,denmsg	;Select the sector size
	call	pbuff
	call	rbuff
	cpi	'N'
	jz	CROK
	sui	'1'		;Strip off ASCII bias
	jp	sizchk
sizerr	lxi	d,bmessg
	call	pbuff
	jmp	fmtsiz

sizchk	cpi	4
	jnc	sizerr
	sta	newsize

SENDI	LXI	d,IMESSG	;send out the
	LXI	h,JMESSG	;   diskette insert
	CALL	SENDMP		;   message

GETIN2	CALL	rbuff		;wait for response
	CPI	'N'		;test for new
	JZ	CROK		;   parameter request

PROCED	lxi	d,acralf
	call	pbuff

	LDA	DRVNO		;get the drive no
	MOV	C,A
	MVI	A,177Q		;drive select bits

QLOOP	RLC			;rotate select bits
	DCR	C		;   to proper drive
	JP	QLOOP		;   position
	ani	3fh
	STA	SELECT		;save for TRACK routine
	call	stdvsl
	call	model
	lxi	b,lhsdena
	jz	prep
	lxi	b,lhsdenb
prep	mov	a,b
	call	stbits
	mov	a,c
	call	stbits
	lxi	h,cmdreg
	mvi	m,immirq
	mvi	a,40h
wirqd	dcr	a
	jnz	wirqd
	mov	a,m
	rar
	jc	notrdy
	ral
	ral
	jc	notrdy
	lxi	d,wmessg
	ral
	jc	notrdyx
	LXI	D,0
	call	gtindx
IXLOOP	call	gtstat
	ani	index
	xra	b
	JNZ	dorest
	DCX	D
	MOV	A,D
	ORA	E
	JNZ	IXLOOP
NOTRDY	LXI	d,RMESSG	;not ready message
notrdyx	xchg
	LXI	d,AMESSG	;drive message
	CALL	SENDMP		;send error message
	call	unload
	JMP	SENDI		;back for more input

dorest	MVI	M,RESTOR
wrstrs	mov	a,m
	rar
	jnc	wrstrs
wrstrd	MOV	A,M
	rar
	jc	wrstrd
	MOV	A,M
	ANI	TRKZRO
	JZ	NOTRDY

	LXI	B,SDLIST
	LXI	H,DLIST
	CALL	OVLAY
	LXI	B,L128
	LXI	H,SLIST
	CALL	OVLAY
	mvi	a,'*'
	call	pchar
	CALL	INDEXP
	MVI	H,0
	CALL	TRACK
	LXI	D,TABLE
	lda	newsize
	add	a
	jz	findix
	mov	l,a
	mvi	h,0
	DAD	D
	MOV	C,M
	INX	H
	MOV	B,M
	LXI	H,SLIST
	CALL	OVLAY
	LXI	B,DDLIST
	LXI	H,DLIST
	CALL	OVLAY
	call	model
	lxi	b,lhddena
	jz	loadd
	lxi	b,lhddenb
loadd	mov	a,b
	call	stbits
	mov	a,c
	call	stbits
FINDIX	CALL	INDEXP
	MVI	H,1
TKSTEP	mvi	a,'*'
	call	pchar
	LXI	D,CMDREG
	MVI	A,SICMD
	STAX	D
WSICMS	LDAX	D
	RAR
	JNC	WSICMS
WSICMD	LDAX	D
	RAR
	JC	WSICMD
	CALL	TRACK
	MOV	H,D
	INR	H
	MVI	A,77
	CMP	H
	JNZ	TKSTEP
	call	unload
	lxi	d,fmessg
	call	pbuff
	call	rbuff
	cpi	'F'
	jz	crok
	jmp	wboot

SENDMP	PUSH	h		;save second half
	CALL	pbuff		;send first half
	LDA	LETTER		;print the
	CALL	pchar		;   letter
	POP	d		;send the second

pbuff	mvi	c,9
	jmp	bdos

rbuff	lxi	d,inbufx
	mvi	c,10
	call	bdos
	lda	inbufx+1
	ana	a
	mvi	a,acr
	rz
	lda	inbuf
	cpi	'a'
	rc
	cpi	'z'+1
	rnc
	sui	40q
	ret

pchar	push	h
	push	b
	push	d
	push	psw
	mov	e,a
	mvi	c,2
	call	bdos
	pop	psw
	pop	d
	pop	b
	pop	h
	ret

inbufx	db	10,0
inbuf	db	0,0,0,0,0,0,0,0,0,0

STDVSL	STA	DISKIO+1
	RET

STBITS	STA	DISKIO+2
	RET

GTSTAT	LDA	DISKIO+2
	RET

model	lda	diskio-4
	cpi	(ret)
	ret

unload	call	model
	mvi	a,unloada
	jz	stbits
	mvi	a,unloadb
	jmp	stbits

gtindx	call	model
	mvi	b,0
	rz
	mvi	b,index
	ret

INDEXP	call	gtindx
windxh	call	gtstat
	ANI	INDEX
	xra	b
	JZ	windxh
windxl	call	gtstat
	ani	index
	xra	b
	jnz	windxl
	ret
indxw	call	gtindx
	jmp	windxl

TABLE	DW	L128
	DW	L256
	DW	L512
	DW	L1024

TRACK	lda	select
	call	stdvsl
	XRA	A
	STA	SIDENO
	CALL	WTRACK
	call	gtstat
	ANI	DSIDE
	RNZ
	INR	A
	STA	SIDENO
	LDA	SELECT
	ANI	357Q
	call	stdvsl
WIL
	call	indxw
	XCHG

WTRACK	LXI	D,DATREG	;1791 data reg
	MVI	L,1	;initialize sector reg
	LXI	B,4e50H	;data & count
OVLD1	EQU	$-2
	MVI	A,WTCMD	;issue a write
	STA	CMDREG	;   track command
	XCHG		;adjust the registers
WTL1	MOV	M,B	;index mark
	DCR	C	;   preamble
	JNZ	WTL1	;   field
	LXI	B,0CH	;data & count
OVLD2	EQU	$-2
WTL2	MOV	M,B	;index mark
	DCR	C	;   zero leader
	JNZ	WTL2	;   field
OVLD3	MVI	M,0F6H	;special double
	MVI	M,0F6H	;   density clock
	MVI	M,0F6H	;   and data mark
WTL3	MVI	M,0FCH	;index mark
	LXI	B,4E32H	;data & count
OVLD4	EQU	$-2
WTL4	MOV	M,B	;sector mark
	DCR	C	;   preamble
	JNZ	WTL4	;   field
SLOOP	LXI	B,0CH	;data & count
OVLD5	EQU	$-2
WTL5	MOV	M,B	;sector mark
	DCR	C	;   zero leader
	JNZ	WTL5	;   field
OVLD6	MVI	M,0F5H	;special double
	MVI	M,0F5H	;   density clock
	MVI	M,0F5H	;   and data mark
WTL6	MVI	M,0FEH	;sector mark
	MOV	M,D	;track number
	MVI	M,0	;side number
SIDENO	EQU	$-1
	MOV	M,E	;sector number
	MVI	M,1	;sector length
OVLS1	EQU	$-1
	MVI	M,0F7H	;crc data bytes
	INR	E	;increment sector
	LXI	B,4E16H	;data & count
OVLD7	EQU	$-2
WTL7	MOV	M,B	;sector mark
	DCR	C	;   postamble
	JNZ	WTL7	;   field
	LXI	B,0CH	;data & count
OVLD8	EQU	$-2
WTL8	MOV	M,B	;data mark
	DCR	C	;   zero leader
	JNZ	WTL8	;   field
OVLD9	MVI	M,0F5H	;special double
	MVI	M,0F5H	;   density clock
	MVI	M,0F5H	;   and data mark
WTL9	MVI	M,0FBH	;data mark
	LXI	B,0e540H	;data & count
OVLS2	EQU	$-2
WTL10	MOV	M,B	;write first
	DCR	C	;   quarter of
	JNZ	WTL10	;   sector data
	MVI	C,40H	;count
OVLS3	EQU	$-1
WTL11	MOV	M,B	;write second
	DCR	C	;   quarter of
	JNZ	WTL11	;   sector data
	MVI	C,40H	;count
OVLS4	EQU	$-1
WTL12	MOV	M,B	;write third
	DCR	C	;   quarter of
	JNZ	WTL12	;   sector data
	MVI	C,40H	;count
OVLS5	EQU	$-1
WTL13	MOV	M,B	;write fourth
	DCR	C	;   quarter of
	JNZ	WTL13	;   sector data
	MVI	M,0F7H	;crc data bytes
	MVI	A,27	;last sector + 1
OVLS6	EQU	$-1
	LXI	B,4E36H	;count & data
OVLS7	EQU	$-2
WTL14	MOV	M,B	;data
	DCR	C	;   postamble
	JNZ	WTL14	;   field
	CMP	E	;last sector test
	JNZ	SLOOP
WTL15	MOV	M,B	;fill data
	DCR	C	;   to index hole
	JNZ	WTL15
WTL16	MOV	M,B	;fill data
	DCR	C	;   to index hole
	JNZ	WTL16
WTL17	MOV	M,B	;fill data
	DCR	C	;   to index hole
	JNZ	WTL17
	RET

OVLAY	MOV	A,M	;length of list
	PUSH	H	;save list ptr
	LXI	H,WTRACK	;overlay area
	XTHL	;	recover	pointer
	MVI	D,0
OVLAY1	INX	H	;increment pointer
	MOV	E,M	;get offset
	XTHL		;exchange pointers
	DAD	D	;add the offset
	PUSH	PSW	;save length count
	LDAX	B	;get replacement data
	MOV	M,A	;do the replacement
	INX	B	;next replacement data
	POP	PSW	;recover length count
	DCR	A	;decrement
	XTHL		;exchange pointers
	JNZ	OVLAY1
	POP	H	;adjust stack
	RET

* The lists below represents the distances
* between successive locations of the WTRACK
* routine that are to be overlaid in order
* that the same routine will serve to format
* different sized and density diskette
* sector formats

DLIST	DB	18	;length of list
	DB	OVLD1-WTRACK
	DB	1
	DB	OVLD2-OVLD1-1
	DB	OVLD3-OVLD2
	DB	1
	DB	1
	DB	OVLD4-OVLD3-2
	DB	1
	DB	OVLD5-OVLD4-1
	DB	OVLD6-OVLD5
	DB	1
	DB	1
	DB	OVLD7-OVLD6-2
	DB	1
	DB	OVLD8-OVLD7-1
	DB	OVLD9-OVLD8
	DB	1
	DB	1

SLIST	DB	9	;list length
	DB	OVLS1-WTRACK
	DB	OVLS2-OVLS1
	DB	1
	DB	OVLS3-OVLS2-1
	DB	OVLS4-OVLS3
	DB	OVLS5-OVLS4
	DB	OVLS6-OVLS5
	DB	OVLS7-OVLS6
	DB	1

* Overlay data for single density
* formatted diskettes

SDLIST	DW	0FF28H
	DB	6
	JMP	WTL3
	DW	0FF1AH
	DB	6
	JMP	WTL6
	DW	0FF0BH
	DB	6
	JMP	WTL9

* Overlay data for double density
* formatted diskettes

DDLIST	DW	4E50H
	DB	0CH
	MVI	M,0F6H
	DB	36H
	DW	4E32H
	DB	0CH
	MVI	M,0F5H
	DB	36H
	DW	4E16H
	DB	0CH
	MVI	M,0F5H
	DB	36H

* Overlay data for single density
* 26 sectors/track 128 byte sectors

L128	DB	0
	DW	0e520H
	DB	20H
	DB	20H
	DB	20H
	DB	27
	DW	0FF1BH

* Overlay data for double density
* 26 sectors/track 256 byte sectors

L256	DB	1
	DW	0e540H
	DB	40H
	DB	40H
	DB	40H
	DB	27
	DW	4E36H

* Overlay data for double density
* 15 sectors/track 512 byte sectors

L512	DB	2
	DW	0e580H
	DB	80H
	DB	80H
	DB	80H
	DB	16
	DW	4E6AH

* Overlay data for double density
* 8 sectors/track 1024 byte sectors

L1024	DB	3
	DW	0e500H
	DB	0
	DB	0
	DB	0
	DB	9
	DW	4E74H

AMESSG	db	acr,alf
	db	'Drive $'
RMESSG	db	' Is Not Ready.$'

WMESSG	db	' Is Write Protected.$'

BMESSG	db	acr,alf
	db	'Improper Response.$'

DMESSG	db	acr,alf
	db	'Select Drive (A,B,C,or D): $'

SMESSG	db	'Disk Jockey 2D Disk Format program'
	db	' Revision '
	db	'0'+revnum/10,'.','0'+(revnum mod 10)
	db	acr,alf
	db	'Type "^C" to return to CP/M, "N" to restart.$'

IMESSG	db	acr,alf
	db	'Insert a Write Enabled Diskette in Drive $'

JMESSG	db	acr,alf
	db	'Close the Drive Door and then Press RETURN: $'

FMESSG	db 	acr,alf
	db	'Function Complete.'
	db	acr,alf
	db	'Type RETURN to return to CP/M, or F to Format another: $'

denmsg	db	acr,alf
	db	'Select a sector size:'
	db	acr,alf
	db	'     1) 128 Byte Single density.'
	db	acr,alf
	db	'     2) 256 Byte Double density.'
	db	acr,alf
	db	'     3) 512 Byte Double Density.'
	db	acr,alf
	db	'     4) 1024 Byte Double Density.'
	db	acr,alf
	db	'Sector Size: $'

acralf	db	acr,alf,'$'

letter	db	0
drvno	db	0
select	db	0
newsize	db	0

	ds	50
stack	equ	$
	end
