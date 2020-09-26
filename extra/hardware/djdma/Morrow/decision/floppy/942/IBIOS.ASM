*************************************************************************
*									*
* Morrow Designs IBIOS for CP/M Version 2.2.				*
* Copyright (c) 1982, Morrow Designs					*
*									*
* Thió CBIOÓ haó beeî reconfigureä sï thaô iô caî ruî iî thå		*
* INSTALÌ environment®  Generallù thió meanó thaô alì thå consolå	*
* drivers¬ lisô drivers¬ anä thå warí booô codå iî thå disë driveró	*
* haó beeî removed®  Also¬ systeí dependanô codå haó beeî modified®	*
* (systeí dependanô codå ió codå thaô useó memorù locationó belo÷	*
* thå IBIOS oò codå thaô directlù affectó thå placemenô oæ variouó	*
* codå moduleó iî memory)						*
*									*
* Noteº  Onå importanô execption®  Thå DJDMÁ driveró uså addresseó	*
* 50è tï 5Bè foò commanä channeì information®  Thió areá ió		*
* 'reserved' (and not used) by CP/M.					*
*									*
* Anotheò noteº Thå IBIOS'ó colä booô jumð ió no÷ á pointeò tï thå	*
ª HDDMÁ commanä channel®  Thió pointeò waó formerlù aô 40è tï 42h®	*
*									*
* This IBIOS can be configured to run with the following devices. 	*
* The disks may be configured to run with any or all of the disk	*
* systems.  The logical order of the disks can be set to any order.	*
*									*
* Disk systems:								*
*	HDC3 10, 20 and 26 megabyte hard disks.				*
*	HDDMA 5, 10, 16, megabyte hard disk systems.			*
*	DJDMA floppy disk controller with 8 and 5 1/4 inch disks.	*
*	DJ 2D/B floppy disk controller with 8 inch disks.		*
*									*
* Written by Les Kent and Marc Kupper  		3/4/82			*
*									*
*  Date    Programmer	Description					*
*									*
* 12 30 82 Marc		Fixed bad map error return in GETBAD		*
**11 10 82 Marc		Public release of revision E.31			*
* 11  9 82 Marc		Reduced bad map size to 1 for non MW systems	*
* 10 18 82 Marc		Fixed SETHIGH for 2 sided DJDMA 8 inch disks	*
**10  1 82 Marc		Public release of revision E.3			*
*  9 29 82 Marc		40H now points to the HDDMA command channel	*
*  9 28 82 Marc		MW's now have 1024 directory entries		*
*  9 28 82 Marc		Deleted the Centronics drivers			*
*  9 27 82 Marc		Changed login message to look like a label	*
*  9 27 82 Marc		Changed the login messages to say M5, M10, ...	*
*  9 27 82 Marc		Redefined the dparam table structure		*
*  9 22 82 Marc		Added a serial console for the Switchboard	*
*  9 22 82 Marc		Added initialization code for serial group 2	*
*  9 22 82 Marc		Added sector size byte to the hdca DPB's	*
*  9 22 82 Marc		Added sector size parameter to DPBGEN		*
*  9  9 82 Marc		Fixed system length checks for 64K systems	*
*  9  9 82 Marc		SETHIGH was botching 2 sided DPB pointers	*
*  8 31 82 Marc		Changed TRACKS in HD driver to HDTRAK		*
*  8 27 82 Marc		Added code/system length checker		*
*  8 27 82 Marc		mwreset save/restores the track number		*
*  8 26 82 Marc		mwreset now sets *step and *dir	for CMI		*
*  8 20 82 Marc		Added 'equ'ed handshaking to the serial LST:	*
*  8 19 82 Marc		Removed clock switching code from HDCA driver	*
*  8 18 82 Marc		Added handshake configuration code		*
*  8 18 82 Marc		Added handshake configuration bytes		*
*  8 18 82 Marc		Removed 'equ'ed handshaking from LST:		*
*  8 12 82 Marc		Added configuration entries for a0 & d0		*
*  8 11 82 Marc		Added the autostart command structure		*
*  8 11 82 Marc		Redefined the configuration table		*
*  8 11 82 Marc		Added DJDMA drive parameter table		*
*  8  9 82 Marc		Added clock switching to HDCA code		*
*  8  9 82 Marc		Added seek complete clearing in HDCA		*
*  8  6 82 Marc		Added buffer disable on home			*
*  8  6 82 Marc		Fixed 8250 UART initialization sequence		*
*  8  6 82 Marc		Strip parity on conout to clear up glitches	*
*  8  6 82 Marc		Fixed the 8 inch dpb256ss DPB's EXM		*
*  8  6 82 Marc		Increased the HD capacities slightly		*
*  8  6 82 Marc		Deleted all non-supported MW drives		*
*  8  6 82 Marc		Deleted call to flush in conout			*
*  8  6 82 Marc		Moved printer back to port 3			*
*  7 28 82 Marc		Moved conin flush call to conout		*
*  7 27 82 Marc		Fixed double sided head settle time		*
*  7 14 82 Marc		Optimized MWissue				*
*  7 14 82 Marc		Clean up login message for HD a bit		*
*  6 30 82 Marc		Fixed MF multi density problems			*
*  6 29 82 Marc		Added Olivetti HD561/1 HD561/2 drives		*
*  6 28 82 Marc		Added a MW error reporter			*
*  6 18 82 Marc		Added nonstandard system mode flag		*
*  6 17 82 Marc		Added a buffer error flag			*
*  6 17 82 Marc		Added save/restore of 50-52 to MW driver	*
*  6 17 82 Marc		Fixed Centronics drivers			*
*  6  7 82 Marc		Fixed allocation map sizes			*
*  6  7 82 Marc		Fixed MW partitioning				*
*  6  7 82 Marc		Fixed HD partitioning (again)			*
*  5 13 82 Marc		Fixed illegal MAC labels			*
*  5 11 82 Marc		Fixed North Star drive configurations		*
*  4 30 82 Marc		Fixed Quantum Q2040 tracks to 512		*
*  4 29 82 Marc		Fixed ST412 step constant to 0			*
*  4 26 82 Marc		Added unallocated writing			*
*  4 22 82 Marc		Fixed HD partition overlap			*
*  4 20 82 Marc		Started testing and debugging of E.3		*
*  4 19 82 Marc		Added 1 sector to HD warm boot loader		*
*  4 19 82 Marc		Added mod. number to IBIOS rev. number		*
*  4 19 82 Marc		Clean up login message 'if's			*
*  4 15 82 Marc		Fixed MCR Initialization for LST:		*
*  4 15 82 Marc		Added Seagate ST412 drive			*
*  4  6 82 Marc		Moved serial LST: device to port 2		*
*  4  1 82 Marc		Added common group select routines		*
*  4  1 82 Marc		Fixed Diablo HyType II initialization		*
*  4  1 82 Marc		Fixed LISTST for PROM driver			*
*  3 16 82 Marc		Added Tandon TM602 and TM603 drives		*
*  3 16 82 Marc		Use 'part number' equates for MW drives		*
*  3 15 82 Marc		Dropped hdrev and mwrev equates			*
*  3 15 82 Marc		Seagate ST506 head settle is 0 ms.		*
*  3 15 82 Marc		Added MiniScribe 1006 and 1012 drives		*
* *3  1 82 Marc		Public release of revision E.2			*
*  2 -- 82 Marc		Pre-release testing and debugging		*
*  2  1 82 Les + Marc	Initial coding of revision E			*
*									*
*************************************************************************

	title	'IBIOS Revision E for CP/M Version 2.2 - March 4, 1982'

revnum	equ	53		;IBIOS revision number 5.x = E.x
cpmrev	equ	22		;CP/M revision number 2.2

*************************************************************************
*									*
* The following equates set up the disk systems to be included		*
* along with the types of drives and the logical order of the		*
* drives.								*
*									*
*************************************************************************

numprd	equ	5		;Number of disks products supported

maxhd	equ	0		;Set to number of HDC3 hard disk drives
maxmw	equ	1		;Set to number of HDDMA hard disks
maxfd	equ	0		;Set to number of 2D/B floppies
maxdm	equ	0		;Set to number of DJ DMA floppies 8 inch
maxmf	equ	0		;Set to number of DJ DMA floppies 5 1/4 inch

hdorder	equ	0		;Set the order of logical drives ELSE 0 if
mworder	equ	1		; not included.
fdorder	equ	0
dmorder	equ	0
mforder	equ	0
				;HDC3 controller disk drives. Set only one
m10f	equ	0		;Fujitsu M2301B
m20	equ	0		;Fujitsu M2302B
m26	equ	0		;Shugart SA4000
m10m	equ	0		;Memorex

				;HDDMA controller disk drives. Set only one
st506	equ	1		;Seagate ST-506
st412	equ	0		;Seagate ST-412
cm5619	equ	0		;CMI CM-5619

*************************************************************************
*									*
* Since most hard disk drives hold more than 8 megabytes we		*
* partition the drive.  We partition our drives using two different	*
* formulas.								*
*									*
* One is the so called 'standard partitioning' where we try to		*
* create as many 8 megabyte partitions as possible plus a small		*
* partition to take up the slack on the end of the drive.		*
*									*
* Another way the drives are partitioned is the so called 'even		*
* partition' formula.  This means that the drive is split into		*
* equale sized partitions with the only restriction being that no	*
* partition be over 8 megabytes in length.				*
*									*
* All hard disk drives shipped from Morrow Designs are partitioned	*
* using the standard partition formula.  If the user wishes to		*
* implement even partitioning then he/she must set HDPART or MWPART	*
* to the number of partitions desired.					*
*									*
*************************************************************************

hdpart	equ	0		;Set to number of non standard partitions
mwpart	equ	0		;Set to number of non standard partitions

*************************************************************************
*									*
* The following equates are internal to the IBIOS.			*
*									*
*************************************************************************

	if	maxmw ne 0
badsiz	equ	32		;Number of badmap entries
	else
badsiz	equ	1		;No badmap if no MW drives
	endif

m10	equ	m10f or m10m

	if	hdpart ne 0 		;Use non standard partitions
hdlog	equ	hdpart
	else
hdlog	equ	m10*2+m20*3+m26*3	;Logical disks per drive for HDC3
	endif

	if	mwpart ne 0		;Use non standard partitions
mwlog	equ	mwpart
	else
mwlog	set	st506+st412*2++cm5619*2	;Logical disks per drive for HDDMA
	endif

hdc3	equ	m26 or m20 or m10		;HDC3 controller
fujitsu	equ	m20  or m10f
hdspt	equ	32*m26+21*m20+21*m10		;Sectors per track

hdma	set	st506 or st412 or cm5619	;HD DMA controller
mwspt	equ	9				;Sectors per track

maxlog	equ	(maxhd*hdlog)+(maxmw*mwlog)+maxfd+maxdm+maxmf

	if	maxlog gt 16	;Test for too many drives
	'Fatal error, more that 16 drives specified.'
	end
	endif

cdisk	equ	4		;Address of last logged disk
bdos	equ	5		;BDOS entry point
buff	equ	80h		;Default buffer address

cpmout	equ	2		;BDOS's console output routine

*************************************************************************
*									*
* The following are internal IBIOS equates. Most are misc. constants.	*
*									*
*************************************************************************

retries	equ	10		;Max retries on disk i/o before error
clear	equ	'Z'-64		;Clear screen on an ADM 3

anul	equ	0		;Null
aetx	equ	'C'-64		;ETX character
aack	equ	'F'-64		;ACK character
abel	equ	'G'-64		;Bell
abs	equ	'H'-64		;Back Space
aht	equ	'I'-64		;Horizontal tab
alf	equ	'J'-64		;Line feed
avt	equ	'K'-64		;Vertical tab
aff	equ	'L'-64		;Form Feed
acr	equ	'M'-64		;Carriage return
xon	equ	'Q'-64		;Xon character
xoff	equ	'S'-64		;Xoff character
aesc	equ	1bh		;Escape character
ars	equ	1eh		;RS character
aus	equ	1fh		;US character
asp	equ	' '		;Space
adel	equ	7fh		;Delete

*************************************************************************
*									*
* The following are the macros used in generating the DPH, DPB and 	*
* allocation tables.							*
*									*
*************************************************************************

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

*************************************************************************
*									*
* The following marco is used in generating the logical order of the	*
* CP/M drives.								*
*									*
*************************************************************************

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

*************************************************************************
*									*
* The folloing are offset numbers of Device Specification Tables.	*
*									*
*************************************************************************

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

*************************************************************************
*									*
* The jump table below must remain in the same order, the routines	*
* may be changed, but the function executed must be the same.		*
*									*
*************************************************************************

bios	equ	$		;Starting location

	jmp	$		;Cold boot entry point
owboot:	jmp	wboot		;Warm boot entry point
	jmp	$		;Console status routine
	jmp	$		;Console input
ocout:	jmp	$		;Console output
	jmp	$		;List device output
	jmp	$		;Punch device output
	jmp	$		;Reader device input
ohome:	jmp	home		;Home drive
osetdrv:jmp	setdrv		;Select disk
osettrk:jmp	settrk		;Set track
osetsec:jmp	setsec		;Set sector
osetdma:jmp	setdma		;Set DMA address
oread:	jmp	read		;Read the disk
owrite:	jmp	write		;Write the disk
	jmp	$		;List device status
osect:	jmp	sectran		;Sector translation
	jmp	$		;Hookup for SINGLE.COM program

	nop			;End of table

*************************************************************************
*									*
* Drive configuration table.						*
*									*
*************************************************************************

drconf:	db	0		;Revision 0 structure
	db	32		;32 bytes long now

*************************************************************************
*									*
* The following is the table of pointers to the Device			*
* Specification Tables.  The order of this table defines the		*
* logical order of the CP/M drives.					*
*									*
*************************************************************************

dsttab:	equ	$

dn	set	1
	rept	numprd
	order	%dn
dn	set	dn+1
	endm

*************************************************************************
*									*
* The following table are drive parameters for drives connected to	*
* the DJDMA floppy disk controller.  There is one entry for each of	*
* the the eight drive that the controller can address.  The first	*
* four entries are for the 8 inch drives and the last four are for	*
* the 5 1/4 inch drives.  Users with fast stepping 8 inch drives	*
* (SA850/1) or slow 5 1/4 inch drives (SA400) should adjust this	*
* table for optimal device performace.					*
*									*
* Each table entry contains four fixed length fields.  The fields	*
* are defined as follows:						*
*									*
*	tracks	This byte contains the number of tracks on the		*
*               drive.  Most 8 inch drives have 77 tracks and		*
*               most 5 1/4 inch drives have 35 or 40 tracks.		*
*									*
*	config	This a a flag byte that indicates as to whether		*
*               or not this drive has been configured.  Set to		*
*               0 to force reconfiguration.				*
*									*
*	step	This word contains the stepping rate constant. 		*
*               The DJDMA's delay routines tick 34.1 times per		*
*               millisecond.  Thus the step constant would be the	*
*               drive manufactors recomended stepping delay times	*
*               34.1.  Example.  Shugart SA 850's step at 3		*
*               milliseond intervals.  The step constant would be	*
*               3 * 43.1 or 102.					*
*									*
*	rfu	The next two words are reserved for future use.		*
*		They must be zero.					*
*									*
*	settle	This word is similar to the previously defined		*
*               step word.  This specifies the head settle timing	*
*               after the heads have been stepped.  Example,		*
*               Shugart's SA 850 head settle time is 15			*
*               milliseconds.  The settle constant would be 15 *	*
*               34.1 or 512.						*
*									*
* An assembler macro (DCONF) has been provided to assist in		*
* generating the DPARAM table.  This macros parameters are the		*
* number of tracks, the step rate in milliseconds, and the head		*
* settle time in milliseconds.  For example:				*
*									*
*				;Shugart SA 850				*
*	dconf	77, 3, 15	;77 tracks, 3 ms step, 15 ms settle	*
*									*
*				;Shugart SA 400				*
*	dconf	35, 40, 10	;35 tracks, 40 ms step, 10 ms settle	*
*									*
* Note: Caution should be used when defining the drive parameters. 	*
* Incorrect definations may damage the floppy disk drive.  Morrow	*
* Designs takes no responsibility for damage that occures through	*
* the misuse of this macro.						*
*									*
*************************************************************************

	if	(maxdm ne 0) or (maxmf ne 0)	;DJDMA present?

dconf	macro	tracks, step, settle
	db	tracks			;Number of tracks
	db	0			;Reset the calibrated flag
	dw	step*341/10		;Step time
	dw	0			;Reserved for future use, must be zero
	dw	0			;Reserved for future use, must be zero
	dw	settle*341/10		;Head settle time
	endm

dmarap:	db	0, 10*8			;Revision 0, length 80 bytes

dparam:	equ	$			;Drive parameter table

*************************************************************************
*									*
* Define 8 inch drive parameters					*
* Use SA800 parameters: 77 tracks, 8 ms step, 8 ms settle		*
*									*
*************************************************************************

	dconf	77, 8, 8		;Drive 0
	dconf	77, 8, 8		;Drive 1
	dconf	77, 8, 8		;Drive 2
	dconf	77, 8, 8		;Drive 3

*************************************************************************
*									*
* Define 5 1/4 inch drive parameters					*
* Use Tandon parameters: 40 tracks, 5 ms step, 15 ms settle		*
*									*
*************************************************************************

	dconf	40, 5, 15		;Drive 0
	dconf	40, 5, 15		;Drive 1
	dconf	40, 5, 15		;Drive 2
	dconf	40, 5, 15		;Drive 3

	endif

*************************************************************************
*									*
* DPH save area.  Each entry is 4 bytes long:				*
*	0 - LSB of DPH address						*
*	1 - MSB of DPH address						*
*	2 - Sector size code (1 = 128, 2 = 256, 3 = 512...		*
*	3 - Bad map has been initilized (0 = Uninitilized)		*
*									*
*************************************************************************

dphtab:	rept	maxlog
	dw	0, 0
	endm

*************************************************************************
*									*
* Instalì packagå warí boot®  Thió routinå inspectó thå commanä		*
* buffeò foò á drivå specification®  Iæ presenô theî thå locaì		*
* driveó arå 'relocated§ tï thå useò specifieä loaction®  Á 'map§	*
* of the drive configuration is printed.				*
*									*
*************************************************************************

wboot:	mvi	a,0ffh			;One time code flag
	inr	a
	jnz	owboot
	sta	wboot+1			;Clear one time flag
	jmp	insetu			;Do install package setup routines

*************************************************************************
*									*
* Instalì packageó drivå selecô routine®  Thió routinå checkó thå	*
* externaì drivå namå tï determinå iæ thå drivå belongó tï thå hosô	*
* systeí oò tï thå instalì driveró (locaì drives)®  Thå flaç INSERÖ	*
* (iî service© ió seô accordingly®  Iæ thå drivå belongó tï thå		*
* hosô systeí theî thå drivå namå ió adjusteä (sï thaô thå hosô		*
* systeí doeó noô kno÷ thaô wå arå here¬ shhh© anä thå hosô drivå	*
* selecô routinå ió called.						*
*									*
*************************************************************************

drvsel:	mov	a,c			;Get the external drive name
	lxi	h,dlocal		;Test against the first local drive
	sub	m
	jc	drvsl1			;Skip if the host drive is below us
	cpi	maxlog			;See if this drive is within our range
	jnc	drvsl0
	mov	c,a			;Load the local drive name
	mvi	a,0ffh			;Set the in service flag
	jmp	drvsl2

drvsl0:	mov	a,c			;Adjust the host drives above us
	sui	maxlog
	mov	c,a
drvsl1:	xra	a			;Clear the in service flag

drvsl2:	sta	inserv

	ret				;Return to IBIOS's setdrv

*****************************************************************
*								*
* Determine if the installed drivers are currently giving	*
* service to CP/M.  If so then return else pop off return	*
* address and jump to (hl).					*
*								*
*****************************************************************

serv:	lda	inserv			;Test the in service flag
	ora	a
	rnz				;Return if we are servicing this one
	xthl				;Clear out the call to SERV
donop:	ret				;'jump' to the host call

*************************************************************************
*									*
* Install package variables.						*
*									*
*************************************************************************

dlocal:	db	16-maxlog		;First external drive used locally
inserv:	db	0			;Local drive in service flag

*************************************************************************
*									*
* At the first page boundry following the IBIOS we have a series of	*
* pointers that point to various internal tables. At the start of	*
* each of these tables we have a revision byte and a length byte.  	*
* The revision byte is the current revision number for that		*
* particular structure and the length byte is the length of that	*
* structure.  This length does not include the revision byte nor	*
* the length byte itself.						*
*									*
*	Revision	Description					*
*	E.0		1 and 2 defined					*
*	E.3		This table is moved to a page boundry		*
*	E.3		0, 3 and 4 defined				*
*									*
* The pointers defined so far are as follows:				*
*									*
* 0)	High byte is the page number of the IBIOS.  Low byte is		*
*	the IBIOS revision number.  Used to determine pointer		*
*	structure.							*
*									*
* 1)	This points to the drive configuration table.			*
*									*
* 2)	This points to the I/O configuration bytes for the serial	*
*       drivers.  Eg, the console, printer, reader, and punch		*
*       devices.							*
*									*
* 3)	This points to the drive parameter table for DJDMA floppy	*
*       disk drives.  If no DJDMA is present then this pointer is	*
*       null (0).							*
*									*
* 4)	This points to the autostart command structures.  Used to	*
*	automatically invoke a command on cold or warm boot		*
*									*
* 5)	This will be a null (0) pointer.  It marks the end of the	*
*       table.								*
*									*
*************************************************************************

	if	$ gt bios+256	;Test for code overlap
	'Fatal error, pointer table placement.'
	else
	ds	bios+256-$	;Pad spaces till the next page
	endif

	db	high ($-1)	;IBIOS page number
	db	revnum		;IBIOS revision number
	dw	drconf		;Drive configuration table pointer
	dw	0		;I/O configuration pointer (DELETED BY INSTALL)
	if	(maxdm ne 0) or (maxmf ne 0)	;DJDMA present?
	dw	dmarap		;Drive parameter table pointer
	else
	dw	0
	endif
	dw	0		;Auto command structure pointer (DELETED)
	dw	0		;End of table marker

*****************************************************************
*								*
* Setsec just saves the desired sector to seek to until an	*
* actual read or write is attempted.				*
*								*
*****************************************************************

setsec:	mov	h,b		;Enter with sector number in (bc)
	mov	l,c
	shld	cpmsec
	jmp	osetsec

*****************************************************************
*								*
* Setdma saves the DMA address for the data transfer.		*
*								*
*****************************************************************

setdma:	mov	h,b		;Enter with DMA address in (bc)
	mov	l,c
	shld	cpmdma		;CP/M dma address
	jmp	osetdma

*****************************************************************
*								*
* Home is translated into a seek to track zero.			*
*								*
*****************************************************************

home:	lda	bufwrtn		;Test buffer dirty flag
	ora	a
	jnz	dohome		;Skip buffer disable if buffer dirty
	xra	a		;Invalidate buffer on home call
	sta	bufsec
dohome:	lxi	b,0		;Track to seek to

*****************************************************************
*								*
* Settrk saves the track # to seek to. Nothing is done at this	*
* point, everything is deffered until a read or write.		*
*								*
*****************************************************************

settrk:	mov	h,b	;Enter with track number in (bc)
	mov	l,c
	shld	cpmtrk
	jmp	osettrk

*****************************************************************
*								*
* Sectran translates a logical sector number into a physical	*
* sector number. 						*	
*								*
*****************************************************************

sectran:lxi	h,osect
	call	serv
	lda	cpmdrv		;Get the Drive Number
	mov	h,a		;Drive in (h)
	mvi	l,d$stran
	jmp	jumper		;See device level sector translation routines

*****************************************************************
*								*
* Setdrv selects the next drive to be used in read/write	*
* operations.  If the drive has never been selected it calls	*
* a low level drive select routine that should perform some	*
* sort of check if the device is working.  If not working then	*
* it should report an error.  If the logical drive has been	*
* selected before then setdrv just returns the DPH without	*
* checking the drive.						*
*								*
*****************************************************************

setdrv:	call	drvsel		;Do INSTALL checking
	lxi	h,osetdrv	;Vector to the host drive select 
	call	serv
	mov	a,c		;Save the logical drive number
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
	ora	l		; zero to indicate an error
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

setd3:	push	d		;Save DPH address
	mov	h,c		;Drive in (h)
	mvi	l,d$sel2	;Select drive
	call	jumper
	call	gdph		;Quick select
	pop	d
	mov	e,m		;DPH -> (de)
	inx	h
	mov	d,m
	inx	h
	mov	c,m		;Sector size -> (c)
	xchg			;DPH -> (hl)
	jmp	setd0

gdph:	lda	cpmdrv		;Return pointer to DPH save area
	rlc			;Each entry is 4 bytes long
	rlc
	mov	e,a
	mvi	d,0
	lxi	h,dphtab	;DPH save area table
	dad	d		;Add offset
	ret			;(hl) = DPH save area for current drive

zret:	lxi	h,0		;Seldrv error exit
	lda	lastdrv		;Get last selected drive
	mov	c,a
	lda	cdisk		;Pick up user/drive
	ani	0f0h		;Save user number
	ora	c		;Put together with old drive
	sta	cdisk
	ret

*****************************************************************
*								*
* Getbad - Check if a device has a bad map.  If the device has	*
* a bad sector map then append bad entries to end of badmap	*
* table.							*
*								*
*****************************************************************

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
	jc	badret		;Ingore the bad map if it can't be read

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

omes:	db	0dh, 0ah, 'BAD MAP OVERFLOW!', 0dh, 0ah, 0

nobad:	lxi	h,0		;Used by device drives to indicate no bad
	ret			; sector map

badptr:	dw	badmap		;Pointer to next available bad map entry

*****************************************************************
*								*
* Write routine moves data from memory into the buffer. If the	*
* desired CP/M sector is not contained in the disk buffer, the	*
* buffer is first flushed to the disk if it has ever been	*
* written into, then a read is performed into the buffer to get	*
* the desired sector. Once the correct sector is in memory, the	*
* buffer written indicator is set, so the buffer will be	*
* flushed, then the data is transferred into the buffer.	*
*								*
*****************************************************************

write:	lxi	h,owrite
	call	serv
	mov	a,c		;Save write command type
	sta	writtyp
	mvi	a,1		;Set write command
	jmp	rwent

*****************************************************************
*								*
* Read routine to buffer data from the disk. If the sector	*
* requested from CP/M is in the buffer, then the data is simply	*
* transferred from the buffer to the desired dma address. If	*
* the buffer does not contain the desired sector, the buffer is	*
* flushed to the disk if it has ever been written into, then	*
* filled with the sector from the disk that contains the	*
* desired CP/M sector.						*
*								*
*****************************************************************

read:	lxi	h,oread
	call	serv
	xra	a		;Set the command type to read
rwent:	sta	rdwr		;Save command type

*****************************************************************
*								*
* Redwrt calculates the physical sector on the disk that	*
* contains the desired CP/M sector, then checks if it is the	*
* sector currently in the buffer. If no match is made, the	*
* buffer is flushed if necessary and the correct sector read	*
* from the disk.						*
*								*
*****************************************************************

redwrt:	mvi	b,0		;The 0 is modified to contain the log2
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
				;	of the physical sectors
	mov	l,a
	jmp	divloop		;
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
	lxi	d,buffer	;Beginning address of buffer
	dad	d		;Form beginning address of sectgr to transfer
	xchg			;DE = address in buffer
	lxi	h,0		;Get DMA address, the 0 is modified t/
				;	contain the DMA address
cpmdma	equ	$-2
	mvi	a,0		;The zero gets modified to contain
				;	a zero if a read, or a 1 if write
rdwr	equ	$-1
	ana	a		;Test which kind of operation
	jnz	into		;Transfer data into the buffer
outof:	call	mov128
	lda	error		;Get the buffer error flag
	ret

into:	xchg			;
	call	mov128		;Move the data, HL = destination
				;	DE = source
	mvi	a,1
	sta	bufwrtn		;Set buffer written into flag
	mvi	a,0		;Check for directory write
writtyp	equ	$-1
	dcr	a		;Test for a directory write
	mvi	a,0
	rnz			;No error exit

*****************************************************************
*								*
* Flush writes the contents of the buffer out to the disk if	*
* it has ever been written into.				*
*								*
*****************************************************************

flush:	mvi	a,0		;The 0 is modified to reflect if
				;	the buffer has been written into
bufwrtn	equ	$-1
	ora	a		;Test if written into
	rz			;Not written, all done
	mvi	a,d$write
	sta	rwop+1
	call	prep		;Do the physical write
	sta	error		;Set up the error flag
	ret

*****************************************************************
*								*
* Prep prepares to read/write the disk. Retries are attempted.	*
* Upon entry, H&L must contain the read or write operation	*
* address.							*
*								*
*****************************************************************

prep:	call	alt		;Check for alternate sectors
	di			;Reset interrupts
	xra	a		;Reset buffer written flag
	sta	bufwrtn

	mvi	b,retries	;Maximum number of retries to attempt
retrylp:push	b		;Save the retry count

	mvi	l,d$sel2	;Select drive
	call	jumpbuf

	lhld	alttrk		;Track number -> (hl)

	mov	a,h		;Test for track zero
	ora	l

	push	h		;Save track number
	mvi	l,d$home
	cz	jumpbuf
	pop	b		;Restore track #

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

rwop:	mvi	l,0		;Get operation address
	call	jumpbuf

	pop	b		;Restore the retry counter
	mvi	a,0		;No error exit status
	rnc			;Return no error
	dcr	b		;Update the retry counter
	stc			;Assume retry count expired
	mvi	a,0ffh		;Error return
	rz			;Return sad news
	mov	a,b
	cpi	retries/2
	jnz	retrylp		;Try again
	push	b		;Save retry count
	mvi	l,d$home	;Home drive after (retries/2) errors
	call	jumpbuf
	pop	b
	jmp	retrylp		;Try again

*****************************************************************
*								*
* Fill fills the buffer with a new sector from the disk.	*
*								*
*****************************************************************

fill:	call	flush		;Flush buffer first
	rc			;Check for error
	lxi	d,cpmdrv	;Update the drive, track, and sector
	lxi	h,bufdrv
	lxi	b,5		;Number of bytes to move
	call	movbyt		;Copy the data

	lda	rdwr		;Test read write flag
	ora	a
	jz	fread		;Skip write type check if reading
	lda	writtyp		;0 = alloc, 1 = dir, 2 = unalloc

	sui	2		;Test for an unallocated write
	rz

fwritin:lda	secsiz		;Check for 128 byte sectors
	dcr	a
	rz			;No deblocking needed

fread:	mvi	a,d$read
	sta	rwop+1
	call	prep		;Read the physical sector the buffer
	sta	error		;Set the error status
	ret

*****************************************************************
*								*
* Jumpbuf, jumper are used to dispatch to a low level device	*
* subroutine.  Jumper is called with the drive in (h) and the	*
* routine number (see description above) in (l).  It passes	*
* along the (bc) and (de) registers unaltered.  Jumpbuf is	*
* a call to jumper with the drive number from bufdrv.		*
*								*
*****************************************************************

jumpbuf:lda	bufdrv		;Dispatch with bufdrv for drive
	mov	h,a

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

*****************************************************************
*								*
* Check for alternate sectors in bad sector table.  If an	*
* alternate sector is found replace alttrk and altsec with	*
* new sector number else pass along unaltered.			*
*								*
*****************************************************************

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

*****************************************************************
*								*
* Mover moves 128 bytes of data. Source pointer in DE, Dest	*
* pointer in HL.						*
*								*
*****************************************************************

mov128:	lxi	b,128		;Length of transfer
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

*****************************************************************
*								*
* Return DPH pointer.  Enter with (de) with DPH base address	*
* and (a) with logical drive number.  Returns with DPH address	*
* in (hl).							*
*								*
*****************************************************************

retdph	mov	l,a		;Move logical drive into (l)
	mvi	h,0
	dad	h		;Multiply by 16 (size of DPH)
	dad	h
	dad	h
	dad	h
	dad	d		;(hl) = pointer to DPH
	ret

*****************************************************************
*								*
* Utility routine to output the message pointed at by (hl)	*
* terminated with a null.					*
*								*
*****************************************************************

message:mov	a,m		;Get a character of the message
	inx	h		;Bump text pointer
	ora	a		;Test for end
	rz			;Return if done
	push	h		;Save pointer to text
	mov	c,a		;Output character in C
	call	ocout		;Output the character
	pop	h		;Restore the pointer
	jmp	message		;Continue until null reached

*****************************************************************
*								*
* The following code is for the Diskus Hard disk		*
*								*
*****************************************************************

	if	hdc3 ne 0		;Want HDC3 or 4 controller included ?

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

*****************************************************************
*								*
* Device Specification Table for HDCA controller driver		*
*								*
*****************************************************************

hddst:	db	maxhd*hdlog		;Number of logical drives
	dw	donop			;No warm boot for INSTALL
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

hdtran:	mov	h,b			;Sector translation is handled via
	mov	l,c			;   physical sector header skewwing
	inx	h
	ret

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

hddrv:	sta	hdcur
	call	divlog			;Get the physical drive #
hdd2:	mov	a,c
	sta	hddisk			;Select the drive
	ori	null
	out	hdfunc
	mvi	a,hdfren+hdrun+hdclok+hdwprt	;Write protect
	out	hdcntl
	ret

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

hddma:	mov	h,b			;Save the DMA address
	mov	l,c
	shld	hdadd
	ret

wsdone:	in	hdstat			;Wait for seek complete to finish
	ani	complt
	jz	wsdone
	in	hdskomp			;Clear sdone bit on an HDCA4
	ret

	if	m26
hdsec:	mvi	a,01fh			;For compatibility with IBIOS revs.
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

*****************************************************************
*								*
* The following equates relate the Morrow Designs 2D/B		*
* controller. If the controller is non standard (0F800H)	*
* only the FDORIG equate need be changed.			*
*								*
*****************************************************************

	if	maxfd ne 0		;Include Discus 2D ?
fdorig	equ	0f800H			;Origin of Disk Jockey PROM
fdboot	equ	fdorig+00h		;Disk Jockey 2D initialization
fdcin	equ	fdorig+03h		;Disk Jockey 2D character input routine
fdcout	equ	fdorig+06h		;Disk Jockey 2D character out routine
fdhome	equ	fdorig+09h		;Disk Jockey 2D track zero seek
fdseek	equ	fdorig+0ch		;Disk Jockey 2D track seek routine
fdsec	equ	fdorig+0fh		;Disk Jockey 2D set sector routine
fddma	equ	fdorig+12h		;Disk Jockey 2D set DMA address
fdread	equ	fdorig+15h		;Disk Jockey 2D read routine
fdwrite	equ	fdorig+18h		;Disk Jockey 2D write routine
fdsel	equ	fdorig+1bh		;Disk Jockey 2D select drive routine
fdtstat	equ	fdorig+21h		;Disk Jockey 2D terminal status routine
fdstat	equ	fdorig+27h		;Disk Jockey 2D status routine
fderr	equ	fdorig+2ah		;Disk Jockey 2D error, flash led
fdden	equ	fdorig+2dh		;Disk Jockey 2D set density routine
fdside	equ	fdorig+30h		;Disk Jockey 2D set side routine
fdram	equ	fdorig+400h		;Disk Jockey 2D RAM address
dblsid	equ	20h			;Side bit from controller
io	equ	fdorig+3f8h		;Start of I/O registers
dreg	equ	io+1
cmdreg	equ	io+4
clrcmd	equ	0d0h

*****************************************************************
*								*
* Device Specification Table for the Disk Jockey 2D/B		*
*								*
*****************************************************************

fddst:	db	maxfd			;Number of logical drives
	dw	donop			;No warm boot for INSTALL
	dw	fdtran			;Sector translation
	dw	fdldrv			;Select drive 1
	dw	fdsel2			;Select drive 2
	dw	fdlhome			;Home drive
	dw	fdseek			;Seek to specified track
	dw	fdssec			;Set sector
	dw	fddma			;Set DMA address
	dw	fdread			;Read a sector
	dw	fdwrite			;Write a sector
	dw	nobad			;No bad sector map

*****************************************************************
*								*
* Floppy disk warm boot loader					*
*								*
*****************************************************************

fdtran:	inx	b
	push	d			;Save table address
	push	b			;Save sector #
	call	fdget			;Get DPH for current drive
	lxi	d,10			;Load DPH pointer
	dad	d
	mov	a,m
	inx	h
	mov	h,m
	mov	l,a
	mov	a,m			;Get # of CP/M sectors/track
	ora	a			;Clear carry
	rar				;Divide by two
	sub	c			;Subtract sector number
	push	psw			;Save adjusted sector
	jm	sidetwo
sidea:	pop	psw			;Discard adjusted sector
	pop	b			;Restore sector requested
	pop	d			;Restore address of xlt table
sideone:xchg				;hl <- &(translation table)
	dad	b			;bc = offset into table
	mov	l,m			;hl <- physical sector
	mvi	h,0
	ret

sidetwo:call	fdgsid			;Check out number of sides
	jz	sidea			;Single sided
	pop	psw			;Retrieve adjusted sector
	pop	b
	cma				;Make sector request positive
	inr	a
	mov	c,a			;Make new sector the requested sector
	pop	d
	call	sideone
	mvi	a,80h			;Side two bit
	ora	h			;	and sector
	mov	h,a
	ret

fdldrv:	sta	fdlog			;Save logical drive
	mov	c,a			;Save drive #
	mvi	a,0			;Have the floppies been accessed yet ?
flopflg	equ	$-1
	ana	a
	jnz	flopok

	mvi	b,17			;Floppies havn't been accessed
	lxi	h,fdboot		;Check if 2D controller is installed
	mvi	a,(jmp)
clopp:	cmp	m			;Must have 17 jumps
	jnz	zret
	inx	h
	inx	h
	inx	h
	dcr	b
	jnz	clopp
	lxi	d,fdinit		;Initialization sequence
	lxi	h,fdorig+7e2h		;Load address
	lxi	b,30			;Byte count
	call	movbyt			;Load controller RAM
	mvi	a,0ffh			;Start 1791
	sta	dreg
	mvi	a,clrcmd		;1791 reset
	sta	cmdreg
	mvi	a,1			;Set 2D initialized flag
	sta	flopflg

flopok:	call	flush			;Flush buffer since we are using it
	lda	fdlog			;Select new drive
	mov	c,a
	call	fdsel
	call	fdlhome			;Recalibrate the drive
	lxi	h,1			;Select sector 1 of track 2
	shld	truesec
	inx	h
	shld	cpmtrk
	xra	a			;Make sure we are doing a read
	sta	rdwr
	call	fill			;Fill in buffer with sector
	jc	zret			;Test for error return
	call	fdstat			;Get status on current drive
	sta	fdldst			;Save drive status
	ani	0ch			;Mask in sector size bits
	push	psw			;Used to select a DPB
	rar
	lxi	h,xlts			;Table of XLT addresses
	mov	e,a
	mvi	d,0
	dad	d
	push	h			;Save pointer to proper XLT
	call	fdget			;Get pointer to proper DPH
	pop	d
	lxi	b,2			;Copy XLT pointer into DPH
	call	movbyt
	lxi	d,8			;Offset to DPB pointer in DPH
	dad	d			;HL <- &DPH.DPB
	push	h
	call	fdgsid			;Get pointer to side flag table entry
	lda	fdldst			;Get drive status
	ani	dblsid			;Check double sided bit
	mov	m,a			;Save sides flag
	lxi	d,dpb128s		;Base for single sided DPB's
	jz	sideok
	lxi	d,dpb128d		;Base of double sided DPB's
sideok:	xchg
	pop	d			;(HL) -> DPB base, (DE) -> &DPH.DPB
	pop	psw			;Offset to correct DPB
	ral
	ral				;Make 0, 10, 20, 30
	mov	c,a
	mvi	b,0			;Make offset
	dad	b			;(hl) is now a DPB pointer
	xchg				;Put proper DPB address in DPH.DPB
	mov	m,e
	inx	h
	mov	m,d
	lxi	h,15			;Offset to DPB.SIZ
	dad	d
	mov	c,m			;Fetch sector size code
fdget:	lda	fdlog			;Return proper DPH
	lxi	d,dphfd0
	jmp	retdph

fdsel2:	sta	fdlog
	mov	c,a
	jmp	fdsel

fdlhome:mvi	c,0			;Select side 0
	call	fdside
	jmp	fdhome			;Do actual home

fdssec:	push	b			;Save sector number
	mov	a,b			;Check side select bit
	rlc				;Move high bit to bit zero
	ani	1
	mov	c,a
	call	fdside			;Call select side 0/1
	pop	b
	jmp	fdsec

fdgsid:	lxi	h,fdlsid		;Side flag table
	lda	fdlog			;Drive number
	push	d
	mov	e,a			;Make offset
	mvi	d,0
	dad	d			;Offset to proper entry
	pop	d
	mov	a,m			;Set up flags
	ora	a
	ret

fdinit:	dw	0			;Initialization bytes loaded onto 2D/B
	dw	1800h			;Head loaded timeout
	dw	0			;DMA address
	db	0			;Double sided flag
	db	0			;Read header flag
	db	07eh			;Drive select constant
	db	0			;Drive number
	db	8			;Current disk
	db	0			;Head loaded flag
	db	9			;Drive 0 parameters
	db	0ffh			;Drive 0 track address
	db	9			;Drive 1 parameters
	db	0ffh			;Drive 1 track address
	db	9			;Drive 2 parameters
	db	0ffh			;Drive 2 track address
	db	9			;Drive 3 parameters
	db	0ffh			;Drive 3 track address
	db	9			;Current parameters
	db	0			;Side desired
	db	1			;Sector desired
	db	0			;Track desired

	db	0			;Header image, track
	db	0			;Sector
	db	0			;Side
	db	0			;Sector
	dw	0			;CRC

fdlog:	db	0
fdldst:	db	0			;Floppy drive status byte

fdlsid:	rept	maxfd
	db	0ffh			;Number of sides on each drive 0/1
	endm
	endif

	if	(maxfd ne 0) or (maxdm ne 0)

*****************************************************************
*								*
* Xlts is a table of address that point to each of the xlt	*
* tables for each sector size.					*
*								*
*****************************************************************

xlts:	dw	xlt128			;Xlt for 128 byte sectors
	dw	xlt256			;Xlt for 256 byte sectors
	dw	xlt512			;Xlt for 512 byte sectors
	dw	xlt124			;Xlt for 1024 byte sectors

*************************************************************************
*									*
* Xlô tableó (sectoò ske÷ tables© foò CP/Í 2.2®  Theså tableó		*
* definå thå sectoò translatioî thaô occuró wheî mappinç CP/Í		*
* sectoró tï physicaì sectoró oî thå disk® Therå ió onå ske÷ tablå	*
* foò eacè oæ thå possiblå sectoò sizes.				*
*									*
*************************************************************************

xlt128:	db	0
	db	1,7,13,19,25
	db	5,11,17,23
	db	3,9,15,21
	db	2,8,14,20,26
	db	6,12,18,24
	db	4,10,16,22

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

xlt512:	db	0
	db	1,2,3,4,17,18,19,20
	db	33,34,35,36,49,50,51,52
	db	5,6,7,8,21,22,23,24
	db	37,38,39,40,53,54,55,56
	db	9,10,11,12,25,26,27,28
	db	41,42,43,44,57,58,59,60
	db	13,14,15,16,29,30,31,32
	db	45,46,47,48

xlt124:	db	0
	db	1,2,3,4,5,6,7,8
	db	25,26,27,28,29,30,31,32
	db	49,50,51,52,53,54,55,56
	db	9,10,11,12,13,14,15,16
	db	33,34,35,36,37,38,39,40
	db	57,58,59,60,61,62,63,64
	db	17,18,19,20,21,22,23,24
	db	41,42,43,44,45,46,47,48

*************************************************************************
*									*
* Each of the following tables describes a diskette with the		*
* specified characteristics.						*
*									*
*************************************************************************

*************************************************************************
*									*
* Thå followinç DPÂ defineó á  diskettå foò 12¸ bytå sectors¬		*
* singlå density¬ anä singlå sided.					*
*									*
*************************************************************************

dpb128s:dw	26			;CP/M sectors/track
	db	3			;BSH
	db	7			;BLM
	db	0			;EXM
	dw	242			;DSM
	dw	63			;DRM
	db	0c0h			;AL0
	db	0			;AL1
	dw	16			;CKS
	dw	2			;OFF
	db	1			;128 byte sectors

*************************************************************************
*									*
* The following DPB defines a diskette for 256 byte sectors,		*
* double density, and single sided.					*
*									*
*************************************************************************

dpb256s:dw	52			;CP/M sectors/track
	db	4			;BSH
	db	15			;BLM
	db	1			;EXM
	dw	242			;DSM
	dw	127			;DRM
	db	0c0h			;AL0
	db	0			;AL1
	dw	32			;CKS
	dw	2			;OFF
	db	2			;256 byte sectors

*************************************************************************
*									*
* The following DPB defines a diskette as 512 byte sectors,		*
* double density, and single sided.					*
*									*
*************************************************************************

dpb512s:dw	60			;CP/M sectors/track
	db	4			;BSH
	db	15			;BLM
	db	0			;EXM
	dw	280			;DSM
	dw	127			;DRM
	db	0c0h			;AL0
	db	0			;AL1
	dw	32			;CKS
	dw	2			;OFF
	db	3			;512 byte sectors

*************************************************************************
*									*
* The following DPB defines a diskette as 1024 byte sectors,		*
* double density, and single sided.					*
*									*
*************************************************************************

dp1024s:dw	64			;CP/M sectors/track
	db	4			;BSH
	db	15			;BLM
	db	0			;EXM
	dw	299			;DSM
	dw	127			;DRM
	db	0c0h			;AL0
	db	0			;AL1
	dw	32			;CKS
	dw	2			;OFF
	db	4			;1024 byte sectors

*************************************************************************
*									*
* The following DPB defines a diskette for 128 byte sectors,		*
* single density, and double sided.					*
*									*
*************************************************************************

dpb128d:dw	52			;CP/M sectors/track
	db	4			;BSH
	db	15			;BLM
	db	1			;EXM
	dw	242			;DSM
	dw	127			;DRM
	db	0c0h			;AL0
	db	0			;AL1
	dw	32			;CKS
	dw	2			;OFF
	db	1			;128 byte sectors

*************************************************************************
*									*
* The following DPB defines a diskette as 256 byte sectors,		*
* double density, and double sided.					*
*									*
*************************************************************************

dpb256d:dw	104			;CP/M sectors/track
	db	4			;BSH
	db	15			;BLM
	db	0			;EXM
	dw	486			;DSM
	dw	255			;DRM
	db	0f0h			;AL0
	db	0			;AL1
	dw	64			;CKS
	dw	2			;OFF
	db	2			;256 byte sectors

*************************************************************************
*									*
* The following DPB defines a diskette as 512 byte sectors,		*
* double density, and double sided.					*
*									*
*************************************************************************

dpb512d:dw	120			;CP/M sectors/track
	db	4			;BSH
	db	15			;BLM
	db	0			;EXM
	dw	561			;DSM
	dw	255			;DRM
	db	0f0h			;AL0
	db	0			;AL1
	dw	64			;CKS
	dw	2			;OFF
	db	3			;512 byte sectors

*************************************************************************
*									*
* The following DPB defines a diskette as 1024 byte sectors,		*
* double density, and double sided.					*
*									*
*************************************************************************

dp1024d:dw	128			;CP/M sectors/track
	db	4			;BSH
	db	15			;BLM
	db	0			;EXM
	dw	599			;DSM
	dw	255			;DRM
	db	0f0h			;AL0
	db	0			;AL1
	dw	64			;CKS
	dw	2			;OFF
	db	4			;1024 byte sectors

	endif

*****************************************************************
*								*
* The following equates relate the Morrow Designs DJDMA		*
* controller.							*
*								*
*****************************************************************

	if	(maxdm ne 0) or (maxmf ne 0)
dmchan	equ	50h			;Default channel address
dmkick	equ	0efh			;Kick I/O port address

rdsect	equ	20h			;Read sector command
wrsect	equ	21h			;Write a sector command
gstat	equ	22h			;Get drive status
dmsdma	equ	23h			;Set DMA address
intrqc	equ	24h			;Set Interrupt request
dmhaltc	equ	25h			;Halt command
bracha	equ	26h			;Channel branch
setcha	equ	27h			;Set channel address
setcrc	equ	28h			;Set CRC retry count
rdtrck	equ	29h			;Read track command
wrtrck	equ	2ah			;Write track command
serout	equ	2bh			;Serial console ouput
senabl	equ	2ch			;Enable serial input
trksiz	equ	2dh			;Set number of tracks
setlog	equ	2eh			;Set logical drives
readm	equ	0a0h			;Read from controller memory
writem	equ	0a1h			;Write to controller memory

dmfstp	equ	3*341/10		;Fast stepping rate = 3 ms * 34.1
dmfset	equ	15*341/10		;Fast settling rate = 15 ms * 34.1

n$dubl	equ	80h			;Double density
n$2side	equ	40h			;2 sided drive

serin	equ	03eh			;Address of serial input data

*****************************************************************
*								*
* Device Specification Table for the Disk Jockey DMA floppy	*
*								*
*****************************************************************

	if	maxdm ne 0
dmdst:	db	maxdm			;Number of logical drives
	dw	donop			;No warm boot for INSTALL
	dw	dmtran			;Sector translation
	dw	dmldrv			;Select drive 1
	dw	dmselr			;Select drive 2
	dw	dmhome			;Home drive
	dw	dmseek			;Seek to specified track
	dw	dmssec			;Set sector
	dw	dmdma			;Set DMA address
	dw	dmread			;Read a sector
	dw	dmwrite			;Write a sector
	dw	nobad			;No bad sector map

dmselr:	sta	dmlog
	mvi	b,0			;8 inch logical drives start at zero
	jmp	dmsel2

dmtran:	inx	b
	push	d			;Save table address
	push	b			;Save sector #
	call	dmget
	lxi	d,10
	dad	d
	mov	a,m
	inx	h
	mov	h,m
	mov	l,a
	mov	a,m			;Get # of CP/M sectors/track
	ora	a			;Clear cary
	rar				;Divide by two
	sub	c
	push	psw			;Save adjusted sector
	jm	dmside2
dmsidea:pop	psw			;Discard adjusted sector
	pop	b			;Restore sector requested
	pop	d			;Restor address of xlt table
dmside1:xchg				;hl <- &(translation table)
	dad	b			;bc = offset into table
	mov	l,m			;hl <- physical sector
	mvi	h,0
	ret

dmside2:call	dmstat
	ani	20h
	jz	dmsidea
	pop	psw			;Retrieve adjusted sector
	pop	b
	cma				;Make sector request positive
	inr	a
	mov	c,a			;Make new sector the requested sector
	pop	d
	call	dmside1
	mvi	a,80h			;Side two bit
	ora	h			;	and sector
	mov	h,a
	ret

dmldrv:	sta	dmlog
	call	dminit			;Test for a drive
	jc	zret
	lxi	h,1			;Select sector 1 of track 2
	shld	truesec
	inx	h
	shld	cpmtrk
	xra	a			;Make sure we are doing a read
	sta	rdwr
	call	fill			;Flush buffer and refill
	jc	zret			;Test for error return

	call	dmstat			;Get status on current drive
	ani	0ch			;Mask in sector size bits
	push	psw			;Used to select a DPB
	rar
	lxi	h,xlts			;Table of XLT addresses
	mov	e,a
	mvi	d,0
	dad	d
	push	h			;Save pointer to proper XLT
	call	dmget
	pop	d
	lxi	b,2			;Number of bytes to move
	call	movbyt			;Move the address of XLT
	lxi	d,8			;Offset to DPB pointer
	dad	d			;HL <- &DPH.DPB
	push	h
	call	dmstat
	ani	20h			;Check double sided bit
	lxi	d,dpb128s		;Base for single sided DPB's
	jz	dmsok
	call	sethigh			;Set controller for fast steping
	lxi	d,dpb128d		;Base of double sided DPB's
dmsok:	xchg				;HL <- DBP base, DE <- &DPH.DPB
	pop	d			;Restore DE (pointer into DPH)
	pop	psw			;Offset to correct DPB
	ral
	ral
	mov	c,a
	mvi	b,0
	dad	b
	xchg				;Put DPB address in DPH
	mov	m,e
	inx	h
	mov	m,d
	lxi	h,15
	dad	d
	mov	c,m
dmget:	lda	dmlog
	lxi	d,dphdm0
	jmp	retdph

;
;	The current drive is double sided.  Thus is it safe to set the
;	stepping rate to 3 ms with 15 ms settling.
;

sethigh:lhld	dmlog			;Get the current drive number
	mvi	h,0
	dad	h			;Ten bytes per parameter table entry
	mov	d,h
	mov	e,l
	dad	h
	dad	h
	dad	d
	lxi	d,dparam+1		;Parameter table address
	dad	d			;Skip the track size byte
	mvi	m,0			;Force reparamitization of this drive
	inx	h			;Offset to the Stepping rate constant
	mvi	m,(low dmfstp)		;Fast stepping rate constant
	inx	h
	mvi	m,(high dmfstp)
	lxi	d,5			;Skip over the reserved fields
	dad	d
	mvi	m,(low dmfset)		;Fast settling rate constant
	inx	h
	mvi	m,(high dmfset)
	call	dmparm			;Set drive parameters for the SA850
	ret

	endif

*****************************************************************
*								*
* Drive specification table for DJDMA 5 1/4 inch drives		*
*								*
*****************************************************************

	if	maxmf ne 0
mfdst:	db	maxmf			;Number of logical drives
	dw	donop			;No warm boot for INSTALL
	dw	mftran			;Sector translation
	dw	mfldrv			;Select drive 1
	dw	mfsel2			;Select drive 2
	dw	dmhome			;Home drive
	dw	mfseek			;Seek to specified track
	dw	mfssec			;Set sector
	dw	dmdma			;Set DMA address
	dw	dmread			;Read a sector
	dw	dmwrite			;Write a sector
	dw	nobad			;No bad sector map

mfssec:	dcr	c			;Minnie floppy sectors start at zero
	lda	dblflg			;Get double sided flags
	ora	a
	jz	dmssec			;Nope, single sided
	mvi	b,80h			;Set high bit for double sided select
	jmp	dmssec

dblflg:	db	0

mfseek:	xra	a			;Clear double sided select
	sta	dblflg
	lda	mfpcon
	ani	n$2side
	jz	dmseek			;Only single sided

	mov	a,c			;Move selected track in (a)
	sbi	35			;Subtract by track by number of tracks
	jc	dmseek			;Less than track 35

	mov	d,a			;Save adjusted track number
	mvi	a,34
	sub	d			;Adjust to count tracks back out
	mov	c,a			;Resave new track number

	mvi	a,0ffh			;Set double sided flag
	sta	dblflg
	jmp	dmseek

mfsel2:	sta	mflog

	mov	c,a			;Get proper physical configuration byte
	mvi	b,0
	lxi	h,mfscon
	dad	b
	mov	a,m
	sta	mfpcon
	mov	a,c			;Shhh, pretend that nothing happened

	mvi	b,4			;5 1/4 inch drives start at drive 4
	jmp	dmsel2

mftran:	lda	mfpcon
	ani	n$dubl
	lxi	h,mfxltd		;Point to double sided sectran table
	jnz	mftdubl			;Single density sector translation
	lxi	h,mfxlts
mftdubl:dad	b			;Add offset sector number to table
	mov	l,m			;Pick up sector number from table
	mvi	h,0			;MSB of sector number equal 0
	ret

mfldrv:	sta	mflog
	call	dminit			;Test for a controller
	jc	zret

	lda	mflog			;Get proper physical configuration byte
	mov	c,a
	mvi	b,0
	lxi	h,mfscon
	dad	b
	mvi	a,n$dubl
	mov	m,a
	sta	mfpcon

	lxi	h,1			;Select sector 1 of track 0
	shld	truesec
	dcx	h
	shld	cpmtrk
	xra	a			;Make sure we are doing a read
	sta	rdwr
	call	fill			;Flush buffer and refill
	jc	zret			;Test for error return
	lda	buffer+5ch		;Get diskette configuration byte

	push	psw			;Save configuration byte
	lxi	h,1
	shld	cpmtrk			;Load track 1 sector 1
	call	fill			;DJDMA firmware returns single density
	jc	zret			;   status on track 0
	pop	psw

	ora	a
	jnz	mfl9			;Non zero

	mvi	a,90h			;Double density default configuration
	call	dmstat			;If zero then determine sector size
	ani	80h			;Check density bit
	jnz	mfl9			;Its double density
	mvi	a,10h			;Single density configuration byte

mfl9:	mov	c,a			;Move configuration byte into (c)

	lxi	h,mfs			;Address of configuration table -> (hl)
mfl2:	mov	a,m			;Get an entry
	ora	a			;Check for end of the table
	jz	zret			;Yes, select error
	cmp	c			;Check if entry matches selected drive
	jz	mfl3
	inx	h			;Skip onfiguration byte
	inx	h			;Skip drive type
	inx	h			;Skip DPB address
	inx	h
	jmp	mfl2

mfl3:	inx	h
	mov	a,m			;Pick up drive type
	sta	mfpcon
	mov	e,a

	push	h
	lda	mflog			;Get proper physical configuration byte
	mov	c,a
	mvi	b,0
	lxi	h,mfscon
	dad	b
	mov	m,e
	pop	h

	inx	h
	mov	a,m
	inx	h
	mov	h,m
	mov	l,a			;DPB address -> (hl)
	push	h			;Save DPB address
	call	mfgdph			;Get DPH
	lxi	d,10			;Offset to DPB address in DPH
	dad	d
	pop	d
	mov	m,e			;Store DPB address in DPH
	inx	h
	mov	m,d
	call	mfgdph
	push	h
	call	dmstat			;Get status
	pop	h
	ani	80h			;Check density bit
	mvi	c,3			;512 byte sectors
	rnz
	mvi	c,2			;256 byte sectors
	ret

mfgdph	lda	mflog
	lxi	d,dphmf0
	jmp	retdph

mfpcon:	db	0			;Physical configuration byte
mflog:	db	0

mfscon:	db	0, 0, 0, 0		;Saved physical configuration bytes

mfs:	db	10h			;North Star CP/M 1.4
	db	0			;Single density, 35 tracks, one sided
	dw	dpbmf0			;1K groups

	db	90h			;North Star CP/M 1.4
	db	n$dubl			;Double density, 35 tracks, two sided
	dw	dpbmf1			;1K groups

	db	0b0h			;North Star CP/M 2.x
	db	n$dubl			;Double density, 35 tracks, one sided
	dw	dpbmf2			;2K groups

	db	0f0h			;North Star CP/M 2.x
	db	n$dubl+n$2side		;Double density, 35 tracks, two sided
	dw	dpbmf3			;2K groups

	db	0e5h			;North Star CP/M 1.4
	db	n$dubl			;Double density, 35 tracks, one sided
	dw	dpbmf1			;1K groups

	db	0a0h			;North Star CP/M 2.x  (fake 40 track)
	db	n$dubl			;Double density, 35 tracks, one sided
	dw	dpbmf2			;2K groups

	db	0d0h			;North Star CP/M 2.x (fake 40 track)
	db	n$dubl+n$2side		;Double density, 35 tracks, two sided
	dw	dpbmf3			;2K groups

	db	0			;End of configuration table

mfxltd	db	 1, 2, 3, 4
	db	21,22,23,24
	db	 5, 6, 7, 8
	db	25,26,27,28
	db	 9,10,11,12
	db	29,30,31,32
	db	13,14,15,16
	db	33,34,35,36
	db	17,18,19,20
	db	37,38,39,40

mfxlts	db	 1, 2
	db	 3, 4
	db	 5, 6
	db	 7, 8
	db	 9,10
	db	11,12
	db	13,14
	db	15,16
	db	17,18
	db	19,20
	endif

*****************************************************************
*								*
* Common routines for the DJDMA with 8 and 5 1/4 inch drives	*
*								*
*****************************************************************

dmsel2:	mov	c,a			;Move drive into (c)
	lxi	h,dmchan
	mvi	m,setlog		;Set logical drives
	inx	h
	mov	m,b			;Drive in (b)
	push	b
	call	docmd
	pop	b
	jmp	dmsel

dmssec:	push	b			;Save sector number
	mov	a,b
	rlc
	ani	1
	mov	c,a
	call	dmside
	pop	b
	jmp	dmsec

dmdma	lxi	h,dmchan		;Default channel address
	mvi	m,dmsdma		;Set DMA address
	inx	h
	mov	m,c			;Low byte first
	inx	h
	mov	m,b			;High byte next

docmd	xra	a
	inx	h
	mov	m,a
docmd2	inx	h
	mvi	m,dmhaltc
	inx	h
	mov	m,a
	out	dmkick
tests	ora	m
	jz	tests
	ret

dminit:	lxi	h,dmchan		;See if controller will halt
	mvi	m,dmhaltc
	inx	h
	mvi	m,0
	out	dmkick			;Start controller
	lxi	d,0			;Set up timeout counter
dminwt	mov	a,m
	ora	a
	jnz	dmiok			;Controller has responded
	dcx	d			;Bump timeout counter
	mov	a,d
	ora	e
	jnz	dminwt
	stc				;Set error flag
	ret

dmiok	push	h			;Set drive parameters
	call	dmparm
	pop	h
	dcx	h			;Back to start of command
	mvi	m,setcrc		;Disable monitor
	inx	h
	mvi	m,1
	xra	a
	jmp	docmd2			;Do command

;
;	Set floppy drive parameters
;
;	This routine reads the dparam table and if the a drive has not
;	previously been calibrated then that drives track count,
;	stepping rate, and head settling time are loaded.
;

dmparm:	mvi	a,8			;Eight drives
	lxi	d,1340h			;Start with drive 0's table
	lxi	h,dparam+1		;Drive parameter table

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

dmhome	xra	a
	mov	c,a			;Put a zero into (c) for track zero

dmseek	mov	a,c			;Enter with track in (c)
	sta	lltrk			;Save for use later
	ret

dmsec	lda	llss			;Load sector
	ani	80h			;Save side select bit
stores	ora	c
	sta	llss
	ret

dmside:	mov	a,c			;Move side bit into (a)
	ani	1
	rrc				;Move around to bit 7
	mov	c,a			;Resave in (c)
	lda	llss
	ani	7fh			;Mask out old side select bit
	jmp	stores

dmsel:	mov	a,c			;Move drive into (a)
	sta	lldrv
dmden:	ret				;Double density only

;
; Return status in the (a) register in the form:
;
;			7  6  5  5  3  2  1  0
;			^  ^  ^  ^  ^  ^  ^  ^
; Density --------------+  |  |  |  |  |  |  |
; Side select -------------+  |  |  |  |  |  |
; Double sided ---------------+  |  |  |  |  |
; 5 1/4 -------------------------+  |  |  |  |
; Sector size MSB ------------------+  |  |  |
; Sector size LSB ---------------------+  |  |
; Drive select MSB -----------------------+  |
; Drive select LSB --------------------------+
;

dmstat	lxi	h,dmchan
	mvi	m,gstat			;Set up read status
	inx	h
	lda	lldrv			;Get last selected drive
	mov	m,a			;Store drive in command
	inx	h			;Skip over returned status
	inx	h
	inx	h
	call	docmd			;Issue command
	lda	llss			;Get side bit of last operation
	ani	80h
	rrc				;Move to bit 7
	mov	c,a
	lxi	h,dmchan+1		;Point to drive
	mov	a,m			;Load drive
	ora	c
	ani	4			;Mask upper drive select bit for 5 1/4
	rlc
	rlc				;Move to bit 4
	ora	m			;Put together with lower drive bits
	ora	c
	mov	c,a
	inx	h
	mvi	a,10h			;Double density bit
	ana	m
	rlc				;20h
	rlc				;40h
	rlc				;80h for density bit
	ora	c
	mov	c,a
	inx	h
	mvi	a,3			;Sector length mask
	ana	m			;And in
	rlc				;Move to bits 2 & 3
	rlc
	ora	c
	mov	c,a
	inx	h
	mvi	a,4			;Mask for double sided bit
	ana	m
	rlc				;8
	rlc				;10
	rlc				;20
	ora	c
	ret

dmwrite	mvi	a,wrsect
	db	01			;Ugh...
dmread	mvi	a,rdsect
	lxi	h,dmchan
	lxi	d,lltrk-1
	mvi	b,4
cload	mov	m,a
	inx	h
	inx	d
	ldax	d
	dcr	b
	jnz	cload
	dcx	h
	call	docmd
	lda	dmchan+4
	cpi	80h
	cmc
	ret

;
;	Execute a DJDMA command, no command status is returned
;
;	Entry:
;		DE = offset to the halt status
;		HL = pointer to the start of the command
;
;	Returns:
;		nothing
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

	db	dmhaltc			;Controller halt
	db	0			;Status

;
;	Driver variables
;

lltrk	db	0
llss	db	1
lldrv	db	0
dmlog	db	0

	endif

*****************************************************************
*								*
* The follwing equates are for the HDDMA hard disk controller	*
*								*
*****************************************************************

	if	maxmw ne 0	;HDDMA controller present ?
	if	st506		;Specifications for a Seagate Technology 506
cyl	equ	153		;Number of cylinders
heads	equ	4		;Number of heads per cylinder
precomp	equ	64		;Cylinder to start write precomensation
lowcurr	equ	128		;Cylinder to start low current
stepdly	equ	30		;Step delay (0-12.7 milliseconds)
steprcl	equ	30		;Recalibrate step delay
headdly	equ	0		;Settle delay (0-25.5 milliseconds)
	endif

	if	st412		;Specifications for a Seagate ST412
cyl	equ	306
heads	equ	4
precomp	equ	128
lowcurr	equ	128
stepdly	equ	0
steprcl	equ	30
headdly	equ	0
	endif

	if	cm5619		;Specifications for an CMI 5619
cyl	equ	306
heads	equ	6
precomp	equ	128
lowcurr	equ	128
stepdly	equ	2
steprcl	equ	30
headdly	equ	0
	endif

sectsiz	equ	7		;Sector size code (must be 7 for this IBIOS)
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

*****************************************************************
*								*
* Drive Specification Table for the HD DMA hard disk controller *
*								*
*****************************************************************

mwdst:	db	maxmw*mwlog	;Number of logical drives
	dw	donop		;No warm boot for INSTALL
	dw	mwtran		;Sector translation
	dw	mwldrv		;Select logical drive 1 (First time select)
	dw	mwdrv		;Select logical drive 2 (General select)
	dw	mwhome		;Home current selected drive
	dw	mwseek		;Seek to selected track
	dw	mwsec		;Select sector
	dw	mwdma		;Set DMA address
	dw	mwread		;Read a sector
	dw	mwwrite		;Write a sector
	if	heads > 2	;Test if drive is big enough for a bad spot map
	dw	mwbad		;Return bad sector map info
	else
	dw	nobad
	endif

*****************************************************************
*								*
* The following are the lowest level drivers for the Morrow	*
* Designs Hard Disk DMA controller.				*
*								*
*****************************************************************

mwldrv	sta	mwcurl		;Save current logical drive
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

mwdrv	sta	mwcurl
	call	mwdlog
	mov	a,c
	sta	mwdrive		;Save new selected drive
mwsel	mvi	a,dmanoop
	jmp	mwprep		;Execute disk command

mwdlog:	mvi	c,0
mwllx:	sui	mwlog
	rc
	inr	c
	jmp	mwllx

mwstat	mvi	a,dmassta	;Sense status operation code
	jmp	mwprep		;Execute disk command

mwhome	call	mwreset		;Reset controller, do a load constants
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

mwbad:	lxi	h,mwbtab	;Return pointer to bad sector location
	ret

mwbtab:	dw	0		;Track 0
	dw	19		;Head 2, sector 0  = (2 * SPT + 0) + 1

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

mwreset	lhld	chan		;Save the command channel for a while
	shld	tempb
	lda	chan+2
	sta	tempb+2
	out	reset		;Send reset pulse to controller
	lxi	h,dmachan	;Address of command channel
	shld	chan		;Default channel address
	xra	a
	sta	chan+2		;Clear extended address byte
	shld	bios		;Set up a pointer to the command channel
	sta	bios+2
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

mwread	mvi	a,dmaread	;Load disk read command
	jmp	mwprep

mwwrite	mvi	a,dmawrit	;Load disk write command

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

	if	0		;Set to 1 for MW error reporter
mwissue	call	mwdoit		;Do desired operation
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

	endif

*****************************************************************
*								*
* IBIOS ram locations that don't need initialization.		*
*								*
*****************************************************************

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

*****************************************************************
*								*
* DPB and DPH area.						*
*								*
*****************************************************************

	if	maxhd ne 0

dphdsk	set	0		;Generate DPH's for the hdc3 hard disks
	rept	maxhd
ldsk	set	0
	rept	hdlog
	dphgen	hd,%dphdsk,dpbhd,%ldsk
ldsk	set	ldsk+1
dphdsk	set	dphdsk+1
	endm
	endm

	if	hdpart ne 0	;Use non-standard partitioning

*****************************************************************
*								*
* hdsectp is the number of 128 byte sectors per cylinder.	*
*								*
* hdtrks is the total number of data cylinders.  Eg.  it is	*
* the number of cyliders on the drive minus the number of	*
* cylinders that are used for the system.  If the number of	*
* 'system tracks' is not one then the initial value of		*
* 'off' should be adjusted accordingly.				*
*								*
* hdtrks = tracks - 1						*
*								*
*****************************************************************

	if	m10 ne 0
	hdsectp	equ	336		;Sectors per track
	hdtrks	equ	243		;Total data tracks
	endif

	if	m20 ne 0
	hdsectp	equ	672
	hdtrks	equ	243
	endif

	if	m26 ne 0
	hdsectp	equ	1024
	hdtrks	equ	201
	endif

ldsk	set	0		;Use non-standard partitioning
tracks	set	hdtrks/hdlog	;Number of tracks per partition
dsm	set	hdsectp/8*tracks/4-1	;Number of groups per partition
off	set	1

	rept	hdlog
	dpbgen	hd,%ldsk,%hdsectp,5,31,1,%dsm,511,0ffh,0ffh,0,%off,3
off	set	off+tracks
ldsk	set	ldsk+1
	endm

	else			;Else use standard DPB's

	if	m26 ne 0
dpbhd0	dw	1024		;CP/M sectors/track
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

dpbhd1	dw	1024		;CP/M sectors/track
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

dpbhd2	dw	1024		;CP/M sectors/track
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
dpbhd0	dw	336		;CP/M sectors/track
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

dpbhd1	dw	336		;CP/M sectors/track
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
dpbhd0	dw	672		;CP/M sectors/track
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

dpbhd1	dw	672		;CP/M sectors/track
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

dpbhd2	dw	672		;CP/M sectors/track
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
	endif
	endif			;End of HD DPH's and DPB's

	if	maxmf ne 0

	dpbgen	mf, 0, 20, 3,  7, 0, 04fh,  63, 0c0h, 0, 16, 3, 2
	dpbgen	mf, 1, 40, 3,  7, 0, 0a4h,  63, 0c0h, 0, 16, 2, 3
	dpbgen	mf, 2, 40, 4, 15, 1, 051h,  63,  80h, 0, 16, 2, 3
	dpbgen	mf, 3, 40, 4, 15, 1, 0a9h,  63,  80h, 0, 16, 2, 3

dn	set	0
	rept	maxmf
	dphgen	mf,%dn,dpbmf,%dn
dn	set	dn+1
	endm
	endif

	if	maxfd ne 0
dn	set	0
	rept	maxfd
	dphgen	fd,%dn,0,0
dn	set	dn+1
	endm
	endif

	if	maxdm ne 0
dn	set	0
	rept	maxdm
	dphgen	dm,%dn,0,0
dn	set	dn+1
	endm
	endif

	if	maxmw ne 0

*****************************************************************
*								*
* mwsectp is the number of 128 byte sectors per cylinder.	*
* mwsectp = 72 * heads						*
*								*
* mwtrks is the total number of data cylinders.			*
* mwtrks = tracks - 1						*
*								*
*****************************************************************

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

	if	mwpart ne 0	;Generate DPB's for a HDDMA hard disk

ldsk	set	0		;Use non-standard partitioning
tracks	set	mwtrks/mwlog	;Number of tracks per partition
dsm	set	mwsectp/8*tracks/4-1	;Number of groups per partition
off	set	1

	rept	mwlog
	dpbgen	mw,%ldsk,%mwsecpt,5,31,1,%dsm,1023,0ffh,0ffh,0,%off,4
off	set	off+tracks
ldsk	set	ldsk+1
	endm

	else			;Use standard partitioning

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
	endif

buffer	equ	$

*************************************************************************
*									*
* Alì codå placeä herå (thå disë deblockinç buffer© wilì dissapeaò	*
* thå firsô timå á disë read/writå calì ió executed®  However¬ iô	*
* ió safå tï placå onå timå initializatioî codå here®			*
*									*
*************************************************************************

*************************************************************************
*									*
* Set up the INSTALL package variables, set up the IBIOS		*
*									*
*************************************************************************

insetu:	lxi	sp,100h			;Set up a nice stack
	lxi	h,buff
	mov	a,m			;Get the text buffer contents
	ora	a			;Test for no flags
	jz	inse0			;Drive names default to O:, P:
	sui	'A'			;Convert to binary drive value
	cpi	16			;Test for legal values
	jnc	inse0			;Use default configuration if not legal
	sta	dlocal			;Bottom of the local drive list
	inx	h			;Bump to the next character

inse0:	mov	a,m			;Test the message flag
	cpi	'X'
	cnz	prhead			;Print the header if no x flag

	lxi	d,badmap		;Clear out bad map
	stax	d
	lxi	h,badmap+1
	lxi	b,9*badsiz		;32 map entries
	call	movbyt
	mvi	m,0ffh			;End marker

	xra	a			;Select disk A
	sta	cpmdrv

	lxi	h,buff			;Set up initial DMA address
	call	setdma
	xra	a			;A <- 0
	sta	bufsec			;Set buffer to unknown state
	sta	bufwrtn			;Set buffer not dirty flag
	sta	error			;Clear buffer error flag
	jmp	owboot			;Do host warm boot

;
;	Print the disk configuration message.
;
;	A: Host system drive B:
;	B: DJDMA 8" drive 0
;	C: DJDMA 8" drive 1
;	D: DJDMA 8" drive 2
;	E: DJDMA 8" drive 3
;	F: DJDMA 5 1/4" drive 0
;	G: DJDMA 5 1/4" drive 1
;	H: DJDMA 5 1/4" drive 2
;	I: DJDMA 5 1/4" drive 3
;	J: HDDMA M10 drive 0, partition A
;	L: HDDMA M10 drive 0, partition B
;	...
;

prhead:	lxi	h,prmpt0		;Print a login message
	call	prmesg
	lda	dlocal			;Print host drives before local drives
	moö	e,á			;Numbeò oæ drives
	mvi	d,0			;External drive 0
	mvi	c,0			;Start at host drive 0 ('A:')
	call	prhost			;Print the host lines

	lxi	h,prmpt1-1		;Replace all the '~'s with names
	lda	dlocal			;Starting drive
	adi	'A'
	mov	d,a

prhd0:	inx	h			;Bump to the next character
	mov	a,m			;Get a character from the message
	ora	a			;Test for the end of the message
	jz	prhd1			;Skip at end
	cpi	'~'			;Drive name marker
	jnz	prhd0			;Check out the next character
	mov	m,d			;Load a name
	inr	d			;Bump the name
	mov	a,d			;Test for drive overflow
	cpi	'A'+17			;16 drives max
	jnc	prhd1			;Skip on overflow
	jmp	prhd0			;Press on to the next character

prhd1:	mvi	m,0			;Terminate the message
	lxi	h,prmpt1		;Print the local configuration
	call	prmesg

	lxi	h,dlocal
	mov	c,m			;Load the starting host drive
	mov	a,c			;Figure the starting external drive
	adi	maxlog
	mov	d,a
	mvi	a,16			;Figure the number of drives
	sub	d
	jc	prhd2			;Skip if our drives went past drive P:
	mov	e,a
	call	prhost

prhd2:	ret

;
;	Print a the assignment lines, C = starting host drive,
;	D = Starting external drive, E = number of drives
;

prhost:	mov	a,e			;Test the count
	ora	a
	rz				;Return if done
	mov	a,d			;Load the External drive name
	adi	'A'
	sta	prmpt2
	mov	a,c			;Load the host drive name
	adi	'A'
	sta	prmpt3
	push	b			;Print a line
	push	d
	lxi	h,prmpt2
	call	prmesg
	pop	d
	pop	b
	inr	c			;Bump the host drive name
	inr	d			;Bump the external drive name
	dcr	e			;Bump the drive count
	jmp	prhost

;
;	Print a NULL terminated string via CP/M's conout routine
;

prmesg:	mov	a,m			;Test for the end of the string
	ora	a
	rz				;Return if done
	push	h			;Save the character pointer
	mov	e,a			;Print a character via CP/M
	mvi	c,cpmout
	call	bdos
	pop	h
	inx	h			;Bump the character pointer
	jmp	prmesg

msprnt	macro	dname, ndrive, npart	;Print a drive name
msdrv	set	'0'			;;Drive counter
	rept	ndrive
mspart	set	'A'			;;Partition counter
	rept	npart
	db	'~: ', &dname, ' drive ', msdrv	;;Print name and drive #
	if	npart gt 1		;;Print partition #'s only if > 1 parts
	db	', partition ', mspart
	endif
	db	acr, alf		;;End of the configuration line
mspart	set	mspart+1		;;Bump the partition counter
	endm
msdrv	set	msdrv+1			;;Bump the drive counter
	endm
	endm

prmpt0:	db	acr, alf, 'Morrow Designs installable IBIOS, revision '
	db	revnum/10+'@', '.', (revnum mod 10) + '0'
	db	'.', acr, alf, alf, 0

prmpt1:	equ	$			;Local drive configuration prompt

dn	set	1			;Generate the drive messages

	rept	numprd			;Cover all of our products

	if	dn eq hdorder		;Generate the HDCA's message
	if	m10 ne 0
	if	m10m ne 0
	msprnt	'HDCA + Memorex M10', maxhd, hdlog
	else
	msprnt	'HDCA + Fujitsu M10', maxhd, hdlog
	endif
	endif
	if	m20 ne 0
	msprnt	'HDCA + Fujitsu M20', maxhd, hdlog
	endif
	if	m26 ne 0
	msprnt	'HDCA + Shugart M26', maxhd, hdlog
	endif
	endif

	if	dn eq mworder		;Generate the HDDMA's message
	if	st506 ne 0
	msprnt	'HDDMA + M5', maxmw, mwlog
	endif
	if	st412 ne 0
	msprnt	'HDDMA + M10', maxmw, mwlog
	endif
	if	cm5619 ne 0
	msprnt	'HDDMA + M16', maxmw, mwlog
	endif
	endif

	if	dn eq fdorder		;Generate the 2D/B message
	if	fdorig eq 0e000h
	msprnt	'DJ2D/B @ E000', maxfd, 1
	else
	if	fdorig eq 0f000h
	msprnt	'DJ2D/B @ F000', maxfd, 1
	else
	if	fdorig eq 0f800h
	msprnt	'DJ2D/B @ F800', maxfd, 1
	else
	msprnt	'DJ2D/B @ ????', maxfd, 1
	endif
	endif
	endif
	endif

	if	dn eq dmorder		;Generate the DJDMA 8 message
	msprnt	'DJDMA 8"', maxdm, 1
	endif

	if	dn eq mforder		;Generate the DJDMA 5 1/4 message
	msprnt	'DJDMA 5 1/4"', maxmf, 1
	endif

dî	seô	dn+1
	endm

	db	0			;End of message

prmpt2:	db	' : Host system drive '
prmpt3:	db	' :', acr, alf, 0

	db	0,0ffh,0

	ds	512-($-buffer)		;Buffer for 512 byte sectors

	if	(maxfd ne 0) or (maxdm ne 0) or (maxmw ne 0)
	ds	512			;Additional space for 1k sector devices
	endif

*****************************************************************
*								*
* Each bad map entry consists of 9 bytes:			*
*	Logical drive number (1 byte)				*
*	Track number of bad sector (2 bytes)			*
*	Sector number of bad sector (2 bytes)			*
*	Track number of alternate sector (2 bytes)		*
*	Sector number of alternate sector (2 bytes)		*
*								*
*****************************************************************

badmap:	ds	badsiz*9+1		;32 entries + end marker

dirbuf:	ds	128			;Directory buffer

tempb:	ds	16			;A little temporary buffer

*****************************************************************
*								*
* Allocation and checked directory table area			*
*								*
*****************************************************************

	if	maxhd ne 0
	if	hdpart ne 0		;Use non-standard partitioning

tracks	set	hdtrks/hdlog		;Number of tracks per partition
dsm	set	hdsectp/8*tracks/4-1	;Number of groups per partition
alv	set	(dsm/8)+1

dn	set	0
	rept	maxhd*hdlog		;Generate CKS and ALV tables
	alloc	hd,%dn,%alv,0
dn	set	dn+1
	endm

	else				;Standard partitioning

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

	endif

	if	maxfd ne 0
dn	set	0

	rept	maxfd
	alloc	fd,%dn,75,64
dn	set	dn+1
	endm
	endif

	if	maxdm ne 0
dn	set	0

	rept	maxdm
	alloc	dm,%dn,75,64
dn	set	dn+1
	endm
	endif

	if	maxmf ne 0
dn	set	0
	rept	maxmf
	alloc	mf,%dn,22,16
dn	set	dn+1
	endm
	endif

	if	maxmw ne 0
	if	mwpart ne 0		;Use non-standard partitioning

tracks	set	mwtrks/mwlog		;Number of tracks per partition
dsm	set	mwsectp/8*tracks/4-1	;Number of groups per partition
alv	set	(dsm/8)+1
dn	set	0

	rept	maxmw*mwlog		;Generate CKS and ALV tables
	alloc	mw,%dn,%alv,0
dn	set	dn+1
	endm

	else				;Use standard partitioning

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
	endif

	db	0			;Force a complete hex file

	end
