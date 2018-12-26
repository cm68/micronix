*****************************************************************
*								*
* Low Level Hard Disk Drivers. The following routines are the	*
* lowest level drivers for the hard disk.			*
*								*
* Written By Bobby Dale Gifford.				*
* 12/8/80							*
*								*
*****************************************************************

hdrev	equ	13		;Revision number
maxhd	equ	4		;Maximum # of Hard Disk Drives
mrev	equ	20		;Set to the type of hard disk (10, 20, 26)

hdorg	equ	50h		;Hard Disk Controller origin
hdstat	equ	hdorg		;Hard Disk Status
hdcntl	equ	hdorg		;Hard Disk Control
hddata	equ	hdorg+3		;Hard Disk Data
hdfunc	equ	hdorg+2		;Hard Disk Function
hdcmnd	equ	hdorg+1		;Hard Disk Command
hdreslt	equ	hdorg+1		;Hard Disk Result
retry	equ	2		;Retry bit of result
tkzero	equ	1		;Track zero bit of status
opdone	equ	2		;Operaction done bit of status
complt	equ	4		;Complete bit of status
tmout	equ	8		;Time out bit of status
wfault	equ	10h		;Write fault bit of status
drvrdy	equ	20h		;Drive ready bit of status
index	equ	40h		;Index bit of status
pstep	equ	4		;Step bit of function
nstep	equ	0fbh		;Step bit mask of function
hdrlen	equ	4		;Sector header length
seclen	equ	512		;Sector data length
wenabl	equ	0fh		;Write enable
wreset	equ	0bh		;Write reset of function
scenbl	equ	5		;Controller control
dskclk	equ	7		;Disk clock for control
mdir	equ	0f7h		;Direction mask for function
null	equ	0fch		;Null command
idbuff	equ	0		;Initialize data command
isbuff	equ	8		;Initialize header command
rsect	equ	1		;Read sector command
wsect	equ	5		;Write sector command
	if	mrev ne 26
seccnt	equ	21		;21 sectors per track for M10 and M20
	else
seccnt	equ	32		;32 for M26
	endif

	org	100h

	lxi	sp,stack
	mvi	c,0
	call	setdrv
	call	home
	lxi	b,bxx
	call	setdma
loop	mvi	c,0
	call	settrk
	mvi	c,0
	call	sethead
	mvi	c,1
	call	setsec
	mvi	c,80h
	call	setkey
	call	hdread
	nop
	mvi	c,200
	call	settrk
	mvi	c,0
	call	sethead
	mvi	c,1
	call	setsec
	mvi	c,0
	call	setkey
	call	hdread
	nop
	jmp	loop


bxx	ds	512

	ds	50
stack	equ	$


setdrv	jmp	hddrv		;Select disk
home	jmp	hdhome		;Recalibrate
settrk	jmp	hdtrk		;Seek to specified track
setsec	jmp	hdsec		;Prep for sector #
setdma	jmp	hddma		;Prep for DMA address
sethead	jmp	hdhead		;Set head #
setkey	jmp	hdkey		;Set the key in next transfer
read	jmp	hdread		;Read one sector
write	jmp	hdwrite		;Write one sector
dmastat	jmp	dmahd		;Return DMA address
getstat	jmp	stathd		;Get drive status

hddrv	mvi	a,3
	ana	c
	sta	hddisk
	ori	null		;Select drive
	out	hdfunc
	mvi	a,scenbl	;Enable the controller
	out	hdcntl
	mvi	c,239		;Wait for Disk to ready
				; 2 minutes for M26
				; 30 seconds for M10 & M20
	lxi	h,0
tdelay	dcx	h
	mov	a,h
	ora	l
	cz	dcrc
	stc
	rz
	in	hdstat		;Test if ready yet
	ani	drvrdy
	jnz	tdelay

	if	mrev ne 20	;M20 will do settle delay in hardware
	lhld	settle
	mov	a,h
	ora	l
	rz
	lxi	h,0		;Time one revolution of the drive
	mvi	c,index
	in	hdstat
	ana	c
	mov	b,a		;Save current index level in B
indx1	in	hdstat
	ana	c
	cmp	b		;Loop util index level changes
	jz	indx1
indx2	inx	h
	in	hdstat		;Start counting until index returns to
	ana	c		;	previous state
	cmp	b
	jnz	indx2
	shld	settle		;Save the Count for timeout delay
	endif
	ret

dcrc	dcr	c		;Conditional decrement C routine
	ret

hdkey	mov	a,c
	sta	nkey
	ret

hdhome	call	drvptr
	mvi	m,0		;Set track to zero
	inx	h		;Point to seek flag
	mvi	m,1		;Set not seeking, but must delay
stepo	in	hdstat		;Test status
	ani	tkzero		;At track zero ?
	rz

	if	mrev ne 20
	mvi	a,1
	stc
	call	accok		;Take one step out
	call	wsdone		;Wait for previous seek to finish
	jmp	stepo

	else

	xra	a		;Make (a) into zero will do 255 steps
	stc			;and on an m20 this will do a recalibrate
	jmp	accok
	endif

delay	equ	$
	if	mrev ne 20
	lxi	h,0		;Get delay
settle	equ	$-2
deloop	dcx	h		;Wait 20ms
	mov	a,h
	ora	l
	inx	h
	dcx	h
	jnz	deloop
	endif

	lxi	h,drives-1
	mvi	b,maxhd+1
delup	inx	h
	inx	h
	dcr	b
	rz
	mov	a,m
	dcr	a
	jnz	delup
	mov	m,a
	jmp	delup

hdtrk	call	drvptr		;Get pointer to current track
	mov	e,m		;Get current track
	push	h		;Save pointer to current track
	inr	e		;Ever homed this drive ?
	cz	hdhome
	pop	h		;Restore track pointer
	mov	e,m		;Get current track
	mov	m,c		;Update the track
	mov	a,e		;Need to seek at all ?
	sub	c
	rz
	push	psw		;Save # of steps
	inx	h		;Point to the seek complete flag
	mov	a,m		;Get current seek progress flag
	inr	a		;Currently seeking ?
	push	h		;Save seek flag pointer
	cz	wsdone		;Wait if currently seeking
	pop	h
	mvi	m,0ffh		;Set seek in progress flag
	pop	psw
	cmc			;Get carry into direction
	jc	accok
	cma
	inr	a
accok	mov	b,a		;Prep for build
	call	build
sloop	ani	nstep		;Get step pulse low
	out	hdfunc		;Output low step line
	ori	pstep		;Set step line high
	out	hdfunc		;Output high step line
	dcr	b		;Update repeat count
	jnz	sloop		;Keep going the required # of tracks
	ret

hddma	mov	h,b		;Save the DMA address
	mov	l,c
	shld	hdadd
	ret

wsdone	in	hdstat		;Wait for seek complete to finish
	ani	complt
	jz	wsdone
	lxi	h,drives-1	;Update all seek in progress flags
	mvi	b,maxhd+1
wsup	inx	h
	inx	h
	dcr	b
	rz
	mov	a,m
	ani	1
	mov	m,a
	jmp	wsup

hdsec	xra	a
	ora	c
	stc
	rz
	mvi	a,seccnt
	sub	c
	rc
	mov	a,c
	sta	hdsectr
	ret

hdhead	mov	a,c
	ani	7		; 7 for M26 & M20, 3 for M10
	sta	head
	ret

hdread	call	hdprep
	rc
	xra	a
	out	hdcmnd
	cma
	out	hddata
	out	hddata
	mvi	a,rsect		;Read sector command
	out	hdcmnd
	call	process
	rc
	xra	a
	out	hdcmnd
	mvi	b,seclen/4
	lxi	h,0
hdadd	equ	$-2
	in	hddata
	in	hddata
rtloop	in	hddata		;Move four bytes
	mov	m,a
	inx	h
	in	hddata
	mov	m,a
	inx	h
	in	hddata
	mov	m,a
	inx	h
	in	hddata
	mov	m,a
	inx	h
	dcr	b
	jnz	rtloop
	ret

hdwrite	call	hdprep		;Prepare header
	rc
	xra	a
	out	hdcmnd
	lhld	hdadd
	mvi	b,seclen/4
wtloop	mov	a,m		;Move 4 bytes
	out	hddata
	inx	h
	mov	a,m
	out	hddata
	inx	h
	mov	a,m
	out	hddata
	inx	h
	mov	a,m
	out	hddata
	inx	h
	dcr	b
	jnz	wtloop
	mvi	a,wsect		;Issue write sector command
	out	hdcmnd
	call	process
	rc
	mvi	a,wfault
	ana	b
	stc
	rz
	xra	a
	ret

process	in	hdstat		;Wait for command to finish
	mov	b,a
	ani	opdone
	jz	process
	mvi	a,dskclk
	out	hdcntl
	in	hdstat
	ani	tmout		;Timed out ?
	stc
	rnz
	in	hdreslt
	ani	retry		;Any retries ?
	stc
	rnz
	xra	a
	ret

hdprep	in	hdstat
	ani	drvrdy
	stc
	rnz
	mvi	a,isbuff	;Initialize pointer
	out	hdcmnd
	call	build
	ori	0ch
	out	hdfunc
	lda	head
	out	hddata		;Form head byte
	call	drvptr
	mov	a,m		;Form track byte
	out	hddata
	mvi	a,0		;Form sector byte
hdsectr	equ	$-1
	out	hddata
	mvi	a,0
nkey	equ	$-1
	out	hddata
	inx	h		;Bump to seek flag
	mov	a,m
	inr	a		;Update the seek in progress flag
	push	h
	cz	wsdone
	pop	h
	mov	a,m
	dcr	a		;Test for delay also
	cz	delay
	mvi	a,dskclk
	out	hdcntl
	mvi	a,wenabl
	out	hdcntl
	xra	a
	ret

drvptr	lhld	hddisk
	xchg
	mvi	d,0
	lxi	h,drives
	dad	d
	dad	d
	ret

build	mvi	a,0
head	equ	$-1
	ral
	ral
	ral
	ral
	ori	0
hddisk	equ	$-1
	xri	0f0h
	ret

dmahd	push	h
	lhld	hdadd
	mov	b,h
	mov	c,l
	pop	h
	ret

stathd	in	hdreslt
	ani	3
	mov	b,a
	in	hdstat
	xri	31h
	ret

drives	dw	0ffffh
	dw	0ffffh
	dw	0ffffh
	dw	0ffffh

	end
