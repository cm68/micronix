*****************************************************************
*								*
*	 Morrow Designs CP/M vers 2.2 Cold Boot Loader.		*
*		  Cbios revision E.2,  3/4/82.			*
*								*
* The following routines will boot CP/M from the		*
* Disk Jockey 2D Rev. B 8 inch disk controller (DJ2D/B),	*
* Disk Jockey DMA 8 + 5 1/4 inch controller (DJDMA),		*
* Hard Disk Controller revision 3 (HDC3),			*
* or the Hard Disk DMA (HDDMA) disk controllers.		*
*								*
* Provisions have been made for a Micronix boot loader.		*
* This loader always gets loaded to 0100h.			*
*								*
*	     8 inch floppy disk boot loader for the		*
*            Morrow Designs Disk Jockey 2D/B (DJ2DB)		*
*								*
* The 'order' column is the interleave sequence used by the	*
* loader during the load.					*
*								*
*   track   sector    sysgen	load order	Name		*
*								*
*	0	 1	 900	ff00	 0	Boot loader	*
*	0	 2	 980			Unused		*
*	0	 3	 a00					*
*	0	 4	 a80					*
*	0	 5	 b00	9100	 1	CCP		*
*	0	 6	 b80	9180	12			*
*	0	 7	 c00	9200	 2			*
*	0	 8	 c80	9280	13			*
*	0	 9	 d00	9300	 3			*
*	0	10	 d80	9380	14			*
*	0	11	 e00	9400	 4			*
*	0	12	 e80	9480	15			*
*	0	13	 f00	9500	 5			*
*	0	14	 f80	9580	16			*
*	0	15	1000	9600	 6			*
*	0	16	1080	9680	17			*
*	0	17	1100	9700	 7			*
*	0	18	1180	9780	18			*
*	0	19	1200	9800	 8			*
*	0	20	1280	9880	19			*
*	0	21	1300	9900	 9	BDOS		*
*	0	22	1380	9980	20			*
*	0	23	1400	9a00	10			*
*	0	24	1480	9a80	21			*
*	0	25	1500	9b00	11			*
*	0	26	1580	9b80	22			*
*								*
* Track 1 is recorded in double density format.  There are	*
* 1024 bytes per sector.					*
*								*
*	1	 1	1600	9c00	 4			*
*	1	 2	1a00	a000	 1			*
*	1	 3	1e00	a400	 5	CBIOS (@ a700h)	*
*	1	 4	2200	a800	 2			*
*	1	 5	2600	ac00	 6			*
*	1	 6	2a00	b000	 3			*
*	1	 7	2e00	b400	 7			*
*	1	 8	3200	b800		Unused		*
*								*
* Three spare sectors (track 0, sectors 2 to 4) have been	*
* provided for a more advanced boot loader at a later date.	*
*								*
* The warm boot loader starts on track 0, sector 5 and		*
* continues through to track 1 sector 3.  Only the first	*
* 3/4 K bytes of track 1, sector 3 is loaded since CP/M		*
* requires that the warm boot loader load up to the start	*
* of (but not past) the Cbios jump table.			*
*								*
*								*
*	     8 inch floppy disk boot loader for the		*
*	     Morrow Designs Disk Jockey DMA (DJDMA)		*
*								*
* The loading is identical to that of of the DJ2DB except	*
* that the loader itself is loaded at 80h and the 'order'	*
* column does not apply.  The DJDMA is capable of loading a	*
* complete track at a time and thus it merely zaps the		*
* tracks in all at once.					*
*								*
*								*
*	   5 1/4 inch floppy disk boot loader for the		*
*	     Morrow Designs Disk Jockey DMA (DJDMA)		*
*								*
* The cold boot loader (track 0, sector 0) is loaded into	*
* RAM at 80h.  This loader will start loading from track 0,	*
* sector 1 and stops at track 1, sector 9.  The load		*
* sequence is as follows:					*
*								*
*   track   sector    sysgen	load order	Name		*
*	0	 0	 900	  80	 0	Cold boot	*
*	0	 1	 b00	9500	 1	CCP		*
*	0	 2	 d00	9700	 2			*
*	0	 3	 f00	9900	 3			*
*	0	 4	1100	9b00	 4			*
*	0	 5	1300	9d00	 5	BDOS		*
*	0	 6	1500	9f00	 6			*
*	0	 7	1700	a100	 7			*
*	0	 8	1900	a300	 8			*
*	0	 9	1b00	a500	 9			*
*								*
*	1	 0	1d00	a700	10			*
*	1	 1	1f00	a900	11			*
*	1	 2	2100	ab00	12	CBIOS		*
*	1	 3	2300	ad00	13			*
*	1	 4	2500	af00	14			*
*	1	 5	2700	b100	15			*
*	1	 6	2900	b300	16			*
*	1	 7	2b00	b500	17			*
*	1	 8	2d00	b700	18			*
*	1	 9	2f00	b900	19			*
*								*
* The warm boot starts from track 0 sector 1 and continues	*
* through to track 1 sector 1.					*
*								*
*								*
*	Shugart SA4000 disk interface boot loader for the	*
*	Morrow Designs Hard Disk Controller rev. 3 (HDC3)	*
*								*
* The cold boot loader (track 0, sector 1) is loaded into	*
* RAM in the very last part of the Cbios.  This area is		*
* used for uninitialized tables and thus is a safe place	*
* for the loader.  This cold boot loader will start loading	*
* the CCP from track 0, sector 2 and will finish up with	*
* the last part of the Cbios on track 0, sector 20.		*
*								*
*   track   sector    sysgen	load order	Name		*
*	0	 1	 900	fc00	 1	Cold boot	*
*	0	 2	 b00	9500	 2	CCP		*
*	0	 3	 d00	9700	 3			*
*	0	 4	 f00	9900	 4			*
*	0	 5	1100	9b00	 5			*
*	0	 6	1300	9d00	 6	BDOS		*
*	0	 7	1500	9f00	 7			*
*	0	 8	1700	a100	 8			*
*	0	 9	1900	a300	 9			*
*	0	10	1b00	a500	10			*
*	0	11	1d00	a700	11			*
*	0	12	1f00	a900	12			*
*	0	13	2100	ab00	13	CBIOS		*
*	0	14	2300	ad00	14			*
*	0	15	2500	af00	15			*
*	0	16	2700	b100	16			*
*	0	17	2900	b300	17			*
*	0	18	2b00	b500	18			*
*	0	19	2d00	b700	19			*
*	0	20	2f00	b900	20			*
*	0	21	3100			Unused		*
*								*
* The warm boot load sequence starts at track 0, sector 2	*
* and goes straight through to sector 12.  There is still	*
* plenty of room left in this loader for more advanced		*
* things like sector interleaving although this is hardly	*
* necessary on a hard disk.					*
*								*
*								*
*	Shugart SA1000 disk interface boot loader for the	*
*	 Morrow Designs Hard Disk DMA controller (HDDMA)	*
*								*
*   track   sector    sysgen	load order	Name		*
*	0	 1	 900	 100	 0	Cold boot + CCP	*
*	0	 2	 d00	9300	 1			*
*	0	 3	1100	9700	 2			*
*	0	 4	1500	9b00	 3	Bdos (@ 9d00)	*
*	0	 5	1900	9f00	 4			*
*	0	 6	1d00	a300	 5			*
*	0	 7	2100	a700	 6	Cbios		*
*	0	 8	2500	ab00	 7			*
*	0	 9	2900	af00	 8			*
*								*
*	1	10	2d00	b300	 9			*
*								*
* Since 1k byte sectors were implemented on this disk;  	*
* track 0, sector 1 contains both the cold boot loader and	*
* part of the CCP.  The cold boot loader relocates this		*
* peice of the CCP to it proper resting place as part of	*
* the boot process.						*
*								*
*****************************************************************

*****************************************************************
*								*
* The following table gives a general idea as to where the	*
* various parts of of the operating system are in memory.	*
*								*
* The only changes to the map that I see in the future is	*
* the increasing the space for the uninitialized tables		*
* following the Cbios.  The amount of code and table space	*
* that can actually be loaded from the disk is fixed by the	*
* amount of space available on the system tracks.		*
*								*
* Our most restrictive (smallest) drive is the 5 1/4 inch	*
* 'minnie floppy'.  This drive has 20 512 byte sectors for	*
* a total of 10k bytes on the system tracks.  The 8 inch	*
* floppy disk drive is also very close to being filled up.	*
*								*
* Since 512 bytes are reserved for the cold boot loader we	*
* have a total of 9.5k bytes for the operating system.  Out	*
* of this 5.5k bytes are used by the (CCP + BDOS) leaving	*
* us with a total of 4k bytes of loaded code and data space	*
* to play with.  Right now we are using all of this space	*
* so any major additions will have to result in a little	*
* (lot?) of code shuffeling or in the creation of a Cbios	*
* that simply will not fit on a small disk drive.		*
*								*
*	sysgen	48k	56k	60k	62k	64k		*
*	 image	CP/M	CP/M	CP/M	CP/M	CP/M		*
*								*
*	 900	~~~~	~~~~	~~~~	~~~~	~~~~	Loader	*
*	 b00	9500	b500	c500	cd00	d500	CCP	*
*	1300	9d00	bd00	cd00	d500	dd00	BDOS	*
*	2100	ab00	cb00	db00	e300	ed00	Cbios	*
*	3100	bb00	db00	eb00	f300	fb00	Tables	*
*	35ff	bfff	dfff	efff	f7ff	ffff	The End	*
*								*
*	~~~~	8d00	ad00	bd00	c500	cd00	DDT	*
*								*
*****************************************************************
	
micron	equ	0		;Set to 1 for Micronix boot loader

	if	micron eq 0

msize	equ	48		;Memory size of target CP/M
			
biosln	equ	1600h		;BIOS length
codlen	equ	11h		;Code length
	
ccpln	equ	800h
bdosln	equ	0e00h

size	equ	(msize*1024)	
ccp	equ	size-(biosln+ccpln+bdosln)
bdos	equ	ccp+ccpln
bios	equ	ccp+ccpln+bdosln

cboot	equ	bios		;Cold boot address for CP/M
loaddr	equ	ccp		;Load address for floppy

	else			;Micronix boot loader

cboot	equ	0100h		;Cold boot address for the loader
loaddr	equ	0100h
	
				;If the load address is moved forward from
				;0100 then the starting extended address for
				;the djdma boot loader should be adjusted.
	endif

retries	equ	10		;Maximum # of disk retries

*****************************************************************
*								*
* Only one of the following equates should be set.  The		*
* others sould be 0.  These equates define the boot loader	*
* that is to be used.						*
*								*
******************************************************************

maxhd	equ	0		;Set to boot an HDC3 controller
maxmw	equ	1		;Set to boot a HDDMA controller
maxfd	equ	0		;Set to boot a DJ2D/B controller
maxdm	equ	0		;Set to boot a DJDMA controller with 8 inch
maxmf	equ	0		;Set to boot a DJDMA controller with 5 1/4 inch


*****************************************************************
*								*
* The following equates are for the Hard Disk Controller 3.	*
*								*
*****************************************************************

	if	maxhd ne 0
hdorg	equ	50h		;Hard Disk Controller
hdstat	equ	hdorg		;Hard Disk Status
hdcntl	equ	hdorg		;Hard Disk Control
hddata	equ	hdorg+3		;Hard Disk Data
hdfunc	equ	hdorg+2		;Hard Disk Function
hdcmnd	equ	hdorg+1		;Hard Disk Command
hdreslt	equ	hdorg+1		;Hard Disk Result
retry	equ	2		;Retry bit of result
tkz	equ	1		;Track zero bit of status
opdone	equ	2		;Operaction done bit of status
complt	equ	4		;Complete bit of status
tmout	equ	8		;Time out bit of status
wfault	equ	10h		;Write fault bit of status
drvrdy	equ	20h		;Drive ready bit of status
indx	equ	40h		;Index bit of status
pstep	equ	4		;Step bit of function
nstep	equ	0fbh		;Step bit mask of function
hdrlen	equ	4		;Sector header length
secln	equ	512		;Sector data length
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
	endif

*****************************************************************
*								*
* The following equates are for the Hard Disk DMA.		*
*								*
*****************************************************************
	
	if	maxmw ne 0
cyl	equ	153		;Specifications for a Seagate Technology 506
heads	equ	4		;Number of heads per cylinder
spt	equ	9		;Sectors per track
precomp	equ	64		;Cylinder to start write precomensation
lowcurr	equ	128		;Cylinder to start low current
stepdly	equ	30		;Step delay (0-12.7 milliseconds)
headdly	equ	20		;Settle delay (0-255 milliseconds)
sectsiz	equ	7		;Sector size code (must be 3 for this Cbios)
				; 0 =  128 byte sectors
				; 1 =  256 byte sectors
				; 3 =  512 byte sectors
				; 7 = 1024 byte sectors (default for CP/M)
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
	endif

*****************************************************************
*								*
* The following equates are for the Disk Jockey 2D/B.		*	
*								*
*****************************************************************

	if	maxfd ne 0
origin	equ	0f800h		;Orgin of DJ 2D Mod B PROM
djram	equ	origin+400h
				;Disk Jockey 2D Mod B routines
tkzero	equ	origin+9h	;Track 0 seek
trkset	equ	origin+0ch	;Set track
setsec	equ	origin+0fH	;Set sector
setdma	equ	origin+12h	;Set DMA address
dread	equ	origin+15h	;Read sector
dmast	equ	origin+24h	;Get DMA address
status	equ	origin+27h	;Disk status
dskerr	equ	origin+2ah	;Flash error light
setden	equ	origin+2dh	;Set density
	endif

*****************************************************************
*								*
* The following equates are for the Disk Jockey DMA if wanted.	*
*								*
*****************************************************************

	if	(maxdm ne 0) or (maxmf ne 0)

djkick	equ	0efh		;Kick port for DJDMA controller
channl	equ	50h
	
	if	maxdm ne 0	;8 inch boot loader
trkoff	equ	22*128		;Number of bytes loaded from track 0
	else			;5 1/4 inch boot loader
trkoff	equ	9*512		;Number of bytes loaded from track 0
	endif

setdma	equ	23h		;Set DMA address
djhalt	equ	25h		;Halt controller
djbran	equ	26h		;Branch controller command
redtrk	equ	29h		;Read track command

	endif

*****************************************************************
*								*
* Define the origin address for the various boot loaders.	*
*								*
*****************************************************************

	if	maxhd ne 0	;HDC3
boot	equ	bios+biosln-512	;Very last part of CP/M system
	endif

	if	maxmw ne 0	;HDDMA
boot	equ	100h
	endif

	if	maxfd ne 0	;DJ2D/B
boot	equ	djram+300h	;Upper 3/4 of on board floppy RAM
	endif

	if	(maxdm ne 0) or (maxmf ne 0)	;DJDMA
boot	equ	80h
	endif

offset	equ	900h-boot	;DDT offset

	org	boot

*****************************************************************
*								*
* Cold Boot loader for a hard disk.				*
*								*
*****************************************************************

	if	(maxhd ne 0) or (maxmw ne 0)

	lxi	sp,cstkhd	;Set up stack at end of this sector

	if	maxhd ne 0
	lxi	b,19*100h+2	;B = sector count, C = sector #
	call	clodhd
	jmp	cboot		;Go to CP/M
	else
	lxi	h,boot+200h	;Copy part of CCP up
	lxi	d,loaddr
	lxi	b,200h
movlop:	mov	a,m		;Get a byte
	stax	d		;Save it
	inx	h		;Bump pointers
	inx	d
	dcx	b		;Bump counter
	mov	a,b		;Test for end
	ora	c
	jnz	movlop
	lxi	b,10*100h+2	;B = sector count, C = sector #
	call	clodhd
	jmp	cboot		;Go to CP/M
	endif

clodhd	push	b		;Save sector and count
	mov	a,c		;Load sector
	sta	hdsec
	lxi	h,loaddr-200h	;Get DMA address (self modifying)
cdmahd	equ	$-2		;Storage for previous DMA address
	if	maxhd ne 0
	lxi	d,200h		;Offset to new DMA address
	else
	lxi	d,400h
	endif
	dad	d		;Add in offset, HL = new DMA address
	shld	cdmahd		;Save new DMA address
	call	crdhd		;Attempt a read
	pop	b		;Recover sector number and count
				;	B = count, C = number
	dcr	b		;Update sector count
	rz			;All done ?
	inr	c		;Bump sector number
	jmp	clodhd		;Continue reading

*****************************************************************
*								*
* crdhd does the actual read from the controller, the DMA	*
* address and sector # have already been set up.		*
*								*
*****************************************************************

	if	maxhd ne 0	;Low level HDC3 drivers
crdhd	lxi	b,retries*100h+1 ;Maximum # of attempts
crhd	push	b		;Save error count
	call	hdread		;Attempt the read
	pop	b		;Restore the error count
	rnc			;Return if no error
	dcr	b		;Update error count
	jnz	crhd		;Try again if not too many errors
	jmp	$		;Dynamic error halt

hdread	call	hdprep		;Prepare the sector header image
	rc			;Error exit
	mvi	a,rsect		;Read sector command
	out	hdcmnd
	call	process		;Process the read
	rc			;Error exit
	xra	a		;Pointer to data buffer
	out	hdcmnd
	mvi	b,secln/4	;Number of bytes to read
	lhld	cdmahd		;Get destination of data
	in	hddata		;Two dummy data bytes
	in	hddata
rtloop	in	hddata		;Move four bytes
	mov	m,a		;Byte one
	inx	h
	in	hddata		;Byte two
	mov	m,a
	inx	h
	in	hddata		;Byte three
	mov	m,a
	inx	h
	in	hddata		;Byte four
	mov	m,a
	inx	h
	dcr	b		;Update byte count
	jnz	rtloop
	ret

process	in	hdstat		;Wait for command to finish
	mov	b,a
	ani	opdone
	jz	process
	mvi	a,dskclk	;Turn on Disk Clock
	out	hdcntl
	in	hdstat
	ani	tmout		;Timed out ?
	stc
	rnz
	in	hdreslt
	ani	retry		;Any retries ?
	stc
	rnz
	xra	a		;No error exit
	ret

hdprep	in	hdstat		;Is Drive ready ?
	ani	drvrdy
	stc
	rnz
	mvi	a,isbuff	;Initialize pointer to header buffer
	out	hdcmnd
	mvi	a,null
	out	hdfunc		;Select drive A
	xra	a
	out	hddata		;Form head byte
	out	hddata		;Form track byte
	mvi	a,0		;Form sector byte
hdsec	equ	$-1
	out	hddata
	mvi	a,80h		;Form Key
	out	hddata
	mvi	a,dskclk	;Turn on Disk clock
	out	hdcntl
	mvi	a,wenabl	;Write enable on
	out	hdcntl
	ret

	org	boot+200h-2	;Last word on sector is load address

cstkhd	equ	$
	dw	boot
	endif

	if	maxmw ne 0	;Low level HDDMA routines
crdhd	call	hdsetup		;Set up parameters
	lxi	b,retries*100h+1 ;Maximum # of attempts
crhd	push	b		;Save error count
	call	hdissue		;Attempt the read
	pop	b		;Restore the error count
	rnc			;Return if no error
	dcr	b		;Update error count
	jnz	crhd		;Try again if not too many errors
error	jmp	$		;Dynamic error halt

hdsetup	shld	dmadma		;Set up DMA address
	call	hdreset		;Reset controller
	lda	hdsec		;Set logical sector number
	dcr	a		;Range is actaully 0-16
	call	divspt		;Figure out head number -> (c)
	adi	spt		;Make real sector number
	sta	dmarg3
	mov	a,c
	sta	dmarg2		;Save head number
	cma			;Negative logic for the controller
	ani	7		;3 bits of head select
	rlc			;Shove over to bits 2 - 4
	rlc
	sta	dmasel1		;Save in command channel head select
	ret

divspt	mvi	c,0		;Clear head counter
divsptx	sui	spt		;Subtract a tracks worth of sectors
	rc			;Return if all done
	inr	c		;Bump to next head
	jmp	divsptx

hdreset	mvi	a,(ret)		;One time code
	sta	hdreset
	out	reset		;Send reset pulse to controller
	lxi	h,dmachan	;Address of command channel
	shld	chan		;Default channel address
	xra	a
	sta	chan+2		;Clear extended address byte
	xthl			;Wait for reset (around 10 uSEC's)
	xthl
	call	hdissue		;Do load constants
	jc	error		;Controller not present
	mvi	a,dmassta	;Sense status command
	sta	dmaop
rdychek	call	hdissue
	ani	dready		;Check for drive ready
	jnz	rdychek		;Loop if not ready
	lxi	h,0ffffh
	shld	dmastep		;Do recalibrate
	call	hdissue
	lxi	h,0
	shld	dmastep		;Clear step counter
	shld	dmarg0		;Clear cylinder #
	shld	dmarg3		;Clear sector # + read disk command
	ret

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
	xthl			;Waste some time
	xthl
	xthl
	xthl
	dcx	d		;Bump timeout counter
	mov	a,d
	ora	e
	jnz	hdiloop		;Loop if still busy
	stc			;Set error flag
	ret

hdsec	db	0		;Currently selected sector

dmachan	equ	$		;Command channel area
dmasel0	db	10h		;Drive select (step out, drive 0)
dmastep	dw	0		;Relative step counter
dmasel1	db	0		;Head select
dmadma	dw	0		;DMA address
	db	0		;Extended address
dmarg0	db	0		;First argument
dmarg1	db	stepdly		;Second argument (stepping time)
dmarg2	db	headdly		;Third argument (Settle time)
dmarg3	db	sectsiz		;Fourth argument (Sector size)
dmaop	db	dmalcon		;Operation code
dmastat	db	0		;Controller status byte
dmalnk	dw	dmachan		;Link address to next command channel
	db	0		;extended address

	org	boot+200h
cstkhd	equ	$		;Stack area at end of sector
	endif	
	endif

*****************************************************************
*								*
* Cold boot loader for the Disk Jockey 2D Revision B controller	*
*								*
*****************************************************************

	if	maxfd ne 0
t0boot	mvi	a,5-2		;First sector - 2
newsec	equ	$-1
	inr	a		;Update sector #
	inr	a
	cpi	27		;Size of track in sectors + 1
trksiz	equ	$-1
	jc	nowrap		;Skip if not at end of track
	jnz	t1boot		;Done with this track
exit	equ	$-2
	sui	27-6		;Back up to sector 6
backup	equ	$-1
	lxi	h,loaddr-80h	;Memory address of sector - 100h
nxtdma	equ	$-2
	shld	newdma
nowrap	sta	newsec		;Save the updated sector #
	mov	c,a
	call	setsec		;Set up the sector
	lxi	h,loaddr-100h	;Memory address of sector - 100h
newdma	equ	$-2
	lxi	d,100h		;Update DMA address
secsiz	equ	$-2
	dad	d
nowrp	shld	newdma		;Save the updated DMA address
	mov	b,h
	mov	c,l
	call	setdma		;Set up the new DMA address
	lxi	b,retries*100h+0;Maximum # of errors, track #
nxtrty	equ	$-2
fread	push	b
	call	trkset		;Set up the proper track
	call	dread		;Read the sector
	pop	b
	jnc	t0boot		;Continue if no error
	dcr	b
	jnz	fread		;Keep trying if error
	jmp	dskerr		;Too many errors, flash the light

t1boot	lxi	h,cboot		;We jump to cboot next time
	shld	exit
	mvi	c,1		;Select double density
	call	setden
	xra	a		;First sector - 2
	sta	newsec
	mvi	a,8		;Size of (logical) track + 1
	sta	trksiz
	dcr	a		;Number of sectors to back up
	sta	backup
	lxi	h,loaddr+0700h	;DMA start address for first revolution - 2048
	shld	newdma
	lxi	h,loaddr+0300h	;DMA start address for second revolution - 2048
	shld	nxtdma
	lxi	h,2048		;Difference between DMA addresses
	shld	secsiz
	lxi	h,retries*100h+1;Maximum # of errors, track #
	shld	nxtrty
	jmp	t0boot		;Go load in track 1
	endif

*****************************************************************
*								*
* Cold boot loader for the Disk Jockey DMA controller		*
*								*
*****************************************************************

	if	(maxdm ne 0) or (maxmf ne 0)	;Set up DJDMA loader

	mvi	a,djbran	;Load branch channel command
	sta	channl
	lxi	h,commnd	;Load new command channel address
	shld	channl+1
	xra	a
	sta	channl+3

djstrt:	out	djkick		;Start controller
djwait:	lda	djdone		;Get final status
	ora	a		;0 = still busy
	jz	djwait		;Loop if busy
	lxi	h,sectb0	;Check for bad load
	lxi	b,40ffh		;b = ok, c = loaded
	lxi	d,endtbl-sectb0	;Error count + # of sectors
djloop:	mov	a,m		;Load sector code
	cmp	c		;Check for 0ffh (already loaded)
	jz	djcont		;Skip if load was 'ok'
	mov	m,c		;Load 'loaded' flag
	cmp	b		;Check for 'ok' status
	jz	djcont		;Skip if load ok
	inr	m		;Make flag = 0
	inr	d		;Bump error counter
djcont:	dcr	e		;Bump sector count
	inx	h		;Bump table pointer
	jnz	djloop
	mov	a,d		;Check out error counter
	ora	a
	jz	cboot		;Start CP/M if no errors
	dcr	m		;Drop retry counter
	jnz	djstrt		;Retry load operation
	jmp	$		;Dynamic error halt

commnd:	db	setdma		;Set DMA address
	dw	loaddr-512	;Start at CCP
	if	micron eq 0
	db	0		;Extended address for CP/M
	else
	db	0ffh		;Wrap around from ffff00 to 000100
	endif

	db	redtrk		;Read track
	db	0		;Track 0
	db	0		;Side 0
	db	0		;Drive 0
	dw	sectb0		;Sector table 0
	db	0		;Extended address
	db	0		;Returned status

	db	setdma		;Set DMA address
	dw	loaddr+trkoff	;Load address for track 1
	db	0		;Extended address

	db	redtrk		;Read track
	db	1		;Track 1
	db	0		;Side 0
	db	0		;Drive 0
	dw	sectb1		;Sector table 1
	db	0		;Extended address
	db	0		;Returned status

	db	djhalt		;Halt controller
djdone:	db	0		;Returned status
	
	org	boot+5dh	;Boot + 5ch contains 'configuration byte'

	if	maxdm ne 0	;Booting from 8 inch drives
sectb0:	dw	0ffffh, 0ffffh	;Do not load boot loader
	dw	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	;22 sectors to be loaded

sectb1:	dw	0, 0, 0, 0ff00h	;First seven sectors

	db	retries		;Retry counter
	else			;Booting from 5 1/5 inch drives
sectb0:	dw	0ffh, 0, 0, 0, 0;Do not load boot loader
	
sectb1:	dw	0, 0, 0, 0, 0	;Load ten sectors

	db	retries		;Retry counter
	endif

endtbl	equ	$-1		;end of table marker

	endif

	end
