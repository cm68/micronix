*****************************************************************
*								*
* Boot program for HDDMA controller.				*
*								*
*****************************************************************

;
;	Define normal Cbios stuff
;

alf	equ	'J'-64		;A line feed
acr	equ	'M'-64		;A carraige return

maxhd	equ	1		;Number of hard disk drives
hdspt	equ	9		;Number of sectors per track
retries	equ	10		;Number of disk retries before error

bdos	equ	5		;Bdos entry point
wmess	equ	9		;Write a message

	org	0100h
	
buffer	jmp	start
	ds	1021		;Space for boot loader
start	lxi	h,0		;65536 retries
again	push	h
	call	hdreset		;Reset controller
	call	hdstat		;Check drive status
	pop	h
	ora	a
	jnz	ok		;The controller has responded
	lxi	d,nocon		;Missing controller message
	jmp	errpr
ok	ani	dready		;Check for drive ready
	jz	ready
	dcx	h		;Bump retry count
	mov	a,h
	ora	l
	jnz	again
	lxi	d,nodrv		;Drive does not respond
	jmp	errpr
ready	mvi	c,0		;Select drive 0
	call	hddrv
	call	hdhome		;Recalibrate
	mvi	c,1		;Sector 1
	call	hdsec
	lxi	b,buffer	;DMA address
	call	hddma
	mvi	c,retries	;Number of read retries
retry	push	b
	call	hdread
	pop	b
	jnc	0100h		;Jump to boot loader if no error
	dcr	c		;Bump retry count
	jnz	retry
	lxi	d,nosec

errpr	mvi	c,wmess		;Print string command
	call	bdos
	jmp	0		;Return to CP/M

nocon	db	acr, alf, 'No HDDMA controller present.', acr, alf, '$'
nodrv	db	acr, alf, 'Drive not ready.', acr, alf, '$'
nosec	db	acr, alf, 'Read error, track 0 sector 0.', acr, alf, '$'

*****************************************************************
*								*
* The follwing equates are for the HDDMA hard disk controller	*
*								*
*****************************************************************

cyl	equ	153		;Specifications for a Seagate Technology 506
heads	equ	4		;Number of heads per cylinder
precomp	equ	64		;Cylinder to start write precomensation
lowcurr	equ	128		;Cylinder to start low current
stepdly	equ	30		;Step delay (0-12.7 milliseconds)
headdly	equ	20		;Settle delay (0-255 milliseconds)
sectsiz	equ	7		;Sector size code (must be 3 for this Cbios)
				; 0 =  128 byte sectors
				; 1 =  256 byte sectors
				; 3 =  512 byte sectors (default)
				; 7 = 1024 byte sectors
				; f = 2048 byte sectors

				;Define controller commands
dmaread	equ	0		;Read sector
dmawrit	equ	1		;Write sector
dmarhed	equ	2		;Find a sector
dmawhed	equ	3		;Write headers (format a track)
dmalcon	equ	4		;Load disk parameters
dmassta	equ	5		;Sense disk drive status
dmanoop	equ	6		;Null controller operation

reset	equ	54h		;Reset controller
attn	equ	55h		;Send a controller attention

chan	equ	50h		;Default channel address
stepout	equ	10h		;Step direction out
stepin	equ	0		;Step direction in
band1	equ	40h		;No precomp, high current
band2	equ	0c0h		;Precomp, high current
band3	equ	80h		;precomp, low current
track0	equ	1		;Track zero status
wfault	equ	2		;Write fault from drive
dready	equ	4		;Drive ready
sekcmp	equ	8		;Seek complete

*****************************************************************
*								*
* Thå followinç arå thå lowesô leveì driveró foò thå HDDMÁ	*
* controller.							*
*								*
*****************************************************************

hddrv	lda	hddrive		;Get drive
	cmp	c		;Check to see if drive is already selected
	jnz	hddrv2
	ora	a		;Return carry reset if ok
hdside	ret

hddrv2	mov	a,c		;Get selected drive
	ani	3		;Mask for lower two bits
	mov	c,a
	sta	hddrive		;Save new selected drive
	sta	dmasel0		;Save in command channel drive select
hdsel	mvi	a,dmanoop
	sta	dmaop		;Set up operation code
	jmp	hdissue		;Execute disk command

hdstat	mvi	a,dmassta	;Sense status operation code
	sta	dmaop
	jmp	hdissue		;Execute disk command

hdhome	call	drvptr		;Get pointer to current cylinder number
	mvi	m,0ffh		;Fake at cyl 255 for max head travel
	mvi	c,0		;Seek to cylinder 0

hdtrk	call	drvptr		;Get track pointer
	mov	e,m		;Get old track number
	mov	m,c		;Store new track number
	mov	l,c		;Build cylinder word
	mvi	h,0
	shld	dmarg0		;Set command channel cylinder number
	mov	a,c		;Load new track address
	sub	e		;Subtract from old track
	rz			;Already on track
	mov	l,a		;Set up step count
	lda	hddrive		;Get drive number
	jnc	stepi		;Skip if stepping in
	ori	stepout		;Tack on step out bit
	push	psw		;Save for a while
	mov	a,l		;Complement track count
	cma
	inr	a		;Twos complement that is
	mov	l,a
	pop	psw
stepi	sta	dmasel0		;Load command channel, drive select
	shld	dmastep		;Load command channel, number of tracks to step
	mvi	a,dmanoop	;No-operation command for the channel
	sta	dmaop
	call	hdissue		;Step to proper track
	lxi	h,0		;Clear step counter
	shld	dmastep
	ret

hddma	mov	h,b		;Set DMA address
	mov	l,c
	shld	dmadma
	ret

hdsec	mov	a,c		;Load sector number
	dcr	a		;Range is actaully 0-16
	call	divspt		;Figure out head number -> (c)
	adi	hdspt		;Make sector number
	sta	hdsectr
	mov	a,c
	sta	hdhead		;Save head number
	ret

divspt	mvi	c,0		;Clear head counter
divsptx	sui	hdspt		;Subtract a tracks worth of sectors
	rc			;Return if all done
	inr	c		;Bump to next head
	jmp	divsptx

hdread	mvi	a,dmaread	;Load disk read command
	jmp	hdprep

hdwrite	mvi	a,dmawrit	;Load disk write command

hdprep	sta	dmaop		;Save command channel op code
	lda	dmarg0		;Get cylinder address
	mvi	c,band1		;Handle write precompensation and
	cpi	precomp		;   low current flags
	jc	hdpreps		;cylinder = (0 - precomp)
	mvi	c,band2
	cpi	lowcurr
	jc	hdpreps
	mvi	c,band3		;cylinder > low_current
hdpreps	lda	hdhead		;Load head address
	sta	dmarg2
	cma			;Negative logic for the controller
	ani	7		;3 bits of head select
	rlc			;Shove over to bits 2 - 4
	rlc
	ora	c		;Add on low current and precomp bits
	mov	c,a
	lda	hddrive		;Load drive address
	ora	c		;Slap in drive bits
	sta	dmasel1		;Save in command channel head select
	lda	hdsectr		;Load sector address
	sta	dmarg3
	jmp	hdissue		;Execute command

hdreset	out	reset		;Send reset pulse to controller
	lxi	h,dmachan	;Address of command channel
	shld	chan		;Default channel address
	xra	a
	sta	chan+2		;Clear extended address byte
	lxi	h,1000h		;Delay for reset complete
hdrl	dcx	h
	mov	a,h
	ora	l
	jnz	hdrl
	lxi	h,dmarg1	;Load arguments
	mvi	m,stepdly	;Load step delay
	inx	h
	mvi	m,headdly	;Head settle delay
	inx	h
	mvi	m,sectsiz	;Sector size code
	inx	h
	mvi	m,dmalcon	;Load constants command

hdissue	lxi	h,dmastat	;Status byte
	mvi	m,0
	out	attn		;Start controller
	lxi	d,0		;Time out counter
	mov	b,e		;Controller busy status
hdiloop	mov	a,m		;Get status
	ora	a		;Set up CPU flags
	rm			;Return no error (carry reset)
	stc
	rnz			;Return error status
	xthl			;Waste some ime
	xthl
	xthl
	xthl
	dcx	d		;Bump timeout counter
	mov	a,d
	ora	e
	jnz	hdiloop		;Loop if still busy
	stc			;Set error flag
	ret

drvptr	lhld	hddrive		;Get currently select drives track address
	xchg
	mvi	d,0
	lxi	h,drives
	dad	d		;Offset into track table
	ret

drives	equ	$		;Collection of track addresses
	rept	maxhd
	db	0ffh		;Initialize to (way out on the end of the disk)
	endm
	db	0ffh

hddrive	db	0ffh		;Currently selected drive
hdhead	db	0		;Currently selected head
hdsectr	db	0		;Currently selected sector

dmachan	equ	$		;Command channel area
dmasel0	db	0		;Drive select
dmastep	dw	0		;Relative step counter
dmasel1	db	0		;Head select
dmadma	dw	0		;DMA address
	db	0		;Extended address
dmarg0	db	0		;First argument
dmarg1	db	0		;Second argument
dmarg2	db	0		;Third argument
dmarg3	db	0		;Fourth argument
dmaop	db	0		;Operation code
dmastat	db	0		;Controller status byte
dmalnk	dw	dmachan		;Link address to next command channel
	db	0		;extended address

	end
