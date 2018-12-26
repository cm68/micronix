*
* REGEN.ASM  vers 1.1  september 30, 1980
*
* By	Bryan G. Moore
*	Design Technology
*	4888-H Ronson Court
*	San Diego, CA 92111
*
* Modified for all revision Disk Jockey 2D'S by Bobby Dale Gifford
* 9/30/80
*
* The program is used to  correct the  format on Single  Density
* IBM 3740 compatible diskettes. Attempts to use other diskettes
* will probably result in an error message.
*
*
vernum	equ	11	;Version number * 10
origin	equ	0F800h	;Disk Jockey 2D prom
bdos	equ	5	;CP/M entry point
wboot	equ	0	;Warm boot

diskio	equ	origin+3f8h
datreg	equ	diskio+7
drvsel	equ	diskio+1
cstall	equ	diskio+3
cmdreg	equ	diskio+4
secreg	equ	diskio+6
trkreg	equ	diskio+5
sbegin	equ	origin+3deh
ramins	equ	origin+7e2h
dside	equ	10Q
unloadb	equ	17Q
sicmd	equ	131Q
immirq	equ	320Q
unloada	equ	30Q
restor	equ	11Q
rtcmd	equ	0e4h
wsec	equ	0a0h
rsec	equ	80h
index	equ	20Q
trkzro	equ	4
intrqa	equ	1
intrqb	equ	4
lhsdenb	equ	90dh
lhddenb	equ	80ch
lhsdena	equ	111h
lhddena	equ	10h
acr	equ	0dh
alf	equ	0ah

	org	100h		;CP/M Tpa

begin	lxi	sp,stack	;Stack pointer
	lxi	d,prompt	;Sign-on
	call	pbuff		;To console
	lhld	origin+7	;Adjust the calling routines for different
	inx	h		;	revisions of the Disk Jockey
	mov	a,m
	lxi	h,stdvsl+1
	lxi	d,4
	mov	m,a
	xri	3
	dad	d
	mov	m,a
	dad	d
	mov	m,a

start	lxi	d,srcmsg	;Prompt the user to select drive
	call	readdrv		;Get a drive designator
	sta	sdisk		;Save source disk
	lxi	d,dstmsg	;Prompt the user to select drive
	call	readdrv		;Get drive designator
	sta	ddisk		;Save destination disk
mount	lxi	d,mntmsg	;Prompt the user to
	call	prbuff		;Read console
	jnz	start		;Ignore anything else

regena	lxi	d,acralf
	call	pbuff
	xra	a		;Select source
	call	select		;Select the drive
	lxi	h,cmdreg
	mvi	m,immirq	;Immediate interupt request
	mvi	a,40h		;Simple delay
wirqda	dcr	a
	jnz	wirqda
	mov	a,m		;Check ready
	rar
	jc	notrdy
	ral
	ral
	jc	notrdy
	lxi	d,0
	call	gtindx		;Get polarity of index pulse
	call	gtstat		;Test for double sided
	ani	dside
	jnz	ixlopa
dblside	lxi	d,sglside	;No double sided diskettes
	jmp	notrdyx

ixlopa	call	gtstat		;Wait for index pulses
	ani	index
	xra	b
	jnz	restora
	dcx	d
	mov	a,d
	ora	a
	jnz	ixlopa
notrdy	lxi	d,rmessg	;Not ready message
notrdyx	xchg
	lxi	d,amessg
	push	h
	call	pbuff		;Print drive message
	lda	cdisk
	ana	a
	lda	sdisk
	jz	notdrv
	lda	ddisk
notdrv	adi	'A'		;And drive letter
	mov	e,a
	mvi	c,2
	call	bdos
	pop	d
	call	pbuff
	call	unload		;Unload the heads
	jmp	mount

restora	call	restore
regenb	mvi	a,1		;Select destination
	call	select		;Select the drive
	lxi	h,cmdreg
	mvi	m,immirq	;Immediate interupt request
	mvi	a,40h		;Simple delay
wirqdb	dcr	a
	jnz	wirqdb
	mov	a,m		;Check ready
	rar
	jc	notrdy
	ral
	ral
	jc	notrdy
	lxi	d,wmessg
	ral
	jc	notrdyx
	lxi	d,0
	call	gtindx		;Get polarity of index pulse
	call	gtstat
	ani	dside		;Test for double sided diskette
	jz	dblside
ixlopb	call	gtstat		;Wait for index pulses
	ani	index
	xra	b
	jnz	regen
	dcx	d
	mov	a,d
	ora	a
	jnz	ixlopb
	jmp	notrdy

restore	lxi	h,cmdreg
	mvi	m,restor	;Home to track 0 command
wrstrs	mov	a,m
	rar			;Wait for busy
	jnc	wrstrs
wrstrd	mov	a,m
	rar			;Wait for not busy
	jc	wrstrd
	mov	a,m
	ani	trkzro
	jz	notrdy
	ret

regen	call	restore
	xra	a		;Initialize track number
	sta	tkno		;Start with track 0
	mvi	a,10		;Reset read retry
	sta	trcnt		;  counter to 10
dloop	xra	a
	call	select
	call	wnbusy		;Wait until not busy
	lxi	d,datreg	;Pointer acts as Data Register
	lxi	h,tkbuf		;Track buffer
	mvi	c,24		;C is page count
	mvi	a,rtcmd		;Setup controller
	sta	cmdreg
datin	ldax	d		;Load a data byte
	mov	m,a		;Store in memory
	inr	l		;Bump pointer / counter
	jnz	datin		;Get next byte
	inr	h		;Bump high order
	dcr	c		;Hit page count
	jnz	datin		;Loop til track read in
*
* Scan track for Sector Address Mark (FE Hex)
* Since this value is not always read properly using the Read
* Track command, this search assumes the high four bits of the
* Sector Address Mark are ones.
*
	lxi	h,sectb	;Initialize sector data pointer table
	shld	secix	;  to first sector
	lxi	h,tkbuf	;Initial Track buffer pointer
	mvi	a,1	;Initial sector number (1)
	sta	secno	;Save sector number
sloop	lxi	b,0feh	;Give up after 256 bytes
	call	searc	;Search for Sector Address mark
	lxi	d,4	;Field byte count
	call	setcrc	;Evaluate CRC status
	inx	h	;Offset to track number
	jnz	sloop	;Error - Continue Sector mark search
	lda	tkno	;Get current track
	cmp	m	;Compare with memory image
	jnz	terr	;Illegal track number
	inx	h	;Next byte must be 0
	mov	a,m	;Is it a zero ?
	ora	a
	jnz	terr	;No - jump
	inx	h	;Sector number
	lda	secno	;Get current sector number
	cmp	m	;Correct sector ?
	jnz	terr	;No - jump
	inx	h	;Next byte
	mov	a,m	;  must also be
	ora	a	;  a zero
	jnz	terr	;Not a zero - jump
	inx	h	;Skip CRC word
	inx	h
sc1	lxi	b,0fbh	;Search pattern and retry
	call	searc	;Search for mark
	lxi	d,128	;Sector length (including Data Mark)
	call	setcrc	;Get the CRC status
	inx	h	;Offset pointer to data
	jnz	sc1	;Error - continue search
	push	d	;Save pointer to last CRC byte
	xchg		;Flip Data address into D,E
	lhld	secix	;Get sector pointer index
	mov	m,e	;Store the
	inx	h	;  new sector
	mov	m,d	;  data address
	inx	h	;Point to next address storage
	shld	secix	;Save sector index
	pop	d	;Restore pointer to last CRC byte
	inx	d	;Bump to next raw data byte
	lxi	h,secno	;Point to the sector number
	inr	m	;  and bump to next sector
	mov	a,m	;Get this number
	xchg		;Flip data pointer back to HL
	cpi	27	;More sectors to go ?
	jc	sloop	;Yes - jump
	lhld	secix	;Get sector data address index
	dcx	h	;To last high order
	mov	d,m
	dcx	h	;Low order
	mov	e,m	;Load the start address
	lxi	h,130	;Sector length + CRC word
	dad	d	;Compute last address used
	lxi	d,-nbuf	;Negative of last track buffer address
	dad	d	;Is sector data address out of bounds ?
	jc	terr	;Yes - jump
	mvi	a,1
	call	select	;Select destination drive
	mvi	a,10	;Reset some counters
	sta	trcnt	;Reset track read error counter
	sta	rwcnt	;Reset the Re-Write counter
rewrt	lxi	h,sectb	;Point to Sector Data address table
	shld	secix	;Initialize index value
	mvi	a,1	;Initialize sector number
	sta	secno
twloop	call	wnbusy	;Wait for Controller Ready
	lda	secno	;Fetch sector number
	sta	secreg	;Send to controller
	lhld	secix	;Get sector address index
	mov	e,m	;Load the next dma address
	inx	h
	mov	d,m	;High order
	inx	h
	shld	secix	;Save for next loop
	lxi	h,datreg;Controller data register
	mvi	a,wsec	;Write Sector command
	sta	cmdreg	;Start controller
	mvi	c,128	;Bytes in a sector
wloop	ldax	d	;Get source byte from memory
	mov	m,a	;Write the data to the controller
	inx	d	;Next data byte
	dcr	c	;Hit data byte count
	jnz	wloop	;Loop until data written
	lxi	h,secno	;Point to the sector number
	inr	m	;Increment to next
	mov	a,m	;Load the value
	cpi	27	;More sectors to write ?
	jc	twloop	;Yes - jump
*
* Read the track back. Ignore the data, but verify the CRC
* status from the Disk Jockey 1791.
*
	call	wnbusy		;Wait until not busy
	call	windex		;Wait for index hole
	lxi	h,datreg	;Data register pointer
	mvi	a,1		;Initialize the
rtl1	sta	secreg		;  sector number
	mvi	c,128		;Bytes in a sector
	mvi	a,rsec		;Read Sector command
	sta	cmdreg		;Start controller
rtloop	mov	a,m		;Read a byte
	dcr	c		;Done this sector ?
	jnz	rtloop		;No - Loop until sector is read
	call	wnbusy		;Wait until not busy
	lda	cmdreg		;Load controller status
	ani	9ch		;Extract Not Ready, Not Found,
				;  CRC Error, Lost Data
	jnz	rwerr		;Go re-write if error
	lda	secreg		;Load sector register
	inr	a		;Next sector
	cpi	27		;More sectors to do ?
	jc	rtl1		;Yes - Loop
	lxi	h,tkno		;Point to Track number
	inr	m		;Bump it
	mov	a,m		;Get next track number
	cpi	77		;Is re-formatting complete ?
	jc	tkstep		;No - do next track
	call	unload
finish	lxi	d,fmessg	;Send completion

done	call	prbuff		;  message to console
	cpi	'R'		;More Re-formatting ?
	jz	start		;Yes - jump
exit	xra	a
	call	selda
	call	restore
	call	unload
	jmp	wboot		;Ignore anything else

*
* Step to next track
*
tkstep	lda	sdisk
	mov	b,a
	lda	ddisk
	cmp	b
	jz	samedrv
	mvi	a,'*'
	call	pchar
	call	takstep
	lda	trkreg
	dcr	a
	sta	trkreg
samedrv	xra	a
	call	select
	call	takstep
	jmp	dloop

takstep	lxi	d,cmdreg
	mvi	a,sicmd
	stax	d
wsicms	ldax	d
	rar
	jnc	wsicms
wsicmd	ldax	d
	rar
	jc	wsicmd
	ret

*
* Re-write error, track did not verify
* Bump counter, then flag permanent error
*
rwerr	lxi	h,rwcnt	;Point to Track Verify Retry counter
	dcr	m	;Decrement it
	jnz	rewrt	;Go re-write if not expired
	lxi	d,vmsg	;Permanent verify error

*
* Permanent error common routine
* No error codes determined yet
* Unload head, then abort
*
perr	call	pbuff		;Print the error message
	call	unload
	lda	trkreg		;Get current track number
	mvi	l,'0'-1		;L is one less than 0
per1	inr	l		;Bump digit value
	sui	10		;Hit tens digit count
	jnc	per1		;Loop til C = ASCII tens digit
	adi	'0'+10		;Restore + ASCII bias for units
	mov	h,a		;Place units digit
	mov	a,l		;Get the tens digit
	cpi	'0'		;Is it an ASCII 0 ?
	jnz	per2		;No - use it
	mvi	l,' '		;Else make it a space
per2	shld	tdsply		;Save track number display
	lxi	d,retmsg	;Prompt to press return
	jmp	done		;Treat as if done

*
* Track read error - decrement counter and try again
*
terr	lxi	h,trcnt		;Point to the track read counter
	dcr	m		;Have we tried too many times ?
	jnz	dloop		;No - try again
	lxi	d,tmessg	;Track read message
	jmp	perr		;Continue error routine

*
* Wait until drive not busy
*
wnbusy	lda	cmdreg		;Wait for controller to accomplish
	ani	1
	jnz	wnbusy		;Loop until it does
	ret			;Return when Ready

*
* Wait for index hole to go by
*
windex	call	gtindx
windxh	call	gtstat
	ani	index
	xra	b
	jz	windxh
windxl	call	gtstat
	ani	index
	xra	b
	jnz	windxl
	ret
*
* Search up to (B) bytes at HL for pattern represented
* by (C). Assumes upper 4 bits are 1s. Exit to TERR if
* byte not found.
*
searc	mov	a,m	;Load a byte
	ori	0f0h	;Turn on upper nibble
	mov	m,a	;Save in memory
	cmp	c	;Match ?
	rz		;Yes - Return
	inx	h	;Bump data pointer
	dcr	b	;Hit byte search count
	jnz	searc	;Loop til search complete
	pop	psw	;Clear stack
	jmp	terr	;Go to error routine

*
* Checksum checker
* Adapted from the Disk Jockey I Shugart Firmware
* Performs the Cyclic Redundancy Check (CRC)
* The polynomial is G(X) = X^16 + X^12 + X^5 + 1.
*
* Entry SETCRC - Pointer to data block in HL,
*	Field length (in bytes) - 1 in D,E
*	Verify checksum at end of block
* Return with Z set if match
*
setcrc	push	h	;Save start address pointer
	xchg		;Flip start addr to DE
	dad	d	;HL points to last byte
	xchg		;Swap back again
crech	lxi	b,-1	;Initial value per formula
crechx	push	d	;Save terminating pointer
	mov	a,m
	xra	c
	mov	d,a
	rrc
	rrc
	rrc
	rrc
	ani	0fh
	xra	d
	mov	e,a
	rrc
	rrc
	rrc
	mov	d,a
	ani	1fh
	xra	b
	mov	c,a
	mov	a,d
	ani	0e0h
	xra	e
	mov	b,a		;Update high order CRC word
	mov	a,d
	rrc
	ani	0f0h
	xra	c
	mov	c,a		;Save low order CRC word
	inx	h		;Bump pointer to next data byte
	pop	d		;Restore terminating pointer
	mov	a,d		;Get high order
	cmp	h		;Test for done CRECH
	jc	crec1		;Done - Test CRC and set flags
	jnz	crechx		;Continue - next data byte
	mov	a,e		;Get low order and
	cmp	l		;  test terminating pointer
	jnc	crechx		;Loop if more bytes to scan
crec1	xchg			;Swap CRC word pointer to D,E
	pop	h		;Restore data start address pointer
	ldax	d		;Test low order
	cmp	c		;Clear Z flag if no match
	inx	d		;Next byte
	rnz			;Done if error
	ldax	d		;High order
	cmp	b		;Set flags and
	ret			;Return

*
* Select selects the disk specified by drive A and
* loads the head
*
select	mov	b,a		;Save in B
	lda	cdisk		;Get currently selected disk
	cmp	b
	jz	loadhd
	mov	a,b		;Recover request
	sta	cdisk		;Update selected drive
	ana	a
	lda	sdisk		;Assume source
	lxi	d,mntsrc	;Mount source message
	jz	sselec		;Source select ?
	lda	ddisk		;Not source, destination
	lxi	d,mntdst	;Mount destination message
sselec	push	psw		;Save drive to select
	lda	sdisk		;Get source drive
	mov	b,a		;Save in B
	lda	ddisk		;Get destination drive
	cmp	b		;Same ?
	push	psw
	cz	unload
	pop	psw
	push	psw
	cz	pbuff
	pop	psw
	cz	conin		;Print string, and read console
	pop	psw		;Recover drive to select
selda	mov	c,a
	mvi	a,7fh		;Drive select bits
qloop	rlc			;Rotate select bits
	dcr	c		; to proper drive position
	jp	qloop
	ani	3fh
	call	stdvsl

loadhd	call	model
	lxi	b,lhsdena
	jz	selprep
	lxi	b,lhsdenb
selprep	mov	a,b
	call	stbits
	mov	a,c
	jmp	stbits

*
* Get index level
*
gtindx	call	model
	mvi	b,0
	rz
	mvi	b,index
	ret

*
* Get disk Jockey 2D model
*
model	lda	diskio-4
	cpi	(ret)
	ret
*
* Prompt the console for a drive, then read and verify the drive
* selected
*
readdrv	push	d		;Save prompt
	call	pbuff		;Send the prompt
	call	rbuff		;Read the console character
	pop	d		;Recover prompt
	jz	exit
	sui	'A'		;Subtract ASCII bias
	jc	readdrv		;Invalid input
	cpi	4		;Upper limit
	jp	readdrv		;Invalid input
	ret

*
* Pbuff is the CP/M print buffer function
*
pbuff	mvi	c,9
	jmp	bdos

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

*
* Rbuff is the CP/M read buffer function
*
prbuff	call	pbuff
rbuff	lxi	d,inbuff
	mvi	c,10
	call	bdos
	lda	inbuff+1
	ana	a
	rz
	lda	inbuff+2
	cpi	'a'
	rc
	cpi	'z'+1
	rnc
	sui	' '
	ret

inbuff	db	10,10
	ds	10

conin	mvi	c,1
	call	bdos
	ani	7fh
	cpi	3
	jz	finish
	ret

stdvsl	sta	diskio+1
	ret
stbits	sta	diskio+2
	ret
gtstat	lda	diskio+2
	ret

unload	call	model
	mvi	a,unloada
	jz	stbits
	mvi	a,unloadb
	jmp	stbits

*
* Messages and data
*
prompt	db	acr,alf
	db	'Format correction program, VERS '
	DB	(VERNUM/10)+'0','.',(VERNUM MOD 10)+'0'
	DB	acr,alf,'by Design Technology, San Diego, CA$'

srcmsg	db	acr,alf
	db	'Select source drive A,B,C, or D (RETURN to exit): $'

dstmsg	db	acr,alf
	db	'Select destination drive A,B,C or D (RETURN to exit): $'

mntmsg	db	acr,alf
	db	'Press RETURN to correct the format'
	db	acr,alf,'on the above specified diskette: $'

fmessg	db	acr,alf
	db	'Function Complete'
	db	acr,alf
	db	'Type R to reformat another, RETURN to exit: $'

wmessg	db	' is Write Protected.$'

rmessg	db	' is Not Ready.$'

amessg	db	acr,alf
	db	'Drive $'

sglside	db	' is not single sided.$'

mntsrc	db	acr,alf
	db	'Insert source diskette, then press RETURN: $'

mntdst	db	acr,alf
	db	'Insert destination diskette, then press RETURN: $'

tmessg	db	acr,alf
	db	'Track read error'
	db	acr,alf
	db	'Invalid CRC, Illegal Density, or'
	db	acr,alf
	db	'Illegal track or sector sequence.$'

retmsg	db	acr,alf
	db	'Error occured on Track: '
tdsply	db	'  .'
	db	acr,alf
	db	'Type R to reformat another, RETURN to exit: $'

vmsg	db	acr,alf
	db	'Diskette will not accept re-formatting.'
	db	acr,alf
	db	'Permanent Verification Error and'
	db	acr,alf
	db	'probable physical damage.$'

acralf	db	acr,alf,'$'

*
* Data space
*
sdisk	db	0ffh
ddisk	db	0ffh
cdisk	db	0ffh

trcnt	DS	1	;Track read retry counter
rwcnt	DS	1	;Re-write counter
tkno	DS	1	;Track number
secno	DS	1	;Sector number
secix	DS	2	;Sector data pointer table pointer
sectb	ds	2*26	;Space for sector address table
	ds	40
stack	equ	$
	org	(($+255)shr 8)shl 8	;Must start on page
tkbuf	ds	1800H	;Lots of space for track buffer
nbuf	equ	$-1	;Last byte in track buffer storage area

	end
