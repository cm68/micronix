*****************************************************************
*								*
* Formathd formats the Discus M26 or the Discus M10 hard disk	*
* drive. It write sector header information on the entire disk	*
* surface. When the formatting has been completed, a map is	*
* created on track 0, head 1, last two sectors of all the 	*
* encountered bad sectors. Each entry of the map consists of	*
* the three bytes identifying the track, head, and sector that	*
* was bad.							*
*								*
* Written By Bobby Dale Gifford.				*
* 5/18/81							*
*								*
*****************************************************************

	title	'*** Format Program for Discus M26 & M10 under CP/M 2.2 ***'

revnum	equ	22		;Revision number x.x
hdorg	equ	050h		;Hard disk origin
status	equ	hdorg		;Status register
cntrol	equ	hdorg		;Control register
data	equ	hdorg+3		;Data register
functn	equ	hdorg+2		;Function register
comnd	equ	hdorg+1		;Command register
result	equ	hdorg+1		;Result register
retry	equ	2
retries	equ	20		;Maximum error count
tkzero	equ	1		;Track zero flag bit
opdone	equ	2		;Operation complete flag bit
complt	equ	4		;Complete flag bit
tmout	equ	8		;Time out flag bit
wfault	equ	10h		;Write fault flag bit
drvrdy	equ	20h		;Drive ready flag bit
index	equ	40h		;Index hole flag bit
halt	equ	80h		;Halt flag bit
cstrt	equ	3		;Command start
drvmsk	equ	3		;Drive bit mask
pstep	equ	4		;Positive step bit
hdrlen	equ	4		;Header length in bytes
seclen	equ	512		;Sector length in bytes
wenabl	equ	0fh		;Write enable
cpuclk	equ	0bh		;Cpu clock mask
wreset	equ	0bh		;Write fault reset mask
scenbl	equ	5
dskclk	equ	7		;Disk drive clock mask
secm26	equ	32		;Sector count for m26
secm10	equ	21		;Sector count for m10
trkm26	equ	202		;Tracks on an M26
trkm10	equ	244		;Tracks on an M10
hedm26	equ	8		;Heads on an M26
hedm10	equ	4		;Heads on an M10
hedm20	equ	8		;Heads on an M20
wprot	equ	80h		;Write protect flag bit
mdir	equ	0f7h		;Direction mask
nstep	equ	0fbh		;Negative step mask
null	equ	0fch		;Null command
idbuff	equ	0		;Initialize data buffer command
isbuff	equ	8		;Initialize sector buffer command
rsect	equ	1		;Read sector command
rhedr	equ	3		;Read header command
wsect	equ	5		;Write sector command
whedr	equ	7		;Write header command

tpa	equ	100h		;CP/M transient program area
tbuff	equ	80h
wboot	equ	0		;CP/M warm boot location
bdos	equ	5		;BDOS entry point
acr	equ	0dh		;Carriage return
alf	equ	0ah		;Line feed
atab	equ	9		;Tab

*****************************************************************
*								*
* Main code begins here.					*
*								*
*****************************************************************

	org	tpa

start	lxi	sp,stack	;Initialize the stack
	lxi	d,prompt
	call	pbuff

askall	lxi	d,funcmsg
	call	pbuff
	call	rbuff
	cpi	acr
	jz	wboot
	cpi	'C'
	jz	continu
	cpi	'D'
	jz	doall
	cpi	'F'
	jz	fmtonly
	cpi	'L'
	jz	logical
	jmp	askall

doall	call	howmuch
	call	getphy
	call	prep
	mvi	a,1
	sta	phase
	lda	much
	ani	1
	jnz	dohead
preask	lxi	d,prefmt
	call	pbuff
	call	rbuff
	jz	wboot
	cpi	'Y'
	jz	skiphd
	cpi	'N'
	jnz	preask
	call	preform
	jmp	skiphd

dohead	call	headmap
skiphd	mvi	a,2
	sta	phase
	lda	much
	ani	2
	cnz	datamap		;Make the bad sector map
	lda	much
	ani	4
	cnz	seeks
	lxi	d,donmsg
	jmp	exit

continu	lxi	d,notyet
	jmp	exit

howmuch	lxi	d,muchmsg
	call	pbuff
	call	rbuff
	cpi	acr
	jz	wboot
	cpi	'1'
	jc	howmuch
	cpi	'7'+1
	jnc	howmuch
	sui	'0'
	sta	much
	ret

getphy	lxi	d,phnum
	call	pbuff
	call	rbuff
	cpi	acr
	jz	wboot
	cpi	'1'
	jc	getphy
	cpi	'4'+1
	jnc	getphy
	sui	'1'
	sta	drive
gettyp	lxi	d,drvtyp
	call	pbuff
	call	rbuff
	cpi	acr
	jz	wboot
	cpi	'A'
	jc	gettyp
	cpi	'C'+1
	jnc	gettyp
	sui	'A'
	sta	disktyp
	cpi	1
	mvi	a,hedm10
	sta	heads
	jz	gtype
	mvi	a,hedm20
	sta	heads
	ret

gtype	xra	a
	sta	sdelay
	lxi	d,dtype
	call	pbuff
	call	rbuff
	cpi	acr
	jz	wboot
	cpi	'F'
	rz
	cpi	'M'
	jnz	gtype
	mvi	a,1
	sta	sdelay
	ret

getlog	lxi	d,lognum
	call	pbuff
	call	rbuff
	cpi	acr
	jz	wboot
	cpi	'1'
	jc	getlog
	push	psw
	call	phytyp
	mvi	b,'3'+1
	jz	cmptrk1
	lda	heads
	cpi	hedm20
	mvi	b,'2'+1
	jnz	cmptrk1
	mvi	b,'3'+1
cmptrk1	pop	psw
	cmp	b
	jnc	getlog
	sui	'1'
	mov	e,a
	mvi	d,0
	call	phytyp
	lxi	h,trksm26
	jz	trksok
	lda	heads
	cpi	hedm20
	lxi	h,trksm10
	jnz	trksok
	lxi	h,trksm20
trksok	dad	d
	mov	a,m
	sta	frsttrk
	inx	h
	mov	a,m
	sta	lasttrk
	ret

trksm26	db	0,64,127,203
trksm10	db	0,122,244
trksm20	db	0,98,195,244

headata	lxi	d,hdmsg
	call	pbuff
	xra	a
	sta	nodata
	call	rbuff
	cpi	acr
	jz	wboot
	cpi	'H'
	rz
	cpi	'D'
	jnz	headata
	mvi	a,1
	sta	nodata
	ret

phytyp	lda	disktyp
	ana	a
	ret

*****************************************************************
*								*
* Prep makes sure the drive is up and running.			*
*								*
*****************************************************************

prep	lda	drive		;Select the drive
	ori	null		;Nop
	sta	dfnreg
	out	functn
	mvi	a,scenbl
	out	cntrol
	mvi	c,239
	lxi	h,0
	in	status
	ani	drvrdy
	jz	measure
	lxi	d,rdywait
	call	pbuff
tdelay	in	status
	ani	drvrdy
	jz	measure
	dcx	h
	mov	a,h
	ora	l
	jnz	tdelay
	dcr	c
	jnz	tdelay
	lxi	d,rdymsg
	jmp	exit

*****************************************************************
*								*
* Measure measures the time in machine cycles for one 		*
* revolution of the disk.					*
*								*
*****************************************************************

measure	lxi	h,0		;Initialize the counter to 0
	mvi	c,index		;Index flag initialized
	in	status
	ana	c		;Get state of the index line now
	mov	b,a		;Save state in B
waitm1	in	status		;Get state now
	ana	c		;Mask off the index bit
	cmp	b		;Same state ?
	jz	waitm1		;Wait for toggle
waitm2	inx	h		;Increment counter
	in	status		;Wait for another toggle
	ana	c		;Strip off index
	cmp	b		;Has it toggled yet ?
	jnz	waitm2
	shld	settle		;Save the counter
	in	status		;Test status
	ani	halt		;Test for halted
	lxi	d,hltmsg
	jz	exit
	mvi	a,wenabl	;Write enable the drive
	out	cntrol
	mvi	a,dskclk
	out	cntrol		;Change to Clock from drive
	mvi	a,wenabl
	out	cntrol		;Finish write enable toggle
	call	delay		;Wait 20 ms
	in	status
	ani	wfault		;Check for write fault
	lxi	d,wfmsg
	jz	exit
	lxi	h,mapbeg
	shld	mappnt
	lxi	b,400h
noers	mvi	m,0ffh
	inx	h
	dcx	b
	mov	a,b
	ora	c
	jnz	noers

*****************************************************************
*								*
* Trkzro moves the disk heads to track 0.			*
*								*
*****************************************************************

trkzro	in	status		;Check for already at track 0
	ani	tkzero
	rz
hloop	lxi	b,101h		;Command to mhead
	call	mhead		;Move the head
cwait	in	status
	ani	complt		;Wait for seek complete
	jz	cwait
	in	status
	ani	tkzero		;At track 0 yet ?
	jnz	hloop

*****************************************************************
*								*
* Delay uses the counter created by measure to wait for 20ms.	*
*								*
*****************************************************************

	
delay	call	phytyp
	cpi	2
	rz			;If its an M20 it will do the settle delay
	cpi	1
	jnz	dodel
	lda	sdelay
	ora	a
	rz
dodel	lhld	settle
deloop	dcx	h
	mov	a,h
	ora	l
	inx	h
	dcx	h
	in	status		;Waste a little extra time to get 33 ms
	jmp	$+3
	jnz	deloop
	ret

*****************************************************************
*								*
* Headmap scans sector headers, locating bad ones. Each bad	*
* header is entered into the bad sector map.			*
*								*
*****************************************************************

headmap	lxi	d,headmsg
	call	pbuff
	call	trkzro
	lxi	h,0
	shld	buffer
	shld	buffer+2
	xra	a
	sta	head
newhed	mvi	a,1
	sta	hedbuf1
	sta	sflg
	call	zers
	lda	head
	rlc
	rlc
	rlc
	rlc
	ani	70h
	cma
	ani	0fch
	mov	b,a
	lda	drive
	add	b
	sta	dfnreg
	out	functn
hedtst	lxi	h,hedbuf1
	mvi	a,1
	call	initfld
	call	panic
	call	indx
	lxi	h,hedbuf1
	call	phytyp
	mvi	b,secm26
	jz	wrtloop
	mvi	b,secm10
wrtloop	mvi	c,hdrlen
	mvi	a,isbuff
	out	comnd
wrout	mov	a,m
	inx	h
	out	data
	dcr	c
	jnz	wrout
	mvi	a,whedr
	out	comnd
wrwait	in	status
	ani	opdone
	jz	wrwait
	dcr	b
	jnz	wrtloop
	lxi	h,hedbuf2
	call	phytyp
	mvi	b,secm26
	jz	rdloop
	mvi	b,secm10
rdloop	mvi	a,rhedr
	out	comnd
rdwait	in	status
	ani	opdone
	jz	rdwait
	mvi	a,isbuff
	out	comnd
	mvi	c,hdrlen
	in	data
	in	data
rdin	in	data
	mov	m,a
	inx	h
	dcr	c
	jnz	rdin
	dcr	b
	jnz	rdloop
	in	status
	ani	wfault		;Check for write fault
	lxi	d,wfmsg
	jz	exit
	lxi	h,hedbuf1
	lxi	d,hedbuf2
	call	phytyp
	mvi	b,secm26*hdrlen
	jz	comp
	mvi	b,secm10*hdrlen
comp	ldax	d
	cmp	m
	cnz	icheder
	inx	h
	inx	d
	dcr	b
	jnz	comp
	lxi	h,hedbuf1
	call	newpat
	jnz	hedtst
	call	zrolok
	call	makbuf0
	call	cformat
	call	inchead
	jnz	newhed
	call	update
	call	inctrk
	rz
	jmp	newhed

icheder	push	h
	push	d
	lxi	d,-hedbuf1
	dad	d
	mov	a,l
	ani	0fch
	rar
	call	incerr
	pop	d
	pop	h
	ret

zrolok	lxi	h,errors
	call	phytyp
	mvi	b,secm26
	jz	zrolook
	mvi	b,secm10
zrolook	mov	e,m
	inx	h
	mov	d,m
	inx	h
	mov	a,e
	ora	d
	jz	goodsec
	push	b
	push	d
	push	h
	lxi	h,1
	call	hlcde
	lxi	d,softmsg
	jnc	phs
	lxi	d,hardmsg
phs	call	pbuff
	lxi	d,errmsg
	call	pbuff
	pop	h
	push	h
	dcx	h
	dcx	h
	lxi	d,-errors
	dad	d
	mov	a,l
	rar
	ani	1fh
	inr	a
	inr	a
	push	psw
	call	phytyp
	mvi	b,secm26+1
	jz	maxsec
	mvi	b,secm10+1
maxsec	pop	psw
	cmp	b
	jnz	nowrp
	mvi	a,1
nowrp	sta	badsec
	lda	buffer+1
	call	putadc
	lxi	d,hedmsg
	call	pbuff
	lda	head
	call	putadc
	lxi	d,secmsg
	call	pbuff
	lda	badsec
	call	putadc
	lxi	d,rtymsg
	call	pbuff
	pop	h
	xthl
	push	h
	call	putdc
	lxi	d,acralf
	call	pbuff
	pop	h
	push	h
	lxi	d,2
	call	hlcde
	lhld	mappnt
	lxi	d,-mapbeg
	dad	d
	lxi	d,seclen*2-3
	call	hlcde
	lxi	d,fullmsg
	jnc	exit
	lhld	mappnt
	lda	buffer+1
	mov	m,a
	inx	h
	lda	head
	mov	m,a
	inx	h
	lda	badsec
	mov	m,a
	inx	h
	mvi	m,0ffh
	shld	mappnt
	mvi	a,1
	sta	errflg
wassft	pop	d
	pop	h
	pop	b
goodsec	dcr	b
	jnz	zrolook
	lda	errflg
	ana	a
	mvi	a,0
	sta	errflg
	lxi	d,acralf
	jnz	pbuff
	ret

*****************************************************************
*								*
* Datamap locates bad sector data fields.			*
*								*
*****************************************************************

datamap	lxi	d,datmsg
	call	pbuff
	call	trkzro
	lxi	h,0
	shld	buffer
	shld	buffer+2
	xra	a
	sta	head
	inr	a
	sta	buffer+2
nxthed	call	makbuf0
	lda	buffer+1
	ana	a
	mvi	a,0
	jnz	zkey
	mvi	a,80h
zkey	sta	buffer+3
	mvi	a,1
	sta	datbuf1
	sta	sflg
	call	zers
samhed	call	panic
	lxi	h,datbuf1
	push	h
	call	phytyp
	mvi	a,seclen/(secm26*hdrlen)
	jz	goinit
	mvi	a,seclen/(secm10*hdrlen)
goinit	call	initfld
	pop	h
wrdlp	lda	buffer+2
	xra	m
	mov	m,a
	mvi	c,retries
wagan	push	b
	push	h
	call	preprw
	pop	h
	push	h
	call	wdata
	jnc	wok
	pop	h
	pop	b
	dcr	c
	jnz	wagan
	push	b
	push	h
	lda	buffer+2
	ora	a
	ral
	sui	4
	call	incerr
wok	pop	h
	pop	b
	lda	buffer+2
	xra	m
	mov	m,a
	call	skew
	jnz	wrdlp
rddlp	mvi	c,retries
rdagan	push	b
	push	h
	call	preprw
	mvi	a,rsect
	out	comnd
rddwat	in	status
	ani	opdone
	jz	rddwat
	in	status
	ani	tmout
	jnz	rderr
	in	result
	ani	retry
	jnz	rderr
	mvi	a,idbuff
	out	comnd
	in	data
	in	data
	lda	buffer+2
	mov	b,a
	pop	h
	push	h
	in	data
	xra	b
	cmp	m
	cnz	secerr
	inr	l
	in	data
	cmp	m
	cnz	secerr
	inr	l
cmpdat	in	data
	cmp	m
	cnz	secerr
	inr	l
	in	data
	cmp	m
	cnz	secerr
	inr	l
	jnz	cmpdat
	pop	h
	pop	b
rdok	call	skew
	jnz	rddlp
	lxi	h,datbuf1
	call	newpat
	jnz	samhed
	call	zrolok
	call	inchead
	jnz	nxthed
	call	update
	call	inctrk
	rz
	jmp	nxthed

rderr	call	secerr
	pop	h
	pop	b
	dcr	c
	jnz	rdagan
	jmp	rdok

secerr	push	h
	lda	buffer+2
	ora	a
	ral
	sui	4
	call	incerr
	pop	h
	ret

skew	lda	buffer+2
	inr	a
	sta	buffer+2
	push	psw
	call	phytyp
	mvi	b,secm26+1
	jz	skewsec
	mvi	b,secm10+1
skewsec	pop	psw
	cmp	b
	rc
	sub	b
	inr	a
	sta	buffer+2
	dcr	a
	ret

wdata	mvi	a,idbuff
	out	comnd
	lxi	b,seclen
wdat	mov	a,m
	out	data
	inx	h
	dcr	c
	jnz	wdat
	dcr	b
	jnz	wdat
wsame	mvi	a,wsect
	out	comnd
wdwait	in	status
	ani	opdone
	jz	wdwait
	in	status
	ani	tmout
	stc
	rnz
	in	result
	ani	retry
	stc
	rnz
	in	status
	ani	wfault
	stc
	rz
	cmc
	ret

preprw	mvi	a,isbuff
	out	comnd
	lxi	h,buffer
	mvi	b,hdrlen
whed	mov	a,m
	out	data
	inx	h
	dcr	b
	jnz	whed
	ret

*****************************************************************
*								*
* Update writes the bad sector map onto the drive.		*
*								*
*****************************************************************

update	lhld	mappnt
	mvi	m,0ffh
	inx	h
	lda	buffer+1
	mov	m,a
	inx	h
	mvi	b,1
	mov	c,a
	push	b
	lda	tsttyp
	mov	m,a
	inx	h
	lda	phase
	mov	m,a
	inx	h
	mvi	m,0ffh
	lda	buffer+2
	push	psw
	xra	a
	sta	buffer+1
	lda	disktyp
	ana	a
	jz	upm26
	mvi	a,1	
upm26	sta	head
	mvi	a,80h
	sta	buffer+3
	call	phytyp
	mvi	a,secm26-1
	jz	updt1
	mvi	a,secm10-1
updt1	sta	buffer+2
	call	seek
	call	makbuf0
	call	preprw
	lxi	h,mapbeg
	call	wdata
	cc	fail
	call	phytyp
	mvi	a,secm26
	jz	updt2
	mvi	a,secm10
updt2	sta	buffer+2
	call	makbuf0
	call	preprw
	lxi	h,mapbeg+seclen
	call	wdata
	cc	fail
	pop	psw
	sta	buffer+2
	xra	a
	sta	head
	sta	buffer
	sta	buffer+3
	pop	b
	mvi	b,0
	mov	a,c
	sta	buffer+1
seek	call	mhead
seekwt	in	status
	ani	complt
	jz	seekwt
	jmp	delay

fail	lxi	d,failmsg
	jmp	pbuff

*****************************************************************
*								*
* Indx waits until the index hole just passes by.		*
*								*
*****************************************************************

indx	mvi	c,index
	in	status
	ana	c
	mov	b,a
indxwt	in	status
	ana	c
	cmp	b
	jz	indxwt
	ret

logical	call	getphy
	call	getlog
	call	headata
	call	prep
	lxi	d,logmsg
	call	pbuff
	call	form
	lxi	d,donmsg
	jmp	exit

*****************************************************************
*								*
* Format routine does the actual sector header formatting on	*
* the entire disk.						*
*								*
*****************************************************************

fmtonly	call	getphy
	call	headata
	call	prep
	call	preform
	lxi	d,donmsg
	jmp	exit

preform	lxi	d,formmsg
	call	pbuff
	lxi	h,0		;Zero the header information
	shld	buffer
	shld	buffer+2
	call	phytyp
	mvi	a,trkm26
	jz	setlst
	mvi	a,trkm10
setlst	sta	lasttrk
	xra	a
	sta	frsttrk

form	call	trkzro
	xra	a
	sta	head

	lda	frsttrk
	sta	buffer+1
	mov	c,a
	mvi	b,0
	call	seek

	lxi	d,acralf
	call	pbuff

	mvi	a,idbuff
	out	comnd
	mvi	b,seclen/4
	mvi	a,0e5h
file5	out	data
	out	data
	out	data
	out	data
	dcr	b
	jnz	file5

mainl	call	makbuf0
	call	cformat
	in	status
	ani	wfault		;Check for write fault
	lxi	d,wfmsg
	jz	exit
fmttrk	lda	nodata
	ana	a
	push	h
	jz	fmtskp
	pop	h
	call	phytyp
	jz	fmm26
	lda	buffer
	cpi	1
	jnz	fmtdat
	jmp	fmmon
fmm26	lda	buffer
	ana	a
	jnz	fmtdat
fmmon	lda	buffer+1
	ana	a
	jnz	fmtdat
	push	h
	call	phytyp
	mvi	b,secm26-1
	jz	cmp1x
	mvi	b,secm10-1
cmp1x	lda	buffer+2
	cmp	b
	jc	fmtskp
	jz	fmtskp
	pop	h
fmtdat	push	h
	call	preprw
	pop	h
	push	h
	call	wsame
fmtskp	call	skew
	pop	h
	jnz	fmttrk
	call	inchead
	jnz	mainl
	call	inctrk
	lda	buffer+1
	mov	b,a
	lda	lasttrk
	cmp	b
	jnz	mainl
	ret

*****************************************************************
*								*
* Inchead increments the head.					*
*								*
*****************************************************************

inchead	lxi	h,head		;Increment the head number
	inr	m
	call	phytyp
	mvi	a,hedm26
	jz	cmphed
	lda	heads		;Test if done with this head
cmphed	sub	m
	rnz
	mov	m,a
	ret

*****************************************************************
*								*
* Inctrk increments the track, and then steps the heads out one.*
*								*
*****************************************************************

inctrk	lxi	h,buffer+1	;Track #
	call	phytyp
	mvi	a,trkm26
	jz	cmptrk
	mvi	a,trkm10
cmptrk	inr	m		;Increment the track #
	cmp	m		;All done ?
	rz
	call	pdot

nocr	lxi	b,1		;Move out
	call	seek
	ori	1
	ret

pdot	lda	quiet
	ana	a
	rnz
	lxi	d,astrk		;Print an astrisk
	call	pbuff
	lda	buffer+1
	ani	3fh
	rnz
	lxi	d,acralf
	jmp	pbuff

*****************************************************************
*								*
* Mhead moves the disk head the desired # of steps. Bit 0 of	*
* register B determines the directios, 1=out, 0=in. Register C	*
* determins the # of tracks to step.				*
*								*
*****************************************************************

mhead	mov	a,b		;Test direction
	ani	1
	ral			;Move into direction bit
	ral
	ral
	mov	b,a
	mov	a,c
	ana	a
	rz
	lda	dfnreg		;Get current function register
	ani	mdir		;Mask off the direction bit
	ora	b		;Put in the direction bit
mhloop	ani	nstep		;Mask off the step bit
	out	functn		;Output the step bit low
	ori	pstep		;Change the step bit high
	out	functn		;Output the step bit high
	dcr	c		;Update the # of steps to go
	jnz	mhloop
	ret

*****************************************************************
*								*
* Seeks does track seek tests.					*
*								*
*****************************************************************

seeks	lxi	d,seekmsg
	call	pbuff
	mvi	a,10
	sta	sekcnt
sklopa	lda	sekcnt
	dcr	a
	sta	sekcnt
	rz
	call	pdot
	lxi	h,seektbl
sklopb	mov	e,m
	inx	h
	mov	d,m
	inx	h
	mov	a,e
	ora	d
	jz	sklopa
	push	h
	xchg
	call	dopchl
	pop	h
	jmp	sklopb

dopchl	pchl

seektbl	dw	seekinc
	dw	seekdec
	dw	seekjrk
	dw	seekfly
	dw	seekmax
	dw	0

seekinc	xra	a
	sta	first
	sta	last
	sta	dltafst
	inr	a
	sta	dltalst
seekcom	call	phytyp
	mvi	a,trkm26-1
	jz	sekcom
	mvi	a,trkm10-1
sekcom	sta	reps
	jmp	doseek

seekdec	call	phytyp
	mvi	a,trkm26-1
	jz	sekdec
	mvi	a,trkm10-1
sekdec	sta	first
	sta	last
	xra	a
	sta	dltafst
	dcr	a
	sta	dltalst
	jmp	seekcom

seekjrk	xra	a
	sta	first
	inr	a
	sta	dltalst
	sta	dltafst
	inr	a
	sta	last
	call	phytyp
	mvi	a,trkm26-3
	jz	sekjrka
	mvi	a,trkm10-3
sekjrka	sta	reps
	sta	last
	inr	a
	inr	a
	sta	first
	mvi	a,-1
	sta	dltalst
	sta	dltafst
	jmp	doseek

seekmax	xra	a
	sta	first
	sta	dltafst
	sta	dltalst
	sta	reps
	call	phytyp
	mvi	a,trkm26-1
	jz	sekmax
	mvi	a,trkm10-1
sekmax	sta	last
	jmp	doseek

seekfly	mvi	a,-1
	sta	dltafst
	inr	a
	sta	first
	inr	a
	sta	dltafst
	call	phytyp
	mvi	a,trkm26-1
	jz	sekfly
	mvi	a,trkm10-1
sekfly	sta	last
	sta	reps

doseek	call	sek00
dosek	call	panic
	lda	first
	mov	c,a
	lda	dltafst
	add	c
	sta	first
	call	seekit
	lda	last
	mov	c,a
	lda	dltalst
	add	c
	sta	last
	call	seekit
	lda	reps
	dcr	a
	sta	reps
	jnz	dosek
	ret

seekit	mov	b,a
	lda	buffer+1
	sub	b
	push	psw
	mov	a,b
	sta	buffer+1
	pop	psw
	mvi	b,0
	jc	movin
	mvi	b,1
	cma
	inr	a
movin	cma
	inr	a
	mov	c,a
	call	seek
	mvi	a,rhedr
	out	comnd
sekwait	in	status
	ani	opdone
	jz	sekwait
	mvi	a,isbuff
	out	comnd
	in	data
	in	data
	in	data
	in	data
	mov	b,a
	lda	buffer+1
	cmp	b
	rz
	push	b
	lxi	d,sekmsg1
	call	pbuff
	lda	buffer+1
	call	putadc
	lxi	d,sekmsg2
	call	pbuff
	pop	b
	mov	a,b
	call	putadc
	lxi	d,sekmsg3
	call	pbuff
sek00	call	trkzro
	xra	a
	sta	buffer+1
	ret

*****************************************************************
*								*
* Cformat formats one track on the current head.		*
*								*
*****************************************************************

cformat	mvi	a,1		;Initialize the sector count
	sta	buffer+2
	lda	buffer+1
	ana	a
	mvi	a,80h
	jz	putkey
	xra	a
putkey	sta	buffer+3
	call	indx
	call	phytyp
	mvi	b,secm26
	lxi	d,skewm26
	jz	floop
	mvi	b,secm10
	lxi	d,skewm10
floop	lxi	h,buffer+2
incsec	ldax	d
	mov	m,a
	inx	d
	dcx	h		;Adjust the pointer to the sector header
	dcx	h
	mvi	a,isbuff	;Initialize the controller pointer
	out	comnd		;Give the command
	mvi	c,hdrlen	;Header length
ldloop	mov	a,m		;Transfer the contents of the buffer to 
	out	data		;	the controller
	inx	h		;Bump pointer to next byte
	dcr	c		;Update the counter
	jnz	ldloop
	mvi	a,whedr		;Write header command
	out	comnd		;Issue the write header command
bwait	in	status		;Wait for the write to complete
	ani	opdone
	jz	bwait
	dcr	b		;Test for sector count equal to 0
	jnz	floop
	ret

skewm26	db	22,12,2,23,13,3,24,14,4,25,15,5,26,16,6
	db	27,17,7,28,18,8,29,19,9,30,20,10,31,21,11,32,1

skewm10	db	8,15,2,9,16,3,10,17,4,11
	db	18,5,12,19,6,13,20,7,14,21,1

zers	lxi	h,errors
	call	phytyp
	lxi	b,secm26
	jz	zersx
	lxi	b,secm10
zersx	mvi	m,0
	inx	h
	mvi	m,0
	inx	h
	dcx	b
	mov	a,b
	ora	c
	jnz	zersx
	ret

initfld	sta	cnt
	mov	a,m
	push	psw
	call	phytyp
	mvi	b,secm26
	jz	iniflx
	mvi	b,secm10
iniflx	pop	psw
inilp	push	psw
	push	h
	lxi	h,cnt
	mov	d,m
	pop	h
inilop	mvi	c,hdrlen
iniloop	mov	m,a
	inx	h
	cma
	dcr	c
	jnz	iniloop
	inr	a
	dcr	d
	jnz	inilop
	pop	psw
	inr	a
	dcr	b
	jnz	inilp
	ret

makbuf0	lxi	h,drive		;Get drive #
	mov	a,m
	rlc
	rlc			;Move to left four bits
	rlc
	rlc
	mov	b,a		;Save drive # in B
	lda	head		;Add in the head #
	sta	buffer
	add	b
	ani	7		;Strip off the drive #
	rlc
	rlc
	rlc			;Head # in to four significant bits
	rlc
	cma			;Head # is negative logic
	ani	0fch		;Strip off the drive bits
	add	m		;Add in this drive
	sta	dfnreg		;Save the function register
	out	functn		;Output the function
	ret

incerr	ani	3fh
	mov	l,a
	mvi	h,0
	lxi	d,errors
	dad	d
	mov	e,m
	inx	h
	mov	d,m
	inx	d
	mov	m,d
	dcx	h
	mov	m,e
	ret

newpat	lda	tsttyp
	ana	a
	mov	a,m
	jz	long
short	lda	sflg
	ana	a
	mvi	a,0
	sta	sflg
	mov	a,m
	jnz	shortx
	mvi	a,1
	sta	sflg
	mov	a,m
	cma
	ral
	cma
shortx	cma
	dcr	a
long	inr	a
	mov	m,a
	ana	a
	ret

*****************************************************************
*								*
* Pbuff is the CP/M print buffer command.			*
*								*
*****************************************************************

pbuff	lda	quiet
	ana	a
	jz	pbuffx
	push	d
	lxi	d,nulls
	call	pbuffx
	pop	d
pbuffx	mvi	c,9		;Print buffer command
	jmp	bdos		;Go and enter the BDOS

*****************************************************************
*								*
* Rbuff is the CP/M read buffer command.			*
*								*
*****************************************************************

rbuff	lxi	d,bufmax	;Beginning of buffer
	mvi	c,10		;Read console command
	call	bdos		;Read and fill the buffer
	lda	buflen		;Check for zero length
	ana	a		;if zero the return with acr in A
	mvi	a,acr
	rz
	lda	bufdat		;Otherwise return with the character
	cpi	'a'		;Fold to upper case
	rc
	cpi	'z'+1
	rnc
	sui	20h		;Perform the mapping to upper case
	ret

bufmax	db	10		;Maximum length
buflen	db	0		;Current length
bufdat	ds	10		;Storage or read in data

*****************************************************************
*								*
* Exit prints a message on the CP/M console then does a warm	*
* boot.								*
*								*
*****************************************************************

exit	call	pbuff
	lxi	d,insmsg
	call	pbuff
	call	rbuff
	jmp	wboot

hlcde	mov	a,h
	cmp	d
	rnz
	mov	a,l
	cmp	e
	ret

putadc	mov	l,a
	mvi	h,0

*****************************************************************
*								*
* Putdc prints the ascii decimal equivalent of the number in HL	*
*								*
*****************************************************************

putdc	lxi	b,-10
phl	push	d
	mov	d,b
	mov	e,b
phllp	dad	b
	inx	d
	jc	phllp
	xthl
	xchg
	mov	a,h
	ora	l
	cnz	phl
	pop	h
	mvi	a,'0'
	add	l
	sub	c

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

*****************************************************************
*								*
* Panic checks if a character has been hit on the console.	*
* If a character has been pressed, then panic acknowledges the	*
* fact and asks for an abort.					*
*								*
*****************************************************************

panic	mvi	c,11
	call	bdos
	ani	1
	rz
	lxi	d,panmsg
	call	pbuff
	mvi	c,1
	call	bdos
	call	rbuff
	cpi	'Y'
	jz	wboot
	lxi	d,acralf
	jmp	pbuff

*****************************************************************
*								*
* Messages output by the program.				*
*								*
*****************************************************************

prompt	db	'Discus M26 and M10 Hard Disk Format Program Rev. '
	db	revnum/10+'0'
	db	'.'
	db	(revnum mod 10)+'0'
	db	'$'

phnum	db	acr,alf
	db	'Enter Physical Drive Number to be Tested or Formated '
	db	'(1 - 4, RETURN to exit): $'

lognum	db	acr,alf
	db	'Enter Logical Drive Number to be Formated '
	db	'(1 - 3, RETURN to exit): $'

hdmsg	db	acr,alf
	db	'Enter Amount of Formatting Desired:'
	db	acr,alf,atab
	db	'H = Format Headers Only (Data Remains Intact).'
	db	acr,alf,atab
	db	'D = Erase Data Field Also.'
	db	acr,alf
	db	'Function: $'

rdymsg	db	acr,alf
	db	'Drive is not ready.$'

hltmsg	db	acr,alf
	db	'Controller not halted.$'

wfmsg	db	acr,alf
	db	'Write fault on drive.$'

astrk	db	'.$'

hardmsg	equ	$
softmsg	equ	$
acralf	db	acr,alf,'$'

rdywait	db	acr,alf
	db	'Waiting for drive to become ready, could take '
	db	'as long as 2 minutes.$'

headmsg	db	acr,alf
	db	'Testing sector headers, will take about 30 minutes.'
	db	acr,alf,'$'

formmsg	db	acr,alf
	db	'Formatting the entire physical disk, '
	db	'will take about 4 minutes.$'

logmsg	db	acr,alf
	db	'Formatting a logical drive, will take about 1.5 minutes.$'

errmsg	db	'Sector Error, Track $'
secmsg	db	', Sector $'
hedmsg	db	', Head $'
rtymsg	db	', Error count $'

donmsg	db	acr,alf
	db	'All Finished, Returning to CP/M.$'

panmsg	db	acr,alf
	db	'Want to abort ? (Yes or No): $'

datmsg	db	acr,alf
	db	'Testing sector data field, will take about 12-14 Hours.'
	db	acr,alf,'$'

failmsg	db	acr,alf
	db	'Error Writing Sector Map to Track Zero.$'

fullmsg	db	acr,alf
	db	'Bad Sector Map Overflow.$'

seekmsg	db	acr,alf
	db	'Testing Track Seek Mechanism, will take about 15 minutes.'
	db	acr,alf,'$'

sekmsg1	db	acr,alf
	db	'Seek error, Looking For Track $'
sekmsg2	db	', Found Track $'
sekmsg3	db	'.',acr,alf,'$'

unknown	db	acr,alf
	db	'Unknown Command.$'

bdrvmsg	db	acr,alf
	db	'Bad Drive Number.$'

funcmsg	db	acr,alf
	db	'Choose The Desired Function:'
	db	acr,alf,atab
	db	'L = Format a Logical Drive.'
	db	acr,alf,atab
	db	'F = Format an Entire Physical Drive.'
	db	acr,alf,atab
	db	'C = Continue An Interupted Test.'
	db	acr,alf,atab
	db	'D = Run a Diagnostic Test.'
	db	acr,alf
	db	'Function (RETURN to Exit): $'

notyet	db	acr,alf
	db	'Not Yet Implemented.$'

drvtyp	db	acr,alf
	db	'Select The Drive Type:'
	db	acr,alf,atab
	db	'A = Discus M26, 26 Megabyte Drive.'
	db	acr,alf,atab
	db	'B = Discus M10, 10 Megabyte Drive.'
	db	acr,alf,atab
	db	'C = Discus M20, 20 Megabyte Drive.'
	db	acr,alf
	db	'Drive type (RETURN to Exit): $'

dtype	db	acr, alf
	db	'Select drive type:'
	db	acr, alf, atab
	db	'F = Fujitsu 2301'
	db	acr, alf, atab
	db	'M = Memorex 101'
	db	acr, alf
	db	'Type (RETURN to Exit): $'

muchmsg	db	acr,alf
	db	'How Much of the Diagnostic Do You Want to Run: '
	db	acr,alf,atab
	db	'1 = Sector Header Field Test Only.'
	db	acr,alf,atab
	db	'2 = Sector Data Field Test Only.'
	db	acr,alf,atab
	db	'4 = Seek Mechanism Test Only.'
	db	acr,alf
	db	'Choose the Diagnostic by Adding together the '
	db	'Desired Options.'
	db	acr,alf
	db	'Options (RETURN to Exit): $'

prefmt	db	acr,alf
	db	'Has the Drive Been Preformated ? (Y or N, RETURN to Exit): $'

nulls	db	0,0,0,0,0,0,0,0,0,0,'$'

insmsg	db	acr,alf
	db	'Press RETURN to return to CP/M: $'

*****************************************************************
*								*
* Dynamic data locations.					*
*								*
*****************************************************************

quiet	db	0
sdelay	db	0
much	db	0
sekcnt	db	0
first	db	0
last	db	0
dltafst	db	0
dltalst	db	0
reps	db	0
disktyp	db	0
heads	db	0
nodata	db	0
frsttrk	db	0
lasttrk	db	0
settle	dw	0		;20ms time constant
buffer	ds	hdrlen
head	db	0
dfnreg	db	0fch
errflg	db	0
cnt	db	0
badsec	db	0
mappnt	dw	0		;Pointer into map table
drive	db	0ffh		;If this location is NOT 0ffh then 
				;	NO prompting will occur
sflg	db	0
tsttyp	db	1
phase	db	0
hedbuf1	ds	secm26*hdrlen
hedbuf2	ds	secm10*hdrlen
	ds	100h-($ mod 100h)
datbuf1	ds	seclen
mapbeg	ds	seclen*2
errors	ds	secm26*2
	ds	30
stack	equ	$

	end
