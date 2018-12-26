*****************************************************************
*								*
* Cbios for CP/M Ver 2.2 for Disk Jockey 2D controller (all	*
* revs). Handles diskettes with sector sizes of 128 bytes	*
* single density, 256, 512, 1024 bytes double density.		*
*								*
* Written by Bobby Dale Gifford.				*
* 9/1/79							*
*								*
* Disk Map of sectors used by Cold Boot, Warm Boot, Firmware,	*
* and CP/M:							*
*								*
* trk 0 sec  1 = First sector of cold boot.		e700h	*
*     0      2 = Cold boot 256.				  80h	*
*     0      3 = Cold boot 512.				  80h	*
*     0      4 = Cold boot 1024.			  80h	*
*     0      5 = Warm boot 256.				  80h	*
*     0      6 = Warm boot 512.				  80h	*
*     0      7 = Warm boot 1024.			  80h	*
*     0      8 = Cold/Warm boot.			3200h	*
*     0      9 = Firmware.				e400h	*
*     0     10 = Firmware+80h.				e480h	*
*     0     11 = Firmware+100h				e500h	*
*     0     12 = Firmware+180h.				e580h	*
*     0     13 = Firmware+200h.				e600h	*
*     0     14 = Firmware+280h.				e680h	*
*     0     15 = Firmware+300h.				e700h	*
*     0     16 = Firmware+380h.				e780h	*
*     0     17 = CCP.					2d00h	*
*     0     10 = CCP+80h.				2d80h	*
*     0     12 = CCP+100h.				2e00h	*
*     0     14 = CCP+180h.				2e80h	*
*     0     16 = CCP+200h.				2f00h	*
*     0     18 = CCP+280h.				2f80h	*
*     0     20 = CCP+300h.				3000h	*
*     0     22 = CCP+380h.				3080h	*
*     0     24 = CCP+400h.				3100h	*
*     0     26 = CCP+480h.				3180h	*
*     1        = Rest of CP/M.			  3200h-4fffh	*
*								*
*****************************************************************

	title	'*** Cbios For CP/M Ver. 2.2 ***'

*****************************************************************
*								*
* The following revision number is in reference to the CP/M	*
* 2.0 Cbios. 							*
*								*
*****************************************************************

revnum	equ	31		;Cbios revision number
cpmrev	equ	22		;CP/M revision number

*****************************************************************
*								*
* The following equates relate the Thinker Toys 2D controller.	*
* If the controller is non standard (0E000H) only the ORIGIN	*
* equate need be changed. This version of the Cbios will work	*
* with 2D controller boards rev 0, 1, 3, 3.1, 4.		*
*								*
*****************************************************************

origin	equ	0F800H
djram	equ	origin+400h	;Disk Jockey 2D RAM address
djcin	equ	djram+3h	;Disk Jockey 2D character input routine
djcout	equ	djram+6h	;Disk Jockey 2D character output routine
djhome	equ	djram+9h	;Disk Jockey 2D track zero seek
djtrk	equ	djram+0ch	;Disk Jockey 2D track seek routine
djsec	equ	djram+0fh	;Disk Jockey 2D set sector routine
djdma	equ	djram+012h	;Disk Jockey 2D set DMA address
djread	equ	djram+15h	;Disk Jockey 2D read routine
djwrite	equ	djram+18h	;Disk Jockey 2D write routine
djsel	equ	djram+1bh	;Disk Jockey 2D select drive routine
djtstat	equ	djram+21h	;Disk Jockey 2D terminal status routine
djstat	equ	djram+27h	;Disk Jockey 2D status routine
djerr	equ	djram+2ah	;Disk Jockey 2D error, flash led
djden	equ	djram+2dh	;Disk Jockey 2D set density routine
djside	equ	djram+30h	;Disk Jockey 2D set side routine

*****************************************************************
*								*
* CP/M system equates. If reconfiguration of the CP/M system	*
* is being done, the changes can be made to the following	*
* equates.							*
*								*
*****************************************************************

msize	equ	24		;Memory size of target CP/M
bias	equ	(msize-20)*1024	;Memory offset from 20k system
ccp	equ	2d00h+bias	;Console command processor
bdos	equ	ccp+800h	;BDOS address
bios	equ	ccp+1600h	;CBIOS address
cdisk	equ	4		;Address of last logged disk
buff	equ	80h		;Default buffer address
tpa	equ	100h		;Transient memory
intioby	equ	0		;Initial IOBYTE
iobyte	equ	3		;IOBYTE location
wbot	equ	0		;Warm boot jump address
entry	equ	5		;BDOS entry jump address

*****************************************************************
*								*
* The following are internal Cbios equates. Most are misc. 	*
* constants.							*
*								*
*****************************************************************

retries	equ	10		;Max retries on disk i/o before error
acr	equ	0dh		;A carriage return
alf	equ	0ah		;A line feed
aetx	equ	3		;A ETX char
aack	equ	6		;A ACK char
clear	equ	1ah		;Clear screen char on ADM3 terminal
maxdisk	equ	4		;Maximum # of disk drives
dblsid	equ	8		;Side bit from controller

*****************************************************************
*								*
* The jump table below must remain in the same order, the 	*
* routines may be changed, but the function executed must be	*
* the same.							*
*								*
*****************************************************************

	org	bios		;CBIOS starting address

	jmp	cboot		;Cold boot entry point
wboote	jmp	wboot		;Warm boot entry point
	jmp	const		;Console status routine
	jmp	conin		;Console input
cout	jmp	conout		;Console output
	jmp	list		;List device output
	jmp	punch		;Punch device output
	jmp	reader		;Reader device input
	jmp	home		;Home drive
	jmp	setdrv		;Select disk
	jmp	settrk		;Set track
	jmp	setsec		;Set sector
	jmp	setdma		;Set DMA address
	jmp	read		;Read the disk
	jmp	write		;Write the disk
	jmp	listst		;List device status
	jmp	sectran		;Sector translation
djdrv	jmp	djsel		;Hook for SINGLE.COM program

*****************************************************************
*								*
* Signon message output during cold boot.			*
*								*
*****************************************************************

prompt	db	acr,alf,alf
	db	'0'+msize/10		;CP/M memory size
	db	'0'+(msize mod 10)
	db	'K CP/M Vers. '		;CP/M version number
	db	cpmrev/10+'0'
	db	'.'
	db	(cpmrev mod 10)+'0'
	db	', Cbios rev '
	db	revnum/10+'0','.'	;Cbios revision number
	db	revnum mod 10+'0'
	db	acr,alf
	db	'For Thinker Toys Disk Jockey 2D Controller '
	db	'@ 0'

	if	origin/4096 > 10	;Controller origin (HEX)
	db	origin/4096+'A'-10
	else
	db	origin/4096+'0'
	endif

	if	(origin/256 and 0fh) > 10
	db	(origin/256 and 0fh)+'A'-10
	else
	db	(origin/256 and 0fh)+'0'
	endif
	db	'00H.'
	db	acr,alf,0

*****************************************************************
*								*
* Utility routine to output the message pointed at by H&L,	*
* terminated with a null.					*
*								*
*****************************************************************

message	mov	a,m		;Get a character of the message
	inx	h		;Bump text pointer
	ana	a		;Test for end
	rz			;Return if done
	push	h		;Save pointer to text
	mov	c,a		;Output character in C
	call	cout		;Output the character
	pop	h		;Restore the pointer
	jmp	message		;Continue until null reached

*****************************************************************
*								*
* Cboot is the cold boot loader. All of CP/M has been loaded in	*
* when control is passed here.					*
*								*
*****************************************************************

cboot	lxi	sp,tpa		;Set up stack
	call	tinit		;Initialize the terminal
	lxi	h,prompt	;Prep for sending signon message
	call	message		;Send the prompt
	xra	a		;Select disk A
	sta	cpmdrv
	sta	cdisk

*****************************************************************
*								*
* Gocpm is the entry point from cold boots, and warm boots. It	*
* initializes some of the locations in page 0, and sets up the	*
* initial DMA address (80h).					*
*								*
*****************************************************************

gocpm	lxi	h,buff		;Set up initial DMA address
	call	setdma
	mvi	a,(jmp)		;Initialize jump to warm boot
	sta	wbot
	sta	entry		;Initialize jump to BDOS
	lxi	h,wboote	;Address in warm boot jump
	shld	wbot+1
	lxi	h,bdos+6	;Address in BDOS jump
	shld	entry+1
	xra	a		;A <- 0
	sta	bufsec		;Disk Jockey buffer empty
	sta	bufwrtn		;Set buffer not dirty flag
	lda	cdisk		;Jump to CP/M with currently selected disk in C
	mov	c,a
	lxi	d,cmndbeg	;Beginning of initial command
	lxi	h,ccp+8		;Command buffer
	mvi	a,cmndend-cmndbeg+1 ;Length of command
	sta	ccp+7
	mov	b,a
	call	movlop
	lda	cwflg
	ana	a
	lda	autoflg
	jz	cldbot
	rar
cldbot	rar
	jc	ccp
	jmp	ccp+3		;Enter CP/M

cwflg	db	0		;Cold/warm boot flag

*****************************************************************
*								*
* The following byte determines if an initial command is to be	*
* given to CP/M on warm or cold boots. The value of the byte is	*
* used to give the command to CP/M:				*
*								*
* 0 = never give command.					*
* 1 = give command on cold boots only.				*
* 2 = give the command on warm boots only.			*
* 3 = give the command on warm and cold boots.			*
*								*
*****************************************************************

autoflg	db	1		;Auto command feature

*****************************************************************
*								*
* If there is a command inserted here, it will be given if the	*
* auto feature is enabled.					*
*	For Example:						*
*								*
*	cmndbeg	db	'MBASIC MYPROG'				*
*	cmndend	db	0					*
*								*
* will execute microsoft basic, and mbasic will execute the	*
* "MYPROG" basic program.					*
*								*
*****************************************************************

cmndbeg	db	''		;Initial command goes here
cmndend	db	0

*****************************************************************
*								*
* Wboot loads in all of CP/M except the CBIOS, then initializes	*
* system parameters as in cold boot. See the Cold Boot Loader	*
* listing for exactly what happens during warm and cold boots.	*
*								*
*****************************************************************

wboot	lxi	sp,tpa		;Set up stack pointer
	mvi	a,1
wflg	equ	$-1		;Test if beginning or
	ana	a		;	ending a warm boot
	mvi	a,1
	sta	wflg
	sta	cwflg		;Set cold/warm boot flag
	jz	gocpm		
	xra	a
	sta	wflg		
	mov	c,a
	call	djdrv		;Select drive A
	mvi	c,0		;Select single density
	call	djden
	mvi	c,0		;Select side 0
	call	djside
	mvi	a,15		;Initialize the sector to read
	sta	newsec
	lxi	h,ccp-100h	;And the DMA address
	shld	newdma
	call	warmlod		;Read in CP/M
	lxi	b,ccp+500h	;Load address for rest of warm boot
	call	djdma
	mvi	c,8
	call	djsec
	call	warmrd
	jmp	ccp+503h

warmlod	mvi	a,15		;Previous sector
newsec	equ	$-1
	inr	a		;Update the previous sector
	inr	a
	cpi	27		;Was it the last ?
	jc	nowrap
	sui	9		;Yes
	cpi	19
	rz
	lhld	newdma
	lxi	d,-480h
	dad	d
	shld	newdma
nowrap	sta	newsec		;Save the new sector to read
	mov	c,a
	call	djsec
	lxi	h,ccp-100h	;Get the previous DMA address
newdma	equ	$-2
	lxi	d,100h		;Update the DMA address
	dad	d
	shld	newdma		;Save the DMA address
	mov	b,h
	mov	c,l
	call	djdma		;Set the DMA address
	call	warmrd
	jmp	warmlod

warmrd	lxi	b,retries*100h+0;Maximum # of errors
wrmread	push	b
	call	djtrk		;Set the track
	call	djread		;Read the sector
	pop	b
	rnc			;Continue if successful
	dcr	b
	jnz	wrmread		;Keep trying
	jmp	djerr

*****************************************************************
*								*
* Setsec just saves the desired sector to seek to until an	*
* actual read or write is attempted.				*
*								*
*****************************************************************

setsec	mov	a,c		;Save the sector number
	sta	cpmsec		;CP/M sector #
	ret

*****************************************************************
*								*
* Setdma saves the DMA address for the data transfer.		*
*								*
*****************************************************************

setdma	mov	h,b		;hl <- bc
	mov	l,c
	shld	cpmdma		;CP/M dma address
	ret

*****************************************************************
*								*
* Home is translated into a seek to track zero.			*
*								*
*****************************************************************

home	mvi	c,0		;Track to seek to

*****************************************************************
*								*
* Settrk saves the track # to seek to. Nothing is done at this	*
* point, everything is deffered until a read or write.		*
*								*
*****************************************************************

settrk	mov	a,c		;A <- track #
	sta	cpmtrk		;CP/M track #
	ret

*****************************************************************
*								*
* Sectran translates a logical sector # into a physical sector	*
* #.								*
*								*
*****************************************************************

sectran	inx	b
	push	d		;Save table address
	push	b		;Save sector #
	call	getdpb		;Get DPB address into HL
	mov	a,m		;Get # of CP/M sectors/track
	ora	a		;Clear cary
	rar			;Divide by two
	sub	c
	push	psw		;Save adjusted sector
	jm	sidetwo
sidea	pop	psw		;Discard adjusted sector
	pop	b		;Restore sector requested
	pop	d		;Restor address of xlt table
sideone	xchg			;hl <- &(translation table)
	dad	b		;bc = offset into table
	mov	l,m		;hl <- physical sector
	mvi	h,0
	ret

sidetwo	lxi	b,15		;Offset to side bit
	dad	b
	mov	a,m
	ani	8		;Test for double sided
	jz	sidea		;Media is only single sided
	pop	psw		;Retrieve adjusted sector
	pop	b
	cma			;Make sector request positive
	inr	a
	mov	c,a		;Make new sector the requested sector
	pop	d
	call	sideone
	mvi	a,80h		;Side two bit
	ora	l		;	and sector
	mov	l,a
	ret

*****************************************************************
*								*
* Setdrv selects the next drive to be used in read/write	*
* operations. If the drive has never been selected before, a	*
* parameter table is created which correctly describes the 	*
* diskette currently in the drive. Diskettes can be of four	*
* different sector sizes:					*
*	1) 128 bytes single density.				*
*	2) 256 bytes double density.				*
*	3) 512 bytes double density.				*
*	4) 1024 bytes double density.				*
*								*
*****************************************************************

setdrv	mov	a,c		;Save the drive #
	sta	cpmdrv
	cpi	maxdisk		;Check for a valid drive #
	jnc	zret		;Illegal drive #
	mov	a,e		;Test if drive ever logged in before
	ani	1
	jnz	setdrv1		;Bit 0 of E = 0 -> Never selected before
	mvi	a,1		;Select sector 1 of track 1
	sta	truesec
	sta	cpmtrk
	call	fill		;Flush buffer and refill
	jc	zret		;Test for error return
	call	djstat		;Get status on current drive
	ani	0ch		;Strip off unwanted bits
	push	psw		;Used to select a DPB
	rar
	lxi	h,xlts		;Table of XLT addresses
	mov	e,a
	mvi	d,0
	dad	d
	push	h		;Save pointer to proper XLT
	call	getdpb		;Get DPH pointer into DE
	xchg			;
	pop	d
	mvi	b,2		;Number of bytes to move
	call	movlop		;Move the address of XLT
	lxi	d,8		;Offset to DPB pointer
	dad	d		;HL <- &DPH.DPB
	push	h
	lhld	origin+7	;Get address of DJ terminal out routine
	inx	h		;Bump to look at address of
				;	uart status location
	mov	a,m		
	xri	3		;Adjust for proper rev DJ
	mov	l,a
	mvi	h,(origin+300h)/100h
	mov	a,m
	ani	dblsid		;Check double sided bit
	lxi	d,dpb128s	;Base for single sided DPB's
	jnz	sideok
	lxi	d,dpb128d	;Base of double sided DPB's
sideok	xchg			;HL <- DBP base, DE <- &DPH.DPB
	pop	d		;Restore DE (pointer into DPH)
	pop	psw		;Offset to correct DPB
	ral
	ral
	mov	c,a
	mvi	b,0
	dad	b
	xchg			;Put DPB address in DPH
	mov	m,e
	inx	h
	mov	m,d
setdrv1	call	getdpb		;Get address of DPB in HL
	lxi	b,15		;Offset to sector size
	dad	b
	mov	a,m		;Get sector size
	ani	7h
	sta	secsiz
	mov	a,m
	rar
	rar
	rar
	rar
	ani	0fh
	sta	secpsec
	xchg			;HL <- DPH
	ret

zret	lxi	h,0		;Seldrv error exit
	ret

*****************************************************************
*								*
* Getdpb returns HL pointing to the DPB of the currently	*
* selected drive, DE pointing to DPH.				*
*								*
*****************************************************************

getdpb	lda	cpmdrv		;Get drive #
	mov	l,a		;Form offset
	mvi	h,0
	dad	h
	dad	h
	dad	h
	dad	h
	lxi	d,dpzero	;Base of DPH's 
	dad	d
	push	h		;Save address of DPH
	lxi	d,10		;Offset to DPB
	dad	d
	mov	a,m		;Get low byte of DPB address
	inx	h
	mov	h,m		;Get low byte of DPB
	mov	l,a
	pop	d
	ret

*****************************************************************
*								*
* Xlts is a table of address that point to each of the xlt	*
* tables for each sector size.					*
*								*
*****************************************************************

xlts	dw	xlt128		;Xlt for 128 byte sectors
	dw	xlt256		;Xlt for 256 byte sectors
	dw	xlt512		;Xlt for 512 byte sectors
	dw	xlt124		;Xlt for 1024 byte sectors

*****************************************************************
*								*
* Write routine moves data from memory into the buffer. If the	*
* desired CP/M sector is not contained in the disk buffer, the	*
* buffer is first flushed to the disk if it has ever been	*
* written into, then a read is performed into the buffer to get	*
* the desired sector. Once the correct sector is in memory, the	*
* buffer written indicator is set, so the buffer will be	*
* flushed, then the data is transferred into the buffer. 	*
*								*
*****************************************************************

write	mov	a,c		;Save write command type
	sta	writtyp
	mvi	a,1		;Set write command
	db	(mvi) or (b*8)	;This "mvi b" instruction  causes
				;	the following "xra a" to
				;	be skipped over.

*****************************************************************
*								*
* Read routine to buffer data from the disk. If the sector 	*
* requested from CP/M is in the buffer, then the data is simply	*
* transferreä froí thå buffeò tï thå desireä dma address. If	*
* the buffer does not contain the desired sector, the buffer is	*
* flushed to the disk if it has ever been written into, then	*
* filled with the sector from the disk that contains the	*
* desired CP/M sector.						*
*								*
*****************************************************************

read	xra	a		;Set the command type to read
	sta	rdwr		;Save command type

*****************************************************************
*								*
* Redwrt calculates the physical sector on the disk that	*
* contains the desired CP/M sector, then checks if it is the	*
* sector currently in the buffer. If no match is made, the	*
* buffer is flushed if necessary and the correct sector read	*
* from the disk.						*
*								*
*****************************************************************

redwrt	mvi	b,0		;The 0 is modified to contain the log2
secsiz	equ	$-1		;	of the physical sector size/128
				;	on the currently selected disk.
	lda	cpmsec		;Get the desired CP/M sector #
	push	psw		;Temporary save
	ani	80h		;Save only the side bit
	mov	c,a		;Remember the side
	pop	psw		;Get the sector back
	ani	7fh		;Forget the side bit
	dcr	a		;Temporary adjustment
divloop	dcr	b		;Update repeat count
	jz	divdone
	ora	a		;Clear the cary flag
	rar			;Divide the CP/M sector # by the size
				;	of the physical sectors
	jmp	divloop		;
divdone	inr	a
	ora	c		;Restore the side bit
	sta	truesec		;Save the physical sector number
	lxi	h,cpmdrv	;Pointer to desired drive,track, and sector
	lxi	d,bufdrv	;Pointer to buffer drive,track, and sector
	mvi	b,4		;Count loop
dtslop	dcr	b		;Test if done with compare
	jz	move		;Yes, match. Go move the data
	ldax	d		;Get a byte to compare
	cmp	m		;Test for match
	inx	h		;Bump pointers to next data item
	inx	d
	jz	dtslop		;Match, continue testing

*****************************************************************
*								*
* Drive, track, and sector don't match, flush the buffer if	*
* necessary and then refill.					*
*								*
*****************************************************************

	call	fill		;Fill the buffer with correct physical sector
	rc			;No good, return with error indication

*****************************************************************
*								*
* Move has been modified to cause either a transfer into or out	*
* the buffer.							*
*								*
*****************************************************************

move	lda	cpmsec		;Get the CP/M sector to transfer
	dcr	a		;Adjust to proper sector in buffer
	ani	0		;Strip off high ordered bits
secpsec	equ	$-1		;The 0 is modified to represent the # of
				;	CP/M sectors per physical sectors
	mov	l,a		;Put into HL
	mvi	h,0
	dad	h		;Form offset into buffer
	dad	h
	dad	h
	dad	h
	dad	h
	dad	h
	dad	h
	lxi	d,buffer	;Beginning address of buffer
	dad	d		;Form beginning address of sector to transfer
	xchg			;DE = address in buffer
	lxi	h,0		;Get DMA address, the 0 is modified to
				;	contain the DMA address
cpmdma	equ	$-2
	mvi	a,0		;The zero gets modified to contain
				;	a zero if a read, or a 1 if write
rdwr	equ	$-1
	ana	a		;Test which kind of operation
	jnz	into		;Transfer data into the buffer
outof	call	mover
	xra	a
	ret

into	xchg			;
	call	mover		;Move the data, HL = destination
				;	DE = source
	mvi	a,1
	sta	bufwrtn		;Set buffer written into flag
	mvi	a,0		;Check for directory write
writtyp	equ	$-1
	dcr	a
	mvi	a,0
	sta	writtyp		;Set no directory write
	rnz			;No error exit

*****************************************************************
*								*
* Flush writes the contents of the buffer out to the disk if	*
* it has ever been written into.				*
*								*
*****************************************************************

flush	mvi	a,0		;The 0 is modified to reflect if
				;	the buffer has been written into
bufwrtn	equ	$-1
	ana	a		;Test if written into
	rz			;Not written, all done
	lxi	h,djwrite	;Write operation

*****************************************************************
*								*
* Prep prepares to read/write the disk. Retries are attempted.	*
* Upon entry, H&L must contain the read or write operation	*
* address.							*
*								*
*****************************************************************

prep	xra	a		;Reset buffer written flag
	sta	bufwrtn
	shld	retryop		;Set up the read/write operation
	mvi	b,retries	;Maximum number of retries to attempt
retrylp	push	b		;Save the retry count
	lda	bufdrv		;Get drive number involved in the operation
	mov	c,a
	call	djdrv		;Select the drive
	lda	buftrk
	ana	a		;Test for track zero
	mov	c,a
	push	b
	cz	djhome		;Home the drive if track 0
	pop	b		;Restore track #
	call	djtrk		;Seek to proper track
	lda	bufsec		;Get sector involved in operation
	push	psw		;Save the sector #
	rlc			;Bit 0 of A equals side #
	ani	1		;Strip off unnecessary bits
	mov	c,a		;C <- side #
	call	djside		;Select the side
	pop	psw		;A <- sector #
	ani	7fh		;Strip off side bit
	mov	c,a		;C <- sector #
	call	djsec		;Set the sector to transfer
	lxi	b,buffer	;Set the DMA address
	call	djdma
	call	djread		;The read operation is modified to write
retryop	equ	$-2
	pop	b		;Restore the retry counter
	mvi	a,0		;No error exit status
	rnc			;Return no error
	dcr	b		;Update the retry counter
	stc			;Assume retry count expired
	mvi	a,0ffh		;Error return
	rz
	jmp	retrylp		;Try again

*****************************************************************
*								*
* Fill fills the buffer with a new sector from the disk.	*
*								*
*****************************************************************

fill	call	flush		;Flush buffer first
	rc			;Check for error
	lxi	d,cpmdrv	;Update the drive, track, and sector
	lxi	h,bufdrv	
	mvi	b,3		;Number of bytes to move
	call	movlop		;Copy the data
	lxi	h,djread
	jmp	prep		;Select drive, track, and sector.
				;	Then read the buffer

*****************************************************************
*								*
* Mover moves 128 bytes of data. Source pointer in DE, Dest	*
* pointer in HL.						*
*								*
*****************************************************************

mover	mvi	b,128		;Length of transfer
movlop	ldax	d		;Get a bte of source
	mov	m,a		;Move it
	inx	d		;Bump pointers
	inx	h
	dcr	b		;Update counter
	jnz	movlop		;Continue moving until done
	ret

*****************************************************************
*								*
* Terminal driver routines. Iobyte is initialized by the cold	*
* boot routine, to modify, change the "intioby" equate.	The	*
* I/O routines that follow all work exactly the same way. Using	*
* iobyte, they obtain the address to jump to in order to execute*
* the desired function. There is a table with four entries for	*
* each of the possible assignments for each device. To modify	*
* the I/O routines for a different I/O configuration, just	*
* change the entries in the tables.				*
*								*
*****************************************************************

citty	equ	djcin		;Input from the Disk Jockey 2D
cotty	equ	djcout		;Output to the Disk Jockey 2D

*****************************************************************
*								*
* const: get the status for the currently assigned console	*
*	 device. The console device can be gotten from iobyte,	*
*	 then a jump to the correct console status routine is	*
*	 performed.						*
*								*
*****************************************************************

const	lxi	h,cstble	;Beginning of jump table
	jmp	conin1		;Select correct jump

*****************************************************************
*								*
* csreader: if the console is assigned to the reader then a	*
*	    jump will be made here, where another jump will	*
*	    occur to the correct reader status.			*
*								*
*****************************************************************

csreadr	lxi	h,csrtble	;Beginning of reader status table
	jmp	readera

*****************************************************************
*								*
* conin: take the correct jump for the console input routine.	*
*	 The jump is based on the two least significant bits of	*
*	 iobyte.						*
*								*
*****************************************************************

conin	call	flush		;Flush the disk buffer
	lxi	h,citble	;Beginning of character input table

*
* Entry at conin1 will decode the two least significant bits
* of iobyte. This is used by conin,conout, and const.
*

conin1	lda	iobyte
	ral

*
* Entry at seldev will form an offset into the table pointed
* to by H&L and then pick up the address and jump there.
*

seldev	ani	6h		;Strip off unwanted bits
	mvi	d,0		;Form offset
	mov	e,a
	dad	d		;Add offset
	mov	a,m		;Pick up high byte
	inx	h
	mov	h,m		;Pick up low byte
	mov	l,a		;Form address
	pchl			;Go there !

*****************************************************************
*								*
* conout: take the proper branch address based on the two least	*
*	  significant bits of iobyte.				*
*								*
*****************************************************************

conout	push	b		;Save the character
	call	flush		;Flush the disk buffer
	pop	b		;Restore the character
	lxi	h,cotble	;Beginning of the character out table
	jmp	conin1		;Do the decode

*****************************************************************
*								*
* reader: select the correct reader device for input. The	*
*	  reader is selected from bits 2 and 3 of iobyte.	*
*								*
*****************************************************************

reader	lxi	h,rtble		;Beginning of reader input table

*
* Entry at readera will decode bits 2 & 3 of iobyte, used
* by csreader.
*

readera	lda	iobyte

*
* Entry at reader1 will shift the bits into position, used
* by list and punch.
*

readr1	rar
	jmp	seldev

*****************************************************************
*								*
* punch: select the correct punch device. The selection comes	*
*	 from bits 4&5 of iobyte.				*
*								*
*****************************************************************

punch	lxi	h,ptble		;Beginning of punch table
	lda	iobyte

*
* Entry at pnch1 rotates bits a little more in prep for
* seldev, used by list.
*

pnch1	rar
	rar
	jmp	readr1

*****************************************************************
*								*
* list: select a list device based on bits 6&7 of iobyte	*
*								*
*****************************************************************

list	lxi	h,ltble		;Beginning of the list device routines
list1	lda	iobyte
	rar
	rar
	jmp	pnch1

*****************************************************************
*								*
* Listst: Get the status of the currently assigned list device	*
*								*
*****************************************************************

listst	lxi	h,lstble	;Beginning of the list device status
	jmp	list1

*****************************************************************
*								*
* If customizing I/O routines is being performed, the table	*
* below should be modified to reflect the changes. All I/O	*
* devices are decoded out of iobyte and the jump is taken from	*
* the following tables.						*
*								*
*****************************************************************

*
* console input table
*

citble	dw	citty		;Input from tty (currently assigned 
				;	by intioby,input from 2d)
	dw	cicrt		;Input from crt (currently SWITCHBOARD 
				;	serial port 1)
	dw	reader		;Input from reader (depends on reader 
				;	selection)
	dw	ciuc1		;Input from user console 1 (currently 
				;	SWITCHBOARD serial port 1)

*
* console output table
*

cotble	dw	cotty		;Output to tty (currently assigned 
				;	by intioby,output to 2d)
	dw	cocrt		;Output to crt (currently SWITCHBOARD 
				;	serial port 1)
	dw	list		;Output to list device (depends on 
				;	bits 6&7 of iobyte)
	dw	couc1		;Output to user console 1 (currently 
				;	SWITCHBOARD serial port 1)

*
* list device table
*

ltble	dw	cotty		;Output to tty (currently assigned 
				;	by intioby,output to 2d)
	dw	cocrt		;Output to crt (currently SWITCHBOARD 
				;	serial port 1)
	dw	colpt		;Output to line printer (currently 
				;	SWITCHBOARD serial port 1)
	dw	coul1		;Output to user line printer 1 (currently 
				;	SWITCHBOARD serial port 1)

*
* punch device table
*

ptble	dw	cotty		;Output to the tty (currently assigned 
				;	by intioby,output to 2d)
	dw	coptp		;Output to paper tape punch (currently 
				;	SWITCHBOARD serial port 1)
	dw	coup1		;Output to user punch 1 (currently 
				;	SWITCHBOARD serial port 1)
	dw	coup2		;Output to user punch 2 (currntlly 
				;	SWITCHBOARD serial port 1)

*
* reader device input table
*

rtble	dw	citty		;Input from tty (currently assigned 
				;	by intioby, input from 2d)
	dw	ciptr		;Input from paper tape reader (currently 
				;	SWITCHBOARD serial port 1)
	dw	ciur1		;Input from user reader 1 (currently 
				;	SWITCHBOARD serial port 1)
	dw	ciur2		;Input from user reader 2 (currently 
				;	SWITCHBOARD serial port 1)

*
* console status table
*

cstble	dw	cstty		;Status of tty (currently assigned 
				;	by intioby, ststus from 2d)
	dw	cscrt		;Status from crt (currently SWITCHBOARD 
				;	serial port 1)
	dw	csreadr		;Status from reader (depends on reader device )
	dw	csuc1		;Status from user console 1 (currently 
				;	SWITCHBOARD serial port 1)

*
* status from reader device
*

csrtble	dw	cstty		;Status from tty (currently assigned 
				;	by intioby, status of 2d)
	dw	csptr		;Status from paper tape reader (currently 
				;	SWITCHBOARD serial port 1)
	dw	csur1		;Status from user reader 1 (currently 
				;	SWITCHBOARD serial port 1)
	dw	csur2		;Status of user reader 2 (currently 
				;	SWITCHBOARD serial port 1)

*
* Status from list device
*

lstble	dw	ready		;Console always ready
	dw	ready		;Get list status
	dw	lslpt
	dw	lslpt

*****************************************************************
*								*
* The following equates set output device to output to the	*
* SWITCHBOARD serial port 1.					*
*								*
*****************************************************************

cocrt	equ	$		;Output from crt
couc1	equ	$		;Output from user console 1
coptp	equ	$		;Output from paper tape punch
coup1	equ	$		;Output from user punch 1
coup2	equ	$		;Output from user punch 2
colpt	in	2		;Output from line printer,get status
	ani	80h		;Wait until ok to send
	jz	colpt
	mov	a,c		;output the character
	out	1
	ret

*****************************************************************
*								*
* Custom I/O printer driver for Diablo printer with 1200 baud	*
* ETX/ACK handshake.						*
*								*
*****************************************************************

coul1	call	colpt		;Output the character
	lda	count
	dcr	a
	sta	count
	rnz
	mvi	a,50
	sta	count
	mvi	c,aetx
	call	colpt
pwait	call	ciptr
	cpi	aack
	jnz	pwait
	ret

count	db	50

*****************************************************************
*								*
* The following equates set the input from the devices to come	*
* from the SWITCHBOARD serial port 1.				*
*								*
*****************************************************************

ciuc1	equ	$		;Input from user console 1
cicrt	equ	$		;Input from crt
ciur1	equ	$		;Input from user reader 1
ciur2	equ	$		;Input from user reader 2
ciptr	in	2		;Input from paper tape reader, get status
	ani	40h		;Wait for character
	jz	ciptr
	in	1
	ani	7fh		;Strip off the parity
	ret

*****************************************************************
*								*
* console status routines, test if a character has arrived.	*
*								*
*****************************************************************

cstty	call	djtstat		;Status from Disk Jockey 2D
stat	mvi	a,0		;Prep for zero return
	rnz			;Nothing found
	dcr	a		;Return with 0FFH
	ret

*****************************************************************
*								*
* The following equates cause the devices to get status from	*
* the SWITCHBOARD serial port 1.				*
*								*
*****************************************************************

csur1	equ	$		;Status of user reader 1
csur2	equ	$		;Status of user reader 2
csptr	equ	$		;Status of paper tape reader
csuc1	equ	$		;Status of user console 1
cscrt	in	2		;Status from crt, get status
	ani	40h		;Strip of data ready bit
	xri	40h		;Make correct polarity
	jmp	stat		;Return proper indication

*****************************************************************
*								*
* List device status routines.					*
*								*
*****************************************************************

lslpt	in	2		;All other devices wait
	ani	80h
	rz
ready	mvi	a,0ffh
	ret

*****************************************************************
*								*
* Tinit can be modified for different I/O setups.		*
*								*
*****************************************************************

tinit	mvi	c,clear		;Initialize the terminal routine
	mvi	a,intioby	;Initialize IOBYTE
	sta	iobyte
	jmp	cout

*****************************************************************
*								*
* Xlt tables (sector skew tables) for CP/M 2.0. These tables	*
* define the sector translation that occurs when mapping CP/M	*
* sectors to physical sectors on the disk. There is one skew	*
* table for each of the possible sector sizes. Currently the	*
* tables are located on track 0 sectors 6 and 8. They are	*
* loaded into memory in the Cbios ram by the cold boot routine.	*
*								*
*****************************************************************

xlt128	db	0
	db	1,7,13,19,25
	db	5,11,17,23
	db	3,9,15,21
	db	2,8,14,20,26
	db	6,12,18,24
	db	4,10,16,22

xlt256	db	0
	db	1,2,19,20,37,38
	db	3,4,21,22,39,40
	db	5,6,23,24,41,42
	db	7,8,25,26,43,44
	db	9,10,27,28,45,46
	db	11,12,29,30,47,48
	db	13,14,31,32,49,50
	db	15,16,33,34,51,52
	db	17,18,35,36

xlt512	db	0
	db	1,2,3,4,17,18,19,20
	db	33,34,35,36,49,50,51,52
	db	5,6,7,8,21,22,23,24
	db	37,38,39,40,53,54,55,56
	db	9,10,11,12,25,26,27,28
	db	41,42,43,44,57,58,59,60
	db	13,14,15,16,29,30,31,32
	db	45,46,47,48

xlt124	db	0
	db	1,2,3,4,5,6,7,8
	db	25,26,27,28,29,30,31,32
	db	49,50,51,52,53,54,55,56
	db	9,10,11,12,13,14,15,16
	db	33,34,35,36,37,38,39,40
	db	57,58,59,60,61,62,63,64
	db	17,18,19,20,21,22,23,24
	db	41,42,43,44,45,46,47,48

*****************************************************************
*								*
* Each of the following tables describes a diskette with the	*
* specified characteristics. The tables are currently stored	*
* on track 0 sector 13. They are read into memory by the GOCPM	*
* routine in the CBIOS for CP/M ver 2.0. 			*
*								*
*****************************************************************


*****************************************************************
*								*
* The following DPB defines a  diskette for 128 byte sectors,	*
* single density, and single sided.				*
*								*
*****************************************************************

dpb128s	dw	26		;CP/M sectors/track
	db	3		;BSH
	db	7		;BLM
	db	0		;EXM
	dw	242		;DSM
	dw	63		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	16		;CKS
	dw	2		;OFF
	db	1h		;16*((#cpm sectors/physical sector) -1) +
				;log2(#bytes per sector/128) + 1 +
				;8 if double sided.

*****************************************************************
*								*
* The following DPB defines a diskette for 256 byte sectors,	*
* double density, and single sided.				*
*								*
*****************************************************************

dpb256s	dw	52		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	242		;DSM
	dw	127		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	32		;CKS
	dw	2		;OFF
	db	12h		;16*((#cpm sectors/physical sector) -1) +
				;log2(#bytes per sector/128) + 1 +
				;8 if double sided.

*****************************************************************
*								*
* The following DPB defines a diskette as 512 byte sectors,	*
* double density, and single sided.				*
*								*
*****************************************************************

dpb512s	dw	60		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	280		;DSM
	dw	127		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	32		;CKS
	dw	2		;OFF
	db	33h		;16*((#cpm sectors/physical sector) -1) +
				;log2(#bytes per sector/128) + 1 +
				;8 if double sided.

*****************************************************************
*								*
* The following DPB defines a diskette as 1024 byte sectors,	*
* double density, and single sided.				*
*								*
*****************************************************************

dp1024s	dw	64		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	299		;DSM
	dw	127		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	32		;CKS
	dw	2		;OFF
	db	74h		;16*((#cpm sectors/physical sector) -1) +
				;log2(#bytes per sector/128) + 1 +
				;8 if double sided.

*****************************************************************
*								*
* The following DPB defines a diskette for 128 byte sectors,	*
* single density, and double sided.				*
*								*
*****************************************************************

dpb128d	dw	52		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	1		;EXM
	dw	242		;DSM
	dw	127		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	32		;CKS
	dw	2		;OFF
	db	9h

*****************************************************************
*								*
* The following DPB defines a diskette as 256 byte sectors,	*
* double density, and double sided.				*
*								*
*****************************************************************

dpb256d	dw	104		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	486		;DSM
	dw	255		;DRM
	db	0f0h		;AL0
	db	0		;AL1
	dw	64		;CKS
	dw	2		;OFF
	db	1ah

*****************************************************************
*								*
* The following DPB defines a diskette as 512 byte sectors,	*
* double density, and double sided.				*
*								*
*****************************************************************

dpb512d	dw	120		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	561		;DSM
	dw	255		;DRM
	db	0f0h		;AL0
	db	0		;AL1
	dw	64		;CKS
	dw	2		;OFF
	db	3bh

*****************************************************************
*								*
* The following DPB defines a diskette as 1024 byte sectors,	*
* double density, and double sided.				*
*								*
*****************************************************************

dp1024d	dw	128		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	599		;DSM
	dw	255		;DRM
	db	0f0h		;AL0
	db	0		;AL1
	dw	64		;CKS
	dw	2		;OFF
	db	7ch

*****************************************************************
*								*
* CP/M disk parameter headers, unitialized.			*
*								*
*****************************************************************

dpzero	dw	0		;Address of translation table (filled
				;	in by setdrv)
	dw	0,0,0		;Used by BDOS
	dw	dirbuf		;Address of directory buffer
	dw	0		;Address of DPB (filled in by setdrv)
	dw	csv0		;Directory check vector
	dw	alv0		;Allocation vector

dpone	dw	0
	dw	0,0,0
	dw	dirbuf
	dw	0
	dw	csv1
	dw	alv1

dptwo	dw	0
	dw	0,0,0
	dw	dirbuf
	dw	0
	dw	csv2
	dw	alv2

dpthre	dw	0
	dw	0,0,0
	dw	dirbuf
	dw	0
	dw	csv3
	dw	alv3

*****************************************************************
*								*
* Cbios ram locations that don't need initialization.		*
*								*
*****************************************************************

cpmsec	db	0		;CP/M sector #
cpmdrv	db	0		;CP/M drive #
cpmtrk	db	0		;CP/M track #
truesec	db	0		;Disk Jockey sector that contains CP/M sector
bufdrv	db	0		;Drive that buffer belongs to
buftrk	db	0		;Track that buffer belongs to
bufsec	db	0		;Sector that buffer belongs to
buffer	ds	1024		;Maximum size buffer for 1K sectors

alv0	ds	75		;Allocation vector for drive A
alv1	ds	75		;Allocation vector for drive B
alv2	ds	75		;Allocation vector for drive C
alv3	ds	75		;Allocation vector for drive D
csv0	ds	64		;Directory check vector for drive A
csv1	ds	64		;Directory check vector for drive B
csv2	ds	64		;Directory check vector for drive C
csv3	ds	64		;Directory check vector for drive D
dirbuf	ds	128		;Directory buffer

	end
Í¦Ò¦õ^ÍH¦ñö@Oyþ	ÂH¦ ÍH¦:¨æÂ–¦ÉÍ¬¦ Í³Ã³#ÍH¦ÍÉ¦:¨!¨¾Ð ÍH¦Ã¹¦ÍH¦
ÃH¦
þ$ÈÅOÍ¦ÁÃÓ¦:¨2¨*C¨N#å ÅåÍû¥æáÁþÊÁ§þ
Ê