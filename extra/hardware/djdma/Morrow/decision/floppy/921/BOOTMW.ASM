
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

mwspt	equ	9		;Number of sectors per track
retries	equ	10		;Number of disk retries before error

bdos	equ	5		;Bdos entry point
wmess	equ	9		;Write a message

	org	0100h
	
buffer:	jmp	start

	ds	1021		;Space for boot loader

start:	mvi	c,retries	;Set up retry count
again:	push	b
	xra	a		;Drive 0
	call	mwldrv		;Test controller and login drive
	pop	b
	ora	a
	jnz	ready		;The controller has responded
	dcr	c		;Bump retry count
	jnz	again
	lxi	d,bfail		;Missing controller message
	jmp	errpr

ready:	call	mwhome		;Track 0
	mvi	c,1		;Sector 1
	call	mwsec
	lxi	b,buffer	;DMA address
	call	mwdma
	mvi	c,retries	;Number of read retries
retry:	push	b
	call	mwread		;Read the sector
	pop	b
	jnc	buffer		;Jump to boot loader if no error
	dcr	c		;Bump retry count
	jnz	retry
	lxi	d,nosec		;Return sad news

errpr	mvi	c,wmess		;Print string command
	call	bdos
	jmp	0		;Return to CP/M

bfail:	db	acr, alf, 'HDDMA controller or drive failure.', acr, alf, '$'
nosec	db	acr, alf, 'Read error, track 0 sector 0.', acr, alf, '$'

*****************************************************************
*								*
* The follwing equates are for the HDDMA hard disk controller	*
*								*
*****************************************************************

				;Specifications for a hard disk
cyl	equ	1		;Number of cylinders
heads	equ	1		;Number of heads per cylinder
precomp	equ	1		;Cylinder to start write precomensation
lowcurr	equ	1		;Cylinder to start low current
stepdly	equ	30		;Step delay (0-12.7 milliseconds)
steprcl	equ	30		;Recalibrate step delay
headdly	equ	0		;Settle delay (0-25.5 milliseconds)

sectsiz	equ	7		;Sector size code (must be 7 for this Cbios)
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
wflt	equ	2		;Write fault from drive
dready	equ	4		;Drive ready
sekcmp	equ	8		;Seek complete

*****************************************************************
*								*
* The following are the lowest level drivers for the Morrow	*
* Designs Hard Disk DMA controller.				*
*								*
*****************************************************************

mwldrv:	sta	mwcurl		;Save current logical drive
	call	mwreset		;Reset controller card
	jc	zret		;Controller failure

	lda	mwcurl
	call	mwdrv		;Select drive
	jc	zret		;Select error

	call	mwstat		;Get drive status
	ani	dready		;Check if drive ready
	jnz	zret

	call	mwhome		;Home drive
	ret

zret:	xra	a		;Return error status
	ret

mwdrv:	sta	mwdrive		;Save new selected drive
mwsel:	mvi	a,dmanoop
	jmp	mwprep		;Execute disk command

mwstat:	mvi	a,dmassta	;Sense status operation code
	jmp	mwprep		;Execute disk command

mwhome:	call	mwreset		;Reset controller
	lxi	h,dmarg1	;Load arguments
	mvi	m,steprcl	;Load step delay (slow)
	inx	h
	mvi	m,headdly	;Head settle delay
	call	mwissue		;Do load constants again
	call	mwptr		;Get pointer to current cylinder number
	mvi	m,0ffh		;Fake at cyl 65535 for max head travel
	inx	h
	mvi	m,0ffh
	lxi	b,0		;Seek to cylinder 0
	call	mwseek		;Recal slowly
	call	mwreset		;Back to fast stepping mode
	ret

mwseek	call	mwptr		;Get track pointer
	mov	e,m		;Get old track number
	inx	h
	mov	d,m
	dcx	h
	mov	m,c		;Store new track number
	inx	h
	mov	m,b
	mov	l,c		;Build cylinder word
	mov	h,b
	shld	dmarg0		;Set command channel cylinder number
	mov	a,d
	inr	a
	lxi	h,0ffffh
	jnz	mwskip0
	mvi	c,stepout
	jmp	mwskip

mwskip0:mov	h,b		;(hl) = new track, (de) = old track
	mov	l,c
	call	mwhlmde
	mvi	c,stepout
	mov	a,h
	ani	80h		;Check hit bit for negitive direction
	jnz	mwsout		;Step in
	mvi	c,0
	jmp	mwskip
mwsout:	call	mwneghl
mwskip:	shld	dmastep
	lda	mwdrive
	ora	c
	sta	dmasel0

	mvi	a,dmanoop	;No-operation command for the channel
	call	mwprep		;Step to proper track
	lxi	h,0		;Clear step counter
	shld	dmastep
	ret

mwdma	mov	h,b		;Set DMA address
	mov	l,c
	shld	dmadma
	ret

mwsec	mov	a,c		;Load sector number
	dcr	a		;Range is actaully 0-16
	call	mwdspt		;Figure out head number -> (c)
	adi	mwspt		;Make sector number
	sta	mwsectr
	mov	a,c
	sta	mwhead		;Save head number
	ret

mwdspt	mvi	c,0		;Clear head counter
mwdsptx	sui	mwspt		;Subtract a tracks worth of sectors
	rc			;Return if all done
	inr	c		;Bump to next head
	jmp	mwdsptx

mwreset	out	reset		;Send reset pulse to controller
	lxi	h,dmachan	;Address of command channel
	shld	chan		;Default channel address
	xra	a
	sta	chan+2		;Clear extended address byte
	lhld	dmarg0		;Save the track number
	push	h
	lxi	h,dmarg1	;Load arguments
	mvi	m,stepdly	;Load step delay
	inx	h
	mvi	m,headdly	;Head settle delay
	inx	h
	mvi	m,sectsiz	;Sector size code
	inx	h
	mvi	m,dmalcon	;Load constants command
	call	mwissue		;Do load constants
	pop	h		;Restore the track number
	shld	dmarg0
	ret

mwread	mvi	a,dmaread	;Load disk read command

mwprep:	sta	dmaop		;Save command channel op code

	mvi	c,band1
	lhld	dmarg0
	lxi	d,precomp
	call	mwhlcde
	jc	mwpreps

	mvi	c,band2
	lxi	d,lowcurr
	call	mwhlcde
	jc	mwpreps

	mvi	c,band3		;cylinder > low_current
mwpreps	lda	mwhead		;Load head address
	sta	dmarg2
	cma			;Negative logic for the controller
	ani	7		;3 bits of head select
	rlc			;Shove over to bits 2 - 4
	rlc
	ora	c		;Add on low current and precomp bits
	mov	c,a
	lda	mwdrive		;Load drive address
	ora	c		;Slap in drive bits
	sta	dmasel1		;Save in command channel head select
	lda	mwsectr		;Load sector address
	sta	dmarg3

mwissue	lxi	h,dmastat	;Clear status byte
	mvi	m,0
	out	attn		;Start the controller
	lxi	d,0		;Time out counter (65536 retries)
mwiloop	mov	a,m		;Get status
	ora	a		;Set up CPU flags
	rm			;Return no error (carry reset)
	stc
	rnz			;Return error status
	xthl			;Waste some time
	xthl
	xthl
	xthl
	dcx	d		;Bump timeout counter
	mov	a,d
	ora	e
	jnz	mwiloop		;Loop if still busy
	stc			;Set error flag
	ret

mwptr	lda	mwdrive		;Get currently select drives track address
	rlc
	mov	e,a
	mvi	d,0
	lxi	h,mwtab
	dad	d		;Offset into track table
	ret

mwneghl:mov	a,h
	cma
	mov	h,a
	mov	a,l
	cma
	mov	l,a
	inx	h
	ret

mwhlmde:xchg
	call	mwneghl
	xchg
	dad	d
	ret

mwhlcde:mov	a,h
	cmp	d
	rnz
	mov	a,l
	cmp	e
	ret

mwtab:	dw	-1		;Collection of track addresses
	dw	-1		;Initialize to (way out on the end of the disk)
	dw	-1		
	dw	-1		

mwcurl	db	0		;Current logical drive
mwdrive	db	0ffh		;Currently selected drive
mwhead	db	0		;Currently selected head
mwsectr	db	0		;Currently selected sector

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
