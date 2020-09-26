;#####11-20-87   Warm boot portion (starting line 1932) mod by TRS
; to be usable with 2-sided 5-1/4" floppies.  

;#####CBIOS5F.A, modified by TRS 8-21-85 for M5 hard disk, boot from 5" floppy
;**********************************************************************
Title 'CBIOS-CP/M_2.2 Rev_E.4 07_Sep_83  Copyright 1983 Morrow Designs'
;**********************************************************************
;
;The following drivers are included in this CBIOS.
;-------------------------------------------------
;
; Console Devices:
;	CDRV0	Prom Patch (4 Jumps: conin, conout, const & conint)
;	CDRV1	Patch Area (128 bytes)
;	CDRV2	Multi I/O or Decision I driver
;	CDRV3	2D/B driver
;	CDRV4	DJDMA serial port
;	CDRV5	Switchboard serial port
;	CDRV6	North Star motherboard (2 serial + 1 parallel)
;
; List Devices:
;	LDRV0	Prom Patch (3 Jumps: lstout, lstost & lstset)
;	LDRV1	Patch Area (128 bytes)
;	LDRV2	Multio_Rev3-4 or Wunderbuss i/o Serial, no protocol
;	LDRV3	Multio_Rev3-4 or Wunderbuss i/o Serial, Clear To Send protocol
;	LDRV4	Multio_Rev3-4 or Wunderbuss i/o Serial, Data Set Ready protocol
;	LDRV5	Multio_Rev3-4 or Wunderbuss i/o Serial, Xon/Xoff protocol
;
; Disk systems:
;	DDRV1	DJDMA floppy disk controller with 8 and 5 1/4 inch disks.
;	DDRV2	DJ 2D/B floppy disk controller with 8 inch disks.
;	DDRV3	HDDMA 5, 10, 16, megabyte hard disk systems.
;	DDRV3	HDCA 10, 20 and 26 megabyte hard disks.
;
; Note:	Floppy systems diskette (drive A:) has to have 1024 byte
;	sectors in order for the cold and warm boot loaders to
;	work.  Be sure to format all new system diskettes with
;	1024 byte sectors.  The system diskette can be either
;	single or double sided.  The sector size on normal (non
;	A: drive) diskettes is not restricted.  Thus if you have
;	a diskette with software that is supposed to run on the
;	A: drive then you should mount the diskette in the B:
;	drive and then PIP it over to a 1024 byte sector
;	system diskette.
	page

;*****************************
;Begin User Configuration Area
;*****************************

absasm	equ	1		;set 0 for rmac, set 1 for mac
;
;General System Considerations
;=============================
;Memory Size
;-----------
;	1) The following equate sets the memory size in kilobytes. For
;	   example, 48 denotes a 48k system while 64 equals a 64k system.
;
msize	equ	64		;Memory size of target CP/M
biosln	equ	18h		;BIOS length.  Also in ABOOT&.ASM

;Non-Standard Flag
;-----------------
;	If this CBIOS is used with the CP/M 2.2 system that is shipped on
;	a Morrow Designs diskette then NOSTAND can be set to 1.  This
;	will allow the CBIOS to use various data areas found inside of
;	the CP/M 2.2 BDOS.  If the CBIOS is used with a different
;	operating system then NOSTAND should be set to 0.
;
nostand	equ	1		;Set to 1 for non-standard mode

;Define the console and printer environments
;===========================================
;	The following is a list of possible baud rates and the decimal
;	value needed for the cbaud and lbaud definitions
;
;	  Baud Rate	cbaud/lbaud	Baud Rate	cbaud/lbaud
;		 50	2304		     2000	58
;		 75	1536		     2400	48
;		110	1047		     3600	32
;		134.5	 857		     4800	24
;		150	 768		     7200	16
;		300	 384		     9600	12
;		600	 192		    19200	 6
;	       1200	  96		    38400	 3
;	       1800	  64		    56000	 2
;
;Define the console driver to be used.
;-------------------------------------
;	CONTYP is:	0	Nothing, used for patching to PROM's.
;			1	Provide for 128 bytes of patch space.
;			2	Multi I/O or Decision I driver.
;			3	2D/B driver.
;			4	DJDMA serial port
;			5	Switchboard serial port
;			6	North Star motherboard (2 serial + 1 parallel)
;
contyp	equ	2		;Console type
cbaud	equ	6		;Console Baud Rate

;Define the list driver to be used
;---------------------------------
;	LSTTYP is:	0	Nothing, used for patching to PROM's.
;			1	Provide for 128 bytes of patch space.
;			2	Multio/Wbio serial, no protocol.
;			3	Multio/Wbio serial, Clear To Send protocol.
;			4	Multio/Wbio serial, Data Set Ready protocol.
;			5	Multio/Wbio serial, Xon/Xoff protocol.
;
;	Note:	The Wunderbuss i/o board (Wbio) used in the Decision 1 is
;		functionally identical to the Multio.
;
lsttyp	equ	5		;List Device type
lbaud	equ	6		;List Device Baud Rate
	page

;Setup Disk System
;=================
;
;Select the Number of each type of disk drive
;--------------------------------------------
;	1) This following table tells the system the types and numbers of drives
;	   that are active.
;	2) Drives that are not present should be set to zero.
;	3) An example: If you have 2-8" drives using a DJDMA and one disk
;	   attached to an HDDMA controller you would set maxdm = 2 and
;	   maxmw = 1, with all other selections set to zero.
;
maxdm	equ	2		;DJDMA floppies (8")
maxmf	equ	2		;DJDMA floppies (5 1/4")
maxfd	equ	0		;DJ2D/B floppies (8" only)
maxmw	equ	1		;HDDMA hard disks
maxhd	equ	0		;HDCA hard disk drives

;Set the Logical Ordering of the drives
;--------------------------------------
;	1) You must assign an 'order number' for each drive type selected
;	   in the previous set of equates. Drive types that WERE NOT selected
;	   in the previous set of equates must be set to zero in this set.
;	2) Numbering must start with 1 and be continious.
;	3) An example: Suppose that your system consists of two DJDMA_8"
;	   drives and four DJDMA_1/4" drives along with one drive attached to
;	   an HDDMA controller; Furthermore, suppose that you set dmorder to
;	   3, mforder to 2 and mworder to 1. The HDDMA would be your A: drive.
;	   The 8" drives would be the B: and C: drives; And, finally, the D:,
;	   E:, F:, and G: drives would be assigned to the 5_1/4" DJDMA drives.
;
dmorder	equ	2		;DJDMA floppies (8")
mforder	equ	1		;DJDMA floppies (5 1/4")
fdorder	equ	0		;DJ2D/B floppies (8" only)
mworder	equ	3		;HDDMA hard disks
hdorder	equ	0		;HDCA hard disk drives

;HDDMA controller disk drives
;----------------------------
;	1) If the HDDMA controller has been selected then you must choose one
;	   (and only one) of the following drive types.
;
st506	equ	1		;Seagate ST-506
st412	equ	0		;Seagate ST-412
cm5619	equ	0		;CMI CM-5619

;HDCA controller disk drives
;---------------------------
;	1) If the HDCA controller has been selected then you must choose one
;	   (and only one) of the following drive types.
;
m10f	equ	0		;Fujitsu M2301B
m10m	equ	0		;Memorex
m20	equ	0		;Fujitsu M2302B
m26	equ	0		;Shugart SA4000

;DJDMA controller equates
;------------------------
;
mfslow	equ	0		;set true if slow stepping 5-1/4" floppy

;DJ2D/B controller equates
;-------------------------
;
fdorig	equ	0f800h		;Origin of DJ2D/B Disk Jockey PROM

;Misc Considerations
;-------------------
wmdrive	equ	0		;CP/M logical drive number to warm boot from.
mwquiet	equ	0		;Set for no names printed on login (HDDMA only)

	if	maxmw ne 0
badsiz	equ	32		;Number of badmap entries for HDDMA

	else			;(Only HDDMA drives use the bad map)
badsiz	equ	1		;Leave one entry as filler
	endif
	page

*****************************
;Begin Internal CBIOS equates
;****************************
;
;
;Revision Numbers
;----------------
;	1) The CBIOS revision number is output on a cold boot of the system.
;	2) The first part of the CBIOS revision number is converted to an
;	   alpha character while the the second part is simply output as a
;	   number. For example 54 becomes E4 and 27 becomes B7.

revnum	equ	54		;CBIOS revision number
cpmrev	equ	22		;CP/M revision number 2.2

;Debug Flag
;----------
;	The DEBUG flag merely causes various internal values and
;	addresses to be printed during the assembly process.  This
;	printing is forced via assembly errors and thus should not
;	affect the resulting code in any way.
;
debug	equ	0		;Set to 1 for debugging mode
	page

;General CP/M system equates.
;============================
;
;Location Definitions
;--------------------
wbot	equ	0		;Warm boot jump address
iobyte	equ	3		;IOBYTE location
cdisk	equ	4		;Address of last logged disk
entry	equ	5		;BDOS entry jump address
buff	equ	80h		;Default buffer address
tpa	equ	100h		;Transient memory

;Size Definitions
;----------------
ccpln	equ	800h
bdosln	equ	0e00h
size	equ	(msize*1024)
	if	absasm		;if mac
cbios	equ	size-(biosln*100h)
bios	equ	cbios
offsetc	equ	2100h-bios	;Offset for sysgen
	endif
;Sizes output for Debugging purposes
;-----------------------------------
	if	debug
dbgtmp	set	offsetc		;DDT offset	! <debug>
dbgtmp	set	ccp		;CCP address	! <debug>
dbgtmp	set	bdos		;BDOS address	! <debug>
dbgtmp	set	bios		;CBIOS address	! <debug>
	endif

;Misc. constants
;---------------
;
retries	equ	10		;Max retries on disk i/o before error
	page

;Internal Disk System Equates
;============================
;
;Hard Disk System Equates
;------------------------
;
m10	equ	m10f or m10m
fujitsu	equ	m20  or m10f
hdspt	equ	32*m26+21*m20+21*m10	;Sectors per track for HDCA
mwspt	equ	9			;Sectors per track for HDDMA
hdlog	equ	m10*2+m20*3+m26*3	;Logical disks per drive for HDCA
mwlog	equ	st506+st412*2++cm5619*2	;Logical disks per drive for HDDMA
maxlog	equ	(maxhd*hdlog)+(maxmw*mwlog)+maxfd+maxdm+maxmf

;Disk System Ordering Macros
;---------------------------
;	The following marco is used in generating the logical order of the
;	CP/M drives.
;
order	macro	num
	if	num eq hdorder
	dw	hddst
	endif

	if	num eq mworder
	dw	mwdst
	endif

	if	num eq fdorder
	dw	fddst
	endif

	if	num eq dmorder
	dw	dmdst
	endif

	if	num eq mforder
	dw	mfdst
	endif
	endm
	page

;***********************************
;Begin CBIOS Executable Code Section
;***********************************
;	This section consists of 3 routines
;		1) The Jump Table
;		2) Warm Boot routine
;		3) Go CPM (executed directly after every warm/cold boot)
;
;CBIOS Jump Table
;----------------
;	The jump table below must remain in the same order, the routines
;	may be changed, but the function executed must be the same.
;
	if	absasm		;mac stuff
	org	bios		;Cbios starting address

	else			;rmac stuff
	cseg
	public	codend,savln
	public	cbios,ccp,bdos,bioslen
	public	cbios,cboot,wboot,const,conin,conout,lstout
	public	home,setdrv,settrk,setsec,setdma,read,write
	public	lstost,sectran
biosln	equ	-1		;suppress errors
	endif
cbios:				;label for refrencing base of cbios
bdos	equ	cbios-bdosln
ccp	equ	cbios-(bdosln+ccpln)
	if	nostand ne 0
cblock	equ	cbios-19h	;Current actual block# * blkmsk
	endif			;Used for unallocated writting

;Begin Normal CPM BIOS Jump Table
;--------------------------------
	jmp	cboot		;Cold boot entry point
wboote:	jmp	wboot		;Warm boot entry point

	jmp	const		;Console status routine
cin:	jmp	conin		;Console input
cout:	jmp	conout		;Console output
pout:	jmp	lstout		;List device output

	if	(lsttyp ge 2) AND (lsttyp le 5)
	;has multio or wbio
	jmp	punout		;Punch device output
	jmp	rdrin		;Reader device input
	if	not absasm	;if rmac
	public	punout,rdrin
	endif

	else
	;not multio/wbio
	jmp	cout		;user console out as punch
	jmp	cin		;use console in as reader
	endif

	jmp	home		;Home drive
	jmp	setdrv		;Select disk
	jmp	settrk		;Set track
	jmp	setsec		;Set sector
	jmp	setdma		;Set DMA address
	jmp	read		;Read the disk
	jmp	write		;Write the disk
	jmp	lstost		;List device status
	jmp	sectran		;Sector translation

;The following jumps are extended BIOS calls defined by Morrow Designs
;---------------------------------------------------------------------
	if	maxfd ne 0
	jmp	fdsel		;Hookup for SINGLE.COM program
	else
	jmp	donop
	endif
	jmp	0		;End of the jump table
	page

;Warm Boot
;=========
;	WBOOT loads in all of CP/M except the CBIOS, then initializes
;	system parameters as in cold boot. See the Cold Boot Loader
;	listing for exactly what happens during warm and cold boots.
;
wboot:	lxi	sp,tpa		;Set up stack pointer
	mvi	a,1
	sta	cwflg		;Set cold/warm boot flag

	mvi	h,wmdrive	;Move drive to warm boot off of into (h)
	mvi	l,d$wboot	;Peform warm boot operation
	call	jumper
	jnc	gocpm		;No error

	hlt			;Halt computer
	db	0

	jmp	wboot		;In case user restarts the computer

;Begin Executing CPM (GOCPM)
;===========================
;	Gocpm is the entry point from cold boots, and warm boots. It
;	initializes some of the locations in page 0, and sets up the
;	initial DMA address (80h).
;
gocpm:	lxi	b,buff		;Set up initial DMA address
	call	setdma
	mvi	a,(jmp)		;Initialize jump to warm boot
	sta	wbot
	sta	entry		;Initialize jump to BDOS
	lxi	h,wboote	;Set up low memory entry to CBIOS warm boot
	shld	wbot+1
	lxi	h,bdos+6	;Set up low memory entry to BDOS
	shld	entry+1
	xra	a		;A <- 0
	sta	bufsec		;Set buffer to unknown state
	sta	bufwrtn		;Set buffer not dirty flag
	sta	error		;Clear buffer error flag

	lda	cwflg		;Get cold/warm boot flag
	ora	a
	lxi	h,coldmes	;Pointer to initial cold command
	jz	cldcmnd
	lxi	h,warmes	;Pointer to initial warm command
cldcmnd:mov	e,m		;Do one level of indirection
	inx	h
	mov	d,m
	ldax	d		;Get command length
	inr	a		;Bump length to include length byte itself
	lxi	h,ccp+7		;Command buffer (includes length byte)
	mov	c,a		;Set up for block move
	mvi	b,0
	call	movbyt		;Move command to internal CCP buffer
	lda	cwflg		;Figure out whether or not to send message
	ora	a
	lda	autoflg
	jz	cldbot
	rar
cldbot:	rar
	lda	cdisk		;Jump to CP/M with currently selected disk in C
	mov	c,a
	jc	ccp		;Enter CP/M, send message
	jmp	ccp+3		;Enter CP/M, no message

cwflg:	db	0		;Cold/warm boot flag
	page

;***************
;Misc. Data Area
;***************
;	The following area is a hodge-podge of pointers, constants and
;	revision labels.
;
;Auto Start
;==========
;	The following byte determines if an initial command is to be
;	given to CP/M on warm or cold boots. The value of the byte is
;	used to give the command to CP/M:
;
;		0 = never give command.
;		1 = give command on cold boots only.
;		2 = give the command on warm boots only.
;		3 = give the command on warm and cold boots.
;
autost:	db	0		;Revision 0 structure
	db	100h - (low ($ - cbios))	;The rest of the page is used for this stuff

autoflg:db	0		;Auto command feature enable flag

coldmes:dw	coldcm		;Pointer to the cold start command
warmes:	dw	warmcm		;Pointer to the warm start command

;Define the Auto Start Command
;-----------------------------
;	If there is a command inserted here, it will be passed to the
;	CCP if the auto feature is enabled.  For Example:
;
;		coldcm:	db	coldend-coldcm
;			db	'MBASIC MYPROG'
;		coldend	equ	$
;
;	will execute Microsoft BASIC, and MBASIC will execute the
;	"MYPROG" BASIC program.  Note: The command line must be in
;	upper case for most commands.
;
coldcm:	db	coldend-coldcm		;Length of cold boot command
	db	''			;Cold boot command goes here
coldend	equ	$

warmcm:	db	warmend-warmcm		;Length of warm boot command
	db	''			;Warm boot command goes here
warmend	equ	$
	page

;CBIOS Configuration Data
;========================
;
;Pointer to the Drive configuration table.
;-----------------------------------------
;
drconf:	db	0		;Revision 0 structure
	db	32		;32 bytes long now

;Pointer to Device Specification Tables
;--------------------------------------
;	This macro generates a table of pointers to the start of each active
;	disk driver's dispatch table. The order of this table defines the
;	logical order of the CP/M drives.
;
dsttab	equ	$
dn	set	1
	rept	16
	order	%dn
dn	set	dn+1
	endm

;I/O configuration table.
;------------------------
;	At this CBIOS revision 11 bytes are defined for this table.
;	Several extensive changes are planned for the table.  Future
;	revision of the IOCONF table will have independant entries for
;	three serial ports and will be used by several character drivers.
;	Also the IOBYTE will be implemented for all the character
;	drivers.  I might even write an external program to edit this
;	table.
;
;	The first two bytes show the I/O configuration that the CBIOS was
;	assembled with.  These bytes are used by external software to
;	determine the configuration options that are available.
;
;	The next byte is the initial IOBYTE value.  This value is written
;	to location 3 on cold boots.  See the CP/M 2 alternation guide
;	for a description of the IOBYTE.
;
;	The next byte is to make sure that the group select byte on the
;	Mult I/O or Decsion I stays consistant throughout the Cbios.
;	Only the group bits themselves (bits 0 and 1) should be changed
;	as you output to the group port.  If you modify one of the other
;	bits (such as driver-enable) then you should modify the same bit
;	in this byte.  For example:
;
;		;Select console group
;	lda	group		;Get group byte
;	ori	congrp		;Select the console port
;	out	grpsel		;Select the group
;
;		;Modify a bit in the group byte
;	lda	group		;Get group byte
;	ori	bank		;Set the bank bit
;	sta	group		;Save new group setting
;	ori	group2		;Select second serial port
;	out	grpsel		;Select the desired group
;
;	Note: You should not set the group bits themselves in the
;	      group byte.
;
;	The following two words define the default baud rates for the
;	console and the list devices.  These words are provided so that
;	the user can easily modify them.
;
;	The next two bytes are ued to configure the hardware handshaking
;	protocall used by the serial list drivers with the Multio or
;	Wunderbuss I/O boards.  The first of these two bytes is a mask.
;	This mask is ANDed with the 8250's MODEM Status Register to strip
;	out the desired handshake lines.  Next the result of the ANDing
;	is XORed with the second of the two bytes.  This XORing allows
;	the handshake lines to be inverted.  Common byte values are
;	shown below.
;
;	The last byte in the revision one structure is the last character
;	that was recieved from the printer.  This byte is used to
;	implement Xon/Xoff software handshaking.  This handshaking
;	protocol should not bother printers that have not implemented
;	Xon/Xoff protocol so this driver is enabled all the time.
;
ioconf:	db	2		;Revision 2 structure
	db	11		;11 bytes long now

	db	contyp		;Console device driver number
	db	lsttyp		;List device drive number

iobyt:	db	00000000b	;Initial IOBYT value (All devices go to CON:)
group:	db	0		;Group byte
	if	not absasm	;if rmac
	public	group
	endif

defcon:	dw	cbaud		;Console baud rate divisor value
deflst:	dw	lbaud		;Printer baud rate divisor value

	;Clear To Send protocol
	if	lsttyp eq 3
lstand:	db	cts		;Serial list handshake mask
lstxor:	db	0		;Serial list inversion flag
	endif

	;Data Set Ready protocol
	if	lsttyp eq 4
lstand:	db	dsr		;Serial list handshake mask
lstxor:	db	0		;Serial list inversion flag
	endif

	;Xon/Xofæ protocol
	if	(lsttyp ne 3) and (lsttyp ne 4)
lstand:	db	0		;Serial list handshake mask
lstxor:	db	0ffh		;Serial list inversion flag
	endif

lastch:	db	xon		;Last character recieved from the printer
	page

;Configuration Pointer Table
;===========================
;	At the first page boundry following the CBIOS we have a series of
;	pointers that point to various internal tables. At the start of
;	each of these tables we have a revision byte and a length byte.
;	The revision byte is the current revision number for that
;	particular structure and the length byte is the length of that
;	structure.  This length does not include the revision byte nor
;	the length byte itself.
;
;		Revision	Description
;		E.0		1 and 2 defined
;		E.3		This table is moved to a page boundry
;		E.3		0, 3 and 4 defined
;
;	The pointers defined so far are as follows:
;	-------------------------------------------
;		0) High byte is the page number of the CBIOS.  Low byte is
;		   the CBIOS revision number.  Used to determine pointer
;		   structure.
;		1) This points to the drive configuration table.
; 		2) This points to the I/O configuration bytes for the serial
;		   drivers.  Eg, the console, printer, reader, and punch
;       	   devices.
;		3) This points to the drive parameter table for DJDMA floppy
;       	   disk drives.  If no DJDMA is present then this pointer is
;		   null (0).
;		4) This points to the autostart command structures.  Used to
;		   automatically invoke a command on cold or warm boot
;		5) This will be a null (0) pointer.  It marks the end of
;		   the table.
;
	if	($ - cbios) gt 256	;Test for code overlap
	'Fatal error, pointer table placement.'
	else
	ds	100h - (low ($ - cbios))  ;Start at a page boundry
	endif
	;bpage is filled-in at run-time by init
bpage:	db	0		;CBIOS page number

	db	revnum		;Cbios revision number
	dw	drconf		;Drive configuration table pointer
	dw	ioconf		;I/O configuration table pointer
	if	(maxdm ne 0) or (maxmf ne 0)
	dw	dmarap		;Drive parameter table pointer for DJDMA
	else
	dw	0
	endif
	dw	autost		;Auto command structure pointer
	dw	0		;End of table marker
	page

;************************
;Begin BIOS Disk Routines
;************************
;
;Home the Disk (HOME)
;====================
;	Home is translated into a seek to track zero.
;
home:	lda	bufwrtn		;Test buffer dirty flag
	ora	a
	jnz	dohome		;Skip buffer disable if buffer dirty
	xra	a		;Invalidate buffer on home call
	sta	bufsec
dohome:	lxi	b,0		;Track to seek to
	call	settrk
	ret

;Select a Disk Dirve (SELDSK)
;============================
;	Setdrv selects the next drive to be used in read/write
;	operations.  If the drive has never been selected it calls
;	a low level drive select routine that should perform some
;	sort of check if the device is working.  If not working then
;	it should report an error.  If the logical drive has been
;	selected before then setdrv just returns the DPH without
;	checking the drive.
;
setdrv:	mov	a,c		;Save the logical drive number
	sta	cpmdrv
	cpi	maxlog		;Check for a valid drive number
	jnc	zret		;Illegal drive

	mov	a,e		;Check if bit 0 of (e) = 1
	ani	1
	jnz	setd3		;Drive has allready been accessed

	mov	h,c		;Move logical drive into (h)
	mvi	l,d$sel1
	call	jumper		;Call low level drive select
	mov	a,h		;Check if the low level drive select returned
	ora	l		;zero to indicate an error
	jz	zret		;Yes, an error so report to CP/M

	push	h		;Save DPH address
	call	gdph		;Get entry if DPH save table
	pop	d		;DPH -> (de)
	mov	m,e		;Put address of DPH in table
	inx	h
	mov	m,d
	inx	h
	mov	m,c		;Put sector size in table
	inx	h
	mov	a,m		;Check if bad map has ever been read for this
	ora	a		; drive
	cz	getbad		;Never been read so read in bad map
	xchg			;DPH -> (hl)

setd0:	mov	a,c		;Move sector size code into (a)
	sta	secsiz		;Save sector size
	xra	a
setd1:	dcr	c		;Create number of (128 bytes/physical sector)-1
	jz	setd2
	rlc
	ori	1
	jmp	setd1

setd2:	sta	secpsec		;Save for deblocking
	lda	cpmdrv		;Save current drive as old drive
	sta	lastdrv		; in case of select errors
	ret

setd3:	mov	h,c		;Drive in (h)
	mvi	l,d$sel2	;Select drive
	call	jumper
	call	gdph		;Quick select
	mov	e,m		;DPH -> (de)
	inx	h
	mov	d,m
	inx	h
	mov	c,m		;Sector size -> (c)
	xchg			;DPH -> (hl)
	jmp	setd0

;Return a pointer to the current drive's DPH
;-------------------------------------------
;	1) This routine is only called by SETDRV.
;	2) The drive number should be in location CPMDRV. The DE register
;	   pair is destroyed. The DPH pointer is returned in the HL
;	   register pair								;
;
gdph:	lda	cpmdrv		;Return pointer to DPH save area
	rlc			;Each entry is 4 bytes long
	rlc
	mov	e,a
	mvi	d,0
	lxi	h,dphtab	;DPH save area table
	dad	d		;Add offset
	ret			;(hl) = DPH save area for current drive

;Select a Track (SELTRK)
;=======================
;	Settrk saves the track # to seek to. Nothing is done at this
;	point, everything is deffered until a read or write.
;
settrk:	mov	h,b	;Enter with track number in (bc)
	mov	l,c
	shld	cpmtrk
	ret

;Select a Sector (SETSEC)
;========================
;	Setsec just saves the desired sector to seek to until an
;	actual read or write is attempted.
;
setsec:	mov	h,b		;Enter with sector number in (bc)
	mov	l,c
	shld	cpmsec
donop:	ret

;Set the DMA Address (SETDMA)
;============================
;	Setdma saves the DMA address for the data transfer.
;
setdma:	mov	h,b		;Enter with DMA address in (bc)
	mov	l,c
	shld	cpmdma		;CP/M dma address
	ret

;Read Data from a Disk (READ)
;============================
;	Read routine to buffer data from the disk. If the sector
;	requested from CP/M is in the buffer, then the data is simply
;	transferred from the buffer to the desired dma address. If
;	the buffer does not contain the desired sector, the buffer is
;	flushed to the disk if it has ever been written into, then
;	filled with the sector from the disk that contains the
;	desired CP/M sector.
;
read:	xra	a		;Set the command type to read
	if	nostand ne 0
	sta	unaloc		;Clear unallocated write flag
	endif
	call	rwent
	ret

;Write Data to a Disk (WRITE)
;============================
;	Write routine moves data from memory into the buffer. If the
;	desired CP/M sector is not contained in the disk buffer, the
;	buffer is first flushed to the disk if it has ever been
;	written into, then a read is performed into the buffer to get
;	the desired sector. Once the correct sector is in memory, the
;	buffer written indicator is set, so the buffer will be
;	flushed, then the data is transferred into the buffer.
;
write:	mov	a,c		;Save write command type
	sta	writtyp
	mvi	a,1		;Set write command
	call	rwent
	ret

;Read/Write to/from the disk
;---------------------------
;	Redwrt calculates the physical sector on the disk that
;	contains the desired CP/M sector, then checks if it is the
;	sector currently in the buffer. If no match is made, the
;	buffer is flushed if necessary and the correct sector read
;	from the disk.
;
	if	not absasm	;if rmac
	public	rwent,fill,flush,prep,jumper
	endif
rwent:	sta	rdwr		;Save command type
	mvi	b,0		;The 0 is modified to contain the log2
secsiz	equ	$-1		;	of the physical sector size/128
				;	on the currently selected disk
	lhld	cpmsec		;Get the desired CP/M sector #
	mov	a,h
	ani	80h		;Save only the side bit
	mov	c,a		;Remember the side
	mov	a,h
	ani	7fh		;Forget the side bit
	mov	h,a
	dcx	h		;Temporary adjustment

divloop:dcr	b		;Update repeat count
	jz	divdone
	ora	a
	mov	a,h
	rar
	mov	h,a
	mov	a,l
	rar			;Divide the CP/M sector # by the size
	mov	l,a		;	of the physical sectors
	jmp	divloop

divdone:inx	h
	mov	a,h
	ora	c		;Restore the side bit
	mov	h,a
	shld	truesec		;Save the physical sector number
	lxi	h,cpmdrv	;Pointer to desired drive,track, and sector
	lxi	d,bufdrv	;Pointer to buffer drive,track, and sector
	mvi	b,6		;Count loop
dtslop:	dcr	b		;Test if done with compare
	jz	move		;Yes, match. Go move the data
	ldax	d		;Get a byte to compare
	cmp	m		;Test for match
	inx	h		;Bump pointers to next data item
	inx	d
	jz	dtslop		;Match, continue testing

	;If Drive, track, and sector don't match, flush the buffer if
	;necessary and then refill.
	call	fill		;Fill the buffer with correct physical sector
	rc			;No good, return with error indication

	;Move has been modified to cause either a transfer into or out
	;the buffer.
move:	lda	cpmsec		;Get the CP/M sector to transfer
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
	lxi	d,buffer	;Starting address of buffer
	dad	d		;Form beginning address of sectgr to transfer
	xchg			;DE = address in buffer
	lxi	h,0		;Get DMA address, the 0 is modified t/
cpmdma	equ	$-2		;	contain the DMA address

	mvi	a,0		;The zero gets modified to contain
rdwr	equ	$-1		;	a zero if a read, or a 1 if write

	ana	a		;Test which kind of operation
	jnz	into		;Transfer data into the buffer
outof:	call	mov128
	lda	error		;Get the buffer error flag
	ret

into:	xchg			;
	call	mov128		;Move the data, HL = destination
	mvi	a,1		;	DE = source
	sta	bufwrtn		;Set buffer written into flag
	mvi	a,0		;Check for directory write
writtyp	equ	$-1

	dcr	a		;Test for a directory write (a=1=dir)
	mvi	a,0
	rnz			;	if not dir then exit (a=0=no-error)
	call	flush		;Flush the buffer if this is a dir oper
	ret			;(Accm is setup by the routine PREP)

;Perform Sector Translation (SECTRAN)
;====================================
;Sectran translates a logical sector number into a physical
;	sector number.
;
sectran:lda	cpmdrv		;Get the Drive Number
	mov	h,a		;Drive in (h)
	mvi	l,d$stran
	call	jumper		;See device level sector translation routines
	ret
	page

;Begin CBIOS Disk Routine Utilities
;==================================
;	These are general purpose routines that are used by by one or more
;	of the preceeding CBIOS Disk Routines and/or the Lo_Level drivers.
;
;Fill the Buffer with a new Sector
;---------------------------------
;	Fill fills the buffer with a new sector from the disk. If
;	were no errors then the carry is returned cleared else it is
;	set.
;
fill:	call	flush		;Flush buffer first
	rc			;(carry is set if there were any errors)

	lxi	d,cpmdrv	;Update the drive, track, and sector
	lxi	h,bufdrv
	lxi	b,5		;Number of bytes to move
	call	movbyt		;Copy the data
	lda	rdwr		;Test read write flag
	ora	a
	jz	fread		;Skip write type check if reading

	lda	writtyp		;0 = alloc, 1 = dir, 2 = unalloc

	if	nostand ne 0	;Do non standard (but quick and dirty) check
	ora	a		;(clears the carry)
	jnz	fnaloc		;Skip if not an allocated write

	lda	unaloc		;Check unallocated write in progress flag
	ora	a		;(clears the carry flag)
	jz	fwritin		;We are doing an allocated write
	lhld	cblock		;Get current block address
	xchg
	lhld	oblock		;   and old block address
	mov	a,d		;Compare old versus new
	cmp	h
	jnz	awritin		;Different, clear unallocated writting mode

	mov	a,e
	cmp	l
	jnz	awritin
	lxi	h,cpmdrv	;Test for different drive
	lda	unadrv
	cmp	m		;(reset the carry on equal)
	jnz	awritin		;Drive is different, clear unallocated mode
	ret			;Unallocated write (return with carry=clear)

fnaloc:	dcr	a		;(doesn't affect carry)
	jz	awritin		;Do a directory write
				;We are now doing an unallocated write
	lhld	cblock		;Save current block number
	shld	oblock
	lda	cpmdrv		;Save drive that this block belongs to
	sta	unadrv
	mvi	a,1		;Set unallocated write flag
	sta	unaloc		;   and we do nothing about the write
	ret			;(carry cleared by last ora)

awritin:xra	a		;Clear unallocated writting mode
	sta	unaloc

	else			;Do standard unallocated test
	sui	2		;Test for an unallocated write
	rz			;(carry will be cleared if zero result)

	endif
fwritin:lda	secsiz		;Check for 128 byte sectors
	dcr	a		;(doesn't affect the carry flag)
	rz			;No deblocking (carry cleared by last ora)

fread:	mvi	a,d$read
	sta	rwop
	call	prep		;Read the physical sector the buffer
	ret			;(carry and accm set by prep)

;Flush the Disk Buffer
;---------------------
;	Flush writes the contents of the buffer out to the disk if
;	it has ever been written into. If there are any errors then
;	the carry is returned set else it is cleared.
;
flush:	mvi	a,0		;The 0 is modified to reflect if
bufwrtn	equ	$-1		;	the buffer has been written into

	ora	a		;Test if written into
	rz			;Not written, all done (or clears the carry)

	mvi	a,d$write
	sta	rwop
	call	prep		;Do the physical write
	ret			;(carry and accm set by prep)

;Prepare the Disk for Reading and/or Writing
;-------------------------------------------
;	1) This is actually the place where disks are read/written (contrary
;	   to the name of this routine)
;	2) Prep prepares to read/write the disk. Retries are attempted.
;	   If there are any errors then the carry is returned
;	   set and the location ERROR is set to 0FFh, else the carry is
;	   returned cleared ERROR is reset to zero.
;
prep:	call	alt		;Check for alternate sectors
	di			;Reset interrupts
	xra	a		;Reset buffer written flag
	sta	bufwrtn
	mvi	b,retries	;Maximum number of retries to attempt

retrylp:push	b		;Save the retry count
	mvi	l,d$sel2	;Select drive
	call	jumpbuf

	lhld	alttrk		;Track number -> (hl)
	mov	b,h
	mov	c,l
	mvi	l,d$strk
	call	jumpbuf

	lhld	altsec		;Sector -> (hl)
	mov	b,h
	mov	c,l
	mvi	l,d$ssec
	call	jumpbuf

	lxi	b,buffer	;Set the DMA address
	mvi	l,d$sdma
	call	jumpbuf

	mvi	l,0		;Get operation address offset (8 or 9)
rwop	equ	$-1		;(set by FREAD [read=8] and FLUSH [write=9])

	call	jumpbuf		;Read or write to the disk

	pop	b		;Restore the retry counter
	mvi	a,0		;No error exit status
	jnc	prpret		;Return NO ERROR (accm=0, carry=clear)

	dcr	b		;Update the retry counter
	stc			;Assume retry count expired
	mvi	a,0ffh		;Error return
	jz	prpret		;Return ERROR (accm=ff, carry=set)

	mov	a,b
	cpi	retries/2
	jnz	retrylp		;Try again
	push	b		;Save retry count
	mvi	l,d$home	;Home drive after (retries/2) errors
	call	jumpbuf
	pop	b
	jmp	retrylp		;Try again

prpret:	sta	error		;save the error flag
	ret

;Access a lo-level driver subroutine
;-----------------------------------
;	Jumpbuf, jumper are used to dispatch to a low level device
;	subroutine.  Jumper is called with the drive in (h) and the
;	routine number (see description above) in (l).  It passes
;	along the (bc) and (de) registers unaltered.  Jumpbuf is
;	a call to jumper with the drive number from bufdrv.
;
	;Entry Point_1
jumpbuf:lda	bufdrv		;Dispatch with bufdrv for drive
	mov	h,a

	;Entry Point_2
jumper:	push	d
	push	b
	push	h
	mov	a,h		;Logical drive into (a)
	lxi	d,dsttab	;Drive specification pointer table
jumpl:	mov	c,a		;Save logical in (c)
	ldax	d
	mov	l,a
	inx	d
	ldax	d
	mov	h,a		;Get a DST pointer in (hl)
	inx	d
	mov	a,c		;Logical in (a)
	sub	m		;Subtract from first entry in DST
	jnc	jumpl		;Keep scanning table till correct driver found

	inx	h		;Bump (hl) to point to start of dispatch table
	pop	d		;Real (hl) -> (de)
	mov	a,e		;Move offset number into (a)
	rlc			;Each entry is 2 bytes
	mov	e,a		;Make an offset
	mvi	d,0
	dad	d		;(hl) = **Routine
	mov	a,m		;Pick up address of handler for selected
	inx	h		; function
	mov	h,m
	mov	l,a		;(hl) = *routine
	mov	a,c		;Logical in (a)
	pop	b		;Restore saved registers
	pop	d
	pchl

;Move Data
;---------
;	1) The entry point mov128 forces 128 bytes of data to be moved
;	   from source to destination.
;	2) The second entry point (movbyt) can move upto 65K of data.
;	3) The Source pointer is passed in the DE register pair.
;	4) The Destination pointer is passed in the HL register pair.
;
	;Entry Point_1
mov128:	lxi	b,128		;Length of transfer

	;Entry Point_2
movbyt:	xra	a		;Check if host processor is a Z80
	adi	3
	jpo	z80mov		;Yes, Its a Z80 so use block move

m8080:	ldax	d		;Get a byte of source
	mov	m,a		;Move it
	inx	d		;Bump pointers
	inx	h
	dcx	b		;Update counter
	mov	a,b		;Test for end
	ora	c
	jnz	m8080
	ret

z80mov: xchg			;Source in (hl), Destination in (de)
	dw	0b0edh		;ldir
	xchg
	ret

;Print a Message
;---------------
;Utility routine to output the message pointed at by (hl)
;terminated with a null.
;
message:mov	a,m		;Get a character of the message
	inx	h		;Bump text pointer
	ora	a		;Test for end
	rz			;Return if done
	push	h		;Save pointer to text
	mov	c,a		;Output character in C
	call	cout		;Output the character
	pop	h		;Restore the pointer
	jmp	message		;Continue until null reached

;Drive select error return
;-------------------------
;	1) This routine sets the HL pair to zero (the sel-drive error
;	return condition) and updates the value of CDISK. Notice that
;	this routine is called from both the high level select routine
;	(SETDRV) and from the lo-level routines as well (e.g. MFLDR1
;	in the DJDMA drivers). To stop infinite select error loops by the
;	CCP, cdisk is modified if it specifies the disk in error.
;
zret:	lxi	h,0		;Seldrv error exit
	lda	cdisk
	ani	15		;isolate 'ccp' curr disk (strip user num)
	mov	c,a
	lda	cpmdrv		;get curr selected drive
	cmp	c
	rnz			;exit if not 'ccp' select error
	lda	lastdrv		;Get last valid selected drive
	mov	c,a
	lda	cdisk		;Pick up user/drive
	ani	0f0h		;Save user number
	ora	c		;Put together with old valid drive
	sta	cdisk		;set new default disk for 'ccp'
	ret

;No bad Map
;----------
;	This routine is used by the lo-level drivers to indicate that
;	the selected device has no bad map.
;
nobad:	lxi	h,0		;Used by device drives to indicate no bad
	ret			; sector map

;Return DPH pointer
;------------------
;	Enter with (de) with DPH base address and (a) with logical
;	drive number.  Returns with DPH address in (hl).
;
retdph:	mov	l,a		;Move logical drive into (l)
	mvi	h,0
	dad	h		;Multiply by 16 (size of DPH)
	dad	h
	dad	h
	dad	h
	dad	d		;(hl) = pointer to DPH
	ret
	page
;CBIOS Bad Map Routines (only for HDDMA)
;=======================================
;
;null routines if no HDDMA
;-------------------------
;
	if	maxmw eq 0	;if no HDDMA
;
getbad:	ret			;no bad map to read from disk
;
alt:	lhld	buftrk		;No alternate sector so use selected sector
	shld	alttrk
	lhld	bufsec
	shld	altsec
	ret
;
	else			;have a HDDMA

;Check if a device has a bad map
;-------------------------------
;	1) This routine is only called by SETDRV
;	2) If the device has a bad sector map then append bad entries to end
;	   of badmap table.
;	3) This routine is only required for HDDMA.
;
getbad:	mvi	m,1		;Set drive initilized
	push	b
	push	d
	lda	cpmdrv		;Pick up current drive
	mov	h,a		;Call drive routine to return a pointer to
	mvi	l,d$bad		;the track and sector of the bad map
	call	jumper

	mov	a,h		;If routine returns 0 then the device has
	ora	l		; no bad sector map
	jz	badret

	mov	e,m		;Pick up track number of bad sector map -> (de)
	inx	h
	mov	d,m
	inx	h
	xchg
	shld	cpmtrk
	xchg
	mov	a,m		;Pick up sector number of of bad sector map
	inx	h
	mov	h,m
	mov	l,a
	shld	truesec
	call	fill		;Read in bad sector map into the buffer
	rc

	lhld	badptr		;Pick up bad map pointer
	lxi	d,buffer	;Start at beginning of buffer
badl:	ldax	d		;Pick up an entry from the buffer
	ora	a
	jz	bade		;All done
	mov	a,m		;Pick up entry from bad map table
	inr	a
	jz	overflo		;Bad map overflow
	lda	cpmdrv		;Put drive in table
	mov	m,a
	inx	h
	lxi	b,8
	call	movbyt		;Move the rest of information into the table
	jmp	badl

bade:	shld	badptr		;Restore new bad map pointer
badret:	pop	d
	pop	b
	ret

overflo:lxi	h,omes
	call	message
	jmp	badret

;Check for alternate sectors in bad sector table
;-----------------------------------------------
;	1) This routine is only called by PREP.
;	2) If an alternate sector is found replace alttrk and altsec with
;	   new sector number else pass along unaltered.
;
alt:	lxi	h,badmap	;Address of bad map -> (hl)
	lda	bufdrv		;Pick up drive number currently working on
	mov	c,a		;Move drive into (c) for speed in search
all:	xchg
	lhld	badptr		;Get bad map pointer
	xchg			; -> (de)
	mov	a,d		;Check if at end of bad map table
	cmp	h
	jnz	alt2		;Still more
	mov	a,e
	cmp	l
	jnz	alt2		;Still more
	lhld	buftrk		;No alternate sector so use selected sector
	shld	alttrk
	lhld	bufsec
	shld	altsec
	ret

alt2:	push	h		;Save current bad map entry address
	mov	a,c		;Move drive into (a)
	cmp	m		;Check if drive in table matches
	jnz	altmis		;Does not match skip this entry
	inx	h		;Point to LSB of alternate track
	lda	buftrk		;Pick up LSB of buffer track
	cmp	m
	jnz	altmis
	inx	h		;Point to MSB alternate track
	lda	buftrk+1	;Pick up MSB of buffer track
	cmp	m
	jnz	altmis
	inx	h		;Point to LSB of alternate sector
	lda	bufsec		;Pick up LSB of buffer sector
	cmp	m
	jnz	altmis
	inx	h		;Point to MSB of alternate sector
	lda	bufsec+1	;Pick up MSB of buffer sector
	cmp	m
	jnz	altmis		;Found an alternate sector
	inx	h		;Point to real info on the alternate sector
	lxi	d,alttrk
	xchg			;MOVLOP (de) = source, (hl) = dest
	push	b
	lxi	b,4
	call	movbyt		;Move alternate sector info in correct place
	pop	b
	pop	h
	ret

altmis:	pop	h		;Current alternate did not match
	lxi	d,9		;Bump pointer by the length of an entry
	dad	d
	jmp	all		;Loop for more

;Bad Map Routines Data Areas
;---------------------------
;
omes:	db	0dh, 0ah, 'BAD MAP OVERFLOW!', 0dh, 0ah, 0
badptr:	dw	badmap		;Pointer to next available bad map entry

	endif			;end of Bad Map Routines

	page
;CBIOS Disk Routines Data Area
;=============================
;
;DPH save area
;-------------
;	1) Each entry is 4 bytes long:
;		0 - LSB of DPH address
;		1 - MSB of DPH address
;		2 - Sector size code (1=128, 2=256, 3=512, 4=1024)
;		3 - Bad map has been initilized (0 = Uninitilized)
;
dphtab:	rept	maxlog*4
	db	0
	endm
	page

;**************************
;Begin Disk Driver Routines
;**************************
;
;General Equates and Macros
;==========================
;
;Disk System Dispatch Table Offsets
;----------------------------------
;	The following are offset numbers of Device Specification Tables.
;
d$wboot	equ	0	;Warm boot
d$stran	equ	1	;Sector translation
d$sel1	equ	2	;Drive select, Return DPH
d$sel2	equ	3	;Drive select
d$home	equ	4	;Home drive
d$strk	equ	5	;Set track
d$ssec	equ	6	;Set sector
d$sdma	equ	7	;Set DMA address
d$read	equ	8	;Read a physical sector
d$write	equ	9	;Write a physical sector
d$bad	equ	10	;Return pointer to bad sector info

;Disk System DPB Generation Macros
;---------------------------------
;	The following are the macros used in generating the DPH, DPB and
;	allocation tables.
;
dpbgen	macro	nam,log,dspt,dbsh,dblm,dexm,ddsm,ddrm,dal0,dal1,dcks,doff,ssiz
dpb&nam&log	equ	$
	dw	dspt
	db	dbsh
	db	dblm
	db	dexm
	dw	ddsm
	dw	ddrm
	db	dal0
	db	dal1
	dw	dcks
	dw	doff
	db	ssiz
	endm

dphgen	macro	nam,log,dpb1,dpb2
dph&nam&log	equ	$
	dw	0
	dw	0,0,0
	dw	dirbuf
	dw	&dpb1&dpb2
	dw	csv&nam&log
	dw	alv&nam&log
	endm

alloc	macro	nam,log,al,cs
csv&nam&log:	ds	cs
alv&nam&log:	ds	al
	endm
	page

	if	(maxdm ne 0) or (maxmf ne 0)	;DJDMA present?
;**************************************************************
;Begin the DJDMA Driver (DDRV1)
;******************************
;
;Step Rate tables
;================
;	1) The followinç tablå arå drivå parameteró foò driveó connecteä tï
;	   thå DJDMÁ floppù disë controller®  Therå ió onå entrù foò eacè oæ
;	   thå thå eighô drivå thaô thå controlleò caî address®  Thå firsô
;	   fouò entrieó arå foò thå 8¢ driveó anä thå lasô fouò arå foò thå
;	   5.25¢ drives®  Useró witè fasô steppinç 8¢ driveó (SA850/1© oò
;	   slo÷ 5.25¢ driveó (SA400© shoulä adjusô thió tablå foò optimaì
;	   devicå performace.
;	2) Each table entry contains four fixed length fields.  The fields
;	   are defined as follows:
;
;		tracks	This byte contains the number of tracks on the
;			drive.  Most 8" drives have 77 tracks and
;			most 5.25" drives have 35 or 40 tracks.
;
;		config	This a a flag byte that indicates as to whether
;			or not this drive has been configured.  Set to
;			0 to force reconfiguration.
;
;		step	This word contains the stepping rate constant.
;			The DJDMA's delay routines tick 34.1 times per
;			millisecond.  Thus the step constant would be the
;			drive manufactors recomended stepping delay times
;			34.1.  Example.  Shugart SA 850's step at 3
;			milliseond intervals.  The step constant would be
;			3 * 43.1 or 102.
;
;		rfu	The next two words are reserved for future use.
;			They must be zero.
;
;		settle	This word is similar to the previously defined
;			step word.  This specifies the head settle timing
;			after the heads have been stepped.  Example,
;			Shugart's SA 850 head settle time is 15
;			milliseconds.  The settle constant would be 15 *
;			34.1 or 512.
;
;	3) An assembler macro (DCONF) has been provided to assist in
;	   generating the dparam table.  This macros parameters are the
;	   number of tracks, the step rate in milliseconds, and the head
;	   settle time in milliseconds.  For example:
;
;				;Shugart SA 850
;	dconf	77, 3, 15	;77 tracks, 3 ms step, 15 ms settle
;
;				;Shugart SA 400
;	dconf	35, 40, 10	;35 tracks, 40 ms step, 10 ms settle
;
;	4) Note: Caution should be used when defining the drive parameters.
;	   Incorrect definations may damage the floppy disk drive.  Morrow
;	   Designs takes no responsibility for damage that occures through
;	   the misuse of this macro.
;
dconf	macro	tracks, step, settle
	db	tracks			;Number of tracks
	db	0			;Reset the calibrated flag
	dw	step*341/10		;Step time
	dw	0			;Reserved for future use, must be zero
	dw	0			;Reserved for future use, must be zero
	dw	settle*341/10		;Head settle time
	endm
dmarap:	db	0, 10*8			;Revision 0, length 80 bytes
dparam	equ	$			;Drive parameter table
;
;Define 8" drive parameters
;--------------------------
;	1) Use SA800 parameters: 77 tracks, 8 ms step, 8 ms settle
;
	dconf	77, 8, 8		;Drive 0
	dconf	77, 8, 8		;Drive 1
	dconf	77, 8, 8		;Drive 2
	dconf	77, 8, 8		;Drive 3
;
;Define 5.25" drive parameters
;-----------------------------
;	1) Use Tandon parameters: 40 tracks, 5 ms step, 15 ms settle
;	2) Note: Drive 1 is set up for a 20ms step rate and a 25ms head
;	   settling time so that it will operate properly with our
;	   soft-sectored drives.
;
	if	mfslow
	dconf	40, 20, 20		;Drive 0
	dconf	40, 20, 20		;Drive 1
	dconf	40, 20, 20		;Drive 2
	dconf	40, 20, 20		;Drive 3

	else
	dconf	40,  5, 15		;Drive 0
	dconf	40,  5, 15		;Drive 1
	dconf	40,  5, 15		;Drive 2
	dconf	40,  5, 15		;Drive 3
	endif
	page

;DJDMA equates
;=============
;
;Define DJDMA i/o ports and default channel address
;--------------------------------------------------
;
dmchan	equ	50h		;Default channel address
dmkick	equ	0efh		;Kick I/O port address
serin	equ	03eh		;Address of serial input data

;Define the channel commands
;---------------------------
;
dmrsec	equ	20h		;Read sector command
dmwsec	equ	21h		;Write a sector command
dmstac	equ	22h		;Get drive status
dmsdma	equ	23h		;Set DMA address
intrqc	equ	24h		;Set Interrupt request
dmhalt	equ	25h		;Halt command
bracha	equ	26h		;Channel branch
setcha	equ	27h		;Set channel address
dmserr	equ	28h		;Set CRC retry count
rdtrck	equ	29h		;Read track command
wrtrck	equ	2Ah		;Write track command
serout	equ	2Bh		;Serial character ouput
senabl	equ	2Ch		;Enable/disable serial input
trksiz	equ	2Dh		;Set number of tracks
dmsetl	equ	2Eh		;Set logical drives
readm	equ	0A0h		;Read from controller memory
writem	equ	0A1h		;Write to controller memory

;Define stepping rate equates
;----------------------------
;
dmfste	equ	3*341/10	;SA851 stepping rate constant
dmfset	equ	15*341/10	;SA851 settling rate constant

;Define Internal status byte fields
;----------------------------------
;
dms$t0	equ	01000000b	;Track 0 status mask  (1 = on trk 0)
dms$dd	equ	00100000b	;Double density mask  (1 = double)
dms$wr	equ	00010000b	;Double sided track wrap  (1 = wrap)
dms$ds	equ	00001000b	;Double sided status mask  (1 = double)
dms$hs	equ	00000100b	;Hard sectored status mask  (1 = hard)
dms$ss	equ	00000011b	;Sector size code mask ...
				;... 0 = 128, 1 = 256, 2 = 512, 3 = 1024

;Define North Star status byte fields
;------------------------------------
;
dmn$dä	eqõ	10000000â	;Doublå densitù mask
dmn$ds	equ	01000000b	;Double sided mask
dmn$2x	equ	00100000b	;CP/M version 2.x mask
dmn$ok	equ	00010000b	;Validation mask
dmn$40	equ	00001000b	;40/80 track mask
dmn$dt	equ	00000100b	;Double track density mask
dmn$xx	equ	00000011b	;RFU mask

;Common Subroutines
;------------------


; Return a pointer to the current drives drive parameter entry
;-------------------------------------------------------------
;
dmdpar:	lhld	dmdriv			;Get the current drive number
	mvi	h,0			;Drive number is a byte
	dad	h			;Ten bytes per parameter table entry
	mov	d,h
	mov	e,l
	dad	h
	dad	h
	dad	d
	lxi	d,dparam		;Parameter table address
	dad	d
	ret

	page

	if	maxdm ne 0	;Start 8" drive's unique code section
;====================================================================	
;Devicå Specificatioî Tablå foò DJDMA controlleò witè ¸" drives
;==============================================================
;
dmdst:	db	maxdm			;Number of logical drives
	dw	dmwarm			;Warm boot
	dw	dmtran			;Sector translation
	dw	dmldr1			;Select drive 1
	dw	dmldr2			;Select drive 2
	dw	dmhome			;Home drive
	dw	dmseek			;Seek to specified track
	dw	dmsec			;Set sector
	dw	dmdma			;Set DMA address
	dw	dmread			;Read a sector
	dw	dmwrit			;Write a sector
	dw	nobad			;No bad sector map

	if	dmorder ne 1		;no warm boot possible

;DJDMA 8" warm boot dummy
;------------------------
;	1) If 8" DJDMA is not drive A (i.e. dmorder not equal 1) then
;	   it is not possible to warm boot from 8". So routine not needed.
;
dmwarm:	ret				;return if called

	else

;DJDMÁ 8¢ warí booô loader
;-------------------------
;	1) Thió loadeò loadó froí thå starô oæ thå CCÐ (tracë ° sectoò 5©
;	   tï thå enä oæ thå BDOÓ (tracë ± sector 3)®  Onlù 76¸ (3/4k© byteó
;	   oæ tracë ± sectoò ³ ió reaä iî since thå warí booô routinå ió
;	   noô alloweä tï loaä anù thå CBIOÓ code.
;	2) Sectoò ³ ió reaä intï thå disë buffeò anä copieä intï itó propeò
;	   restinç place.
;
dmcod8	equ	22*128			;Amount of code on track 0 to load

dmwarm:	call	dmsel2			;Select drive 0
dmwbad:	lxi	h,dmwchn		;Warm boot command channel
	lxi	d,dmwoff
	call	dmcmd			;Execute the channel
	jnz	dmwbad			;Retry
	lda	dmwst0			;Get track read status
	lhld	dmwst1			;Track ones status in L
	ora	l
	cpi	40h
	jnz	dmwbad			;Loop on 'terrible' errors like no disk

	lxi	b,300h			;Move .75 Kbytes of sector 3
	lxi	d,buffer		;Sector 3 is in our buffer
	lxi	h,ccp+1300h		;  and this is where we want it to go
	call	movbyt
	xra	a
	ret

dmwchn:	db	dmsdma			;Set track 0 DMA address
	dw	ccp-512			;First track DMA address - boot loader
	db	0
	db	rdtrck			;Read track command
	db	0			;Track 0
	db	0			;Side 0
	db	0			;Drive 0
	dw	dmwmap			;Sector load/status map
	db	0
dmwst0:	db	0			;Track read status
	db	dmsdma
	dw	ccp+dmcod8		;DMA address for track 1
	db	0
	db	rdtrck
	db	1			;Track 1
	db	0			;Side 0
	db	0			;Drive 0
	dw	dmwmap+26		;Load map right after track 0 map
	db	0
dmwst1:	db	0			;Track read status
	db	dmsdma
	dw	buffer			;Sector 3 gets loaded in system buffer
	db	0
	db	dmrsec
	db	1			;Track 1
	db	3			;Side 0, sector 3
	db	0			;Drive 0
	db	0			;Read status
	db	dmhalt			;Controller halt command
	db	0

dmwoff	equ	$-dmwchn-1		;Halt offset for the command channel

dmwmap:	dw	-1, -1, 0, 0, 0, 0, 0	;Do not load the boot loader
	dw	0, 0, 0, 0, 0, 0

	dw	0, -1, -1, -1		;First 2 sectors on track 2

	endif				;end of 8" DJDMA warm boot

;DJDMA 8" sector translation
;---------------------------
;
dmtran:	inx	b			;Ajust sectors to start at 1
	lda	dmpsta			;Test for double sided drives
	ani	dms$ds
	jz	dmtrn0			;Skip if single sided
	lda	dmcspt			;Get SPT/2
	sub	c			;Test for side one sectors
	jnc	dmtrn0			;Skip sector adjustment if on side zero
	cma				;'Knock off' first sides sectors
	inr	a
	mov	c,a
	mvi	b,80h			;Set side one flag
dmtrn0:	mov	l,c			;Make an index to the SECTRAN table
	mvi	h,0
	dad	d
	mov	l,m			;Load the translated sector
	mov	h,b			;Set the side bit
	ret

;DJDMÁ 8¢ drivå selecô 1
;-----------------------
;	1) Determinå thå sectoò sizå anä thå numbeò oæ sideó oî thå drive®
;	2) Returî correcô DPH.
;
dmldr1:	call	dmsel2			;Do logical select	
	call	dminit			;Test for a controller
	jc	zret			;Skip if no controller present

	call	dmstat			;Accm:= Djdma_returned_Drive_Status
	jc	zret			;Skip on status check error
	push	psw
	ani	dms$t0			;Check for track 0
	jz	dmldr0			;Skip if not on track 0 (status valid)

	pop	psw			;Clean stack
	lxi	h,1			;read sector 1 ...
	shld	truesec
	inx	h			;... of track 2
	shld	cpmtrk			;because track 1 always single density
	xra	a
	sta	rdwr			;force read to get valid drive status
	call	fill			;flush buffer and read
	jc	zret			;exit with error if error

	call	dmdpar			;Get the drive parameter address
	inx	h
	mvi	m,0			;Decalibrate the drive
	call	dmparm
	call	dmstat			;Accm:= Djdma_returned_Drive_Status
	jc	zret			;If (error eq true) goto error_return
	push	psw

dmldr0:	pop	psw			;Get drive status
	sta	dmpsta			;Set the physical status mode
	call	dmsptr			;Save status in status table
	mov	m,a

	ani	dms$ss			;Mask in sector size bits
	rlc				;Make a word index
	push	psw			;Used to select a DPB
	mov	e,a
	mvi	d,0
	lxi	h,xlts			;Table of XLT table pointers
	dad	d
	push	h			;Save pointer to proper XLT

	call	dmgdph			;Get a pointer to the drives DPH
	pop	d			;Copy XLT pointer from table to DPH
	lxi	b,2
	call	movbyt
	lxi	d,8			;Offset to DPB pointer
	dad	d

	push	h
	lda	dmpsta			;Test for a double sided drive
	ani	dms$ds
	lxi	d,dpb128s		;Base for single sided DPB's
	jz	dmsok
	call	dmfstp			;Set controller to fast steping mode
	lxi	d,dpb128d		;Base of double sided DPB's
dmsok:	xchg
	poð	ä			;Restorå DPH pointeò to DPB
	pop	psw			;Offset to correct DPB (sector size)

	rlc				;Times   4
	rlc				;	 8
	rlc				;	16 bytes per DPB

	mov	c,a			;Offset to the correct DPB
	mvi	b,0
	dad	b
	xchg				;Load the DPB pointer in the DPH
	mov	m,e
	inx	h
	mov	m,d

	lxi	h,15			;Offset to the sector size code
	dad	d
	mov	c,m

dmgdph:	lda	dmdriv			;Get the DPH pointer
	lxi	d,dphdm0
	call	retdph
	ret

;DJDMÁ 8¢ drivå selecô 2
;-----------------------
;	1) Figurå numbeò oæ sectoró_peò_tracë¯² foò SECTRAN.							;
;
dmldr2:	call	dmsel2			;Perform logical drive select
	call	dmgdph			;Load the DPH pointer
	lxi	d,10			;Offset to the DPB pointer
	dad	d
	mov	a,m			;Load the DPB pointer
	inx	h
	mov	h,m
	mov	l,a
	mov	a,m			;Load the number of CP/M sectors/track
	rrc				;Divide by two
	sta	dmcspt			;Save CPM SPT
	ret

;Set the drive's step rate to 3ms
;--------------------------------
;	1) Thå currenô drivå ió doublå sided®  Thuó ió iô safå tï seô thå
;	   steppinç ratå tï ³ mó witè 1µ mó settling.
;
dmfstp:	call	dmdpar			;Get the parameter table pointer
	inx	h			;Bump to the drive initialized flag
	mvi	m,0			;Force reparamitization of this drive
	inx	h			;Offset to the Stepping rate constant
	mvi	m,(low dmfste)		;Fast stepping rate constant
	inx	h
	mvi	m,(high dmfste)
	lxi	d,5			;Skip over the reserved fields
	dad	d
	mvi	m,(low dmfset)		;Fast settling rate constant
	inx	h
	mvi	m,(high dmfset)
	call	dmparm			;Set drive parameters for the SA850
	ret

;DJDMA 8" driver variables
;-------------------------
;
dmcspt:	db	0		;CPM sectors per track / 2

; 8" Disk parameter headers
;--------------------------
;
dphdm0:	dw	0		;translation table address
	dw	0
	dw	0
	dw	0
	dw	dirbuf		;directory buffer
	dw	0		;pointer to disk parameter block
	dw	csvdm0		;scratch pad area for checking changed disks
	dw	alvdm0		;scratch pad for allocation information

dphdm1:	dw	0		;translation table address
	dw	0
	dw	0
	dw	0
	dw	dirbuf		;directory buffer
	dw	0		;pointer to disk parameter block
	dw	csvdm1		;scratch pad area for checking changed disks
	dw	alvdm1		;scratch pad for allocation information

dphdm2:	dw	0		;translation table address
	dw	0
	dw	0
	dw	0
	dw	dirbuf		;directory buffer
	dw	0		;pointer to disk parameter block
	dw	csvdm2		;scratch pad area for checking changed disks
	dw	alvdm2		;scratch pad for allocation information

dphdm3:	dw	0		;translation table address
	dw	0
	dw	0
	dw	0
	dw	dirbuf		;directory buffer
	dw	0		;pointer to disk parameter block
	dw	csvdm3		;scratch pad area for checking changed disks
	dw	alvdm3		;scratch pad for allocation information

	endif			;End of 8" drive's unique code
	page

	if	maxmf ne 0	;Start of 5" drive's unique code section
;=======================================================================
;Drive specification table for DJDMA 5.25" drives
;================================================
;
mfdst:	db	maxmf			;Number of logical drives
	dw	mfwarm			;Warm boot
	dw	mftran			;Sector translation
	dw	mfldr1			;Select drive 1
	dw	mfsel2			;Select drive 2
	dw	dmhome			;Home drive
	dw	mfseek			;Seek to specified track
	dw	mfssec			;Set sector
	dw	dmdma			;Set DMA address
	dw	mfread			;Read a sector
	dw	mfwrit			;Write a sector
	dw	nobad			;No bad sector map

	if	mforder ne 1		;no warm boot possible

;DJDMA 5.25" warm boot dummy
;---------------------------
;	1) If 5.25" DJDMA is not drive A (i.e. mforder not equal 1) then
;	   it is not possible to warm boot from 5.25". So routine not needed.
;
mfwarm:	ret				;return if called

	else

;DJDMÁ 5.25¢ warí booô loader
;----------------------------
;	1) Loaä froí thå starô oæ thå CCÐ (tracë ° sectoò 1© tï thå enä
;	   oæ thå BDOÓ (tracë ± sectoò 1).

mftrck	equ	9*512	;###This is the amount of code that extends beyond
			; location ccp.  The other 512 bytes in the track
			; are the cold boot loader that start at ccp-512.
			; So second track will be read to locations that start
			; at ccp+mftrck.

mfwarm:	call	mfsel2			;Select drive 0
mfwbad:	lxi	h,mfwchn		;Warm boot command channel
	lxi	d,mfwlen
	call	dmcmd
	jnz	mfwbad			;Loop on 'bad' errors
	lda	mfwst0
	cpi	40h
	jnz	mfwbad

	xra	a			;Return no error
	ret

;##This reading of first track is same for either single or double sided disks
mfwchn:	db	dmsdma			;Set track 0 DMA address
	dw	ccp-512			;First track DMA address - boot loader
	db	0
	db	rdtrck			;Read track command
	db	0			;Track 0
	db	0			;Side 0
	db	4			;mini Drive 0
	dw	mfwsec			;Sector load/status map
	db	0
mfwst0:	db	0			;Track read status

;##Destination for second track is 9*512 (or 5*1024-512) beyond location ccp
	db	dmsdma
	dw	ccp+mftrck		;DMA address for track 1
	db	0
	db	rdtrck
;###	db	1			;Track 1   (##One sided disk)
;###	db	0			;Side 0
	db	0			;##For 2 sides, second track
					; is Track 0 Side 2
	db	80h			;##Dma controller code for Side 2
	db	4			;mini Drive 0
;###	dw	mfwsec+10		;Load map right after track 0 map
	dw 	mfwsec+5		;###Rev. for 2 side soft sector
	db	0
	db	0			;Track read status
	db	dmhalt
	db	0

mfwlen	equ	$-mfwchn-1		;Channel length

;###mfwsec:	dw	0FFh, 0, 0, 0, 0	;Do not load boot loader
;###	dw	0, -1, -1, -1, -1	;first two sectors loaded

MFWSEC: db      0,0,0,0,0	;###CCP on disk starts at the 5th CP/M Sector,
				; which is in the first 1024 byte physical
				; sector, which must be read.
	db	0,0FFh,0FFh,0FFh,0FFh

	endif				;DJDMA 5.25" warm boot routine

;DJDMA 5.25" sector translation
;------------------------------
;
mftran:	lda	dmpsta			;Test for soft sectored media
	ani	dms$hs
	lxi	h,mfxlt1		;Soft sectored SECTRAN table
	jz	mftrn
	lda	dmpsta			;Test disk density
	ani	dms$dd
	lxi	h,mfxltd		;Double density SECTRAN table
	jnz	mftrn
	lxi	h,mfxlts		;Single density SECTRAN table
mftrn:	dad	b
	mov	l,m			;Load physical sector number
	mvi	h,0
	ret

;DJDMÁ 5.25¢ firsô timå select
;-----------------------------
;	1) Thió routinå inpecteó thå disk. Iæ thå disë ió harä sectoreä
;	   thå thå Nortè Staò configuratioî bytå ió reaä froí tracë °,
;	   sectoò ° bytå 5c®  Iæ thå mediá ió soft sectoreä thå thå
;	   Morro÷ Designó Micrï Decisioî formaô ió assumed.
;
mfldr1:	call	mfsel2			;Do logical drive select
	call	dminit			;Test for a controller
	jc	zret

	call	dmstat			;Get the drive status byte
	jc	zret			;Error exit if status not good

	sta	dmpsta
	call	dmsptr			;Save in the status table
	mov	m,a

	ani	dms$hs			;Test for hard sectored drives
	jnz	mfld0			;Skip to hard sectored logger

	call	mfrds1			;get sector 1 of track 0
	lxi	h,buffer+80h+25		;longitudinal parity check the data
	mvi	b,25			;number of bytes to check
	xra	a			;init long parity
	mov	e,a			;0-check byte

mfckl:	dcx	h			;next byte to check
	xra	m			;get long parity
	mov	d,a			;save parity
	ora	e			;catch any 1 bits in 0-check byte
	mov	e,a			;save 0-check
	mov	a,d			;get parity again
	dcr	b
	jnz	mfckl			;loop for whole table
	ora	a			;tests parity (should be 0 for valid)
	jnz	mfsft			;assume single side if bad table
	ora	e			;test 0-check (should not be all 0)
	jz	mfsft			;assume single side in all 0 table

	lda	buffer+81h		;get Morrow soft sector config byte
	ani	4			;check double sided indicator
	mvi	a,0a9h			;double sided config byte
	jnz	mfld1			;skip if double sided

mfsft:	mvi	a,0a1h			;Morrow Soft sectored floppy
	jmp	mfld1

mfld0:	call	mfrds1			;get sector 1 of track 0
	lda	buffer+5Ch		;Get the North Star configuration byte

mfld1:	ora	a			;Old CP/M 1.4 systems did not have a
	cz	mflcl			;   configuration byte.  This routine
	cpi	0E5h			;   will make a configuration byte for
	cz	mflcl			;   these systems.

	mov	c,a

	lxi	h,mfs			;Pointer to configuration table
mfld2:	mov	a,m			;Get an entry
	ora	a			;Check for end of the table
	jz	zret			;Yes, select error
	cmp	c			;Check if entry matches selected drive
	jz	mfld3			;Match, get entry
	inx	h			;Skip to the next entry
	inx	h
	inx	h
	inx	h
	jmp	mfld2

mfld3:	inx	h			;Bump to the true configuration byte
	lda	dmpsta			;Get the physical status
	ora	m			;Fill in the fields the hardware can't
	sta	dmpsta			;   figure out
	push	h
	call	dmsptr			;Load the status byte into the table
	mov	m,a
	pop	h
	inx	h			;Bump to the DPB pointer
	mov	a,m			;Load the DPB pointer
	inx	h
	mov	h,m
	mov	l,a
	push	h			;Save DPB address

	call	mfgdph			;Get a DPH pointer
	lxi	d,10			;Offset to DPB address in DPH
	dad	d
	pop	d
	mov	m,e			;Store DPB address in DPH
	inx	h
	mov	m,d

	call	mfgdph			;Get the DPH pointer
	lda	dmpsta			;Get physical status
	ani	dms$ss			;Mask sector size field
	inr	a			;Make CBIOS sector size code
	mov	c,a
	ret
;
; routine called by first time select to read sector 1 track 0
; for disk configuration byte checks for hard/soft sectored
; minifloppies.
;
mfrds1:	lxi	h,1			;Select sector 1 of track 0
	shld	truesec
	dcx	h
	shld	cpmtrk
	xra	a			;Make sure we are doing a read
	sta	rdwr
	call	fill			;Flush buffer and refill
	rnc				;return if no error
	pop	h			;flush return address
	jmp	zret			;do error return

; Get the configuration byte for a North Star Disk
;-------------------------------------------------
;	1) This routine is only used by MFLDR1: (mini-floppy first time
;	   select
;	2) Nortè Staò configuratioî bytå valuå ió ° oò aî E5®  Checë
;	   physicaì disë densitù anä generatå correcô configuratioî bytå
;	   value.
;
mflcl:	lda	dmpsta			;Get physical status
	ani	dms$dd			;Test the double density bit
	mvi	a,10h			;CP/M 1.4 single density configuration
	rz
	mvi	a,90h			;CP/M 1.4 double density configuration
	ret

; Return a pointer to the current drives DPH
;-------------------------------------------									;
;	1) This routine is only used by MFLDR1: (mini-floppy first time
;	   select
;
mfgdph:	lda	dmdriv			;Get the current drive
	sui	4			;5.25 drives start at drive 4
	lxi	d,dphmf0
	call	retdph
	ret

;Selecô drivå #²
;---------------
;	1) Thió driveò configureó thå µ 1/´" driveó aó driveó ´ tï 7.						;
;
mfsel2:	adi	4			;5.25" drives are drives 4-7
	jmp	dmsel2

;Seô track
;---------
;	1) Nortè Staò implementó doublå sideä driveó bù doublinç
;	   thå numbeò oæ trackó tï 70®  Trackó ° tï 3´ arå oî sidå ° likå á
;	   singlå sideä floppy®  Trackó 3µ tï 6¹ arå oî sidå ± iî reverse
;	   (e.g® Tracë 3µ ió oî tracë 3´ sidå 1 and tracë 6¹ ió oî tracë °
;	   sidå 1).
;
mfseek:	xra	a			;Clear double sided select flag
	sta	mfsid1
	lxi	h,dmpsta		;Get the drive status
	mov	a,m
	ani	dms$hs			;Test for hard sectored drives
	jz	dmsoft			;Skip if soft sectored

	mov	a,m			;Test for double sided drives
	ani	dms$ds
	jz	dmseek			;Skip if single sided

	mov	a,m			;Test for track wrap mode
	ani	dms$wr
	jz	dmseek			;Skip if not wrapping

	mov	a,c			;Test for tracks 35-69
	cpi	35
	jc	dmseek			;Skip if less than track 35

	mvi	a,69			;Adjust tracks 35 -> 69 to 34 -> 0
	sub	c
	mov	c,a
	mvi	a,080h			;Set side one flag
	sta	mfsid1
	jmp	dmseek

dmsoft:	mov	a,m			;get drive status again
	ani	dms$ds			;check for double sided (cy = 0)
	jz	dmseek			;skip if not double sided

	mov	a,c			;get track number
	rar				;divide by 2 for Morrow soft sectored
	mov	c,a
	jnc	dmseek			;skip if on side 1
	mvi	a,80h
	sta	mfsid1			;else indicate side 2
	jmp	dmseek

;DJDMA 5.25" set sector
;----------------------
;
mfssec:	lda	dmpsta			;if (drive .eq. soft_sectored)
	ani	dms$hs			;	goto sector save routine
	jz	dmsec			;else
	dcr	c			;	adjust for first sect = zero
	jmp	dmsec

;DJDMA 5.25" read/write sector
;-----------------------------
;
mfread:	call	mfset			;Set up side flag
	jmp	dmread

mfwrit:	call	mfset			;Set up side flag
	jmp	dmwrit

mfset:	lda	mfsid1			;Get the side flag
	lxi	h,dmsctr		;Merge with the sector number
	ora	m
	mov	m,a
	ret

;DJDMA 5.25" driver variables
;----------------------------
;
;Mini-Floppy Configuration/DPB_Lookup table
;	1) This table is used by the mini-floppy first time select routine
;	   (mfsldr1) and is used to:
;		a) validate the drive configuration byte.
;		b) fill in the parameters that can't be determined by doing
;		   sense drive status (like sensing double sided drives).
;		c) returning a pointer to the proper DPB for the media.
;	2) There are four fields per entry.
;		Field_1: Drive configuration byte.
;		Field_2: Additional drive parameters that can't be determined
;			 by doing a sense drive status (i.e. double_sided and
;			 track_wrap).
;		Field_3: Pointer to the appropriate DPB
;
mfs:	db	10h			;North Star CP/M 1.4
	db	0			;Single density, 35 tracks, 1-sided
	dw	dpbmf0			;1K groups

	db	90h			;North Star CP/M 1.4
	db	0			;Double density, 35 tracks, 1-sided
	dw	dpbmf1			;1K groups

	db	0b0h			;North Star CP/M 2.x
	db	0			;Double density, 35 tracks, 1-sided
	dw	dpbmf2			;2K groups

	db	0f0h			;North Star CP/M 2.x
	db	(dms$ds or dms$wr)	;Double density, 35 tracks, 2-sided
	dw	dpbmf3			;2K groups

	db	0a0h			;North Star CP/M 2.x  (fake 40 track)
	db	0			;Double density, 35 tracks, 1-sided
	dw	dpbmf2			;2K groups

	db	0d0h			;North Star CP/M 2.x (fake 40 track)
	db	(dms$ds or dms$wr)	;Double density, 35 tracks, 2-sided
	dw	dpbmf3			;2K groups

	db	0a1h			;Morrow Designs CP/M 2.x Soft sectored
	db	0			;Double density, 40 tracks, 1-sided
	dw	dpbmf4

	db	0a9h			;Morrow Designs CP/M 2.x Soft sectored
	db	dms$ds			;Double density, 40 tracks, 2-sided
	dw	dpbmf5

	db	0			;End of configuration table

; Hard sectored single sided sector translation table
;
mfxlts:	db	 1,  2
	db	 3,  4
	db	 5,  6
	db	 7,  8
	db	 9, 10
	db	11, 12
	db	13, 14
	db	15, 16
	db	17, 18
	db	19, 20

; Hard sectored double sided sector translation table
;
mfxltd:	db	 1,  2,  3,  4
	db	21, 22, 23, 24
	db	 5,  6,  7,  8
	db	25, 26, 27, 28
	db	 9, 10, 11, 12
	db	29, 30, 31, 32
	db	13, 14, 15, 16
	db	33, 34, 35, 36
	db	17, 18, 19, 20
	db	37, 38, 39, 40

; Soft sectored single sided translation table
;
mfxlt1:	db	 1,  2,  3,  4,  5,  6,  7,  8
	db	25, 26, 27, 28, 29, 30, 31, 32
	db	 9, 10, 11, 12, 13, 14, 15, 16
	db	33, 34, 35, 36, 37, 38, 39, 40
	db	17, 18, 19, 20, 21, 22, 23, 24

mfsid1:	db	0		;On side one flag

;disk parameter headers
;----------------------
;
dphmf0:	dw	0		;translation table address
	dw	0
	dw	0
	dw	0
	dw	dirbuf		;directory buffer
	dw	dpbmf0		;pointer to disk parameter block
	dw	csvmf0		;scratch pad area for checking changed disks
	dw	alvmf0		;scratch pad for allocation information
;
dphmf1:	dw	0		;translation table address
	dw	0
	dw	0
	dw	0
	dw	dirbuf		;directory buffer
	dw	dpbmf1		;pointer to disk parameter block
	dw	csvmf1		;scratch pad area for checking changed disks
	dw	alvmf1		;scratch pad for allocation information
;
dphmf2:	dw	0		;translation table address
	dw	0
	dw	0
	dw	0
	dw	dirbuf		;directory buffer
	dw	dpbmf2		;pointer to disk parameter block
	dw	csvmf2		;scratch pad area for checking changed disks
	dw	alvmf2		;scratch pad for allocation information
;
dphmf3:	dw	0		;translation table address
	dw	0
	dw	0
	dw	0
	dw	dirbuf		;directory buffer
	dw	dpbmf3		;pointer to disk parameter block
	dw	csvmf3		;scratch pad area for checking changed disks
	dw	alvmf3		;scratch pad for allocation information

;disk parameter buffers
;----------------------
;
dpbmf0:	dw	20	;SPT
	db	3	;BSH
	db	7	;BLM
	db	0	;EXM
	dw	79	;DSM
	dw	63	;DRM
	db	0C0h	;AL0
	db	0	;AL1
	dw	16	;CKS
	dw	3	;OFF
	db	2	;SECSIZ
;
dpbmf1:	dw	40	;SPT
	db	3	;BSH
	db	7	;BLM
	db	0	;EXM
	dw	164	;DSM
	dw	63	;DRM
	db	0C0h	;AL0
	db	0	;AL1
	dw	16	;CKS
	dw	2	;OFF
	db	3	;SECSIZ
;
dpbmf2:	dw	40	;SPT
	db	4	;BSH
	db	15	;BLM
	db	1	;EXM
	dw	81	;DSM
	dw	63	;DRM
	db	080h	;AL0
	db	0	;AL1
	dw	16	;CKS
	dw	2	;OFF
	db	3	;SECSIZ
;
dpbmf3:	dw	40	;SPT
	db	4	;BSH
	db	15	;BLM
	db	1	;EXM
	dw	169	;DSM
	dw	63	;DRM
	db	080h	;AL0
	db	0	;AL1
	dw	16	;CKS
	dw	2	;OFF
	db	3	;SECSIZ
;
dpbmf4:	dw	40	;SPT
	db	4	;BSH
	db	15	;BLM
	db	1	;EXM
	dw	94	;DSM
	dw	127	;DRM
	db	0C0h	;AL0
	db	0	;AL1
	dw	32	;CKS
	dw	2	;OFF
	db	4	;SECSIZ

dpbmf5:	dw	40	;SPT
	db	4	;BSH
	db	15	;BLM
	db	1	;EXM
	dw	194	;DSM
	dw	191	;DRM
	db	0E0h	;AL0
	db	0	;AL1
	dw	48	;CKS
	dw	2	;OFF
	db	4	;SECSIZ

	page
	endif			;End of 5" drive's unique code section
;=====================================================================
;Common routines for the DJDMA with 8 and 5.25" drives
;=====================================================
;
;Set up the disk controller
;--------------------------
;
dminit:	lxi	h,dmchan		;See if the controller will halt
	mvi	m,dmhalt
	inx	h
	mvi	m,0
	out	dmkick			;Start controller
	lxi	d,0			;Set up timeout counter
dminwt:	mov	a,m			;Test for status returned
	ora	a
	jnz	dmiok			;Controller has responded
	dcx	d			;Bump timeout counter
	mov	a,d
	ora	e
	jnz	dminwt
	stc				;Set error flag
	ret

dmiok:	call	dmparm			;Set the drive parameters
	lxi	h,dmsetu		;Set more parameters
	lxi	d,6
	call	dmdoit
	ret				;Return no error (C reset)

;Drivå selecô two®
;-----------------
;	1) 8¢ driveó arå driveó 0-3¬ 5.25¢ driveó arå driveó 4-7.
;
dmsel2:	sta	dmdriv			;Save the drive name
	sta	dmgsta+1		;(for sense status command)
	call	dmsptr			;Get status pointer
	mov	a,m
	sta	dmpsta			;Save current status
	ret

;Seek to track 0
;---------------
;
dmhome:	call	dmdpar			;Get the drive parameter address
	inx	h
	mvi	m,0			;Decalibrate the drive
	call	dmparm
	ret

;Set track
;---------
;
dmseek:	mov	a,c			;Set up DJDMA track
	sta	dmtrck
	ret

;Set sector
;----------
;
dmsec:	mov	a,c			;Set the sector number + side bit
	ora	b
	sta	dmsctr
	ret

;Set the DMA pointer
;-------------------
;
dmdma:	mov	h,b			;Set the DMA address
	mov	l,c
	shld	dmcdma
	ret

;Read/write a sector.
;--------------------
;	1) Notice that the carry is returned set if there were any
;	   errors otherwise it is returned cleared. Also notice that
;	   the accm is equal to the djdma returned status (e.g. 40=no_error)
;
dmread:	mvi	a,dmrsec		;Read sector command
	jmp	dmsrw

dmwrit:	mvi	a,dmwsec		;Write sector command

dmsrw:	sta	dmrwcm			;Set the disk command byte
	lxi	h,dmrdwr		;Read/write command channel address
	lxi	d,10
	call	dmcmd			;Do the read/write
	stc				;if (error eq true)
	rnz				;	return (carry_set=ERROR)
 	cmc				;else
	ret				;	return (carry_cleared=NO_ERROR)

;Seô floppù drivå parameters®
;----------------------------
;	1) Thió routinå inspectó thå DPARAÍ tablå anä iæ thå á drivå
;	   haó noô beeî calibrateä previouslù then thaô driveó tracë
;	   count¬ steppinç rate¬ anä heaä settlå timå are loaded.
;
dmparm:	mvi	a,8			;Eight drives
	lxi	d,1340h			;Controllers drive parameter address
	lxi	h,dparam+1		;CBIOS's drive parameter table

dmstr0:	push	psw			;Save the drive count
	mov	a,m			;Load flags
	ora	a			;Does the drive need to be calibrated?
	jnz	dmstr1			;No, do not fiddle around
	push	h			;Save the parameter table pointer
	push	d			;Save the controllers table pointer
	dcr	m			;Set to calibrated mode (0ffh)
	dcx	h			;Back up to the track size byte
	shld	dmntrk			;Set the number of tracks pointer
	inx	h
	inx	h
	shld	dmspar			;Set the stepping constants pointer
	xchg				;Set the local parameter table pointer
	shld	dmloc0
	inx	h			;Offset to the stepping parameters
	inx	h
	inx	h
	inx	h
	shld	dmloc1
	lxi	h,dmwcon		;Write the drive constants out
	lxi	d,17			;Halt status offset
	call	dmdoit
	pop	d			;Retrieve the table pointers
	pop	h

dmstr1:	lxi	b,10			;Bump parameter table pointer
	dad	b
	xchg
	lxi	b,16			;Bump controller tables pointer
	dad	b
	xchg

	pop	psw			;Retrieve drive count
	dcr	a			;Bump count
	jnz	dmstr0			;Set up next drive

	ret

;Return the selected drive's status
;----------------------------------
;	1) The status is returned in the (a) register in the following form:
;
;  bit=1 if				7  6  5  4  3  2  1  0
;  --------				^  ^  ^  ^  ^  ^  ^  ^
;		Reserved ---------------+  |  |  |  |  |  |  |
;  on track 0	Track zero ----------------+  |  |  |  |  |  |
;  dbl dens	Double density ---------------+  |  |  |  |  |
;  wrap trk	Track wrap flag -----------------+  |  |  |  |
;  2-sided	Double sided media -----------------+  |  |  |
;  hard sect	Hard sectored media -------------------+  |  |
;		Sector size MSB --------------------------+  |
;		Sector size LSB -----------------------------+
;		   00 = 128, 01 = 256, 10 = 512, 11 = 1024 bytes.
;
;	2) 5" drives are always reported as being single sided (because the
;	   hardware double sided line is not implemented).
;
dmstat:	lxi	h,dmgsta		;Get controller status command
	lxi	d,7
	call	dmcmd
	stc				;Set the error flag just in case
	rnz				;Return on error
					;76543210 (STATUS REGISTER BITS)
	lda	dmsta1			;?????H?? Double density, hard Sectored
	ani	00010010b		;---D--H-
	mov	l,a			;---D--H-
	lda	dmsta3			;??0??D?? Track 0, Double sided bits
	ani	00100100b		;--0--S--
	ora	l			;--0D-SH-
	rlc				;-0D-SH--
	lxi	h,dmsta2		;-0D-SH--
	ora	m			;-0D-SHXX Sector size code
	ret				;Return no error (C reset)

;Return a pointer to the current drives status byte
;--------------------------------------------------
;
dmsptr:	lxi	d,dmstbl		;Status byte table
	lhld	dmdriv			;Current drive into L
	mvi	h,0
	dad	d
	ret

;Execute a DJDMA command, return command status results
;------------------------------------------------------
;	1) Enter this routine with:
;		DE = offset to the halt status
;		HL = pointer to the start of the command
;	2) This routine returns:
;		 A = command status
;		ZF = set on 40 command status value
;
dmcmd:	call	dmdoit			;Do the desired command
	dcx	h			;Back up to the command status byte
	dcx	h
	mov	a,m			;Load the command status byte
	cpi	40h			;Set flags
	ret

;Execute a DJDMA command, no command status is returned
;------------------------------------------------------
;	1) Enter this routine with:
;		DE = offset to the halt status
;		HL = pointer to the start of the command
;	2) This routine returns no status
;
dmdoit:	mvi	a,bracha		;Branch channel command
	sta	dmchan
	shld	dmchan+1		;Load command vector
	xra	a			;Clear extended address
	sta	dmchan+3
	dad	d			;Offset to the halt status
	mov	m,a			;Clear the halt status indicator
	out	dmkick			;Start the controller
dmwait:	ora	m			;Wait for the operation complete status
	jz	dmwait
	ret

;DJDMA commmand channel routines
;===============================
;
;Set the logical drive assignments and the retry count
;-----------------------------------------------------
;
dmsetu:	db	dmsetl			;Set the logical drive assignments
	db	0			;Drives 0-3 are 8", 4-7 are 5.25"
	db	0			;Old status
	db	dmserr			;Set the error retry count to 0
	db	1			;One retry (the CBIOS does ten)
	db	dmhalt
	db	0

;Get a drive's status
;---------------------
;
dmgsta:	db	dmstac			;Controller/drive status command
	db	0			;Drive to be sensed
dmsta1:	db	0			;Status byte 1
dmsta2:	db	0			;Status byte 2
dmsta3:	db	0			;Status byte 3
	db	0			;Return status
	db	dmhalt
	db	0

;Set the dma address and then do a read or a write
;-------------------------------------------------
;
dmrdwr:	db	dmsdma			;Set DMA address command
dmcdma:	dw	0			;DMA address
	db	0			;X-addr
dmrwcm:	db	0			;Read/write command filled in
dmtrck:	db	0			;Track
dmsctr:	db	0			;Sector
dmdriv:	db	0			;Drive
	db	0			;Status
	db	dmhalt			;Controller halt command
	db	0			;Status

;Write a drive's constants into the controller's memory
;------------------------------------------------------
;
dmwcon:	db	writem			;Write track size
dmntrk:	dw	0			;Number of tracks + desync
	db	0			;X-address
	dw	2			;Two bytes
dmloc0:	dw	0			;Local controller address
	db	writem			;Write stepping rate data
dmspar:	dw	0			;Pointer to the stepping parameters
	db	0
	dw	8
dmloc1:	dw	0
	db	dmhalt			;Controller halt
	db	0			;Status

;Driver variables
;----------------
;
dmpsta:	db	0			;Physical status for the current drive
dmstbl:	db	0,0,0,0,0,0,0,0		;Physical status bytes for each drive

	endif				;End of djdma routines
	page

	if	maxfd ne 0		;Include Discus 2D ?
;***********************************************************
; Begin the DJ2DB Driver (DDRV2)
;*******************************
;
;DJ2DB equates
;-------------
;	1) The following equates relate the Morrow Designs 2D/B
;	controller. If the controller is non standard (0F800H)
;	only the FDORIG equate need be changed.
;
; --NOTE-- 'fdorig' equate moved to top of source module
;fdorig	equ	0xxxxh		;Origin of Disk Jockey PROM
fdboot	equ	fdorig+00h	;Disk Jockey 2D initialization
fdcin	equ	fdorig+03h	;Disk Jockey 2D character input routine
fdcout	equ	fdorig+06h	;Disk Jockey 2D character output routine
fdhome	equ	fdorig+09h	;Disk Jockey 2D track zero seek
fdseek	equ	fdorig+0ch	;Disk Jockey 2D track seek routine
fdsec	equ	fdorig+0fh	;Disk Jockey 2D set sector routine
fddma	equ	fdorig+12h	;Disk Jockey 2D set DMA address
fdread	equ	fdorig+15h	;Disk Jockey 2D read routine
fdwrite	equ	fdorig+18h	;Disk Jockey 2D write routine
fdsel	equ	fdorig+1bh	;Disk Jockey 2D select drive routine
fdtstat	equ	fdorig+21h	;Disk Jockey 2D terminal status routine
fdstat	equ	fdorig+27h	;Disk Jockey 2D status routine
fderr	equ	fdorig+2ah	;Disk Jockey 2D error, flash led
fdden	equ	fdorig+2dh	;Disk Jockey 2D set density routine
fdside	equ	fdorig+30h	;Disk Jockey 2D set side routine
fdram	equ	fdorig+400h	;Disk Jockey 2D RAM address
dblsid	equ	20h		;Side bit from controller
io	equ	fdorig+3f8h	;Start of I/O registers
dreg	equ	io+1
cmdreg	equ	io+4
clrcmd	equ	0d0h

;***************************************************************;
;								;
; Device Specification Table for the Disk Jockey 2D/B		;
;								;
;***************************************************************;

fddst:	db	maxfd		;Number of logical drives
	dw	fdwarm		;Warm boot
	dw	fdtran		;Sector translation
	dw	fdldrv		;Select drive 1
	dw	fdsel2		;Select drive 2
	dw	fdlhome		;Home drive
	dw	fdseek		;Seek to specified track
	dw	fdssec		;Set sector
	dw	fddma		;Set DMA address
	dw	fdread		;Read a sector
	dw	fdwrite		;Write a sector
	dw	nobad		;No bad sector map

	if	fdorder ne 1		;no warm boot possible

;DJ2D/B warm boot dummy
;---------------------------
;	1) If DJ2D/B is not drive A (i.e. fdorder not equal 1) then
;	   it is not possible to warm boot from DJ2D/B. So routine not needed.
;
fdwarm:	ret				;return if called

	else

; DJ2D/B Floppy disk warm boot loader
;------------------------------------
;
fdwarm:	mov	c,a
	call	fdsel		;Select drive A
	mvi	c,0		;Select side 0
	call	fdside
wrmfail:call	fdhome		;Track 0, single density
	jc	wrmfail		;Loop if error

				;The next block of code re-initializes
				;   the warm boot loader for track 0
	mvi	a,5-2		;Initialize the sector to read - 2
	sta	newsec
	lxi	h,ccp-100h	;First revolution DMA - 100h
	shld	newdma
				;Load all of track 0

t0boot:	mvi	a,5-2		;First sector - 2
newsec	equ	$-1
	inr	a		;Update sector #
	inr	a
	cpi	27		;Size of track in sectors + 1
	jc	nowrap		;Skip if not at end of track
	jnz	t1boot		;Done with this track
	sui	27-6		;Back up to sector 6
	lxi	h,ccp-80h	;Memory address of sector - 100h
	shld	newdma
nowrap:	sta	newsec		;Save the updated sector #
	mov	c,a
	call	fdsec		;Set up the sector
	lxi	h,ccp-100h	;Memory address of sector - 100h
newdma	equ	$-2
	lxi	d,100h		;Update DMA address
	dad	d
nowrp:	shld	newdma		;Save the updated DMA address
	mov	b,h
	mov	c,l
	call	fddma		;Set up the new DMA address
	lxi	b,retries*100h+0;Maximum # of errors, track #
wrmfred:push	b
	call	fdseek		;Set up the proper track
	call	fdread		;Read the sector
	pop	b
	jnc	t0boot		;Continue if no error
	dcr	b
	jnz	wrmfred		;Keep trying if error
	jmp	fderr		;Too many errors, flash the light

;Load track 1, sector 1, sector 3 (partial), sector 2 (1024 byte sectors)

t1boot:	mvi	c,1		;Track 1
	call	fdseek
	lxi	b,ccp+0b00h	;Address for sector 1
	lxi	d,10*100h+1	;Retry count + sector 1
	call	wrmread
	lxi	b,ccp+0f00h	;Address for sector 2
	lxi	d,10*100h+3	;Retry count + sector 3
	call	wrmread

	lxi	b,0300h		;Size of partial sector
	lxi	d,ccp+1300h	;Address for sector 3
	lxi	h,ccp+0f00h	;Address of sector 3

wrmcpy:	mov	a,m		;Get a byte and
	stax	d		;   save it
	inx	d		;Bump pointers
	inx	h
	dcx	b		;Bump counter
	mov	a,b		;Check if done
	ora	c
	jnz	wrmcpy		;   if not, loop

	lxi	b,ccp+0f00h	;Address for sector 2
	lxi	d,10*100h+2	;Retry count + sector 2
	call	wrmread

	xra	a		;Clear error indicator
	ret

wrmread:push	d
	call	fddma		;Set DMA address
	pop	b
	call	fdsec		;Set sector
wrmfrd:	push	b		;Save error count
	call	fdread		;Read a sector
	jc	wrmerr		;Do retry stuff on error
	call	fdstat		;Sector size must be 1024 bytes
	ani	0ch		;Mask length bits
	sui	0ch		;Carry (error) will be set if < 0c0h
wrmerr:	pop	b		;Fetch retry count
	rnc			;Return if no error
	dcr	b		;Bump error count
	jnz	wrmfrd
	jmp	fderr		;Error, flash the light

	endif			;end of DJ2D/B warm boot routine

;DJ2D/B Sector Translate Routine
;-------------------------------
;
fdtran:	inx	b
	push	d		;Save table address
	push	b		;Save sector #
	call	fdget		;Get DPH for current drive
	lxi	d,10		;Load DPH pointer
	dad	d
	mov	a,m
	inx	h
	mov	h,m
	mov	l,a
	mov	a,m		;Get # of CP/M sectors/track
	ora	a		;Clear carry
	rar			;Divide by two
	sub	c		;Subtract sector number
	push	psw		;Save adjusted sector
	jm	sidetwo
sidea:	pop	psw		;Discard adjusted sector
	pop	b		;Restore sector requested
	pop	d		;Restore address of xlt table
sideone:xchg			;hl <- &(translation table)
	dad	b		;bc = offset into table
	mov	l,m		;hl <- physical sector
	mvi	h,0
	ret

sidetwo:call	fdgsid		;Check out number of sides
	jz	sidea		;Single sided
	pop	psw		;Retrieve adjusted sector
	pop	b
	cma			;Make sector request positive
	inr	a
	mov	c,a		;Make new sector the requested sector
	pop	d
	call	sideone
	mvi	a,80h		;Side two bit
	ora	h		;	and sector
	mov	h,a
	ret

;DJ2D/B First Time Drive Select Routine
;--------------------------------------
;
fdldrv:	sta	fdlog		;Save logical drive
	mov	c,a		;Save drive #
	mvi	a,0		;Have the floppies been accessed yet ?
flopflg	equ	$-1
	ana	a
	jnz	flopok

	mvi	b,17		;Floppies havn't been accessed
	lxi	h,fdboot	;Check if 2D controller is installed
	mvi	a,(jmp)
clopp:	cmp	m		;Must have 17 jumps
	jnz	zret
	inx	h
	inx	h
	inx	h
	dcr	b
	jnz	clopp
	lxi	d,fdinit	;Initialization sequence
	lxi	h,fdorig+7e2h	;Load address
	lxi	b,30		;Byte count
	call	movbyt		;Load controller RAM
	mvi	a,0ffh		;Start 1791
	sta	dreg
	mvi	a,clrcmd	;1791 reset
	sta	cmdreg
	mvi	a,1		;Set 2D initialized flag
	sta	flopflg

flopok:	call	flush		;Flush buffer since we are using it
	lda	fdlog		;Select new drive
	mov	c,a
	call	fdsel
	call	fdlhome		;Recalibrate the drive
	lxi	h,1		;Select sector 1 of track 2
	shld	truesec
	inx	h
	shld	cpmtrk
	xra	a		;Make sure we are doing a read
	sta	rdwr
	call	fill		;Fill in buffer with sector
	jc	zret		;Test for error return
	call	fdstat		;Get status on current drive
	sta	fdldst		;Save drive status
	ani	0ch		;Mask in sector size bits
	push	psw		;Used to select a DPB
	rar
	lxi	h,xlts		;Table of XLT addresses
	mov	e,a
	mvi	d,0
	dad	d
	push	h		;Save pointer to proper XLT
	call	fdget		;Get pointer to proper DPH
	pop	d
	lxi	b,2		;Copy XLT pointer into DPH
	call	movbyt
	lxi	d,8		;Offset to DPB pointer in DPH
	dad	d		;HL <- &DPH.DPB
	push	h
	call	fdgsid		;Get pointer to side flag table entry
	lda	fdldst		;Get drive status
	ani	dblsid		;Check double sided bit
	mov	m,a		;Save sides flag
	lxi	d,dpb128s	;Base for single sided DPB's
	jz	sideok
	lxi	d,dpb128d	;Base of double sided DPB's
sideok:	xchg
	pop	d		;(HL) -> DPB base, (DE) -> &DPH.DPB
	pop	psw		;Offset to correct DPB
	ral
	ral			;Make 0, 10, 20, 30
	mov	c,a
	mvi	b,0		;Make offset
	dad	b		;(hl) is now a DPB pointer
	xchg			;Put proper DPB address in DPH.DPB
	mov	m,e
	inx	h
	mov	m,d
	lxi	h,15		;Offset to DPB.SIZ
	dad	d
	mov	c,m		;Fetch sector size code
fdget:	lda	fdlog		;Return proper DPH
	lxi	d,dphfd0
	jmp	retdph

;DJ2D/B Non-Initial Drive Select Routine
;---------------------------------------
;
fdsel2:	sta	fdlog
	mov	c,a
	jmp	fdsel

;DJ2D/B Home Drive Routine
;-------------------------
;
fdlhome:mvi	c,0		;Select side 0
	call	fdside
	jmp	fdhome		;Do actual home

;DJ2D/B Set Sector Routine
;-------------------------
;
fdssec:	push	b		;Save sector number
	mov	a,b		;Check side select bit
	rlc			;Move high bit to bit zero
	ani	1
	mov	c,a
	call	fdside		;Call select side 0 = side A, 1 = Side B
	pop	b
	jmp	fdsec

fdgsid:	lxi	h,fdlsid	;Side flag table
	lda	fdlog		;Drive number
	push	d
	mov	e,a		;Make offset
	mvi	d,0
	dad	d		;Offset to proper entry
	pop	d
	mov	a,m		;Set up flags
	ora	a
	ret

fdinit:	dw	0		;Initialization bytes loaded onto 2D/B
	dw	1800h		;Head loaded timeout
	dw	0		;DMA address
	db	0		;Double sided flag
	db	0		;Read header flag
	db	07eh		;Drive select constant
	db	0		;Drive number
	db	8		;Current disk
	db	0		;Head loaded flag
	db	9		;Drive 0 parameters
	db	0ffh		;Drive 0 track address
	db	9		;Drive 1 parameters
	db	0ffh		;Drive 1 track address
	db	9		;Drive 2 parameters
	db	0ffh		;Drive 2 track address
	db	9		;Drive 3 parameters
	db	0ffh		;Drive 3 track address
	db	9		;Current parameters
	db	0		;Side desired
	db	1		;Sector desired
	db	0		;Track desired

	db	0		;Header image, track
	db	0		;Sector
	db	0		;Side
	db	0		;Sector
	dw	0		;CRC

fdlog:	db	0
fdldst:	db	0		;Floppy drive status byte

fdlsid:	rept	maxfd
	db	0ffh		;Double sided flag 0 = single, 1 = double
	endm
	endif
	page

	if	(maxfd ne 0) or (maxdm ne 0)	;DJDMA or DJ2DB present?
;***********************************************************************
; Begin Common Floppy Disk Translation tables and DPB's
;******************************************************
;
;Sector translation pointer table
;--------------------------------
;	1) Xlts is a table of address that point to each of the xlt
;	   tables for each sector size.
;
xlts:	dw	xlt128		;Xlt for 128 byte sectors
	dw	xlt256		;Xlt for 256 byte sectors
	dw	xlt512		;Xlt for 512 byte sectors
	dw	xlt124		;Xlt for 1024 byte sectors

;Sector translation tables
;-------------------------
;
;	1) Xlt tables (sector skew tables) for CP/M 2.0. These tables
;	   define the sector translation that occurs when mapping CP/M
;	   sectors to physical sectors on the disk. There is one skew
;	   table for each of the possible sector sizes. Currently the
;	   tables are located on track 0 sectors 6 and 8. They are
;	   loaded into memory in the Cbios ram by the cold boot routine.
;
xlt128:	db	0
	db	1,7,13,19,25
	db	5,11,17,23
	db	3,9,15,21
	db	2,8,14,20,26
	db	6,12,18,24
	db	4,10,16,22
;
xlt256:	db	0
	db	1,2,19,20,37,38
	db	3,4,21,22,39,40
	db	5,6,23,24,41,42
	db	7,8,25,26,43,44
	db	9,10,27,28,45,46
	db	11,12,29,30,47,48
	db	13,14,31,32,49,50
	db	15,16,33,34,51,52
	db	17,18,35,36
;
xlt512:	db	0
	db	1,2,3,4,17,18,19,20
	db	33,34,35,36,49,50,51,52
	db	5,6,7,8,21,22,23,24
	db	37,38,39,40,53,54,55,56
	db	9,10,11,12,25,26,27,28
	db	41,42,43,44,57,58,59,60
	db	13,14,15,16,29,30,31,32
	db	45,46,47,48
;
xlt124:	db	0
	db	1,2,3,4,5,6,7,8
	db	25,26,27,28,29,30,31,32
	db	49,50,51,52,53,54,55,56
	db	9,10,11,12,13,14,15,16
	db	33,34,35,36,37,38,39,40
	db	57,58,59,60,61,62,63,64
	db	17,18,19,20,21,22,23,24
	db	41,42,43,44,45,46,47,48

;Disk Parameter Buffers
;----------------------
;	1) Each of the following tables describes a diskette with the
;	   specified characteristics.
;
;128 byte sectors, single density, and single sided.
;---------------------------------------------------
;
dpb128s:dw	26		;CP/M sectors/track
	db	3		;BSH
	db	7		;BLM
	db	0		;EXM
	dw	242		;DSM
	dw	63		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	16		;CKS
	dw	2		;OFF
	db	1		;128 byte sectors
;
;256 byte sectors, double density, and single sided.
;---------------------------------------------------
;
dpb256s:dw	52		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	1		;EXM
	dw	242		;DSM
	dw	127		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	32		;CKS
	dw	2		;OFF
	db	2		;256 byte sectors
;
;512 byte sectors, double density, and single sided.
;---------------------------------------------------
;
dpb512s:dw	60		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	280		;DSM
	dw	127		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	32		;CKS
	dw	2		;OFF
	db	3		;512 byte sectors
;
;1024 byte sectors, double density, and single sided.
;----------------------------------------------------
;
dp1024s:dw	64		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	299		;DSM
	dw	127		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	32		;CKS
	dw	2		;OFF
	db	4		;1024 byte sectors
;
;128 byte sectors, single density, and double sided.
;----------------------------------------------------;
;
dpb128d:dw	52		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	1		;EXM
	dw	242		;DSM
	dw	127		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	32		;CKS
	dw	2		;OFF
	db	1		;128 byte sectors
;
;256 byte sectors, double density, and double sided.
;---------------------------------------------------
;
dpb256d:dw	104		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	486		;DSM
	dw	255		;DRM
	db	0f0h		;AL0
	db	0		;AL1
	dw	64		;CKS
	dw	2		;OFF
	db	2		;256 byte sectors
;
;512 byte sectors, double density, and double sided.
;---------------------------------------------------
;
dpb512d:dw	120		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	561		;DSM
	dw	255		;DRM
	db	0f0h		;AL0
	db	0		;AL1
	dw	64		;CKS
	dw	2		;OFF
	db	3		;512 byte sectors
;
;1024 byte sectors, double density, and double sided.
;----------------------------------------------------
;
dp1024d:dw	128		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	599		;DSM
	dw	255		;DRM
	db	0f0h		;AL0
	db	0		;AL1
	dw	64		;CKS
	dw	2		;OFF
	db	4		;1024 byte sectors
	endif
	page

	if	maxmw ne 0	;HDDMA controller present ?
;**********************************************************
; Begin the HDDMA Driver (DDRV3)
;*******************************
;
;HDDMA equates
;=============
;
	;Specifications for a Seagate Technology 506
	if	st506
cyl	equ	153		;Number of cylinders
heads	equ	4		;Number of heads per cylinder
precomp	equ	64		;Cylinder to start write precomensation
lowcurr	equ	128		;Cylinder to start low current
stepdly	equ	30		;Step delay (0-12.7 milliseconds)
steprcl	equ	30		;Recalibrate step delay
headdly	equ	0		;Settle delay (0-25.5 milliseconds)
	endif

	;Specifications for a Seagate ST412
	if	st412
cyl	equ	306		;Number of cylinders
heads	equ	4		;Number of heads per cylinder
precomp	equ	128		;Cylinder to start write precomensation
lowcurr	equ	128		;Cylinder to start low current
stepdly	equ	0		;Step delay (0-12.7 milliseconds)
steprcl	equ	30		;Recalibrate step delay
headdly	equ	0
	endif

	;Specifications for an CMI 5619
	if	cm5619
cyl	equ	306		;Number of cylinders
heads	equ	6		;Number of heads per cylinder
precomp	equ	128		;Cylinder to start write precomensation
lowcurr	equ	128		;Cylinder to start low current
stepdly	equ	2		;Step delay (0-12.7 milliseconds)
steprcl	equ	30		;Recalibrate step delay
headdly	equ	0
	endif

sectsiz	equ	7		;Sector size code (must be 7 for this Cbios)
				; 0 =  128 byte sectors
				; 1 =  256 byte sectors
				; 3 =  512 byte sectors
				; 7 = 1024 byte sectors (default)
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

;Drive Specification Table for the HD DMA hard disk controller
;-------------------------------------------------------------
;
mwdst:	db	maxmw*mwlog	;Number of logical drives
	dw	mwwarm		;Warm boot
	dw	mwtran		;Sector translation
	dw	mwldrv		;Select logical drive 1 (First time select)
	dw	mwdrv		;Select logical drive 2 (General select)
	dw	mwhome		;Home current selected drive
	dw	mwseek		;Seek to selected track
	dw	mwsec		;Select sector
	dw	mwdma		;Set DMA address
	dw	mwread		;Read a sector
	dw	mwwrite		;Write a sector
	if	heads gt 2	;Test if drive is big enough for a bad spot map
	dw	mwbad		;Return bad sector map info
	else
	dw	nobad
	endif

	if	mworder ne 1		;no warm boot possible

;HDDMA warm boot dummy
;---------------------------
;	1) If HDDMA is not drive A (i.e. mworder not equal 1) then
;	   it is not possible to warm boot from HDDMA. So routine not needed.
;
mwwarm:	ret				;return if called

	else

;HDDMA Warm Boot Routine
;-----------------------
;
mwwarm:	xra	a
	call	mwdrv		;Select drive A
	call	mwhome		;Home and reset the drive
	lxi	b,0		;Make sure we are on track 0
	call	mwseek
	xra	a
	sta	mwhead		;Select head zero
	sta	mwsectr		;Select sector 1
	lxi	h,buffer	;Load sector 1 into buffer
	shld	dmadma
	call	mwwread		;Read CCP into buffer
	rc			;Return if error
	lxi	d,buffer+200h
	lxi	h,ccp
	lxi	b,200h		;Move 200h bytes
	call	movbyt
	lxi	h,ccp-200h	;Initial DMA address
	push	h
	xra	a
	push	psw		;Save first sector -1
mwwlod:	pop	psw		;Restore sector
	pop	h		;Restore DMA address
	inr	a
	sta	mwsectr
	cpi	6		;Past BDOS ?
	rz			;Yes, all done
	inr	h		;Update DMA address by 1024 bytes
	inr	h
	inr	h
	inr	h
	shld	dmadma
	push	h
	push	psw
	call	mwwread		;Read in a sector
	jnc	mwwlod
	ret			;Return with error

mwwread:mvi	c,retries	;Retry counter
mwwerr:	push	b		;Save the retry count
	call	mwread		;Read the sector
	pop	b
	rnc
	dcr	c		;Update the error count
	jnz	mwwerr		;Keep trying if not too many errors
	stc			;Set error flag
	ret

	endif			;of HDDMA warm boot routine

;HDDMA First Time Drive Select Routine
;-------------------------------------
;
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

	lxi	d,dphmw0	;Start of hard disk DPH's
	lda	mwcurl
	mov	l,a
	mvi	h,0
	dad	h
	dad	h
	dad	h
	dad	h
	dad	d		;(hl) = pointer to DPH
	mvi	c,4		;Return sector size of 1024
	ret

;HDDMA Non-Initial Drive Select Routine
;--------------------------------------
;
mwdrv:	sta	mwcurl
	call	mwdlog
	mov	a,c
	sta	mwdrive		;Save new selected drive
mwsel:	mvi	a,dmanoop
	jmp	mwprep		;Execute disk command

mwdlog:	mvi	c,0
mwllx:	sui	mwlog
	rc
	inr	c
	jmp	mwllx

mwstat:	mvi	a,dmassta	;Sense status operation code
	jmp	mwprep		;Execute disk command

;HDDMA Home Drive Routine
;------------------------
;
mwhome:	call	mwreset		;Reset controller, do a load constants
	lxi	h,dmarg1	;Load arguments
	mvi	m,steprcl	;Load step delay (slow rate)
	inx	h
	mvi	m,headdly	;Head settle delay
	call	mwissue		;Do load constants again
	call	mwptr		;Get pointer to current cylinder number
	mvi	m,0ffh		;Fake at cylinder 65535 for max head travel
	inx	h
	mvi	m,0ffh
	lxi	b,0		;Seek to cylinder 0
	call	mwseek		;Recal slowly
	jmp	mwreset		;Back to fast stepping mode

;HDDMA Return Bad Map Position Routine
;-------------------------------------
;
mwbad:	lxi	h,mwbtab	;Return pointer to bad sector location
	ret

mwbtab:	dw	0		;Track 0
	dw	19		;Head 2, sector 0  = (2 * SPT + 0) + 1

;HDDMA Set Track Routine
;-----------------------
;
mwseek:	call	mwptr		;Get track pointer
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

;HDDMA Set DMA Address Routine
;-----------------------------
;
mwdma:	mov	h,b		;Set DMA address
	mov	l,c
	shld	dmadma
	ret

;HDDMA Set Sector Routine
;------------------------
;
mwsec:	mov	a,c		;Load sector number
	dcr	a		;Range is actaully 0-16
	call	mwdspt		;Figure out head number -> (c)
	adi	mwspt		;Make sector number
	sta	mwsectr
	mov	a,c
	sta	mwhead		;Save head number
	ret

mwdspt:	mvi	c,0		;Clear head counter
mwdsptx:sui	mwspt		;Subtract a tracks worth of sectors
	rc			;Return if all done
	inr	c		;Bump to next head
	jmp	mwdsptx

mwreset:lhld	chan		;Save the command channel for a while
	shld	tempb
	lda	chan+2
	sta	tempb+2
	out	reset		;Send reset pulse to controller
	lxi	h,dmachan	;Address of command channel
	shld	chan		;Default channel address
	xra	a
	sta	chan+2		;Clear extended address byte
	shld	40h		;Set up a pointer to the command channel
	sta	42h
	lhld	dmarg0		;Save the track number
	push	h
	lxi	h,dmasel1	;Load arguments
	lda	mwdrive		;Get the currently selected drive
	ori	03ch		;Raise *step and *dir
	mov	m,a		;Save in drive select register
	lxi	d,5		;Offset to dmarg1
	dad	d
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
	push	psw		;Save status
	lhld	tempb		;Restore memory used for the channel pointer
	shld	chan
	lda	tempb+2
	sta	chan+2
	pop	psw
	ret

;HDDMA Read/Write Sector Routines
;--------------------------------
;
mwread:	mvi	a,dmaread	;Load disk read commnd
	jmp	mwprep

mwwrite:mvi	a,dmawrit	;Load disk write command

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
mwpreps:lda	mwhead		;Load head address
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

	if	0		;Set to 1 for MW error reporter
mwissue:call	mwdoit		;Do desired operation
	rnc			;Do nothing if no error
	push	psw		;Save error info
	call	hexout		;Print status
	call	dspout		;   and a space
	lxi	h,dmachan
	mvi	c,16		;16 bytes of status
mwerr:	push	b
	push	h
	mov	a,m
	call	hexout		;Print a byte of the status line
	call	spout
	pop	h
	pop	b
	inx	h		;Bump command channel pointer
	dcr	c
	jnz	mwerr
	mvi	c,0ah		;Terminate with a CRLF
	call	pout
	mvi	c,0dh
	call	pout
	pop	psw		;Restore error status
	ret

dspout:	call	spout		;Print two spaces
spout:	mvi	c,' '		;Print a space
	jmp	pout

hexout:	push	psw		;Poor persons number printer
	rrc
	rrc
	rrc
	rrc
	call	nibout
	pop	psw
nibout:	ani	0fh
	adi	'0'
	cpi	'9'+1
	jc	nibok
	adi	27h
nibok:	mov	c,a
	jmp	pout

mwdoit	equ	$

	else

mwissue	equ	$		;Do a disk command, handle timeouts + errors

	endif

	lxi	h,dmastat	;Clear status byte
	mvi	m,0
	out	attn		;Start the controller
	lxi	d,0		;Time out counter (65536 retries)
mwiloop:mov	a,m		;Get status
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

mwptr:	lda	mwdrive		;Get currently select drives track address
	rlc
	mov	e,a
	mvi	d,0
	lxi	h,mwtab
	dad	d		;Offset into track table
	ret

mwtran:	mov	h,b
	mov	l,c
	inx	h
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

mwtab	equ	$		;Collection of track addresses
	rept	maxmw
	db	0ffh		;Initialize to (way out on the end of the disk)
	db	0ffh
	endm
	db	0ffh

mwcurl:	db	0		;Current logical drive
mwdrive:db	0ffh		;Currently selected drive
mwhead:	db	0		;Currently selected head
mwsectr:db	0		;Currently selected sector

dmachan	equ	$		;Command channel area
dmasel0:db	0		;Drive select
dmastep:dw	0		;Relative step counter
dmasel1:db	0		;Head select
dmadma:	dw	0		;DMA address
	db	0		;Extended address
dmarg0:	db	0		;First argument
dmarg1:	db	0		;Second argument
dmarg2:	db	0		;Third argument
dmarg3:	db	0		;Fourth argument
dmaop:	db	0		;Operation code
dmastat:db	0		;Controller status byte
dmalnk:	dw	dmachan		;Link address to next command channel
	db	0		;extended address
tempb:	ds	4		;Command Channel Pointer Buffer

	endif
	page

	if	maxhd ne 0	;Want HDC3 or 4 controller included ?
;****************************************************************************
; Begin the HDCA Driver (DDRV4)
;******************************
;
;HDCA equates
;------------
;
hdorg	equ	50h			;Hard Disk Controller origin

hdstat	equ	hdorg			;Disk Status
hdcntl	equ	hdorg			;Disk Control
hdreslt	equ	hdorg+1			;Disk Results
hdcmnd	equ	hdorg+1			;Disk Commands
hdskomp	equ	hdorg+2			;Seek complete clear port (on HDC4)
hdfunc	equ	hdorg+2			;Function port
hddata	equ	hdorg+3			;Data port

;	Status port (50)

tkzero	equ	01h			;Track zero
opdone	equ	02h			;Operation done
complt	equ	04h			;Seek complete
tmout	equ	08h			;Time out
wfault	equ	10h			;Write fault
drvrdy	equ	20h			;Drive ready
index	equ	40h			;Delta index

;	Control port (50)

hdfren	equ	01h			;Enable external drivers
hdrun	equ	02h			;Enable controllers state machine
hdclok	equ	04h			;Clock source control bit, high = disk
hdwprt	equ	08h			;Write protect a drive

;	Result port (51)

retry	equ	02h			;Retry flag

;	Command port (51)

idbuff	equ	0			;Initialize data buffer pointer
rsect	equ	1			;Read sector
wsect	equ	5			;Write sector
isbuff	equ	8			;Initialize header buffer pointer

;	Function port (52)

pstep	equ	04h			;Step bit
nstep	equ	0ffh-pstep		;Step bit mask
null	equ	0fch			;Null command

;	Misc constants

hdrlen	equ	4			;Sector header length
seclen	equ	512			;Sector data length

;
;Device Specification Table for HDCA controller driver
;-----------------------------------------------------
;
hddst:	db	maxhd*hdlog		;Number of logical drives
	dw	hdwarm			;Warm boot
	dw	hdtran			;Sector translation
	dw	hdldrv			;First time select
	dw	hddrv			;General select
	dw	hdhome			;Home current selected drive
	dw	hdseek			;Seek to selected track
	dw	hdsec			;Select sector
	dw	hddma			;Set DMA address
	dw	hdread			;Read a sector
	dw	hdwrite			;Write a sector
	dw	nobad			;No bad sector map

	if	hdorder ne 1		;no warm boot possible

;HDCA warm boot dummy
;---------------------------
;	1) If HDCA is not drive A (i.e. hdorder not equal 1) then
;	   it is not possible to warm boot from HDCA. So routine not needed.
;
hdwarm:	ret				;return if called

	else

;HDCA Warm Boot Routine
;----------------------
;
hdwarm:	call	divlog			;Get physical drive number in (c)
	xra	a
	lxi	h,ccp-200h		;Initial DMA address
	push	h
	sta	head			;Select head zero
	inr	a			; 1 -> (a)
	push	psw			;Save first sector - 1
	call	hdd2			;Select drive
	mvi	c,0
	call	hdhome			;Home the drive
hdwrld:	pop	psw			;Restore sector
	pop	h			;Restore DMA address
	inr	a
	sta	hdsect
	cpi	13			;Past BDOS ?
	rz				;Yes, all done
	inr	h			;Update DMA address
	inr	h
	shld	hdadd
	push	h
	push	psw
hdwrrd:	lxi	b,retries*100h+0	;Retry counter
hdwr:	push	b			;Save the retry count
	call	hdread			;Read the sector
	pop	b
	jnc	hdwrld			;Test for error
	dcr	b			;Update the error count
	jnz	hdwr			;Keep trying if not too many errors
	stc				;Error flag
	ret

	endif				;HDCA warm boot routine

;HDCA Sector Translate Routine
;-----------------------------
;
hdtran:	mov	h,b			;Sector translation is handled via
	mov	l,c			;   physical sector header skewwing
	inx	h
	ret

;HDCA First Time Drive Select Routine
;------------------------------------
;
hdldrv:	sta	hdcur			;Save logical disk
	call	divlog			;Divide by logical disks per drive
	mov	a,c
	sta	hddisk			;Save new physical drive
	call	hdptr			;Get track pointers
	mov	a,m			;Get current track
	inr	a			;Check if -1
	jnz	hdl2			;Nope, allready accessed
	ori	null			;Select drive
	out	hdfunc
	mvi	a,hdfren+hdclok		;Enable drivers
	out	hdcntl
	mvi	c,239			;Wait 2 minutes for disk ready
	lxi	h,0
hdtdel:	dcx	h
	mov	a,h
	ora	l
	cz	dcrc
	jz	zret			;Drive not ready error
	in	hdstat			;Test if ready yet
	ani	drvrdy
	jnz	hdtdel

	if	not fujitsu
	lxi	h,0			;Time one revolution of the drive
	mvi	c,index
	in	hdstat
	ana	c
	mov	b,a			;Save current index level in B
hdinxd1:in	hdstat
	ana	c
	cmp	b			;Loop untill index level changes
	jz	hdinxd1
hdindx2:inx	h
	in	hdstat			;Start counting untill index returns to
	ana	c			;	previous state
	cmp	b
	jnz	hdindx2

	if	m10			;Memorex M10's have 40 ms head settle
	dad	h			;HL*2
	endif

	if	m26			;Shugart M26's have 30 ms head settle
	xra	a			;HL/2 + HL (same as HL*1.5)
	mov	a,h
	rar
	mov	d,a
	mov	a,l
	rar
	mov	e,a
	dad	d
	endif

	shld	settle			;Save the count for timeout delay
	endif

	call	hdhome

hdl2:	lda	hdcur			;Load logical drive
	lxi	d,dphhd0		;Start of hard disk DPH's
	mvi	c,3			;Hard disk sector size equals 512 bytes
	jmp	retdph

dcrc:	dcr	c			;Conditional decrement C routine
	ret

divlog:	mvi	c,0
divlx:	sui	hdlog
	rc
	inr	c
	jmp	divlx

;HDCA Non-Initial Drive Select Routine
;-------------------------------------
;
hddrv:	sta	hdcur
	call	divlog			;Get the physical drive #
hdd2:	mov	a,c
	sta	hddisk			;Select the drive
	ori	null
	out	hdfunc
	mvi	a,hdfren+hdrun+hdclok+hdwprt	;Write protect
	out	hdcntl
	ret

;HDCA Home Disk Routine
;----------------------
;
hdhome:	call	hdptr			;Get track pointer
	mvi	m,0			;Set track to zero
	in	hdstat			;Test status
	ani	tkzero			;At track zero ?
	rz				;Yes

	if	not fujitsu
hdstepo:in	hdstat			;Test status
	ani	tkzero			;At track zero ?
	jz	hddelay
	mvi	a,1
	stc
	call	accok			;Take one step out
	jmp	hdstepo

	else

	xra	a
	jmp	accok
	endif

	if	not fujitsu
hddelay:lhld	settle			;Get hddelay
deloop:	dcx	h			;Wait 20ms
	mov	a,h
	ora	l
	inx	h
	dcx	h
	jnz	deloop
	ret
	endif

;HDCA Set Track Routine
;----------------------
;
hdseek:	call	hdptr			;Get pointer to current track
	mov	e,m			;Get current track
	mov	m,c			;Update the track
	mov	a,e			;Need to seek at all ?
	sub	c
	rz
	cmc				;Get carry into direction
	jc	hdtrk2
	cma
	inr	a
	if	fujitsu
hdtrk2:	jmp	accok
	else
hdtrk2:	call	accok
	jmp	hddelay
	endif

accok:	mov	b,a			;Prep for build
	call	build
sloop:	ani	nstep			;Get step pulse low
	out	hdfunc			;Output low step line
	ori	pstep			;Set step line high
	out	hdfunc			;Output high step line
	dcr	b			;Update repeat count
	jnz	sloop			;Keep going the required # of tracks
	jmp	wsdone

;HDCA Set DMA Address Routine
;----------------------------
;
hddma:	mov	h,b			;Save the DMA address
	mov	l,c
	shld	hdadd
	ret

wsdone:	in	hdstat			;Wait for seek complete to finish
	ani	complt
	jz	wsdone
	in	hdskomp			;Clear sdone bit on an HDCA4
	ret

;HDCA Set Sector Routine for M26 Disk
;------------------------------------
;
	if	m26

hdsec:	mvi	a,01fh			;For compatibility with Cbios revs.
					;  2.3 and 2.4
	ana	c			;Mask in sector number (0-31)
	cz	getspt			;Translate sector 0 to sector 32
	sta	hdsect			;Save translated sector number (1-32)
	mvi	a,0e0h			;Get the head number
	ana	c
	rlc
	rlc
	rlc
	sta	head			;Save the head number
getspt:	mvi	a,hdspt
	ret

	else

;HDCA Set Sector Routine for M10 and M20 Disks
;---------------------------------------------
;
hdsec:	mov	a,c
	call	divspt
	adi	hdspt
	ana	a
	cz	getspt
	sta	hdsect
	mov	a,c
	sta	head
getspt:	mvi	a,hdspt
	dcr	c
	ret

divspt:	mvi	c,0
divsx:	sui	hdspt
	rc
	inr	c
	jmp	divsx
	endif

;HDCA Read Sector Routine
;------------------------
;
hdread:	call	hdprep
	rc
	xra	a
	out	hdcmnd
	cma
	out	hddata
	out	hddata
	mvi	a,rsect			;Read sector command
	out	hdcmnd
	call	process
	rc
	xra	a
	out	hdcmnd
	mvi	b,seclen/4
	lhld	hdadd
	in	hddata
	in	hddata
rtloop:	in	hddata			;Move four bytes
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

;HDCA Write Sector Routine
;-------------------------
;
hdwrite:call	hdprep			;Prepare header
	rc
	xra	a
	out	hdcmnd
	lhld	hdadd
	mvi	b,seclen/4
wtloop:	mov	a,m			;Move 4 bytes
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
	mvi	a,wsect			;Issue write sector command
	out	hdcmnd
	call	process
	rc
	mvi	a,wfault
	ana	b
	stc
	rz
	xra	a
	ret

process:in	hdstat			;Wait for command to finish
	mov	b,a
	ani	opdone
	jz	process
	mvi	a,hdfren+hdrun+hdclok	;Write protect
	out	hdcntl
	in	hdstat
	ani	tmout			;Timed out ?
	stc
	rnz
	in	hdreslt
	ani	retry			;Any retries ?
	stc
	rnz
	xra	a
	ret

hdprep:	in	hdstat
	ani	drvrdy
	stc
	rnz
	mvi	a,isbuff		;Initialize pointer
	out	hdcmnd
	call	build
	ori	0ch
	out	hdfunc
	lda	head
	out	hddata			;Form head byte
	call	hdptr			;Get pointer to current drives track
	mov	a,m			;Form track byte
	out	hddata
	ana	a
	mvi	b,80h
	jz	zkey
	mvi	b,0
zkey:	lda	hdsect			;Form sector byte
	out	hddata
	mov	a,b
	out	hddata
	mvi	a,hdfren+hdrun+hdclok	;Write protect
	out	hdcntl
	mvi	a,hdfren+hdrun+hdclok+hdwprt	;Write protect
	out	hdcntl
	xra	a
	ret

hdptr:	lhld	hddisk			;Get a pointer to the current drives
	mvi	h,0			;   track position
	xchg
	lxi	h,hdtrak
	dad	d
	ret

build:	lda	head			;Build a controller command byte
	ral
	ral
	ral
	ral
	lxi	h,hddisk
	ora	m
	xri	0f0h
	ret

hdcur:	db	0			;Current logical disk
hdadd:	dw	0			;DMA address
hddisk:	db	0			;Current physical disk number
head:	db	0			;Current physical head number
hdsect:	db	0			;Current physical sector number

hdtrak:	db	0ffh			;Track pointer for each drive
	db	0ffh			;All drive default to an uncalibrated
	db	0ffh			;   state (ff)
	db	0ffh

settle:	dw	0			;Time delay constant for head settle
	endif
	page

;********************
; End of Disk Drivers
;********************
;
; Cbios ram locations that don't need initialization.
;====================================================
;
	if	nostand ne 0	;Unallocated writting variables
unaloc:	db	0		;Unallocated write in progress flag
oblock:	dw	0		;Last unallocated block number written
unadrv:	db	0		;Drive that the block belongs to
	endif

cpmsec:	dw	0		;CP/M sector #

cpmdrv:	db	0		;CP/M drive #
cpmtrk: dw	0		;CP/M track #
truesec:dw	0		;Physical sector that contains CP/M sector

error:	db	0		;Buffer's error status flag
bufdrv:	db	0		;Drive that buffer belongs to
buftrk:	dw	0		;Track that buffer belongs to
bufsec:	dw	0		;Sector that buffer belongs to

alttrk:	dw	0		;Alternate track
altsec:	dw	0		;Alterante sector
lastdrv:db	0		;Last selected drive

;***************************************************************;
;								;
; DPB and DPH area.						;
;								;
;***************************************************************;

	if	maxhd ne 0

dphdsk	set	0		;Generate DPH's for the HDCA hard disks
	rept	maxhd
ldsk	set	0
	rept	hdlog
	dphgen	hd,%dphdsk,dpbhd,%ldsk
ldsk	set	ldsk+1
dphdsk	set	dphdsk+1
	endm
	endm

	if	m26 ne 0
dpbhd0:	dw	1024		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	2015		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	1		;OFF
	db	3		;SECSIZ

dpbhd1:	dw	1024		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	2015		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	64		;OFF
	db	3		;SECSIZ

dpbhd2:	dw	1024		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	2047		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	127		;OFF
	db	3		;SECSIZ
	endif

	if	m10 ne 0
dpbhd0:	dw	336		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	1269		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	1		;OFF
	db	3		;SECSIZ

dpbhd1:	dw	336		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	1280		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	122		;OFF
	db	3		;SECSIZ
	endif

	if	m20 ne 0
dpbhd0:	dw	672		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	2036		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	1		;OFF
	db	3		;SECSIZ

dpbhd1:	dw	672		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	2036		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	98		;OFF
	db	3		;SECSIZ

dpbhd2:	dw	672		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	1028		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	195		;OFF
	db	3		;SECSIZ
	endif
	endif			;End of HD DPH's and DPB's

; DPH's for DJ2DB
;----------------
;
	if	maxfd ne 0
dn	set	0
	rept	maxfd
	dphgen	fd,%dn,0,0
dn	set	dn+1
	endm
	endif

	if	maxmw ne 0

;***************************************************************;
;								;
; mwsectp is the number of 128 byte sectors per cylinder.	;
; mwsectp = 72 * heads						;
;								;
; mwtrks is the total number of data cylinders.			;
; mwtrks = tracks - 1						;
;								;
;***************************************************************;

	if	st506 ne 0
	mwsecpt	equ	288		;Sectors per track
	mwtrks	equ	152		;Total data tracks
	endif

	if	st412 ne 0
	mwsecpt	set	288
	mwtrks	set	305
	endif

	if	cm5619 ne 0
	mwsecpt	set	432
	mwtrks	set	305
	endif

dphdsk	set	0		;Generate DPH's for the HDDMA hard disks
	rept	maxmw
ldsk	set	0
	rept	mwlog
	dphgen	mw,%dphdsk,dpbmw,%ldsk
dphdsk	set	dphdsk+1
ldsk	set	ldsk+1
	endm
	endm

off	set	1			;Initial system track offset
trkoff	set	8192/(mwsecpt/8)+1	;The number of tracks in a partition
blocks	set	mwsecpt/8*mwtrks	;The number of blocks on the drive
psize	set	trkoff*(mwsecpt/8)	;The number of blocks in a partition
ldsk	set	0

	rept	blocks/8192	;Generate some 8 megabyte DPB's
	dpbgen	mw,%ldsk,%mwsecpt,5,31,1,2047,1023,0ffh,0ffh,0,%off,4
off	set	off+trkoff
blocks	set	blocks-psize
ldsk	set	ldsk+1
	endm
blocks	set	blocks/4
	if	blocks gt 256	;If there is any stuff left, then use it
blocks	set	blocks-1
	dpbgen	mw,%ldsk,%mwsecpt,5,31,1,%blocks,1023,0ffh,0ffh,0,%off,4
	endif
	endif

;*********************************************************
;Begin Definitions for the Console and List Device Drivers
;*********************************************************
;
;Define Printer Character Constants
;----------------------------------
acr	equ	0Dh		;Carriage return
alf	equ	0Ah		;Line Feed
clear	equ	1Ah		;Clear screen on an ADM 3
xoff	equ	13h		;Xoff character
xon	equ	11h		;Xon character

	if	(contyp eq 2) or (lsttyp ge 2)	;Multio or Wunderbuss

;Multio/Wunderbuss Equates
;-------------------------
;	The following equates will define the Decision I mother
;	board I/O or the Multi I/O environments if needed.
;
;Location Definitions
;--------------------
mbase	equ	48h		;Base address of Multi I/O or Decision I
rbr	equ	mbase		;Read data buffer
thr	equ	mbase		;Tranmitter data buffer
dll	equ	mbase		;Divisor (lsb)
strobe	equ	mbase		;parallel port strobe out
status	equ	mbase		;parallel port status in
dlm	equ	mbase+1		;Divisor (msb)
ier	equ	mbase+1		;Interupt enable register
sensesw	equ	mbase+1		;Sense switches, only in gp 06
data	equ	mbase+1		;parallel port data buffer
clk	equ	mbase+2		;WB14 printer select port
lcr	equ	mbase+3		;Line control register
mcr	equ	mbase+4
lsr	equ	mbase+5		;Line status register
msr	equ	mbase+6
grpsel	equ	mbase+7		;Group select port
;
;Define Transmitter/Reciever Mask Bytes
;--------------------------------------
dr	equ	01h		;Line status DR bit
cts	equ	10h		;Clear to send
dsr	equ	20h		;Data set ready
thre	equ	20h		;Status line THRE bit
dlab	equ	80h		;Divisor latch access bit
;
wls0	equ	1		;Word length select bit 0
wls1	equ	2		;Word length select bit 1 for 8 bit word
stb	equ	4		;Stop bit count - 2 stop bits
;
; Define Modem Control Register bits
;-----------------------------------
dtrenb	equ	1		;DTR enable
rtsenb	equ	2		;RTS enable
;
;Define group select Masks
;-------------------------
spp	equ	0		;select parallel port
s0	equ	01h		;Group number (0-3)
s1	equ	02h
smask	equ	03h
bank	equ	04h
enint	equ	08h
restor	equ	10h		;Printer restore on Multi I/O
busy	equ	20h		;parallel printer busy mask
denable	equ	20h		;Driver enable on Multi I/O
;
;Group Port Assignments
;----------------------
congrð	eqõ	±		;Consolå porô (1=p1¬ 2=p2¬ 3=p3)
lstgrp	equ	3		;Printer port (1=p1, 2=p2, 3=p3)

	endif			;For Multio/Wunderbuss Definitions
	page

;****************************
;Begin Console Device Drivers
;****************************

	if	contyp eq 0
;************************
;Begin Prom Patch (CDRV0)
;************************
;	This driver simply defines the four jumps normally needed to get to
;	your actual console drivers. The assumption is that you already have
;	these drivers in a ROM; And, furthermore, that the ROM'ed drivers
;	exactly match the specs given in the CPM alteration guide for CONIN,
;	CONOUT and CONST. Conint is involked during the cold boot process.
;
conin:	jmp	$		;Console input
conout:	jmp	$		;Console output
const:	jmp	$		;Console input status
conint:	jmp	$		;Console initialization

	endif			;End of Prom Patch Console Routines

	if	contyp eq 1
;***********************************
;Begin Patch Area (128 byte) (CDRV1)
;***********************************
;	This driver provides you with a 128 byte area for patching in your
;	own i/o routines. This first 12 bytes are taken up by jumps to the
;	appropriate routines (CONIN, CONOUT and CONST). See the CPM user
;	reference manual section on system alteration for a description of
;	these routines and the parameter passing conventions.
;
;	The console initialization routine (conint) is usually placed
;	just after the cold boot loader.
;
conin:	jmp	$			;Console input
conout:	jmp	$			;Console output
const:	jmp	$			;Console input status
conint:	jmp	$			;Console initialization
	ds	116			;(reserve the remaining space)

	endif				;End of Patch Area Console Routines

	if	contyp eq 2
;****************************************************
;Begin Multi I/O or Decision I Console Driver (CDRV2)
;****************************************************
;	1) This driver on cold boot will inspect bits 1-3 of the sense
;	   switches.  If the value found is in the range 0-6 then the
;	   console baud rate will be taken from the rate table.  Otherwise
;	   the baud rate will be set from the DEFCON word which is found
;	   just below the regular Cbios jump table.  The standard divisor
;	   table is given below.
;
;		Sense switch: 123  (0 = off, 1 = on)
;			000 = 110
;			001 = 300
;			010 = 1200
;			011 = 2400
;			100 = 4800
;			101 = 9600
;			110 = 19200
;		     defcon = 9600
;
;	2) If you are using a Multio then the switches will not be
;	   available so the baud rate will be taken from DEFCON.
;	
;Console input
;-------------
;
	db	0		;used by swap.com
conin:	call	const		;select console and test for char
	jz	conin
	in	rbr		;Read character
	ani	7fh		;Strip parity
	ret
 
;Console Output
;--------------
;
	db	1		;used by swap.com
conout:	call	conost		;Select console and test status
	jz	conout
	mov	a,c		;Character is in (c)
	ani	7fh
	out	thr		;Output to transmitter buffer
	ret

;Console Status
;--------------
;	1) Returns zero if character is not ready to be read; Otherwise,
;	   this routine returns 255 indicating a ready condition.
;
const:	call	selcon		;Select console
	in	lsr		;Read status register
	ani	dr
	rz			;No charactter ready
	mvi	a,0ffh		;Character ready
	ret

;Console Output Status Routine
;-----------------------------
;
conost:	call	selcon
	in	lsr
	ani	thre
	rz
	mvi	a,0ffh
	ret

;Console Select Routine
;----------------------
;
selcon:	lxi	d,group		;pass to application
	ldax	d
	ori	congrp
	out	grpsel
	ret

	endif			;End of Multio/Wunderbuss Console Driver

	if	contyp eq 3
;**********************************
;Begin DJ2DB Console Driver (CDRV3)
;**********************************
;
;Console Input
;-------------
;
conin:	jmp	fdcin		;Console input

;Console Output
;--------------
;
conout:	mov	a,c
	ani	7fh
	mov	c,a
	jmp	fdcout		;Console output

;Console Status
;--------------
;
const:	call	fdtstat		;Console status
	mvi	a,0ffh
	rz
	inr	a
	ret

	endif			;End of DJ2DB Console Driver

	if	contyp eq 4
;**********************************
;Begin DJDMA Console Driver (CDRV4)
;**********************************
;
;Console Input
;-------------
;
conin:	lxi	h,serin+1	;Serial input status
	xra	a
ci2:	cmp	m		;Wait till 40h deposited at 3fH
	jz	ci2
	mov	m,a		;Clear status
	dcx	h		;Point to input data
	mvi	a,7Fh		;For masking out parity
	ana	m
	ret

;Console Output
;--------------
;
conout:	lxi	h,dmchot+1	;Character output location
	mov	a,c
	ani	7fh
	mov	m,a		;store character in command
	dcx	h		;Back up to start of command
	lxi	d,4		;offset to returned status
	call	dmdoit		;Write a character
	ret

;Console Status
;--------------
;
const:	lda	serin+1		;Pick up serial input status
	ora	a
	rz			;If zero then no character ready
	mvi	a,0FFh		;Set character ready
	ret

;DJDMA Command Strings For Console I/O
;-------------------------------------
;
dmchot:	db	serout		;Serial output command
	db	0		;The character to be output
	db	0		;dummy status
	db	dmhalt		;Halt Command
	db	0		;returned status

	endif			;End of DJDMA Console Driver

	if	contyp eq 5
;****************************************
;Begin Switchboard Console Driver (CDRV5)
;****************************************
;
;Swithboard Equates
;------------------
;
swbase	equ	0		;Base of the SWITCHBOARD

;Console Input
;-------------
;
conin:	in	swbase+2	;Get switchboard status
	ani	4		;Test for data ready
	jz	conin
	in	swbase		;Get a character
	ani	7Fh		;Strip off parity
	ret

;Console Output
;--------------
;
conout:	in	swbase+2	;Check status
	ani	8		;Wait till output buffer empty
	jz	conout
	mov	a,c		;Write a character
	ani	7fh
	out	swbase
	ret

;Console Status
;--------------
;
const:	in	swbase+2	;Get the first ports status
	ani	4		;Mask the data ready bits
	rz			;Return console not ready
	mvi	a,0ffh
conint:	ret			;NULL terminal initialization

	endif			;End of Switchboard Console Driver

	if	contyp eq 6
;***************************************
;Begin North Star Console Driver (CDRV6)
;***************************************
;
;General Information
;===================
;	The following code implements the North Star console I/O system.
;	This system is for users who purchase a Morrow Designs disk
;	system to replace their North Star disk system.  The Mapping of
;	the logical to physical entry points is performed as follows:
;
;	Device name		Left	Right	Parallel
;				serial	serial	port
;
;		Console	CON: =	TTY:	CRT:	UC1:
;		Reader	RDR: =	TTY:	PTR:	UR1:
;		Punch	PUN: =	TTY:	PTP:	UP1:
;		List	LST: =	TTY:	CRT:	UL1:
;
;	For example, to use a printer connected to the right serial port,
;	use the CP/M command:
;
;		STAT LST:=CRT:
;
;	Likewise, the CP/M command "STAT LST:=UL1:" is used if you have a
;	printer connected to the parallel port.
;
;North Star Equates
;==================
;
nsldat	equ	2		;Left serial port data port
nslsta	equ	3		;Left serial port status port

nsrdat	equ	4		;Right serial port data port
nsrsta	equ	5		;Right serial port status port

nsstbe	equ	1		;Transmitter buffer empty status bit
nssrbr	equ	2		;Reciever buffer ready status bit

				;See the 8251 data sheets for more
				;   configuration information.

nslin1	equ	0ceh		;Left serial port initialization # 1
nsrin1	equ	0ceh		;Right serial port initialization # 1
				;76543210 Bit definations
				;11001110 Default configuration
				;xxxxxx00 Synchronous mode
				;xxxxxx01 1X clock rate
				;xxxxxx10 16X clock rate
				;xxxxxx11 64X clock rate
				;xxxx00xx 5 bit characters
				;xxxx01xx 6 bit characters
				;xxxx10xx 7 bit characters
				;xxxx11xx 8 bit characters
				;xxx0xxxx Parity disbable
				;xxx1xxxx Parity enable
				;xx0xxxxx Odd parity generation/check
				;xx1xxxxx Even parity generation/check
				;00xxxxxx Invalid
				;01xxxxxx 1 stop bit
				;10xxxxxx 1.5 stop bits
				;11xxxxxx 2 stop bits

nslin2	equ	37h		;Left serial port initialization # 2
nsrin2	equ	37h		;Right serial port initialization # 2
				;76543210 Bit definations
				;00110111 Default configuration
				;xxxxxxx1 Enable transmitter
				;xxxxxx1x Assert DTR;
				;xxxxx1xx Enable reciever
				;xxxx1xxx Send break character, TxD low
				;xxx1xxxx Reset PE, OE, FE error flags
				;xx1xxxxx Assert RTS;
				;x1xxxxxx Internal reset
				;1xxxxxxx Enter hunt mode (for sync)

nspdat	equ	0		;Parallel data port
nspsta	equ	6		;Parallel status port

nsprbr	equ	1		;Reciever buffer ready status bit
nsptbe	equ	2		;Transmitter buffer empty status bit

nsram	equ	0c0h		;North Star memory parity port,
				;   set to 0 for no North Star RAM

;North Star IOBYTE Implementation
;================================
;	The following code performs the mapping of logical to physical
;	serial I/O devices.  The physical entry points are CONIN, CONOUT,
;	CONIST, RDRIN, PUNOUT, LSTOUT, and LSTOST.  These entry points
;	are mapped via the Intel standard I/O byte (IOBYTE) at location 3
;	in the base page to the low level device drivers.
;
;	Note:  A naming convention has been chosen to reduce label
;	colisions.  The first three characters of a name indicate the
;	device drivers name, the following three characters indicated the
;	function performed by that particular device routine.  The device
;	names are defined and described in the "An Introduction to CP/M
;	Features and Facilities" manual in the section on the STAT
;	command and in the "CP/M Interface Guide" in the IOBYTE section.
;	The device function postfixes are as follows.
;
;		devSET	Initial device setup and initialzation
;		devIN	Read one character from the device
;		devOUT	Write one character to the device
;		devIST	Return the device character input ready status
;		devOST	Return the device character output ready status
;
;	The setup routine initializes the device and returns.  The input
;	routine returns one character in the A register (parity reset).
;	The output routine write one character from the C register.  The
;	input status routine returns in the A register a 0 if the device
;	does not have a character ready for input for 0ffh if a character
;	is ready for input.  The output status routine returns in the A
;	register a 0 if the device is not ready accept a character and a
;	0ffh if the device is ready.  The input and output routines
;	should wait untill the device is ready for the desired operation
;	before the doing the operation and returning.
;
;	Not all of these functions need to be implemented for all the
;	devices.  The following is a table of the entry points needed for
;	each device handler.
;
;		device	setup	input	output	input	output
;		name				status	status
;
;		CON:		CONIN	CONOUT	CONIST
;		RDR:		RDRIN		RDRIST
;		PUN:			PUNOUT
;		LST:			LSTOUT		LSTOST
;
;		TTY:	TTYSET	TTYIN	TTYOUT	TTYIST	TTYOST
;		CRT:	CRTSET	CRTIN	CRTOUT	CRTIST	CRTOST
;		UC1:	UC1SET	UC1IN	UC1OUT	UC1IST
;
;		PTR:	PTRSET	PTRIN		PTRIST
;		UR1:	UR1SET	UR1IN		UR1IST
;		UR2:	UR2SET	UR2IN		UR2IST
;
;		PTP:	PTPSET		PTPOUT
;		UP1:	UP1SET		UP1OUT
;		UP2:	UP2SET		UP2OUT
;
;		LPT:	LPTSET		LPTOUT		LPTOST
;		UL1:	UL1SET		UL1OUT		UL1OST
;
;	The CONIN, CONOUT, CONIST, RDRIN, RDRIST, PUNOUT, LSTOUT, and
;	LSTOST routines are the logical device driver entry points
;	provided by this device mapper.  The other entry names must be
;	provided by the physical device drivers.
;
;Console Input
;-------------
;
conin:	mvi	e,1			;Console input
	call	redir			;	IOBYTE:	76543210
	dw	ttyin			;CON: = TTY:	xxxxxx00
	dw	crtin			;CON: = CRT:	xxxxxx01
	dw	rdrin			;CON: = BAT:	xxxxxx10
	dw	uc1in			;CON: = UC1:	xxxxxx11

;Console Output
;--------------
;
conout:	mvi	e,1			;Console output
	call	redir			;	IOBYTE:	76543210
	dw	ttyout			;CON: = TTY:	xxxxxx00
	dw	crtout			;CON: = CRT:	xxxxxx01
	dw	lstout			;CON: = BAT:	xxxxxx10
	dw	uc1out			;CON: = UC1:	xxxxxx11

;Console Status
;--------------
;
const:	mvi	e,1			;Console input status
	call	redir			;	IOBYTE:	76543210
	dw	ttyist			;CON: = TTY:	xxxxxx00
	dw	crtist			;CON: = CRT:	xxxxxx01
	dw	rdrist			;CON: = BAT:	xxxxxx10
	dw	uc1ist			;CON: = UC1:	xxxxxx11

;Reader Input
;------------
;
rdrin:	mvi	e,7			;Reader input
	call	redir			;	IOBYTE:	76543210
	dw	ttyin			;RDR: = TTY:	xxxx00xx
	dw	ptrin			;RDR: = PTR:	xxxx01xx
	dw	ur1in			;RDR: = UR1:	xxxx10xx
	dw	ur2in			;RDR: = UR2:	xxxx11xx

;Reader Status
;-------------
;
rdrist:	mvi	e,7			;Reader input status
	call	redir			;	IOBYTE:	76543210
	dw	ttyist			;RDR: = TTY:	xxxx00xx
	dw	ptrist			;RDR: = PTR:	xxxx01xx
	dw	ur1ist			;RDR: = UR1:	xxxx10xx
	dw	ur2ist			;RDR: = UR2:	xxxx11xx

;Punch Output
;-----------
;
punoutº	mvé	e,µ			;Puncè output
	call	redir			;	IOBYTE:	76543210
	dw	ttyout			;PUN: = TTY:	xx00xxxx
	dw	ptpout			;PUN: = PTP:	xx01xxxx
	dw	up1out			;PUN: = UP1:	xx10xxxx
	dw	up2out			;PUN: = UP2:	xx11xxxx

;List Output
;-----------
;
lstout:	mvi	e,3			;List output
	call	redir			;	IOBYTE:	76543210
	dw	ttyout			;LST: = TTY:	00xxxxxx
	dw	crtout			;LST: = CRT:	01xxxxxx
	dw	lptout			;LST: = LPT:	10xxxxxx
	dw	ul1out			;LST: = UL1:	11xxxxxx

;List Status
;-----------
;
lstostº	mvé	e,³			;Lisô outpuô status
	call	redir			;	IOBYTE:	76543210
	dw	ttyost			;LST: = TTY:	00xxxxxx
	dw	crtost			;LST: = CRT:	01xxxxxx
	dw	lptost			;LST: = LPT:	10xxxxxx
	dw	ul1ost			;LST: = UL1:	11xxxxxx

;Redirect the I/O
;----------------
;
redir:	lda	iobyte			;Get the INTEL standard iobyte
redir0:	rlc				;Shift the next field in
	dcr	e			;Bump the shift count
	jnz	redir0

redir1:	ani	110b			;Mask the redirection field
	mov	e,a			;Make the word table offset
	mvi	d,0
	pop	h			;Get the table base
	dad	d			;Offset into our table
	mov	a,m			;Load the low level i/o routine pointer
	inx	h
	mov	h,m
	mov	l,a
	pchl				;Execute the low level i/o driver

;Left serial port routines.  Use TTY: device.
;--------------------------------------------
;
ttyin:	in	nslsta			;Read a character
	ani	nssrbr
	jz	ttyin			;Wait till a character is ready
	in	nsldat			;Get the character
	ani	7fh			;Strip parity
	ret

ttyout:	in	nslsta			;Write a character
	ani	nsstbe
	jz	ttyout			;Wait till the buffer is empty
	mov	a,c			;Write the character
	ani	7fh
	out	nsldat
	ret

ttyist:	in	nslsta			;Return input buffer status
	ani	nssrbr
	rz				;Return not ready
	mvi	a,0ffh
	ret				;There is a character ready

ttyost:	in	nslsta			;Return output buffer status
	ani	nsstbe
	rz				;Return not ready
	mvi	a,0ffh
	ret				;Return ready

;Right serial port routines.  Use CRT:, PTR:, and PTP: devices.
;--------------------------------------------------------------
;
crtin:
ptrin:	in	nsrsta			;Read a character
	ani	nssrbr
	jz	crtin			;Wait till a character is ready
	in	nsrdat			;Get the character
	ani	7fh			;Strip parity
	ret

crtout:
ptpout:	in	nsrsta			;Write a character
	ani	nsstbe
	jz	crtout			;Wait till the buffer is empty
	mov	a,c			;Write the character
	ani	7fh
	out	nsrdat
	ret

crtist:
ptrist:	in	nsrsta			;Return input buffer status
	ani	nssrbr
	rz				;Return not ready
	mvi	a,0ffh
	ret				;There is a character ready

crtost:	in	nsrsta			;Return output buffer status
	ani	nsstbe
	rz				;Return not ready
	mvi	a,0ffh
	ret				;Return ready

;Parallel Port Routines
;----------------------
;	Use UC1: UR1: UP1: UP2: LPT: and UL1: devices.
;
uc1in:
ur1in:
ur2in:	in	nspsta			;Read a character
	ani	nsprbr
	jz	uc1in			;Wait till a character is ready
	in	nspdat			;Get the character
	push	psw
	mvi	a,30h			;Reset the parallel input flag
	out	nspsta
	pop	psw
	ani	7fh			;Strip parity
	ret

uc1out:
up1out:
up2out:
lptout:
ul1out:	in	nspsta			;Write a character
	ani	nsptbe
	jz	uc1out			;Wait till the buffer is empty
	mvi	a,20h			;Reset the parallel output flag
	out	nspsta
	mov	a,c			;Write the character, strobe bit 7
nspout:	ori	80h
	out	nspdat
	ani	7fh
	out	nspdat
	ori	80H
	out	nspdat
	ret

uc1ist:
ur1ist:
ur2ist:	in	nspsta			;Return input buffer status
	ani	nsprbr
	rz				;Return not ready
	mvi	a,0ffh
	ret				;Return ready

lptost:		
ul1ost:	in	nspsta			;Return output buffer status
	ani	nsptbe
	rz				;Return not ready
	mvi	a,0ffh
	ret				;Return ready

	endif				;North Star I/O configuration
	page

;*************************
;Begin List Device Drivers
;*************************
;
	if	lsttyp eq 0
;*******************************
;Begin Prom Patch Driver (LDRV0)
;*******************************
;	The driver entries LSTOUT and LSTOST are defined in the CP/M
;	alternation guide (e.g.  Input parameters are in register C and
;	results are returned in register A). The LSTSET routine is used
;	for initialization code. It should execute a RET when complete.
;
;	The LSTSET routine could be placed just below the CBOOT routine.
;	This space (below CBOOT) is recyled for use as a disk buffer
;	after CBOOT is done.
;
;	These routines all point to lstskp initially so that the system
;	won't hang up, waiting for a non-existant list device to become
;	ready.
;
lstout:	jmp	lstskp		;Printer output
lstost:	jmp	lstskp		;Printer output status
lstset:	jmp	lstskp		;Printer initialization
lstskp:	ret

	endif			;End of Patch Area for List Drivers

	if	lsttyp eq 1
;*******************************************
;Begin Patch Area Driver (128 bytes) (LDRV1)
;*******************************************
;	The driver entries LSTOUT and LSTOST are defined in the CP/M
;	alternation guide (e.g.  Input parameters are in register C and
;	results are returned in register A). The LSTSET routine is used
;	for initialization code. It should execute a RET when complete.
;
;	The LSTSET routine could be placed just below the CBOOT routine.
;	This space (below CBOOT) is recyled for use as a disk buffer
;	after CBOOT is done.
;
;	These routines all point to lstskp initially so that the system
;	won't hang up, waiting for a non-existant list device to become
;	ready.
;
lstout:	jmp	lstskp		;Printer output
lstost:	jmp	lstskp		;Printer output status
lstset:	jmp	lstskp		;Printer initialization
lstskp:	ret
	ds	118		;(reserve the remaining space)

	endif			;End of Patch Area for List Drivers

	if	(lsttyp ge 2) and (lsttyp le 5)
;*****************************************************
;Begin Other List Devices (LDRV2, LDRV3, LDRV4, LDRV5)
;*****************************************************
;	All other list devices are Multio/Wunderbuss Serial I/O With
;	different types of i/o protocols.
;	Altered rev E4, lsttyp 2 thru 5 always include centronics punch
;	NOTE: first instruction in output routine must be call to status
;	so swap program can re-assign list status routine.
;	Also, the pointer to the device name string must be just ahead of
;	the entry point
;
;List Output
;-----------
	db	2		;used by swap.com
;
lstout:	call	lstost		;Check printer status
	ora	a
	jz	lstout		;Loop if not ready

	mov	a,c		;Print the character
	out	thr
	ret

;List Status
;-----------
;
lstost:	call	sellst		;Printer status routine
	in	lsr		;Check if transmitter buffer empty
	ani	thre
	rz			;Return busy if buffer is not empty

	lhld	lstand		;Fetch handshake mask bits
	in	msr		;Get MODEM Status Register
	ana	l		;Strip out hand-shake lines
	xra	h		;Invert status
	rz			;Return busy if printer is busy

	lda	lastch		;Get last character recieved from the printer
	mov	b,a
	in	lsr		;Check for a character from the printer
	ani	dr
	jz	xskip		;Skip if no character present
	in	rbr		;Get the character
	ani	7fh		;Strip parity
	sta	lastch		;Save last character recieved
	mov	b,a
xskip:	mov	a,b
	sui	xoff		;Check for Xoff char (control S)
	jnz	xsdone		;Printer ready
	ret			;Printer not ready (return zero)

;Group select routines
;---------------------
;
sellst:	lxi	d,group		;pass to application
	ldax	d		;Select printer group
	ori	lstgrp
	out	grpsel
	ret

xsdone:	mvi	a,0ffh		;Printer ready for data
	ret

;Reader Input
;-----------
;
	db	4		;used by swap.com
rdrin:
	call	rdrist
	jz	rdrin		;wait for char avail

	in	rbr		;get data
	ani	7fh
	ret

rdrist:
	call	sellst		;it's the list device kiddies
	in	lsr		;check for char
	ani	dr		;data ready?
	rz			;exit false

	mvi	a,0ffh		;true
	ret

;Punch Output
;-----------
	db	3		;used by swap.com
;
punout:	call	punost		;select group zero, sensesw
	jz	punout
	in	sensesw		;read motherboard switches
	cpi	0FFh		;FF means Multio
	jz	pmult

	mov	a,c		;Print the character
	out	data
	mvi	a,0BFh
	out	strobe		;strobe low (asserted)
	xthl
	xthl			;stall 10 us
	mvi	a,0FFH
	out	strobe		;strobe high (inactive)
	ret

pmult:	mov	a,c		;Print the character
	out	strobe		;really 'data'
	mvi	a,0BFh
	out	data		;strobe low (asserted) (really strobe)
	xthl
	xthl			;stall 10 us
	mvi	a,0FFH
	out	data		;strobe high (inactive)	(really strobe)
	ret

;Punch Status
;-----------
;
punost:	lxi	d,group		;pass to application
	ldax	d
	out	grpsel		;select parallel printer group
	in	status
	ani	busy
	rz			;Return busy if buffer is not empty
	mvi	a,0FFh
	ret			;return not busy
	endif			;Multio Wbio Serial Drivers
	page

;************************
;Start of the Disk Buffer
;************************
;	All of the Routines following this equate will be overwritten
;	by disk accesses.
;
buffer	equ	$
	page

;***********************************************
;Console and list device initialization routines
;***********************************************
;
	if	contyp eq 2
;*********************************************************************
;Begin Multio I/O or Wunderbus Console Initialization Routine (CIDRV2)
;*********************************************************************
;	This routine reads the sense switch on the WB-14 and sets
;	the speed accordingly.
;
conint:	call	selg0		;Select group 0
	in	sensesw		;Get sense switch (ff on a Multio)
	push	psw
	call	selcon		;Select console
	pop	psw
	push	psw
	call	tini0		;Initialize the console
	pop	psw
	push	psw
	call	selrdr		;Select the reader/punch
	pop	psw
	call	tini0		;Initialize the reader/punch
	ret

tini0:	ani	0e0h		;Mask in upper three bits
	rlc			;Move into lower 3 bits
	rlc
	rlc
	cpi	7		;check for sense = 7 (Default setting)
	jz	dfbaud		;Use default baud rate

	lxi	h,btab		;Pointer to baud rate table
	add	a		;Table of words so double
	mov	e,a		;Make a 16 bit number into (de)
	mvi	d,0
	dad	d		;Get a pointer into baud rate table
	mov	e,m		;Get lower byte of word
	inx	h		;Bump to high byte of word
	mov	d,m		;Get upper byte. (de) now has divisor
	jmp	setit		;Set baud rate

dfbaud:	lhld	defcon		;Use default baud rate
	xchg

	;Enable divisor access latch
setit:	mvi	a,dlab+wls1+wls0+stb
	out	lcr		;Set the baud rate in (de)
	mov	a,d
	out	dlm		;Set upper divisor
	mov	a,e
	out	dll		;Set lower divisor

	;Clear Divisor latch
	mvi	a,wls1+wls0+stb
	out	lcr
	xra	a
	out	ier		;Set no interrupts
	out	lsr		;Clear status
	mvi	a,dtrenb+rtsenb	;Enable DTR and RTS outputs to terminal
	out	mcr
	in	msr		;Clear MODEM Status Register
	in	lsr		;Clear Line Status Register
	in	rbr		;Clear reciever buffers
	in	rbr
	ret

selg0:	lda	group		;Select group zero
	out	grpsel
	ret

selrdr:	lda	group		;Select reader/punch group
	ori	5-lstgrp	;Use 'other' serial port
	out	grpsel
	ret

btab:	dw	1047		;110 Baud	000
	dw	384		;300		001
	dw	96		;1200		010
	dw	48		;2400		011
	dw	24		;4800		100
	dw	12		;9600		101
	dw	6		;19200		110
				;DEFCON		111

	endif			;End Multi I/O, Decision I Con Init

	if	contyp eq 3
;***************************************************
;Begin DJ2DB Console Initialization Routine (CIDRV3)
;***************************************************
;
conint:	call	fdtstat		;Clean input buffer
	rnz			;All empty
	call	fdcin
	jmp	conint

	endif			;End 2D/B console Initialization

	if	contyp eq 4
;***************************************************
;Begin DJDMA Console Initialization Routine (CIDRV4)
;***************************************************
;
conint:	call	dminit		;See if controller present
	rc			;No controller, return
	lxi	h,0		;clear
	shld	serin		;initialize no char present
	lxi	h,dmaci		;Console initialization sequence
	lxi	d,3		;Halt offset
	call	dmdoit
	ret

;DJDMA Initialization Command String
;-----------------------------------
;
dmaci:	db	senabl		;Enable serial input
	db	1
	db	dmhalt
	db	0

	endif			 ;End DJDMA Console intialization

	if	contyp eq 6
;********************************************************
;Begin North Star Console Initialization Routine (CIDRV6)
;********************************************************
;	Initialize the North Star Mother board, left serial port, right
;	serial port, and North Star RAM parity.
;
	;Initialize mother board
conint:	xra	a		;Set up the parallel port + motherboard
	out	6
	out	6
	out	6
	out	6

	mvi	a,30h		;Reset the parallel port input flag
	out	nspsta
	mvi	a,60h		;Set the parallel port output flag
	out	nspsta
	mvi	a,acr		;Force a CR out the parallel port
	call	nspout

	;Initialize the left serial port
	mvi	a,nslin1	;See the equates for bit definations
	out	nslsta
	mvi	a,nslin2
	out	nslsta
	xra	a		;Clear the input/output buffers
	out	nsldat
	in	nsldat
	in	nsldat

	;Initialize the right serial port
	mvi	a,nsrin1	;See the equates for bit definations
	out	nsrsta
	mvi	a,nsrin2
	out	nsrsta
	xra	a		;Clear the input/output buffers
	out	nsrdat
	in	nsrdat
	in	nsrdat

	if	nsram ne 0	;Reset parity on North Star RAMs
	mvi	a,40h		;Disable parity logic
	out	nsram
	lxi	h,0		;Starting address

nset0:	mov	a,m		;Get a byte
	mov	m,a		;Rewrite, set proper parity
	inr	l		;Bump the address pointer
	jnz	nset0

nset1:	inr	h		;Skip to the next memory page
	jz	nset2		;Skip if all done 
	lxi	d,$ + 100h	;fix for assem with rmac
	mov	a,d
;	mvi	a,(high $) + 1	;Is the pointer above us?
	cmp	h		;Set carry if pointer is <= our page+1
	jc	nset0		;Reset the next pages parity
	mov	a,m		;Test for a PROM or no memory
	mov	b,a		;Save the original byte
	cma			;See if this location will change
	mov	m,a
	cmp	m		;Test for a change
	mov	m,b		;Restore the original value
	jz	nset0		;Value complemented, must be RAM
	ora	a		;Test for no memory present
	jz	nset1		;Skip to the next page if no memory
	lxi	d,700h		;Skip 2K bytes of 'PROM'
	dad	d
	jnc	nset1		;Do a page check if no overflow

nset2:	mvi	a,41h		;Re-enable parity on the memory boards
	out	nsram
	endif

crtset:	;Null routines
ptrset:
ptpset:
uc1set:
ur1set:
ur2set:
up1set:
up2set:
lptset:
ul1set:	ret

	endif			;End North Star Initialization
	page

	if	(lsttyp ge 2) and (lsttyp le 5)
;*******************************************************************
;Begiî Multiï I/Ï oò Wunderbusó Lisô anä Puncè Initializatioî 
;Routine
;*******************************************************************
;
lstset:	call	sellst		;Select printer group
	mvi	a,dlab		;Access divisor latch
	out	lcr
	lhld	deflst		;Get LST: baud rate divisor
	mov	a,h
	out	dlm		;Set upper baud rate
	mov	a,l
	out	dll
	mvi	a,stb+wls0+wls1	;2 stop bits + 8 bit word
	out	lcr
	mvi	a,dtrenb+rtsenb	;DTR + RTS enabled
	out	mcr
	in	rbr		;Clear input buffer
	xra	a
	out	ier		;No interrupts
	;fall thru to centronics init

;*******************************************************************
;Begin Multio I/O or Wunderbuss Parallel List Initialization Routine
;*******************************************************************
;
punset:	lda	group
	out	grpsel		;select parallel port
	in	sensesw		;read motherboard switches
	cpi	0FFh		;FF is Multio
	jz	imult
	mvi	a,0FFh
	out	strobe		;turn strobes off
	mvi	a,0C0h
	out	clk		;turn on drivers
	mvi	a,07Fh
	out	strobe		;assert restore (low true)
	mvi	a,0FFh
	out	strobe		;inactivate restore
	ret

imult:	lda	group
	ori	denable
	sta	group		;turn parallel port drivers on
	out	grpsel		;select parallel port
	mvi	a,0C0h
	out	strobe		;turn data strobe off
	lda	group
	ori	restor
	out	grpsel		;assert restore
	lda	group
	out	grpsel		;de-assert restore
	ret
	endif			;End Multio/Wbio punch / list init

	page

;**********************
;Begin Cold Boot Loader
;**********************
;	Cboot is the cold boot loader. All of CP/M has been loaded in
;	when control is passed here.
;
cboot:	lxi	sp,tpa			;Set up stack

	xra	a			;Clear cold boot flag
	sta	cwflg
	sta	group			;Clear group select byte
	sta	cpmdrv			;Select disk A:
	sta	cdisk

	lxi	h,cbios+3		;Patch cold boot to warm code
	mov	a,h
	sta	bpage			;set CBIOS base page number
	shld	cbios+1

	lda	iobyt			;Initialize the IOBYTE
	sta	iobyte

	xra	a
	lxi	d,badmap		;Clear out bad map
	stax	d
	lxi	h,badmap+1
	lxi	b,9*badsiz		;32 map entries
	call	movbyt
	mvi	m,0ffh			;End marker

	if	contyp ne 6		;Non IOBYTE inits
	call	conint			;Initialize the terminal
	call	lstset			;Initialize the list device

	else				;Do IOBYTE inits
	lxi	h,devset		;Device setup routine pointer table
cboot0:	mov	e,m			;Load a routine address
	inx	h
	mov	d,m
	inx	h
	mov	a,d			;Test for the end of the table
	ora	e
	jz	cboot2
	push	h			;Save the table pointer
	lxi	h,cboot1		;Return address
	push	h
	xchg
	pchl				;'CALL' a device setup routine
cboot1:	pop	h			;Restore the table pointer
	jmp	cboot0

;Device setup routine pointers
;-----------------------------
devset:	dw	conint, crtset, uc1set
	dw	ptrset, ur1set, ur2set
	dw	ptpset, up1set, up2set
	dw	lptset, ul1set, 0

cboot2	equ	$
	endif

	lxi	h,prompt		;Prep for sending signon message
	call	message			;Send the prompt
	jmp	gocpm

;Signon message output during cold boot
;--------------------------------------
;	Print a message like:
;
;	Morrow Designs 48K CP/M 2.2 E4
;	AB: DJ/DMA 8", CD: DJ/DMA 5 1/4", E: HDC/DMA M5
;
;Print String for the first line of the Sign-on message
;------------------------------------------------------
prompt:	db	80h, clear		;Clean buffer and screen
	db	acr, alf, alf
	db	'Morrow Designs '
	db	'0'+msize/10		;CP/M memory size
	db	'0'+(msize mod 10)
	db	'K CP/M '		;CP/M version number
	db	cpmrev/10+'0'
	db	'.'
	db	(cpmrev mod 10)+'0'
	db	' '
	db	(revnum/10)+'A'-1
	db	(revnum mod 10)+'0'
	db	acr, alf

;Macros To generate the second line of the sign-on message
;---------------------------------------------------------
msdrv	set	0			;Start with drive A:

msbump	macro	ndrives			;Print a drive name
	if	dn gt 1
	db	', '
	endif
	rept	ndrives
	db	msdrv+'A'
msdrv	set	msdrv+1
	endm
	db	': '
	endm

prhex	macro	digit			;Write a byte in hex
	prnib	digit/10h
	prnib	digit
	endm

prnib	macro	digit			;Write a digit in hex
temp	set	digit and 0fh

	if	temp lt 10
	db	temp + '0'
	else
	db	temp - 10 + 'A'
	endif
	endm

dn	set	1			;Generate the drive messages

	rept	16			;Run off at least 16 drives

	if	dn eq hdorder		;Generate the HDCA's message
	msbump	maxhd*hdlog
	db	'HDCA '
	if	maxhd gt 1
	db	'(', maxhd+'0', ')'
	endif
	if	m10 ne 0
	if	m10m ne 0
	db	'Memorex'
	else
	db	'Fujitsu'
	endif
	db	' M10'
	endif
	if	m20 ne 0
	db	'Fujitsu M20'
	endif
	if	m26 ne 0
	db	'Shugart M26'
	endif
	endif

	if	dn eq mworder		;Generate the HDDMA's message
	msbump	maxmw*mwlog
	db	'HDC/DMA'
	if	mwquiet eq 0
	db	' '
	if	maxmw gt 1
	db	'(', maxmw+'0', ')'
	endif
	if	st506 ne 0
	db	'M5'
	endif
	if	st412 ne 0
	db	'M10'
	endif
	if	cm5619 ne 0
	db	'M16'
	endif
	endif
	endif

	if	dn eq fdorder		;Generate the 2D/B message
	msbump	maxfd
	db	'DJ2D/B @'
	prhex	fdorig/100h
	prhex	fdorig
	endif

	if	dn eq dmorder		;Generate the DJDMA 8 message
	msbump	maxdm
	db	'DJ/DMA 8"'
	endif

	if	dn eq mforder		;Generate the DJDMA 5 1/4 message
	msbump	maxmf
	db	'DJ/DMA 5 1/4"'
	endif

dn	set	dn+1
	endm

	db	acr,alf
	db	0			;End of message
	page

;Debugging checks
;----------------
codend	equ	$
savln	equ	codend+300h		;for movcpm
codelen	equ	($ - cbios)		;Length of Cbios code

	if	codelen gt 1000h	;Test for SYSGEN problems
	'FATAL ERROR, system is too big for SYSGEN rev. 4.X'
dbgtmp	set	codelen		;Cbios code length   !   <debug>
	endif

	if	debug
dbgtmp	set	codelen		;Cbios code length   !   <debug>
	endif


;Reserve the space for the disk buffer
;-------------------------------------

	ds	512-($-buffer)		;Buffer for 512 byte sectors
	if	(maxfd ne 0) or (maxdm ne 0) or (maxmw ne 0) or (maxmf ne 0)
	ds	512			;Additional space for 1k sector devices
	endif
	page

;******************************
;Begin Uninitialized Data Areas
;******************************
;
;Bad Map Space
;=============
;	Each bad map entry consists of 9 bytes:
;		Logical drive number (1 byte)
;		Track number of bad sector (2 bytes)
;		Sector number of bad sector (2 bytes)
;		Track number of alternate sector (2 bytes)
;		Sector number of alternate sector (2 bytes)
;
badmap:	ds	badsiz*9+1		;32 entries + end marker

;Directory Buffer
;================
;
dirbuf:	ds	128			;Directory buffer

;Allocation and checked directory table area
;============================================
;
;DJDMA 8" Drives
;---------------
;
	if	maxdm ne 0
	;Drive_0
csvdm0:	ds	64
alvdm0:	ds	75

	;Drive_1
csvdm1:	ds	64
alvdm1:	ds	75

	;Drive_2
csvdm2:	ds	64
alvdm2:	ds	75

	;Drive_3
csvdm3:	ds	64
alvdm3:	ds	75

;dn	set	0
;	rept	maxdm
;	alloc	dm,%dn,75,64
;dn	set	dn+1
;	endm
	endif

;DJDMA 5" Drives
;---------------
;
	if	maxmf ne 0
	;Drive_4
csvmf0:	ds	48
alvmf0:	ds	25

	;Drive_5
csvmf1:	ds	48
alvmf1:	ds	25

	;Drive_6
csvmf2:	ds	48
alvmf2:	ds	25

	;Drive_7
csvmf3:	ds	48
alvmf3:	ds	25

;dn	set	0
;	rept	maxmf
;	alloc	mf,%dn,25,48
;dn	set	dn+1
;	endm
	endif

;DJ2DB Drives
;------------
;
	if	maxfd ne 0
dn	set	0
	rept	maxfd
	alloc	fd,%dn,75,64
dn	set	dn+1
	endm
	endif

;HDDMA Drives
;------------
;
	if	maxmw ne 0
dn	set	0
trkoff	set	8192/(mwsecpt/8)+1
psize	set	trkoff*(mwsecpt/8)
	rept	maxmw
blocks	set	mwsecpt/8*mwtrks

	rept	blocks/8192		;Generate some 8 megabyte ALV's
	alloc	mw,%dn,256,0
blocks	set	blocks-psize
dn	set	dn+1
	endm

blocks	set	blocks/4

	if	blocks gt 256		;Use the remainder
blocks	set	blocks-1
alv	set	(blocks/8)+1
	alloc	mw,%dn,%alv,0
dn	set	dn+1
	endif

	endm

	endif

;HDCA Drives
;-----------
;
	if	maxhd ne 0
dn	set	0
	rept	maxhd
	if	m26 ne 0
	alloc	hd,%dn,252,0
dn	set	dn+1
	alloc	hd,%dn,252,0
dn	set	dn+1
	alloc	hd,%dn,256,0
dn	set	dn+1
	endif

	if	m10 ne 0
	alloc	hd,%dn,159,0
dn	set	dn+1
	alloc	hd,%dn,161,0
dn	set	dn+1
	endif

	if	m20 ne 0
	alloc	hd,%dn,255,0
dn	set	dn+1
	alloc	hd,%dn,255,0
dn	set	dn+1
	alloc	hd,%dn,129,0
dn	set	dn+1
	endif
	endm

	endif
	page

;Debugging Aids
;--------------
;
bioslen	equ	(high ($ - cbios)) + 1	;BIOS length in pages

	if	bioslen gt biosln	;Test for overflow
	'FATAL ERROR, system overflow.  BIOSLN must be at least'
dbgtmp	set	bioslen		;BIOSLN! <debug>
	endif

	if	debug
dbgtmp	set	biosln		;Current BIOSLN! <debug>
	if	biosln gt bioslen
dbgtmp	set	bioslen		;Optimal BIOSLN! <debug>
	endif
	endif
	page

;Revision History
;================
;
;
;  Date    Programmer	Description
;
;  9 17 83 west       * Public release of E.4 for dj/dma and hdc/dma.
;  9 13 83 west		repaired dj console init/drivers
;  9 12 83 west		multio initialization selects switches correctly
;  8 23 83 west		all multio/wbio devices have names for swap.com
;  8 22 83 west		PREP: now allows normal seeks to track zero
;  8 21 83 west		all multio/wbio character devices return pointer
;			to 'group' in [de] to TPA program	
;  8 15 83 west		all multio/wbio serial list have centronics punch
;			and rdr: implemented as list input
;  5  5 83 F.R.K.	add parallel centronics printer thru Mulito/Wbio
;  3 28 83 F.R.K.	add dbl-sided soft sector, fix for rmac
;  1 18 83 JZ		Did a cosmetic restructuring of the code Dealing
;			with the disks at both the hi and lo levels.
;  1 17 83 JZ		Now reads 5" soft sectored single density disks
;  1 16 83 JZ		Patched up various holes in the disk error handeling
;			routines in the Hi-Level routines SETDRV, READ, and
;			WRITE and in the DJDMA Drivers
;*11 20 82 Marc		Public release of revision E.31

	end
