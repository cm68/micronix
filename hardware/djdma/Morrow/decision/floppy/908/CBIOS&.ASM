*****************************************************************
*								*
* Morrow Designs CBIOS for CP/M Ver 2.2.			*
*								*
* This CBIOS can be configured to run with the following	*
* devices.  The disks may be configured to run with any or	*
* all of the disk systems.  The logical order of the disks	*
* can be set to any order.					*
*								*
* Disk systems:							*
*	HDC3 10, 20 and 26 megabyte hard disks.			*
*	HDDMA 5, 16, 40 megabyte hard disk systems.		*
*	DJDMA floppy controller with 8 and 5 1/4 inch disks.	*
*	DJ 2D/B floppy controller with 8 inch disks.		*
*								*
* Console I/O:							*
*	Disk Jockey 2D/B serial.				*
*	Disk Jockey DMA serial.					*
*	Multi I/O serial.					*
*	Decision I serial.					*
*								*
* Printer I/O:							*
* 	Multi I/O serial with handshaking.			*
*	Multi I/O Diablo 1620 simulator for the Hytype II.	*
*	Multi I/O Centronics parallel.				*
*								*
* Note:	Floppy systems diskette (drive A:) has to have 1024	*
*	byte sectors in order for the cold and warm boot	*
*	loaders to work.  Be sure to format all new		*
*	system diskettes with 1024 byte sectors.  The		*
*	system diskette can be either single or double		*
*	sided.  The sector size on normal (non A: drive)	*
*	diskettes is not restricted.  Thus if you have a	*
*	diskette with software that is supposed to run on	*
*	the A: drive then you should mount the diskette		*
*	in the B: drive and then PIP it over to a 1024		*
*	byte sector disktte.					*
*								*
* Written by Les Kent and Marc Kupper  		3/4/82		*
*								*
*****************************************************************

	title	'CBIOS Revision E for CP/M Version 2.2 - March 4, 1982'

revnum	equ	52		;Cbios revision number 5.x = E
cpmrev	equ	22		;CP/M revision number 2.2

debug	equ	0		;Set for debugging mode

*****************************************************************
*								*
* The following is set to the memory size of the CP/M the CBIOS	*
* is being created for.						*
*								*
*****************************************************************

msize	equ	48		;Memory size of target CP/M

biosln	equ	1900h		;BIOS length.  Also in ABOOT&.ASM

*****************************************************************
*								*
* The following equates set up the disk systems to be included	*
* along with the types of drives and the logical order of the 	*
* drives.							*
*								*
*****************************************************************

maxhd	equ	0		;Set to number of HDC3 hard disk drives
maxmw	equ	2		;Set to number of HDDMA hard disks
maxfd	equ	2		;Set to number of 2D/B floppies
maxdm	equ	2		;Set to number of DJ DMA floppies 8 inch
maxmf	equ	2		;Set to number of DJ DMA floppies 5 1/4 inch

hdorder	equ	0		;Set the order of logical drives ELSE 0 if
mworder	equ	1		; not included.
fdorder	equ	4
dmorder	equ	2
mforder	equ	3
				;HDC3 controller disk drives. Set only one
M26	equ	0		;Shugart SA4000
M20	equ	0		;Fujitsu M2302B
M10F	equ	0		;Fujitsu M2301B
M10M	equ	0		;Memorex

				;HDDMA controller disk drives. Set only one
M5	equ	1		;Seagate M506
M16A	equ	0		;Ampex DF516
M16C	equ	0		;CMI 5616
M19	equ	0		;CMI 5619
M40	equ	0		;Quantum Q2040

wmdrive	equ	0		;Device to warm boot from.  This is the
				; CP/M logical drive number.

badsiz	equ	32		;Number of badmap entries

*****************************************************************
*								*
* Since most hard disk drives hold more than 8 megabytes we	*
* partition the drive.  We partition our drives using two	*
* different formulas.						*
*								*
* One is the so called 'standard partitioning' where we try	*
* to create as many 8 megabyte partitions as possible plus	*
* a small partition to take up the slack on the end of the	*
* drive.							*
*								*
* Another way the drives are partitioned is the so called	*
* 'even partition' formula.  This means that the drive is	*
* split into equale sized partitions with the only		*
* restriction being that no partition be over 8 megabytes	*
* in length.							*
*								*
* All hard disk drives shipped from Morrow Designs are		*
* partitioned using the standard partition formula.  If the	*
* user wishes to implement even partitioning then he/she	*
* must set hdpart or mwpart to the number of partitions		*
* desired.							*
*								*
*****************************************************************

hdpart	equ	0		;Set to number of non standard partitions
mwpart	equ	0		;Set to number of non standard partitions

*****************************************************************
*								*
* The following equates define the console and printer		*
* environments.							*
*								*
*****************************************************************

*****************************************************************
*								*
* Define the console driver to be used.				*
*								*
* contyp is:	0	Nothing, used for patching to PROM's.	*
*		1	Provide for 128 bytes of patch space.	*
*		2	Multi I/O or Decision I driver.		*
*		3	2D/B driver.				*
*		4	DJDMA serial port			*
*								*
* Set cbaud to the divisor latch value for the console. 	*
* For an explanation of the values look for the defcon		*
* table.							*
*								*
*****************************************************************

contyp	equ	2
cbaud	equ	12

*****************************************************************
*								*
* Define the printer driver to be used.				*
*								*
* lsttyp is:	0	Nothing, used for patching to PROM's.	*
*		1	Provide for 128 bytes of patch space.	*
*		2	Multi I/O serial, no protocol.		*
*		3	Multi I/O serial, CTS protocol.		*
*		4	Multi I/O serial, DSR protocol.		*
*		5	Multi I/O serial, Xon / Xoff protocol.	*
*		6	Multi I/O parallel, Centronics.		*
*		7	Multi I/O parallel, Diablo HyType II.	*
*								*
* Note: The Decision board is functionally identical to the	*
*	Multi I/O board for printer I/O.  Selections 2 - 6	*
*	will work on the Decision I.				*
*								*
* Set pbaud to the divisor latch value for the printer. 	*
* For an explanation of the values look for the deflst		*
* table.							*
*								*
*****************************************************************

lsttyp	equ	2
lbaud	equ	96

*****************************************************************
*								*
* The next equate determines if you have a Multi I/O Rev 3	*
* or a Decision I mother board for parallel i/o.  If are not	*
* using either of these boards then you need not worry about	*
* this equate.  If you are using a Multi I/O rev. other than	*
* 3.x then you should set multr3 to 0.				*
*								*
*****************************************************************

multr3	equ	0		;0 = Decision, 1 = Multi I/O

	if	contyp eq 2
congrp	equ	1		;Cosole port (1 = p1, 2 = p2, 3 = p3)
	endif

	if	lsttyp ge 2
lstgrp	equ	3		;Printer port (1 = p1, 2 = p2, 3 = p3)
	endif

*****************************************************************
*								*
* The following equates are internal to the CBIOS.		*
*								*
*****************************************************************

m10	equ	m10f or m10m

	if	hdpart ne 0		;Use non standard partitions
hdlog	equ	hdpart
	else
hdlog	equ	m10*2+m20*3+m26*3	;Logical disks per drive for HDC3
	endif

	if	mwpart ne 0		;Use non standard partitions
mwlog	equ	mwpart
	else
mwlog	equ	m5*1+m16a*2+m16c*2+m19*2+m40*5	;Logical disks per drive
	endif

hdc3	equ	m26 or m20 or m10		;HDC3 controller
fujitsu	equ	m20  or m10f
hdrev	equ	26*m26+20*m20+10*m10		;Hard disk type
hdspt	equ	32*m26+21*m20+21*m10		;Sectors per track

hdma	equ	m5 or m16a or m16c or m19 or m40	;HD DMA controller
mwrev	equ	05*m5+16*m16a+16*m16c+19*m19+40*m40	;Hard disk type
mwspt	equ	9*m5+9*m16a+9*m16c+9*m19+9*m40		;Sectors per track

maxlog	equ	(maxhd*hdlog)+(maxmw*mwlog)+maxfd+maxdm+maxmf

*****************************************************************
*								*
* The folowing equates will define the Decision I mother	*
* board I/O or the Multi I/O environments if needed.		*
*								*
*****************************************************************

multio	equ	(contyp eq 2) or (lsttyp ge 2)	;Multi I/O board used?

	if	multio		;Define Multi I/O environment
mbase	equ	48h		;Base address of Multi I/O or Decision I
grpsel	equ	mbase+7		;Group select port
dll	equ	mbase		;Divisor (lsb)
dlm	equ	mbase+1		;Divisor (msb)
ier	equ	mbase+1		;Interupt enable register
clk	equ	mbase+2		;WB14 printer select port
lcr	equ	mbase+3		;Line control register
mcr	equ	mbase+4
lsr	equ	mbase+5		;Line status register
msr	equ	mbase+6
rbr	equ	mbase		;Read data buffer
thr	equ	mbase		;Tranmitter data buffer
dlab	equ	80h		;Divisor latch access bit
thre	equ	20h		;Status line THRE bit
cts	equ	10h		;Clear to send
dsr	equ	20h		;Data set ready
dr	equ	1		;Line status DR bit
wls0	equ	1		;Word length select bit 0
wls1	equ	2		;Word length select bit 1 for 8 bit word
stb	equ	4		;Stop bit count - 2 stop bits

; Define multi I/O ports addresses for group zero

gzero	equ	0
daisy0	equ	mbase		;Daisy input ports
daisy1	equ	mbase+1
sensesw	equ	mbase+1		;Sense switches

	if	multr3 eq 0	;Daisy output ports are different
daisi0	equ	mbase		;   for Decision I and Multi I/O.
daisi1	equ	mbase+1		;These two are the Decision I ports
	else
daisi0	equ	mbase+1		;   and these are the Multi I/O's.
daisi1	equ	mbase
	endif

; Define daisy 0 status input bits

ribbon	equ	01h		;End of ribbon
paper	equ	02h		;Paper out
cover	equ	04h		;Cover open
pfrdy	equ	08h		;Paper feed ready
crrdy	equ	10h		;Carriage ready
pwrdy	equ	20h		;Print wheel ready
check	equ	40h		;Printer check (error)
ready	equ	80h		;Printer ready

; Define daisy 0 status input bits for Diablo HyType II driver

crstrd	equ	1020h		;Carriage ready
pfstrd	equ	810h		;Paper feed ready
pwstrd	equ	2040h		;Print wheel ready

; Define daisy 0 output bits

d9	equ	01h		;Data bit  9
d10	equ	02h		;Data bit 10
d11	equ	04h		;Data bit 11
d12	equ	08h		;Data bit 12

pfstb	equ	10h		;Paper feed strobe
crstb	equ	20h		;Carriage strobe
pwstb	equ	40h		;Print wheel strobe
rest	equ	80h		;Printer restore (Ribbon lift on Multi I/O)

; Define clock select bits

rlift	equ	40h		;Ribbon lift
pselect	equ	80h		;Select (Not used by Diablo)

; Define Modem Control Register bits

dtrenb	equ	1		;DTR enable
rtsenb	equ	2		;RTS enable

; Define group select bits

s0	equ	01h		;Group number (0-3)
s1	equ	02h
smask	equ	03h
bank	equ	04h
enint	equ	08h
restor	equ	10h		;Printer restore on Multi I/O
denable	equ	20h		;Driver enable on Multi I/O

; Define special constants for the HyTyp II driver

cperi	equ	10		;Default to 10 characters per inch
lperi	equ	6		;Default lines per inch
hinc	equ	120		;Horizontal increments per inch
vinc	equ	48		;Vertical increments per inch
numtabs	equ	160		;Number of horizontal tabs
maxchrs	equ	1024		;Maximum number of printer characters to queue
maxrgt	equ	1584		;Maximum carriage position
dfrmln	equ	110		;Forms length times 10
autolf	equ	0		;Default to no Auto line feed.

	endif

*****************************************************************
*								*
* CP/M system equates.						*
*								*
*****************************************************************

ccpln	equ	800h
bdosln	equ	0e00h

size	equ	(msize*1024)
ccp	equ	size-(biosln+ccpln+bdosln)
bdos	equ	ccp+ccpln
bios	equ	ccp+ccpln+bdosln

offsetc	equ	2100h-bios	;Offset for sysgen

	if	debug
dbgtmp	set	offsetc		;DDT offset	! <debug>
dbgtmp	set	ccp		;CCP address	! <debug>
dbgtmp	set	bdos		;BDOS address	! <debug>
dbgtmp	set	bios		;Cbios address	! <debug>
	endif

cdisk	equ	4		;Address of last logged disk
buff	equ	80h		;Default buffer address
tpa	equ	100h		;Transient memory
intioby	equ	0		;Initial IOBYTE
iobyte	equ	3		;IOBYTE location
wbot	equ	0		;Warm boot jump address
entry	equ	5		;BDOS entry jump address

*****************************************************************
*								*
* The following are internal Cbios equates. Most are misc.	*
* constants.							*
*								*
*****************************************************************

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

*****************************************************************
*								*
* The following are the macros used in generating the DPH, DPB  *
* and allocation tables.					*
*								*
*****************************************************************

dpbgen	macro	nam,log,dspt,dbsh,dblm,dexm,ddsm,ddrm,dal0,dal1,dcks,doff
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
	db	0
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

*****************************************************************
*								*
* The following marco is used in generating the logical order	*
* of the CP/M drives.						*
*								*
*****************************************************************

order	macro	num
	if	num eq hdorder
	dw	hddst
	endif

	if 	num eq mworder
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

*****************************************************************
*								*
* The folloing are offset numbers of Device Specification 	*
* Tables.							*
*								*
*****************************************************************

d.wboot	equ	0	;Warm boot
d.stran	equ	1	;Sector translation
d.sel1	equ	2	;Drive select, Return DPH
d.sel2	equ	3	;Drive select
d.home	equ	4	;Home drive
d.strk	equ	5	;Set track
d.ssec	equ	6	;Set sector
d.sdma	equ	7	;Set DMA address
d.read	equ	8	;Read a physical sector
d.write	equ	9	;Write a physical sector
d.bad	equ	10	;Return pointer to bad sector info

*****************************************************************
*								*
* The jump table below must remain in the same order, the	*
* routines may be changed, but the function executed must be	*
* the same.							*
*								*
*****************************************************************

	org	bios		;Cbios starting address

	jmp	cboot		;Cold boot entry point
wboote:	jmp	wboot		;Warm boot entry point

	if	contyp ne 0
const:	jmp	cstty		;Console status routine
cin:	jmp	ciflsh		;Console input
cout:	jmp	cotty		;Console output
	else
	jmp	$		;Console status routine PROM pointer
cin:	jmp	$		;Console input PROM pointer
cout:	jmp	$		;Console output PROM pointer
	endif

	if	lsttyp ne 0
	jmp	list		;List device output
	else
	jmp	cout		;List device output
	endif

	jmp	punch		;Punch device output
	jmp	reader		;Reader device input
	jmp	home		;Home drive
	jmp	setdrv		;Select disk
	jmp	settrk		;Set track
	jmp	setsec		;Set sector
	jmp	setdma		;Set DMA address
	jmp	read		;Read the disk
	jmp	write		;Write the disk

	if	lsttyp ne 0
	jmp	listst		;List device status
	else
	jmp	$		;List device status
	endif

	jmp	sectran		;Sector translation

	if	maxfd ne 0
	jmp	fdsel		;Hookup for SINGLE.COM program
	else
	jmp	donop
	endif

*****************************************************************
*								*
* Immediatly following the SINGLE hookup jump we have a		*
* series of pointers that point to various internal tables.  	*
* At the start of each of these tables we have a revision	*
* byte and a length byte.   The revision byte is the		*
* current revision number for that particular structure and	*
* the length byte is the length of that structure.  This	*
* length does not include the revision byte nor the length	*
* byte.								*
*								*
* The pointers defined so far are as follows:			*
*								*
* 1)	This points to the drive configuration table.		*
*								*
* 2) 	This points to the I/O configuration bytes for		*
*	the serial drivers.  Eg, the console, printer,		*
*	reader, and punch devices.				*
*								*
* 3)	This will be a null (0) pointer.  It marks the		*
*	end of the table.					*
*								*
*****************************************************************

	dw	drconf		;Drive configuration table pointer
	dw	ioconf		;I/O configuration table pointer
	dw	0

*****************************************************************
*								*
* Drive configuration table.					*
*								*
*****************************************************************

drconf:	db	0		;Revision 0 structure
	db	32		;32 bytes long now

*****************************************************************
*								*
* The following is the table of pointers to the Device 		*
* Specification Tables.  The order of this table defines	*
* the logical order of the CP/M drives.  			*
*								*
*****************************************************************

dsttab:	equ	$

dn	set	1
	rept	16
	order	%dn
dn	set	dn+1
	endm

*****************************************************************
*								*
* I/O configuration table.					*
*								*
*****************************************************************

ioconf:	db	0		;Revision 0 structure
	db	5		;5 bytes long now

*****************************************************************
*								*
* The following two words define the default baud rate for	*
* the console and the LST: devices.  These words must		*
* immediatly follow the Cbios jump table so that the user	*
* can easly modify them and that they will also be used in	*
* the future by Morrow Designs software.			*
*								*
* The following is a list of possible baud rates and the	*
* value needed for the defcon or deflst words.			*
*								*
* Baud rate	defcon/deflst	Baud rate	defcon/deflst	*
*      	 50	2304		     2000	58		*
*	 75	1536		     2400	48		*
*	110	1047		     3600	32		*
*	134.5	 857		     4800	24		*
*	150	 768		     7200	16		*
*	300	 384		     9600	12		*
*	600	 192		    19200	 6		*
*      1200	  96		    38400	 3		*
*      1800	  64		    56000	 2		*
*								*
*****************************************************************

defcon:	dw	cbaud		;Console baud rate

deflst:	dw	lbaud		;Printer baud rate

*****************************************************************
*								*
* The next byte is to make sure that the group select byte	*
* on the Mult I/O or Decsion I stays consistant throughout	*
* the Cbios.  Only the group bits themselves (bits 0 and 1)	*
* should be changed as you output to the group port.  If	*
* you modify one of the other bits (such a driver-enable)	*
* then you should modify the same bit in the group byte		*
* provided.  Example:						*
*								*
*				;Select console group		*
*	lda	group		;Get group byte			*
*	ori	congrp		;Select the console port	*
*	out	grpsel		;Select the group		*
*								*
*				;Modify a bit in the group byte	*
*	lda	group		;Get group byte			*
*	ori	bank		;Set the bank bit		*
*	sta	group		;Save new group setting		*
*	ori	group2		;Select second serial port	*
*	out	grpsel		;Select the desired group	*
*								*
* Note: You should not set the group bits themselves in		*
*	the group byte.						*
*								*
*****************************************************************

group:	db	0		;Group byte

*****************************************************************
*								*
* Console driver routines.					*
*								*
* Routine used depends on the value of contyp.  Possible	*
* contyp values are listed as follows:				*
*								*
* contyp is:	0	Nothing, used for patching to PROM's.	*
*		1	Provide for 128 bytes of patch space.	*
*		2	Multi I/O or Decision I driver.		*
*		3	2D/B driver.				*
*		4	DJDMA serial port			*
*								*
*****************************************************************

ciflsh:	call	flush		;Flush disk buffers on input
	jmp	citty

*****************************************************************
*								*
* contyp: 1	Blank space for console driver			*
*								*
* Note: If the user plans to utilize this space then the	*
* one time code such as tinit{sould be placed just below	*
* the cboot routine.  This space (belowe cboot) is recyled	*
* for use as a disk buffer after cboot is done.			*
*								*
*****************************************************************

	if	contyp eq 1

tinit	equ	$		;Make it easy to find this place
cotty	equ	$
citty	equ	$
cstty	equ	$
	ret
	ds	127

	endif			;Blank space

*****************************************************************
*								*
* contyp: 2	Multi I/O or Decision I console driver		*
*								*
*****************************************************************

	if	contyp eq 2

*****************************************************************
*								*
* This driver on cold boot will inspect bits 1-3 of the sense	*
* switches.  If the value found is in the range 0-6 then the	*
* console baud rate will be taken from the rate table.		*
* Otherwise the current divisor latch value will be checked.	*
* If the divisor seems to be ok then no action will be taken	*
* as far as the baud rate setting goes.  If the divisor is not	*
* ok then the baud rate will be set from the DEFCON word	*
* which is found just below the regular Cbios jump table.  The	*
* standard divisor table is given below.			*
*								*
* Sense switch: 123  (0 = off, 1 = on)				*
*		000 = 110					*
*		001 = 300					*
*		010 = 1200					*
*		011 = 2400					*
*		100 = 4800					*
*		101 = 9600					*
*		110 = 19200					*
*	     defcon = 9600					*
*								*
* Note: If you are compiling with multr3 (a Multi I/O) then	*
*	the switches will not be available so the baud rate	*
*	will be taken from defcon.				*
*								*
*****************************************************************

*****************************************************************
*								*
* Due to its length, the tinit routine driver is below the	*
* cboot routine.						*
*								*
*****************************************************************

*****************************************************************
*								*
* Read a character from the serial port.			*
*								*
*****************************************************************

citty:	lda	group		;Get group byte
	ori	congrp		;Select console
	out	grpsel

conin1:	in	lsr		;Read status register
	ani	dr		;Wait till character ready
	jz	conin1
	in	rbr		;Read character
	ani	7fh		;Strip parity
	ret

*****************************************************************
*								*
* Output a character to serial port.				*
*								*
*****************************************************************

cotty:	lda	group		;Get group byte
	ori	congrp		;Select console
	out	grpsel

conout1:in	lsr		;Read status
	ani	thre		;Wait till transmitter buffer empty
	jz	conout1
	mov	a,c		;Character is in (c)
	out	thr		;Output to transmitter buffer
	ret

*****************************************************************
*								*
* Return serial port status.  Returns zero if character is not	*
* ready to be read.  Else returns 255 if ready.			*
*								*
*****************************************************************

cstty:	lda	group		;Get group byte
	ori	congrp		;Select console
	out	grpsel

	in	lsr		;Read status register
	ani	dr
	rz			;No charactter ready
	mvi	a,0ffh		;Character ready
	ret

	endif			;Multi I/O or Decision I

*****************************************************************
*								*
* contyp: 3	2DB console driver				*
*								*
*****************************************************************

	if	contyp eq 3

cotty:	jmp	fdcout		;Console output

citty:	jmp	fdcin		;Console input

cstty:	call	fdtstat		;Console status
	mvi	a,0ffh
	rz
	inr	a
	ret

	endif			;2DB

*****************************************************************
*								*
* contyp: 4	DJDMA console driver				*
*								*
*****************************************************************

	if	contyp eq 4
cotty:	lxi	h,dmchan
	mvi	m,serout	;Command for serial output
	inx	h
	mov	m,c
	jmp	docmd

citty:	lxi	h,serin+1	;Serial input status
	xra	a
ci2:	cmp	m		;Wait till 40h deposited at 3fH
	jz	ci2
	mov	m,a		;Clear status
	dcx	h		;Point to input data
	mvi	a,7fh		;For masking out parity
	ana	m
	ret

cstty:	lda	serin+1		;Pick up serial input status
	ora	a
	rz			;If zero then no character ready
	mvi	a,0ffh		;Set character ready
	ret
	endif

*****************************************************************
*								*
* LST: device driver routines.					*
*								*
* Routine used depends on the value of lsttyp.  Possible	*
* lsttyp values are listed as follows:				*
*								*
* lsttyp is:	0	Nothing, used for patching to PROM's.	*
*		1	Provide for 128 bytes of patch space.	*
*		2	Multi I/O serial, no protocol.		*
*		3	Multi I/O serial, CTS protocol.		*
*		4	Multi I/O serial, DSR protocol.		*
*		5	Multi I/O serial, Xon / Xoff protocol.	*
*		6	Multi I/O parallel, Centronics.		*
*		7	Multi I/O parallel, Diablo HyType II.	*
*								*
*****************************************************************

*****************************************************************
*								*
* lsttyp: 1	Blank space for printer driver			*
*								*
* Note: If the user plans to utilize this space then the	*
* one time code such as linit sould be placed just below	*
* the cboot routine.  This space (belowe cboot) is recyled	*
* for use as a disk buffer after cboot is done.			*
*								*
*****************************************************************

	if	lsttyp eq 1

linit	equ	$		;Make it easy to find this place
list	equ	$
listst	equ	$
	ret
	ds	127

	endif			;Blank space

*****************************************************************
*								*
* lsttyp: 2	Serial printer, no protocol			*
*								*
* lsttyp: 3	Serial printer, CTS protocol			*
*								*
* lsttyp: 4	Serial printer, DSR protocol			*
*								*
* lsttyp: 5	Serial printer, Xon / Xoff protocol		*
*								*
*****************************************************************

	if	(lsttyp ge 2) and (lsttyp le 5)

list:	lda	group		;Get group byte
	ori	lstgrp		;Select list device
	out	grpsel

ll:	in	lsr
	ani	thre		;Wait till transmitter buffer empty
	jz	ll

*****************************************************************
*								*
* The CTS driver is used for a printer with hardware		*
* handshaking (TI 810).  It should be connected to the CTS	*
* input on the list device serial port.				*
*								*
*****************************************************************

	if	lsttyp eq 3	;CTS protocol
	in	msr
	ani	cts		;Wait till clear to send
	jz	ll
	endif

*****************************************************************
*								*
* The DSR driver is used for a printer with hardware		*
* handshaking (TI 810).  It should be connected to the DSR	*
* input on the list device serial port.				*
*								*
*****************************************************************

	if	lsttyp eq 4	;DSR protocol
	in	msr
	ani	dsr
	jz	ll		;Wail till DSR comes up
	endif

*****************************************************************
*								*
* The Xon/Xoff driver is used for a printer with software	*
* handshaking (Diablo 630).					*
*								*
*****************************************************************

	if	lsttyp eq 5	;Xon / Xoff protocol
xloop:	call	listst		;Check printer status
	ora	a
	jz	xloop		;Loop if not ready
	endif

	mov	a,c
	out	thr
	ret

listst:	lda	group		;Get group byte
	ori	lstgrp		;Select list device
	out	grpsel

	in	lsr		;Check if transmitter buffer empty
	ani	thr
	rz			;Return not ready

	if	lsttyp eq 3	;CTS protocol
	in	msr
	ani	cts
	rz			;Return not ready if CTS is false
	endif

	if	lsttyp eq 4	;DSR protocol
	in	msr
	ani	dsr
	rz			;Return not ready if DSR is true
	endif

	if	lsttyp eq 5	;Xon / Xoff protocol
	mvi	b,xon		;Last character recieved from printer
lstflg	equ	$-1
	in	lsr
	ani	dr		;Check for a character
	jz	xskip		;No character present
	in	rbr		;Get character
	ani	7fh
	mov	b,a		;Save
	sta	lstflg		;Kludge flag (last character recieved)
xskip:	mov	a,b
	sui	xoff		;Check for Xoff char (control S)
	jnz	xsdone		;Printer ready
	ret			;Printer not ready

xsdone	equ	$		;Printer ready for data
	endif

	mvi	a,0ffh
	ret			;Printer ready

	endif			;Multi I/O serial

*****************************************************************
*								*
* lsttyp: 6	Centronics parallel printer driver.		*
*								*
*****************************************************************

	if	lsttyp eq 6

*****************************************************************
*								*
* Decision I Diablo parallel to Centronics parallel interface.	*
*								*
* The following cable must be made from the 50 pin Diablo	*
* conector to the 35 pin Centronics.				*
*								*
* Centronics			Multi I/O			*
* pin	signal			pin	signal			*
* 1	/strobe		<-	46	/d9			*
* 2	data1		<-	37	/d1			*
* 3	data2		<-	36	/d2			*
* 4	data3		<-	39	/d3			*
* 5	data4		<-	33	/d4			*
* 6	data5		<-	40	/d5			*
* 7	data6		<-	42	/d6			*
* 8	data7		<-	43	/d7			*
* 9	data8		<-	45	/d8			*
* 10	/acknlg		->	12	/check			*
* 11	busy		->	28	/printer ready		*
* 12	pe		->	3	/paper			*
* 13	slct		->	4	/ribbon			*
* 14	/auto feed xt	<-	1	/d10			*
* 15	NC							*
* 16	0V		<->	2	GND			*
* 17	chassis gnd						*
* 18	NC							*
* 19	/strobe rtn	<->	8	GND			*
* 20	data1 rtn	<->	11	GND			*
* 21	data2 rtn	<->	14	GND			*
* 22	data3 rtn	<->	16	GND			*
* 23	data4 rtn	<->	18	GND			*
* 24	data5 rtn	<->	20	GND			*
* 25	data6 rtn	<->	22	GND			*
* 26	data7 rtn	<->	25	GND			*
* 27	data8 rtn	<->	38	GND			*
* 28	/acknlg rtn	<->	41	GND			*
* 29	busy rtn	<->	44      GND			*
* 30	pe rtn		<->	47	GND			*
* 31	/init		<-	9	/d11			*
* 32	/error		->	5	/cover			*
* 33	GND		<->	35	GND			*
* 34	NC							*
* 35	/slct in	<-	10	/d12			*
*				35 <-> 24 /Select		*
*								*
* IMPORTANT: For this interface to work /select (24) on the	*
* parallel conector must be tied to ground (35).		*
*								*
*****************************************************************

list:	lda	group		;Get group byte
	out	grpsel

rl:	in	daisy0		;Wait till printer ready and selected
	ani	ready+paper
	jz	rl
pl:	in	daisy0		;Test if out of paper
	ani	ribbon
	jnz	pl
el:	in	daisy0
	ani	cover
	jnz	el
	mov	a,c		;Move character into (a)
	out	daisi1		;Latch data
	mvi	a,d11+d10+d9	;Make sure strobe is high
	out	daisi0
	dcr	a		;Pulse strobe low
	out	daisi0
	inr	a
	out	daisi0

ack:	in	daisy0		;Wait till ready again
	ani	ready
	jz	ack
	ret

listst:	lda	group		;Get group byte
	out	grpsel		;Select group zero

	in	daisy0		;Wait till printer ready and selected
	ani	ready+paper
	rz
	in	daisy0		;Test if out of paper
	ani	ribbon
	rz
	in	daisy0
	ani	cover
	xri	cover
	rz
	dcr	a
	ret

	endif			;Centronics parallel

	if	lsttyp eq 7	;Diablo HyTyp II

*****************************************************************
*								*
* Diablo 1620 simulator for the Diablo Hytype II.		*
*								*
*****************************************************************

*****************************************************************
*								*
* This routine does all of the character decoding, escape	*
* sequences forward, backward, etc. The list of escape		*
* sequences, and special characters recognized is:		*
*								*
*	adel		ignored					*
*	anul		ignored					*
*	aack		ignored	(when received)			*
*	abel		ignored					*
*	aff		form feed				*
*	aetx		etx/ack handshake			*
*	aht		horizontal tab				*
*	alf		line feed				*
*	asp		space					*
*	abs		backspace				*
*	acr		carriage return				*
*	aesc 0		ignored					*
*	aesc 1		set tab stop at current print position	*
*	aesc 2		clear all tab stops			*
*	aesc 3		graphics mode on			*
*	aesc 4		graphics mode off			*
*	aesc 5		forward print				*
*	aesc 6		backward print				*
*	aesc 8		clear tab stop				*
*	aesc 9		set left margin				*
*	aesc A		ignored					*
*	aesc B		ignored					*
*	aesc D		negative half line feed			*
*	aesc U		half line feed				*
*	aesc alf	negative line feed			*
*	aesc aht c	absolute horizontal tab			*
*	aesc avt c	absolute vertical tab			*
*	aesc ars c	set vmi					*
*	aesc aus c	set hmi					*
*								*
*****************************************************************

list:	lda	group		;Set printer initialized flag
	ori	denable
	sta	group
	mov	a,c		;Get the character to print
	ani	7fh		;Strip off parity
	rz
	cpi	adel		;Ignore delete
	rz
	mov	c,a		;Save character
	lda	escflg
	lxi	h,level0	;Level zero characters
	ana	a
	mov	a,c		;Scan for char in A
	jz	lookup		;Look up activity for this character
	lda	escflg
	lxi	h,level1	;Single character escape sequences
	cpi	aesc
	mov	a,c		;Scan for char in A
	jz	lookup		;Execute single level escape sequence
	lxi	h,level2	;Two character escape sequence
	lda	escflg

*****************************************************************
*								*
* Lookup scans the table pointed at by HL looking for a match	*
* of the character in register A.				*
*								*
*****************************************************************

lookup:	dcr	m		;Test if end of table
	inr	m
	jz	gother		;Execute the default function
	cmp	m		;Otherwise test for a match
	jz	gother
	inx	h		;Bump over character
	inx	h		;Bump over function address
	inx	h
	jmp	lookup
gother:	inx	h		;Bump over character
	mov	a,m		;Get low byte of function address
	inx	h
	mov	h,m		;Get high byte of function address
	mov	l,a		;Form Address of function
	pchl			;Execute it

*****************************************************************
*								*
* Each of the following tables contains entries of the form:	*
*	1 byte character to match				*
*	2 bytes of address to execute				*
* terminated by a first byte of 0.				*
*								*
*****************************************************************

level0:	db	aesc
	dw	doaesc		;Beginning of an escape sequence
	db	aff
	dw	doaff		;Form feed
	db	aetx
	dw	doaetx
	db	aht
	dw	doaht		;horizontal tab
	db	alf
	dw	doalf		;Line feed
	db	asp
	dw	doasp		;Space
	db	abs
	dw	doabs		;Back space
	db	acr
	dw	doacr		;Carriage return
	db	0
	dw	dochar		;Any other character

level1:	db	'1'
	dw	sethtab		;Set horizontal tab
	db	'2'
	dw	clrall		;Clear all horizontal tabs
	db	'3'
	dw	setgrp		;Graphics mode
	db	'4'
	dw	clrgrp		;Clear graphics mode
	db	'5'
	dw	clrdir		;Forward printing
	db	'6'
	dw	setdir		;Backward printing
	db	'8'
	dw	clrhtab		;Clear horizontal tab
	db	'9'
	dw	setlmar		;Set left margin
	db	'0'
	dw	func1		;No operation level 1
	db	'A'
	dw	func1
	db	'B'
	dw	func1
	db	'a'
	dw	func1
	db	'b'
	dw	func1
	db	'D'
	dw	neghlf		;Negative half line feed
	db	'U'
	dw	poshlf		;Half line feed
	db	alf
	dw	neglf		;Negative line feed
	db	aht
	dw	settwo		;Two character escape sequence
	db	avt
	dw	settwo
	db	ars
	dw	settwo
	db	aus
	dw	settwo
	db	0
	dw	func1

level2:	db	aht
	dw	abshtab		;Absolute horizontal tab
	db	avt
	dw	absvtab		;Absolute vertical tab
	db	ars
	dw	setvmi
	db	aus
	dw	sethmi
	db	0
	dw	func2

*****************************************************************
*								*
* The following routines execute escape sequences, etc.		*
*								*
*****************************************************************

settwo:
doaesc:	mov	a,c		;Get the escape character
	sta	escflg
func0:	ret

doaetx:	ret

doalf:	call	lfvmi		;Get line feed vmi
adjvp:	xchg
	lhld	dlvpos		;Get vertical motion displacement
	dad	d
	shld	dlvpos
	ret

lfvmi:	lda	grhflg
	ana	a
	lxi	h,1		;Only 1/48 if in graphics mode
	rnz
	lhld	vmi		;Get vertical motion index
	ret

neglf:	call	lfvmi		;Get line feed vmi
	call	neghl
	call	adjvp
	jmp	func1

doasp:	call	sphmi		;Get space horizontal motion
spdir:	lda	dirflg		;Forward or backwards ?
	ana	a
	cnz	neghl		;Negate HL
adjhp:	xchg			;Adjust Horizontal position
	lhld	dlhpos		;Get current adjustment
	dad	d		;Update it
	shld	dlhpos		;And save
	ret

sphmi:	lda	grhflg		;In graphics mode ?
	ana	a
	lxi	h,2		;Only 1/60 if in graphics mode
	rnz
	lhld	hmi
	ret

doabs:	call	sphmi		;Space increment
	call	neghl		;Negative to start with
	jmp	spdir		;Adjust backwards

doacr:	xra	a
	sta	dirflg		;Forward printing
	sta	grhflg		;No graphics mode
	lhld	hpos		;Get current offset
	xchg
	lhld	lmar		;Get left margin
	call	hlmde
	shld	dlhpos		;Don't move yet though
	mvi	a,autolf	;In Auto line feed mode ?
	ana	a
	jnz	doalf		;Do line feed also
	ret

dochar:	mov	l,c
	mvi	h,0
	call	wheel		;Print the character in register C
	lda	grhflg
	ana	a
	lxi	h,0		;Don't move if in graphics mode
	jnz	spdir
	lhld	hmi
	jmp	spdir

clrall	equ	$		;Clear all horizontal tabs
	lxi	h,tabstp	;Beginning of tab stop array
	mvi	d,tablen	;Size of tab array (bytes)
notblp:	mvi	m,80h		;Reset tabs (reset to 0 later)
kludge	equ	$-1		;Used on first reset (warmboot)
	inx	h		;Next tab stop
	dcr	d		;Update repeat count
	jnz	notblp		;Continue zeroing
func2	equ	$
func1:	xra	a		;Clear escape sequence flag
	sta	escflg
	ret

setgrp:	mvi	a,1		;Set graphics mode on
	sta	grhflg
	jmp	func1

clrgrp:	xra	a		;Turn graphics mode off
	sta	grhflg
	jmp	func1

clrdir:	xra	a		;Forward print mode
	sta	dirflg
	jmp	func1

setdir:	mvi	a,a		;Set backward printing mode
	sta	dirflg
	jmp	func1

setlmar:lhld	hpos		;Get current position
	xchg
	lhld	dlhpos		;Get offset
	dad	d
	shld	lmar
	jmp	func1

setvmi:	mov	l,c		;Set the motion index
	mvi	h,0
	dcx	h
	shld	vmi
	jmp	func2

sethmi:	mov	l,c
	mvi	h,0
	dcx	h
	shld	hmi
	jmp	func2

poshlf:	call	hlfvmi		;Half line feed vmi
	call	adjvp
	jmp	func1

neghlf:	call	hlfvmi		;Negative half line feed
	call	neghl
	call	adjvp
	jmp	func1

hlfvmi:	lhld	vmi		;Get vmi for full line feed
divid2:	mov	a,h		;High byte
	ora	a		;Clear the carry
	rar
	mov	h,a
	mov	a,l
	rar
	mov	l,a
	ret

abshtab:mov	e,c		;Absolute horizontal tab
	mvi	d,0
	dcx	d		;Form 16 bit tab column
	call	newdlh
	jmp	func2

newdlh:	lhld	hmi
	call	hltde		;Multiply by hmi
	xchg
	lhld	hpos		;And subtract current horizontal position
	xchg
	call	hlmde
	shld	dlhpos
	ret

absvtab:mov	e,c		;Absolute vertical tab
	mvi	d,0
	dcx	d
	lhld	vmi
	call	hltde		;Multiply by vmi
	xchg
	lhld	vpos		;And subtract the current vertical position
	xchg
	call	hlmde
	shld	dlvpos
	jmp	func2

sethtab:call	tabcol		;Set horizontal tab
	ora	m		;OR in tab stop
	mov	m,a		;   and save
	jmp	func1

tabcol:	lhld	hpos		;Compute address of current character col
	xchg
	lhld	dlhpos
	dad	d		;Get logical position
	xchg
	lhld	hmi		;And divide by hmi to get character column
	xchg
	call	hldde

mtabp:	;Make a tab pointer
	;HL -> Tab column desired (1-160)
	;HL <- address of tab stop
	; A <- bit mask for tab stop
	lxi	d,8		;Number of stops per byte
	call	hldde		;HL/DE -> HL, HL mod DE -> DE
	mov	c,e		;Save
	inr	c		;Make range (1-8)
	lxi	d,tabstp	;Tab array
	dad	d		;Make array pointer
	xra	a
	stc
mtab0:	rar
	dcr	c		;Bump bit counter
	jnz	mtab0
	ret

clrhtab:call	tabcol		;Clear horizontal tab
	cma
	ana	m		;Mask out tab stop
	mov	m,a
	jmp	func1

doaht:	lhld	hpos		;Compute address of current character col
	xchg
	lhld	dlhpos
	dad	d		;Get logical position
	xchg
	lhld	hmi		;And divide by hmi to get character column
	xchg
	call	hldde
tablop:	lxi	d,numtabs
	inx	h		;Start with next position
	call	hlcde		;Compare position with number of tabs
	jnc	tofar		;Past last tab
	push	h		;Save col pointer
	call	mtabp		;Generate tab pointer
	ana	m		;Check out tab stop
	pop	h		;Restore col pointer
	jz	tablop		;Loop if stop not set
	xchg
	jmp	newdlh		;Set new col position and return
tofar:	lhld	hpos		;Go all the way to the right
	xchg
	lxi	h,maxrgt
	call	hlmde
	shld	dlhpos
	ret

doaff:	lxi	h,dfrmln	;Multiply forms length by 48
	lxi	d,48
	call	hltde
	lxi	d,10
	call	hldde		;And divide it by 10
	push	h		;Save this result
	lhld	vpos		;Get logical vertical position
	xchg
	lhld	dlvpos
	dad	d
	pop	d
	push	d		;Get copy of forms length
	call	hldde		;HL mod DE
	xchg
	pop	d
	xchg
	call	hlmde
	xchg
	lhld	dlvpos
	dad	d
	shld	dlvpos
	jmp	papr

*****************************************************************
*								*
* Neghl forms the twos complement of HL.			*
*								*
*****************************************************************

neghl:	mov	a,h
	cma
	mov	h,a
	mov	a,l
	cma
	mov	l,a
	inx	h
	ret

*****************************************************************
*								*
* Hlmde subtracts DE from HL and returns.			*
*								*
*****************************************************************

hlmde:	xchg
	call	neghl
	xchg
	dad	d
	ret

*****************************************************************
*								*
* Hlcde compares HL with DE. On return the Z flag is set if	*
* they are equal, the Carry flag is set if HL is less than DE.	*
*								*
*****************************************************************

hlcde:	mov	a,h
	cmp	d
	rnz
	mov	a,l
	cmp	e
	ret

*****************************************************************
*								*
* Divide the number in HL by the number in DE.	Return the 	*
* quotient in HL and the remainder in DE.			*
*								*
*****************************************************************

hldde:	mov	a,d		;Start by negating DE and
	cma			;	moving the left operand to BC
	mov	b,a
	mov	a,e
	cma
	mov	c,a
	inx	b
	mvi	a,16		;Repeat count in reg A
	lxi	d,0		;Initial remainder is zero
div3:	dcr	a		;Test if done
	rm			;All done ?
	dad	h		;Shift right operand to the left
	xchg
	push	psw		;Save carry
	dad	h		;Shift left operand to the left
	pop	psw
	jnc	div1		;Does it fit ?
	inx	h
div1:	push	h
	dad	b
	jnc	div2
	xchg
	inx	h
	xthl
	pop	h
	jmp	div3
div2:	pop	h
	xchg
	jmp	div3

*****************************************************************
*								*
* Multiply the contents of HL by the contents of DE.		*
*								*
*****************************************************************

hltde:	mov	c,l
	mov	b,h
	lxi	h,0
mult:	mov	a,b
	ora	c
	rz
	mov	a,b
	ora	a
	rar
	mov	b,a
	mov	a,c
	rar
	mov	c,a
	cc	dadde
	xchg
	dad	h
	xchg
	jmp	mult
dadde:	dad	d
	ret

*****************************************************************
*								*
* The routines below actually interface to the printer,		*
* causing paper feed, carriage, and print wheel motion.		*
*								*
*****************************************************************

carrg:	lhld	dlhpos		;Check for any accumulated motion
	mov	a,h
	ora	l
	rz
	lhld	hpos		;Check for too much motion
	xchg
	lhld	dlhpos
	dad	d
	mov	a,h
	ana	a
	jp	lftok
	lhld	hpos
	call	neghl
	shld	dlhpos
lftok:	lhld	hpos
	xchg
	lhld	dlhpos
	dad	d
	lxi	d,maxrgt
	call	hlcde
	jc	rgtok
	lhld	hpos		;Otherwise move only to maxright
	xchg
	lxi	h,maxrgt
	call	hlmde
	shld	dlhpos
rgtok:	lhld	hpos		;Update the horizontal position
	xchg
	lhld	dlhpos
	dad	d
	shld	hpos
	lhld	dlhpos		;check if required motion is to the left
	mov	a,h
	ana	a
	mvi	c,0
	jp	posh
	call	neghl
	mvi	c,d11
posh:	xchg
	lxi	h,0
	shld	dlhpos		;Reset the horizontal increment
	xchg
	mov	a,l
	ani	1
	jz	nohhlf		;No half spaces
	mov	a,c
	ori	d12
	mov	c,a
nohhlf:	call	divid2
	mov	a,h
	ani	d9+d10
	ora	c
	mov	h,a
	lxi	d,crstrd
	jmp	cmnd

papr:	lhld	dlvpos		;Check for any paper motion
	mov	a,h
	ora	l
	rz			;No motion
	mov	a,h
	ana	a
	mvi	c,0
	jp	posv
	call	neghl
	mvi	c,d11
posv:	mov	a,h
	ani	d9+d10
	ora	c
	mov	h,a
	push	h		;Save paper motion
	lhld	vpos
	xchg
	lhld	dlvpos		;Get logical position
	dad	d
	push	h		;Save for now
	lxi	h,dfrmln	;Get default form length
	lxi	d,48
	call	hltde		;Multiply by 48
	lxi	d,10
	call	hldde		;Divide by 10
	pop	d
	xchg
	call	hldde		;Compute HL mod DE
	xchg
	shld	vpos		;Save new vertical position
	lxi	h,0
	shld	dlvpos		;Reset vertical motion
	pop	h
	lxi	d,pfstrd	;Paper feed strobe
	jmp	cmnd

wheel:	push	h
	call	carrg		;Position the carriage first
	call	papr
	pop	h
	lxi	d,pwstrd

cmnd:	lda	group		;Get group byte
	out	grpsel		;Select group zero

cmnd0:	in	daisy0
	ana	d
	jz	cmnd0
	mov	a,l		;Negate low data bits
	cma
	mov	l,a
	mov	a,h
	ani	d9+d10+d11+d12	;Mask in data bits only
	cma
	if	multr3		;Mask out ribbon lift bit on Multi I/O
	ani	0ffh-rest
	endif
	mov	h,a
	mov	a,l
	out	daisi1		;Output low bits
	mov	a,h
	out	daisi0		;Output high bits
	xra	e		;Slap strobe bits in
	out	daisi0
	mov	a,h		;And drop strobes back down
	out	daisi0
	ret

*****************************************************************
*								*
* New list device status routine. Returns 0ffh if the printer	*
* can except another character, otherwise it returns 0.		*
*								*
*****************************************************************

listst:	lda	group		;Check printer initialized flag
	ani	denable
	rz			;0 = printer not initialized
	lda	group		;Get group byte
	out	grpsel		;Select group zero
	lxi	d,pwstrd
	in	daisy0
	ana	d
	xra	a
	rz
	cma
	ret

*****************************************************************
*								*
* Dynamic data locations used by the simulator.			*
*								*
*****************************************************************

hmi:	dw	0		;Horizontal motion index. Set by linit
				;	and escape sequences.
vmi:	dw	0		;Vertical motion index. Set by linit
				;	and escape sequences.
vpos:	dw	0		;Vertical position. Set by platen motion
dlvpos:	dw	0		;Delta vpos. Set by platen motion
hpos:	dw	0		;Horizontal position. Set by carriage motion
dlhpos:	dw	0		;Delta hpos. Set by carriage motion
lmar:	dw	0		;Left margin
dirflg:	db	0		;Direction flag
grhflg:	db	0		;Graphics mode flag
escflg:	db	0		;Escape sequence in progress flag

tabstp:	ds	numtabs/8+1	;Tab stops bit array
tablen	equ	numtabs/8+1	;Length of tabs array

	endif

*****************************************************************
*								*
* The following routines are used to make the reader and punch	*
* devices peform I/O through the console.  The user may patch	*
* here for their particular devices.				*
*								*
*****************************************************************

punch:	jmp	cout

reader:	jmp	cin

*****************************************************************
*								*
* Gocpm is the entry point from cold boots, and warm boots. It	*
* initializes some of the locations in page 0, and sets up the	*
* initial DMA address (80h).					*
*								*
*****************************************************************

gocpm:	lxi	h,buff		;Set up initial DMA address
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

	lda	cwflg		;Get cold/warm boot flag
	ora	a
	lxi	d,coldbeg	;Beginning of initial command
	mvi	a,coldend-coldbeg+1 ;Length of command
	jz	cldcmnd
	lxi	d,warmbeg
	mvi	a,warmend-warmbeg+1
cldcmnd:lxi	h,ccp+8		;Command buffer
	sta	ccp+7		;Store length in CCP internal buffer
	mov	c,a		
	mvi	b,0
	call	movbyt		;Move command to internal CCP buffer
	lda	cwflg
	ora	a
	lda	autoflg
	jz	cldbot
	rar
cldbot:	rar
	lda	cdisk		;Jump to CP/M with currently selected disk in C
	mov	c,a
	jc	ccp
	jmp	ccp+3		;Enter CP/M

cwflg:	db	0		;Cold/warm boot flag

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

autoflg:db	0		;Auto command feature

*****************************************************************
*								*
* If there is a command inserted here, it will be given if the	*
* auto feature is enabled.					*
*	For Example:						*
*								*
*	coldbeg	db	'MBASIC MYPROG'				*
*	coldend	db	0					*
*								*
* will execute microsoft basic, and mbasic will execute the	*
* "MYPROG" basic program.					*
*								*
*****************************************************************

coldbeg:db	''		;Cold boot command goes here
coldend:db	0
warmbeg:db	''		;Warm boot command goes here
warmend:db	0

*****************************************************************
*								*
* WBOOT loads in all of CP/M except the CBIOS, then initializes	*
* system parameters as in cold boot. See the Cold Boot Loader	*
* listing for exactly what happens during warm and cold boots.	*
*								*
*****************************************************************

wboot:	lxi	sp,tpa		;Set up stack pointer
	mvi	a,1
	sta	cwflg		;Set cold/warm boot flag

	mvi	h,wmdrive	;Move drive to warm boot off of into (h)
	mvi	l,d.wboot	;Peform warm boot operation
	call	jumper
	jnc	gocpm		;No error

	hlt			;Halt computer
	db	0

	jmp	wboot		;In case user restarts the computer

*****************************************************************
*								*
* Setsec just saves the desired sector to seek to until an	*
* actual read or write is attempted.				*
*								*
*****************************************************************

setsec:	mov	h,b		;Enter with sector number in (bc)
	mov	l,c
	shld	cpmsec
donop:	ret

*****************************************************************
*								*
* Setdma saves the DMA address for the data transfer.		*
*								*
*****************************************************************

setdma:	mov	h,b		;Enter with DMA address in (bc)
	mov	l,c
	shld	cpmdma		;CP/M dma address
	ret

*****************************************************************
*								*
* Home is translated into a seek to track zero.			*
*								*
*****************************************************************

home:	lxi	b,0		;Track to seek to

*****************************************************************
*								*
* Settrk saves the track # to seek to. Nothing is done at this	*
* point, everything is deffered until a read or write.		*
*								*
*****************************************************************

settrk:	mov	h,b	;Enter with track number in (bc)
	mov	l,c
	shld	cpmtrk
	ret

*****************************************************************
*								*
* Sectran translates a logical sector number into a physical	*
* sector number. 						*	
*								*
*****************************************************************

sectran:lda	cpmdrv		;Get the Drive Number
	mov	h,a		;Drive in (h)
	mvi	l,d.stran
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

setdrv:	mov	a,c		;Save the logical drive number
	sta	cpmdrv
	cpi	maxlog		;Check for a valid drive number
	jnc	zret		;Illegal drive

	mov	a,e		;Check if bit 0 of (e) = 1
	ani	1
	jnz	setd3		;Drive has allready been accessed.

	mov	h,c		;Move logical drive into (h)
	mvi	l,d.sel1
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
	mvi	l,d.sel2	;Select drive
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

gdph:	lda	cpmdrv		;Return pointer to DPH save area.
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
* DPH save area.  Each entry is 4 bytes long:			*
*	0 - LSB of DPH address					*
*	1 - MSB of DPH address					*
*	2 - Sector size code (1 = 128, 2 = 256, 3 = 512...	*
*	3 - Bad map has been initilized (0 = Uninitilized)	*
*								*
*****************************************************************

dphtab:	rept	maxlog*4
	db	0
	endm

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
	mvi	l,d.bad		;the track and sector of the bad map
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

write:	mov	a,c		;Save write command type
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

read:	xra	a		;Set the command type to read
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
				;	on the currently selected disk.
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
	xra	a
	ret

into:	xchg			;
	call	mov128		;Move the data, HL = destination
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

flush:	mvi	a,0		;The 0 is modified to reflect if
				;	the buffer has been written into
bufwrtn	equ	$-1
	ana	a		;Test if written into
	rz			;Not written, all done
	mvi	a,d.write
	sta	rwop+1

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

	mvi	l,d.sel2	;Select drive
	call	jumpbuf

	lhld	alttrk		;Track number -> (hl)

	mov	a,h		;Test for track zero
	ora	l

	push	h		;Save track number
	mvi	l,d.home
	cz	jumpbuf
	pop	b		;Restore track #

	mvi	l,d.strk
	call	jumpbuf

	lhld	altsec		;Sector -> (hl)
	mov	b,h
	mov	c,l

	mvi	l,d.ssec
	call	jumpbuf

	lxi	b,buffer	;Set the DMA address
	mvi	l,d.sdma
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
	mvi	l,d.home	;Home drive after (retries/2) errors
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

	lda	rdwr
	ana	a
	jz	fread
	lda	writtyp
	dcr	a
	dcr	a
	rz
	lda	secsiz		;Check for 128 byte sectors
	dcr	a
	rz

fread:	mvi	a,d.read
	sta	rwop+1
	jmp	prep		;Select drive, track, and sector.
				;	Then read the buffer

*****************************************************************
*								*
* Jumpbuf, jumper are used to dispatch to a low level device	*
* subroutine.  Jumper is called with the drive in (h) and the	*
* routine number (see description above) in (l).  It passes	*
* along the (bc) and (de) registers unaltered.  Jumpbuf is 	*
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
	inx	h		; function.
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
	jnz	altmis

				;Found an alternate sector
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
	call	cout		;Output the character
	pop	h		;Restore the pointer
	jmp	message		;Continue until null reached

*****************************************************************
*								*
* The following equates are for the Diskus Hard disk		*
*								*
*****************************************************************

	if	hdc3 ne 0	;Want HDC3 controller included ?
hdorg	equ	50h		;Hard Disk Controller origin
hdstat	equ	hdorg		;Hard Disk Status
hdcntl	equ	hdorg		;Hard Disk Control
hdcmnd	equ	hdorg+1		;Hard Disk Command
hdreslt	equ	hdorg+1		;Hard Disk Result
hdfunc	equ	hdorg+2		;Hard Disk Function
hddata	equ	hdorg+3		;Hard Disk Data

retry	equ	2		;Retry bit of result

tkzero	equ	01h		;Track zero bit of status
opdone	equ	02h		;Operaction done bit of status
complt	equ	04h		;Complete bit of status
tmout	equ	08h		;Time out bit of status
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
rsect	equ	1		;Read sector command
wsect	equ	5		;Write sector command
isbuff	equ	8		;Initialize header command

*****************************************************************
*								*
* Device Specification Table for HDC3				*
*								*
*****************************************************************

hddst:	db	maxhd*hdlog	;Number of logical drives
	dw	hdwarm		;Warm boot
	dw	hdtran		;Sector translation
	dw	hdldrv		;Select logical drive 1 (First time select)
	dw	hddrv		;Select logical drive 2 (General sel	ect)
	dw	hdhome		;Home current selected drive
	dw	hdseek		;Seek to selected track
	dw	hdsec		;Select sector
	dw	hddma		;Set DMA address
	dw	hdread		;Read a sector
	dw	hdwrite		;Write a sector
	dw	nobad		;No bad sector map

hdwarm:	call	divlog		;Get physical drive number in (c)
	xra	a
	lxi	h,ccp-200h	;Initial DMA address
	push	h
	sta	head		;Select head zero
	inr	a		; 1 -> (a)
	push	psw		;Save first sector - 1
	call	hdd2		;Select drive
	mvi	c,0
	call	hdhome		;Home the drive
hdwrld:	pop	psw		;Restore sector
	pop	h		;Restore DMA address
	inr	a
	sta	hdsectr
	cpi	12		;Past BDOS ?
	rz			;Yes, all done
	inr	h		;Update DMA address
	inr	h
	shld	hdadd
	push	h
	push	psw
hdwrrd:	lxi	b,retries*100h+0;Retry counter
hdwr:	push	b		;Save the retry count
	call	hdread		;Read the sector
	pop	b
	jnc	hdwrld		;Test for error
	dcr	b		;Update the error count
	jnz	hdwr		;Keep trying if not to many errors
	stc			;Error flag
	ret

hdtran:	mov	h,b
	mov	l,c
	inx	h
	ret

hdldrv:	sta	hdcur		;Save logical disk
	call	divlog		;Divide by logical disks per drive
	mov	a,c
	sta	hddisk		;Save new physical drive
	call	hdptr		;Get track pointers
	mov	a,m		;Get current track
	inr	a		;Check if -1
	jnz	hdl2		;Nope, allready accessed
	ori	null		;Select drive
	out	hdfunc
	mvi	a,scenbl	;Enable the controller
	out	hdcntl
	mvi	c,239		;Wait approx 2 minutes for Disk to ready
	lxi	h,0
hdtdel:	dcx	h
	mov	a,h
	ora	l
	cz	dcrc
	jz	zret		;Drive not ready error
	in	hdstat		;Test if ready yet
	ani	drvrdy
	jnz	hdtdel

	if	not fujitsu
	lxi	h,0		;Time one revolution of the drive
	mvi	c,index
	in	hdstat
	ana	c
	mov	b,a		;Save current index level in B
hdinxd1:in	hdstat
	ana	c
	cmp	b		;Loop util index level changes
	jz	hdinxd1
hdindx2:inx	h
	in	hdstat		;Start counting until index returns to
	ana	c		;	previous state
	cmp	b
	jnz	hdindx2

	if	m10		;Memorex M10's have 40 ms head settle
	dad	h
	endif

	if	m26		;Shugart M26's have 30 ms head settle
	xra	a
	mov	a,h
	rar
	mov	d,a
	mov	a,l
	rar
	mov	e,a
	dad	d
	endif

	shld	settle		;Save the Count for timeout delay
	endif

	call	hdhome

hdl2:	lda	hdcur		;Load logical drive
	lxi	d,dphhd0	;Start of hard disk DPH's
	mvi	c,3		;Hard disk sector size equals 512 bytes
	jmp	retdph

dcrc:	dcr	c		;Conditional decrement C routine
	ret

divlog:	mvi	c,0
divlx:	sui	hdlog
	rc
	inr	c
	jmp	divlx

hddrv:	sta	hdcur
	call	divlog		;Get the physical drive #
hdd2:	mov	a,c
	sta	hddisk		;Select the drive
	ori	null
	out	hdfunc
	mvi	a,wenabl
	out	hdcntl
	ret

hdhome:	call	hdptr
	mvi	m,0		;Set track to zero
	in	hdstat		;Test status
	ani	tkzero		;At track zero ?
	rz			;Yes

	if	not fujitsu
hdstepo:in	hdstat		;Test status
	ani	tkzero		;At track zero ?
	jz	hddelay
	mvi	a,1
	stc
	call	accok		;Take one step out
	jmp	hdstepo

	else

	xra	a
	jmp	accok
	endif

	if	not fujitsu
hddelay:lxi	h,0		;Get hddelay
settle	equ	$-2
deloop:	dcx	h		;Wait 20ms
	mov	a,h
	ora	l
	inx	h
	dcx	h
	jnz	deloop
	ret
	endif

hdseek:	call	hdptr		;Get pointer to current track
	mov	e,m		;Get current track
	mov	m,c		;Update the track
	mov	a,e		;Need to seek at all ?
	sub	c
	rz
	cmc			;Get carry into direction
	jc	hdtrk2
	cma
	inr	a
	if	fujitsu
hdtrk2:	jmp	accok
	else
hdtrk2:	call	accok
	jmp	hddelay
	endif

accok:	mov	b,a		;Prep for build
	call	build
sloop:	ani	nstep		;Get step pulse low
	out	hdfunc		;Output low step line
	ori	pstep		;Set step line high
	out	hdfunc		;Output high step line
	dcr	b		;Update repeat count
	jnz	sloop		;Keep going the required # of tracks
	jmp	wsdone

hddma:	mov	h,b		;Save the DMA address
	mov	l,c
	shld	hdadd
	ret

wsdone:	in	hdstat		;Wait for seek complete to finish
	ani	complt
	jz	wsdone
	ret

	if	m26
hdsec:	mvi	a,01fh		;For compatibility with Cbios rev 2.3, 2.4
	ana	c
	cz	getspt
	sta	hdsectr
	mvi	a,0e0h
	ana	c
	rlc
	rlc
	rlc
	sta	head
getspt:	mvi	a,hdspt
	ret

	else

hdsec:	mov	a,c
	call	divspt
	adi	hdspt
	ana	a
	cz	getspt
	sta	hdsectr
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
rtloop:	in	hddata		;Move four bytes
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

hdwrite:call	hdprep		;Prepare header
	rc
	xra	a
	out	hdcmnd
	lhld	hdadd
	mvi	b,seclen/4
wtloop:	mov	a,m		;Move 4 bytes
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

process:in	hdstat		;Wait for command to finish
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

hdprep:	in	hdstat
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
	call	hdptr
	mov	a,m		;Form track byte
	out	hddata
	ana	a
	mvi	b,80h
	jz	zkey
	mvi	b,0
zkey:	mvi	a,0		;Form sector byte
hdsectr	equ	$-1
	out	hddata
	mov	a,b
	out	hddata
	mvi	a,dskclk
	out	hdcntl
	mvi	a,wenabl
	out	hdcntl
	xra	a
	ret

hdptr:	lhld	hddisk
	xchg
	mvi	d,0
	lxi	h,drives
	dad	d
	ret

build:	mvi	a,0
head	equ	$-1
	ral
	ral
	ral
	ral
	ori	0
hddisk	equ	$-1
	xri	0f0h
	ret

drives	equ	$
	rept	maxhd
	db	0ffh
	endm

hdcur:	db	0		;Current logical disk

	endif

*****************************************************************
*								*
* The following equates relate the Morrow Designs 2D/B		*
* controller. If the controller is non standard (0F800H)	*
* only the FDORIG equate need be changed.			*
*								*
*****************************************************************

	if	maxfd ne 0	;Include Discus 2D ?
fdorig	equ	0f800H		;Origin of Disk Jockey PROM
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

*****************************************************************
*								*
* Device Specification Table for the Disk Jockey 2D/B		*
*								*
*****************************************************************

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

*****************************************************************
*								*
* Floppy disk warm boot loader					*
*								*
*****************************************************************

fdwarm:	mov	c,a
	call	fdsel		;Select drive A
	mvi	c,0		;Select side 0
	call	fdside
wrmfail:call	fdhome		;Track 0, single density
	jc	wrmfail		;Loop if error

				;The next block of code re-initializes
				;   the warm boot loader for track 0.
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
	stax	d		;   save it.
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

fdsel2:	sta	fdlog
	mov	c,a
	jmp	fdsel

fdlhome:mvi	c,0		;Select side 0
	call	fdside
	jmp	fdhome		;Do actual home

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

	if	(maxfd ne 0) or (maxdm ne 0)

*****************************************************************
*								*
* Xlts is a table of address that point to each of the xlt	*
* tables for each sector size.					*
*								*
*****************************************************************

xlts:	dw	xlt128		;Xlt for 128 byte sectors
	dw	xlt256		;Xlt for 256 byte sectors
	dw	xlt512		;Xlt for 512 byte sectors
	dw	xlt124		;Xlt for 1024 byte sectors

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

*****************************************************************
*								*
* Each of the following tables describes a diskette with the	*
* specified characteristics.					*
*								*
*****************************************************************

*****************************************************************
*								*
* The following DPB defines a  diskette for 128 byte sectors,	*
* single density, and single sided.				*
*								*
*****************************************************************

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
	db	1

*****************************************************************
*								*
* The following DPB defines a diskette for 256 byte sectors,	*
* double density, and single sided.				*
*								*
*****************************************************************

dpb256s:dw	52		;CP/M sectors/track
	db	4		;BSH
	db	15		;BLM
	db	0		;EXM
	dw	242		;DSM
	dw	127		;DRM
	db	0c0h		;AL0
	db	0		;AL1
	dw	32		;CKS
	dw	2		;OFF
	db	2

*****************************************************************
*								*
* The following DPB defines a diskette as 512 byte sectors,	*
* double density, and single sided.				*
*								*
*****************************************************************

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
	db	3

*****************************************************************
*								*
* The following DPB defines a diskette as 1024 byte sectors,	*
* double density, and single sided.				*
*								*
*****************************************************************

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
	db	4

*****************************************************************
*								*
* The following DPB defines a diskette for 128 byte sectors,	*
* single density, and double sided.				*
*								*
*****************************************************************

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
	db	1

*****************************************************************
*								*
* The following DPB defines a diskette as 256 byte sectors,	*
* double density, and double sided.				*
*								*
*****************************************************************

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
	db	2

*****************************************************************
*								*
* The following DPB defines a diskette as 512 byte sectors,	*
* double density, and double sided.				*
*								*
*****************************************************************

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
	db	3

*****************************************************************
*								*
* The following DPB defines a diskette as 1024 byte sectors,	*
* double density, and double sided.				*
*								*
*****************************************************************

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
	db	4

	endif

*****************************************************************
*								*
* The following equates relate the Morrow Designs DJDMA		*
* controller.							*
*								*
*****************************************************************

	if	(maxdm ne 0) or (maxmf ne 0)
dmchan	equ	50h		;Default channel address
dmkick	equ	0efh		;Kick I/O port address

rdsect	equ	20h		;Read sector command
wrsect	equ	21h		;Write a sector command
gstat	equ	22h		;Get drive status
dmsdma	equ	23h		;Set DMA address
intrqc	equ	24h		;Set Interrupt request
haltc	equ	25h		;Halt command
bracha	equ	26h		;Channel branch
setcha	equ	27h		;Set channel address
setcrc	equ	28h		;Set CRC retry count
rdtrck	equ	29h		;Read track command
wrtrck	equ	2ah		;Write track command
serout	equ	2bh		;Serial ouput through bit banger serial port
senabl	equ	2ch		;Enable serial input
trksiz	equ	2dh		;Set number of tracks
setlog	equ	2eh		;Set logical drives
readm	equ	0a0h		;Read from controller memory
writem	equ	0a1h		;Write to controller memory

n.dubl	equ	80h		;Double density
n.2side	equ	40h		;2 sided drive

serin	equ	03eh		;Address of serial input data, (status - 1)

*****************************************************************
*								*
* Device Specification Table for the Disk Jockey DMA floppy	*
*								*
*****************************************************************

	if	maxdm ne 0
dmdst:	db	maxdm		;Number of logical drives
	dw	dmwarm		;Warm boot
	dw	dmtran		;Sector translation
 	dw	dmldrv		;Select drive 1
	dw	dmselr		;Select drive 2
	dw	dmhome		;Home drive
	dw	dmseek		;Seek to specified track
	dw	dmssec		;Set sector
	dw	dmdma		;Set DMA address
	dw	dmread		;Read a sector
	dw	dmwrite		;Write a sector
	dw	nobad		;No bad sector map

dmtrck	equ	22*128		;Amount of code on track 0

dmwarm:	call	dmselr		;Select drive 0
	lxi	h,dmchan	;Set up branch
	mvi	m,bracha
	inx	h
	mvi	m,(dmwchn and 0ffh)	;Low address byte
	inx	h
	mvi	m,(dmwchn / 0100h)	;High address byte
	inx	h
	mvi	m,0
dmwbad:	lxi	h,dmwend-1	;Pointer to end of command structure
	call	docmd		;Read in tracks
	lda	dmwst		;Get track read status
	ani	40h
	jz	dmwbad		;Loop on 'terrible' errors like no disk
	lxi	b,300h		;3/4 K bytes of sector 3 needs to be moved
	lxi	d,buffer	;Sector 3 is in our buffer
	lxi	h,ccp+1300h	;  and this is where we want it to go...
	call	movbyt
	xra	a
	ret

dmwchn:	db	dmsdma		;Set track 0 DMA address
	dw	ccp-512		;First track DMA address - boot loader
	db	0
	db	rdtrck		;Read track command
	db	0		;Track 0
	db	0		;Side 0
	db	0		;Drive 0
	dw	dmwsec		;Sector load/status map
	db	0
dmwst:	db	0		;Track read status
	db	dmsdma
	dw	ccp+dmtrck	;DMA address for track 1
	db	0
	db	rdtrck
	db	1		;Track 1
	db	0		;Side 0
	db	0		;Drive 0
	dw	dmwsec+26	;Map is loaded right after track 0 status map
	db	0
	db	0		;Track read status
	db	dmsdma
	dw	buffer		;Sector 3 gets loaded in system buffer
	db	0
	db	rdsect
	db	1		;Track 1
	db	3		;Side 0, sector 3
	db	0		;Drive 0
dmwend:	db	0		;Read status
	dw	0		;Room for the halt

dmwsec:	dw	0ffffh, 0ffffh			;Do not load boot loader
	dw	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	;22 sectors to be loaded
	dw	0, 0ffffh, 0ffffh, 0ffffh	;First 2 sectors on track 2

dmselr:	sta	dmlog
	mvi	b,0		;8 inch logical drives start at zero
	jmp	dmsel2

dmtran:	inx	b
	push	d		;Save table address
	push	b		;Save sector #
	call	dmget
	lxi	d,10
	dad	d
	mov	a,m
	inx	h
	mov	h,m
	mov	l,a
	mov	a,m		;Get # of CP/M sectors/track
	ora	a		;Clear cary
	rar			;Divide by two
	sub	c
	push	psw		;Save adjusted sector
	jm	dmside2
dmsidea:pop	psw		;Discard adjusted sector
	pop	b		;Restore sector requested
	pop	d		;Restor address of xlt table
dmside1:xchg			;hl <- &(translation table)
	dad	b		;bc = offset into table
	mov	l,m		;hl <- physical sector
	mvi	h,0
	ret

dmside2:call	dmstat
	ani	20h
	jz	dmsidea
	pop	psw		;Retrieve adjusted sector
	pop	b
	cma			;Make sector request positive
	inr	a
	mov	c,a		;Make new sector the requested sector
	pop	d
	call	dmside1
	mvi	a,80h		;Side two bit
	ora	h		;	and sector
	mov	h,a
	ret

dmldrv:	sta	dmlog
	call	dminit		;Test for a drive
	jc	zret
	lxi	h,1		;Select sector 1 of track 2
	shld	truesec
	inx	h
	shld	cpmtrk
	xra	a		;Make sure we are doing a read
	sta	rdwr
	call	fill		;Flush buffer and refill
	jc	zret		;Test for error return

	call	dmstat		;Get status on current drive
	ani	0ch		;Mask in sector size bits
	push	psw		;Used to select a DPB
	rar
	lxi	h,xlts		;Table of XLT addresses
	mov	e,a
	mvi	d,0
	dad	d
	push	h		;Save pointer to proper XLT
	call	dmget
	pop	d
	lxi	b,2		;Number of bytes to move
	call	movbyt		;Move the address of XLT
	lxi	d,8		;Offset to DPB pointer
	dad	d		;HL <- &DPH.DPB
	push	h
	call	dmstat
	ani	20h		;Check double sided bit
	lxi	d,dpb128s	;Base for single sided DPB's
	jz	dmsok
	lxi	d,dpb128d	;Base of double sided DPB's
	call	sethigh		;Set controller to know about fast steping
dmsok:	xchg			;HL <- DBP base, DE <- &DPH.DPB
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
	lxi	h,15
	dad	d
	mov	c,m
dmget:	lda	dmlog
	lxi	d,dphdm0
	jmp	retdph

sethi:	db	writem		;Write to controller memory
	dw	stephi		;Point to step rate for fast stepping
	db	0		;Extended address 0
	dw	2		;Move two bytes to controller
add0:	dw	0		;This address is patched from below to point
				;to the drive table for new selected drive
	db	writem		;Write to controller memory
	dw	settlhi
	db	0
	dw	2
add1:	dw	0

dmend:	db	haltc		;Halt command
	db	0		;Status

stephi:	dw	066h		;3 MS track to track step rate
settlhi:dw	200h		;15 MS head settle delay

sethigh:push	d
	lxi	h,1340h		;Pointer to start of Drive Paramter Table in
				;  DJDMA
	lxi	d,10h		;Size of SPT table
	lda	dmlog
seth0:	ora	a		;Still more tables to skip
	jz	seth1
	dad	d		;Offset to next table
	dcr	a
	jmp	seth0

seth1:	xchg			;Move pointer to DPT in (de)
	lxi	h,4
	dad	d
	shld	add0		;Address of step delay

	lxi	h,10
	dad	d
	shld	add1		;Address of head settle delay

	lxi	h,dmchan
	mvi	m,bracha
	inx	h
	mvi	m,sethi and 0ffh
	inx	h
	mvi	m,(sethi/100h) and 0ffh
	inx	h
	mvi	m,0		;Extended address 0
	lxi	h,dmend-1
	xra	a
	call	docmd2
	pop	d
	ret
	endif

*****************************************************************
*								*
* Drive specification table for DJDMA 5 1/4 inch drives		*
*								*
*****************************************************************

	if	maxmf ne 0
mfdst:	db	maxmf		;Number of logical drives
	dw	mfwarm		;Warm boot
	dw	mftran		;Sector translation
 	dw	mfldrv		;Select drive 1
	dw	mfsel2		;Select drive 2
	dw	dmhome		;Home drive
	dw	mfseek		;Seek to specified track
	dw	mfssec		;Set sector
	dw	dmdma		;Set DMA address
	dw	dmread		;Read a sector
	dw	dmwrite		;Write a sector
	dw	nobad		;No bad sector map

mftrck	equ	9*512		;Amount of code on track 0

mfwarm:	call	mfsel2		;Select drive 0
	lxi	h,dmchan	;Set up branch
	mvi	m,bracha
	inx	h
	mvi	m,(mfwchn and 0ffh)	;Low address byte
	inx	h
	mvi	m,(mfwchn / 0100h)	;High address byte
	inx	h
	mvi	m,0
mfwfal:	lxi	h,mfwend-1	;Pointer to end of command structure
	calì	docmä		;Reaä iî tracks
	lda	mfwst		;Check out drive status
	ani	40h		;Test for ok
	jz	mfwfal		;Failed, loop
	xra	a		;Return no error
	ret

mfwchn:	db	dmsdma		;Set track 0 DMA address
	dw	ccp-512		;First track DMA address - boot loader
	db	0
	db	rdtrck		;Read track command
	db	0		;Track 0
	db	0		;Side 0
	db	0		;Drive 0
	dw	mfwsec		;Sector load/status map
	db	0
mfwst	db	0		;Track read status
	db	dmsdma
	dw	ccp+mftrck	;DMA address for track 1
	db	0
	db	rdtrck
	db	1		;Track 1
	db	0		;Side 0
	db	0		;Drive 0
	dw	mfwsec+10	;Map is loaded right after track 0 status map
	db	0
mfwend:	db	0		;Track read status
	dw	0		;Room for the halt

mfwsec:	dw	0ffh, 0, 0, 0, 0	;Do not load boot loader
	dw	0, 0ffffh, 0ffffh, 0ffffh, 0ffffh ;first two sectors loaded

mfssec:	dcr	c		;Minnie floppy sectors start at zero
	lda	dblflg		;Get double sided flags
	ora	a
	jz	dmssec		;Nope, single sided
	mvi	b,80h		;Set high bit for double sided select
	jmp	dmssec

dblflg:	db	0

mfseek:	xra	a		;Clear double sided select
	sta	dblflg
	lda	mfcon
	ani	n.2side
	jz	dmseek		;Only siingle sided

	mov	a,c		;Move selected track in (a)
	sbi	35		;Subtract by track by number of tracks
	jc	dmseek		;Less than track 35

	mov	d,a		;Save adjusted track number
	mvi	a,34
	sub	d		;Adjust to count tracks back out
	mov	c,a		;Resave new track number

	mvi	a,0ffh		;Set double sided flag
	sta	dblflg
	jmp	dmseek

mfsel2:	sta	mflog
	mvi	b,4		;5 1/4 inch drives start at drive 4
	jmp	dmsel2

mftran:	inr	c		;Increment Sector number
	lda	mfcon
	ani	n.dubl
	lxi	h,mfxltd-1	;Point to double sided sector translation table
	jnz	mftdubl		;Single density sector translation
	lxi	h,mfxlts-1
mftdubl:dad	b		;Add offset sector number to table
	mov	l,m		;Pick up sector number from table
	mvi	h,0		;MSB of sector number equal 0
	ret

mfldrv:	sta	mflog
	call	dminit		;Test for a controller
	jc	zret
	mvi	a,n.dubl
	sta	mfcon

	lxi	h,1		;Select sector 1 of track 0
	shld	truesec
	dcx	h
	shld	cpmtrk
	xra	a		;Make sure we are doing a read
	sta	rdwr
	call	fill		;Flush buffer and refill
	jc	zret		;Test for error return
	lda	buffer+5ch	;Get diskette configuration byte

	push	psw		;Save configuration byte
	lxi	h,1
	shld	cpmtrk		;Load track 1 sector 1
	call	fill		;This is to fix bug with DJDMA firmware on
	jc	zret		; returning single density status on track 0
	pop	psw

	ora	a
	jnz	mfl9		;Non zero

	mvi	a,90h		;Double density default configuration
	call	dmstat		;If zero then determine sector size
	ani	80h		;Check density bit
	jnz	mfl9		;Its double density
	mvi	a,10h		;Single density default configuration byte

mfl9:	mov	c,a		;Move configuration byte into (c)

	lxi	h,mfs		;Address of configuration table -> (hl)
mfl2:	mov	a,m		;Get an entry
	ora	a		;Check for end of the table
	jz	zret		;Yes, select error
	cmp	c		;Check if entry matches selected drive
	jz	mfl3
	inx	h		;Skip onfiguration byte
	inx	h		;Skip drive type
	inx	h		;Skip DPB address
	inx	h
	jmp	mfl2

mfl3:	inx	h
	mov	a,m		;Pick up drive type
	sta	mfcon
	inx	h
	mov	a,m
	inx	h
	mov	h,m
	mov	l,a		;DPB address -> (hl)
	push	h		;Save DPB address
	call	mfgdph		;Get DPH
	lxi	d,10		;Offset to DPB address in DPH
	dad	d
	pop	d
	mov	m,e		;Store DPB address in DPH
	inx	h
	mov	m,d
	call	mfgdph
	push	h
	call	dmstat		;Get status
	pop	h
	ani	80h		;Check density bit
	mvi	c,3		;512 byte sectors
	rnz
	mvi	c,2		;256 byte sectors
	ret

mfgdph	lda	mflog
	lxi	d,dphmf0
	jmp	retdph

mfcon:	db	0
mflog:	db	0

mfs:	db	10h		;North Star
	db	0		;Single density, 35 tracks, single sided.
	dw	dpbmf0

	db	90h		;North Star
	db	n.dubl		;Double density, 35 tracks, single sided.
	dw	dpbmf1

	db	0b0h		;North Star
	db	n.dubl		;Double density, 35 tracks, single sided.
	dw	dpbmf2

	db	0f0h		;North Star
	db	n.dubl+n.2side	;Double density, 35 tracks, souble sided
	dw	dpbmf3

	db	0e5h		;North Star. Default format
	db	n.dubl		;Double density, 35 tracks, single sided.
	dw	dpbmf1

	db	0

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

dmsel2:	mov	c,a		;Move drive into (c)
	lxi	h,dmchan
	mvi	m,setlog	;Set logical drives
	inx	h
	mov	m,b		;Drive in (b)
	push	b
	call	docmd
	pop	b
	jmp	dmsel

dmssec:	push	b		;Save sector number
	mov	a,b
	rlc
	ani	1
	mov	c,a
	call	dmside
	pop	b
	jmp	dmsec

dmdma	lxi	h,dmchan	;Default channel address
	mvi	m,dmsdma	;Set DMA address
	inx	h
	mov	m,c		;Low byte first
	inx	h
	mov	m,b		;High byte next

docmd	xra	a
	inx	h
	mov	m,a
docmd2	inx	h
	mvi	m,haltc
	inx	h
	mov	m,a
	out	dmkick
tests	ora	m
	jz	tests
	ret

dminit	lxi	h,dmchan	;See if controller will halt
	mvi	m,haltc
	inx	h
	mvi	m,0
	out	dmkick		;Start controller
	lxi	d,0		;Set up timeout counter
dminwt	mov	a,m
	ora	a
	jnz	dmiok		;Controller has responded
	dcx	d		;Bump timeout counter
	mov	a,d
	ora	e
	jnz	dminwt
	stc			;Set error flag
	ret
dmiok	dcx	h		;Back to start of command
	mvi	m,setcrc
	inx	h
	mvi	m,1
	xra	a
	jmp	docmd2		;Do command

dmerr   jmp	dmerr

dmhome	xra	a
	mov	c,a		;Put a zero into (c) for track zero

dmseek	mov	a,c		;Enter with track in (c)
	sta	lltrk		;Save for use later
	ret

dmsec	lda	llss		;Load sector
	ani	80h		;Save side select bit
stores	ora	c
	sta	llss
	ret

dmside	mov	a,c		;Move side bit into (a)
	ani	1
	rrc			;Move around to bit 7
	mov	c,a		;Resave in (c)
	lda	llss
	ani	7fh		;Mask out old side select bit
	jmp	stores

dmsel	mov	a,c		;Move drive into (a)
	sta	lldrv
dmden	ret			;Double density only

;
» Returî statuó iî thå (a) register in the form:
;
; 			7  6  5  5  3  2  1  0
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
	mvi	m,gstat		;Set up read status
	inx	h
	lda	lldrv		;Get last selected drive
	mov	m,a		;Store drive in command
	inx	h		;Skip over returned status
	inx	h
	inx	h
	call	docmd		;Issue command
	lda	llss		;Get side bit of last operation
	ani	80h
	rrc			;Move to bit 7
	mov	c,a
	lxi	h,dmchan+1	;Point to drive
 	mov	a,m		;Load drive
	ora	c
	ani	4		;Mask upper drive select bit for 5 1/4
	rlc
	rlc			;Move to bit 4
	ora	m		;Put together with lower drive bits
	ora	c
	mov	c,a
	inx	h
	mvi	a,10h		;Double density bit
	ana	m
	rlc			;20h
	rlc			;40h
	rlc			;80h for density bit
	ora	c
	mov	c,a
	inx	h
	mvi	a,3		;Sector length mask
	ana	m		;And in
	rlc			;Move to bits 2 & 3
	rlc
	ora	c
	mov	c,a
	inx	h
	mvi	a,4		;Mask for double sided bit
	ana	m
	rlc			;8
	rlc			;10
	rlc			;20
	ora	c
	ret

dmwrite	mvi	a,wrsect
	db	01		;Ugh...
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
	if	m5		;Specifications for a Seagate Technology 506
cyl	equ	153		;Number of cylinders
heads	equ	4		;Number of heads per cylinder
precomp	equ	64		;Cylinder to start write precomensation
lowcurr	equ	128		;Cylinder to start low current
stepdly	equ	30		;Step delay (0-12.7 milliseconds)
steprcl	equ	30		;Recalibrate step delay
headdly	equ	200		;Settle delay (0-25.5 milliseconds)
	endif

	if	m16a		;Specifications for an AMPEX DF516
cyl	equ	192
heads	equ	8
precomp	equ	0
lowcurr	equ	96
stepdly	equ	1
steprcl	equ	10
headdly	equ	0
	endif

	if	m16c		;Specifications for an CMI 5616
cyl	equ	256
heads	equ	6
precomp	equ	128
lowcurr	equ	128
stepdly	equ	2
steprcl	equ	30
headdly	equ	0
	endif

	if	m19		;Specifications for an CMI 5619
cyl	equ	306
heads	equ	6
precomp	equ	128
lowcurr	equ	128
stepdly	equ	2
steprcl	equ	30
headdly	equ	0
	endif

	if	m40		;Specifications for a Quantum Q2040
cyl	equ	255
heads	equ	8
precomp	equ	255
lowcurr	equ	255
stepdly	equ	0
steprcl	equ	10
headdly	equ	0
	endif

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
wflt	equ	2		;Write fault from drive
dready	equ	4		;Drive ready
sekcmp	equ	8		;Seek complete

*****************************************************************
*								*
* Drive Specification Table for the HD DMA hard disk controller *
*								*
*****************************************************************

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
	dw	mwbad		;Return bad sector map info

*****************************************************************
*								*
* Thå followinç arå thå lowesô leveì driveró foò thå Morro÷	*
* Designó Harä Disë DMÁ controller.				*
*								*
*****************************************************************

mwwarm	xra	a
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
	push	a		;Save first sector -1
mwwlod	pop	psw		;Restore sector
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

mwwread	mvi	c,retries	;Retry counter
mwwerr	push	b		;Save the retry count
	call	mwread		;Read the sectorŠ	pop	b
	rnc
	dcr	c		;Update the error count
	jnz	mwwerr		;Keep trying if not too many errors
	stc			;Set error flag
	ret


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

mwdrö	sta	mwcurl
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

mwhome	call	mwreset		;Reset controller
	lxi	h,dmarg1	;Load arguments
	mvi	m,steprcl	;Load step delay (slow)
	inx	h
	mvi	m,headdly	;Head settle delay
	call	mwissue		;Do load constants again
	call	mwptr		;Get pointer to current cylinder number
	mvi	m,0ffh		;Fake at cyl 255 for max head travel
	inx	h
	mvi	m,0ffh
	lxi	b,0		;Seek to cylinder 0
	call	mwseek		;Recal slowly
	jmp	mwreset		;Back to fast stepping mode

mwbad:	lxi	h,mwbtab	;Return pointer to bad sector location
	ret	

mwbtab:	dw	0		;Track 0
	dw	19		;Head 2, sector 0  = (2 * SPT + 0) + 1
				
mwseek	calì	mwptr		;Geô tracë pointer
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

	mvé	a,dmanooð	;No-operatioî commanä foò thå channel
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
	lxi	h,dmarg1	;Load arguments
	mvi	m,stepdly	;Load step delay
	inx	h
	mvi	m,headdly	;Head settle delay
	inx	h
	mvi	m,sectsiz	;Sector size code
	inx	h
	mvi	m,dmalcon	;Load constants command
	jmp	mwissue		;Do load constants

mwread	mvi	a,dmaread	;Load disk read command
	jmp	mwprep

mwwrite	mvi	a,dmawrit	;Load disk write command

mwprep	sta	dmaop		;Save command channel op code

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

mwissue	lxi	h,dmastat	;Status byte
	mvi	m,0
	out	attn		;Start controller
	lxi	d,0		;Time out counter
	mov	b,e		;Controller busy status
mwiloop	mov	a,m		;Get status
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
* Cbios ram locations that don't need initialization.		*
*								*
*****************************************************************

cpmsec:	dw	0		;CP/M sector #

cpmdrv:	db	0		;CP/M drive #
cpmtrk: dw	0		;CP/M track #
truesec:dw	0		;Physical sector that contains CP/M sector

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
dphdsk	set	dphdsk+1
ldsk	set	ldsk+1
	endí
	endm

	if	hdpart ne 0	;Use non-standard partitioning

	if	m10 ne 0
	hdsecpt	equ	336	;Sectors per track
	hdbls	equ	2562	;Total blocks
	hdtrks	equ	244	;Total tracks
	endif

	if	m20 ne 0
	hdsecpt	equ	672
	hdbls	equ	5124
	hdtrks	equ	244
	endif

	if	m26 ne 0
	hdsecpt	equ	1024
	hdbls	equ	6464
	hdtrks	equ	202
	endif

ldsk	set	0		;Generate DPB's for a hdc3 hard disk
	rept	hdlog
	if	ldsk eq 0			;On logical drive 0
dsm	set	(hdbls/hdlog)-(hdbls/hdtrks+1)	;   take off a tracks worth
	else
dsm	set	hdbls/hdlog
	endif
off	set	(hdtrks/hdlog)*ldsk+1
	dpbgen	hd,%ldsk,%hdsecpt,5,31,1,%dsm,511,0ffh,0ffh,0,%off
ldsk	set	ldsk+1
	endm

	else			;Else use standard DPH's

	if	m26 ne 0
dpbhd0	dw	1024		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	1973		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	1		;OFF

dpbhd1	dw	1024		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	1973		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	64		;OFF

dpbhd2	dw	1024		;CP/M sectors/track
	db	5		;BSHŠ	db	31		;BLM
	db	1		;EXM
	dw	1973		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	127		;OFF
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
	endif

	if	m20 ne 0
dpbhd0	dw	672		;CP/M sectors/track
	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	2015		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	1		;OFF

dpbhd1	dw	672		;CP/M sectors/trackŠ	db	5		;BSH
	db	31		;BLM
	db	1		;EXM
	dw	2015		;DSM
	dw	511		;DRM
	db	0ffh		;AL0
	db	0ffh		;AL1
	dw	0		;CKS
	dw	98		;OFF

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
	endif
	endif
	endiæ			;Enä oæ HÄ DPH'ó anä DPB's

	if	maxmf ne 0
	dpbgen	mf,0, 20, 3,  7, 0, 04fh,  63, 0c0h, 0, 16, 3
	dpbgen	mf,1, 40, 4, 15, 1, 051h,  63,  80h, 0, 16, 2
	dpbgen	mf,2, 40, 3,  7, 0, 0a4h,  63, 0c0h, 0, 16, 2
	dpbgen	mf,3, 40, 4, 15, 1, 0a9h,  63,  80h, 0, 16, 2

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
* mwsectp = 2.25 * heads					*
*								*
* mwtrks is the total number of data cylinders.			*
* mwtrks = tracks - 1						*
*								*
* mwbls is the total number of 4096 byte blocks.		*
* mwbls = mwsecpt*mwtrks/32					*
*								*
*****************************************************************

 	if	m5 ne 0
	mwsecpt	equ	288		;Sectors per track
	mwbls	equ	1368		;Total data blocks (4096 byte)
	mwtrks	equ	152		;Total data tracks
	endif

	if	m16a ne 0
	mwsecpt	set	576
	mwbls	set	3438
	mwtrks	set	191
	endif

	if	m16c ne 0
	mwsecpt	set	432
	mwbls	set	3442
	mwtrks	set	255
 	endif

	if	m19 ne 0
	mwsecpt	set	432
	mwbls	set	4117
	mwtrks	set	305
 	endif

	if	m40 ne 0
	mwsecpt	set	576
	mwbls	set	9198
	mwtrks	set	511
	endif

dphdsk	set	0		;Generate DPH's for the HDDMA hard disks
	rept	maxmw
ldsk	set	0
	rept	mwlog
	dphgen	mw,%dphdsk,dpbmw,%ldsk
dphdsk	set	dphdsk+1
ldsk	set	ldsk+1
	endí
	endm

	if	mwpart ne 0	;Generate DPB's for a HDDMA hard disk

ldsk	set	0		;Use non-standard partitioning
	rept	mwlog
dsm	set	mwbls/mwlog-1
off	set	(mwtrks/mwlog)*ldsk+1
	dpbgen	mw,%ldsk,%mwsecpt,5,31,1,%dsm,511,0ffh,0ffh,0,%off
ldsk	set	ldsk+1
	endm

	else			;Use standard partitioning

blocks	set	mwbls
off	set	1
trkoff	set	8000h/(mwsecpt*10h/32)+1
ldsk	set	0

	rept	mwbls/2048	;Generate some 8 megabyte DPB's
	dpbgen	mw,%ldsk,%mwsecpt,5,31,1,2047,511,0ffh,0ffh,0,%off
off	set	off+trkoff
blocks	set	blocks-2048
ldsk	set	ldsk+1
	endm
	if	blocks gt 256	;If there is any stuff left, then use it
blocks	set	blocks-1
	dpbgen	mw,%ldsk,%mwsecpt,5,31,1,%blocks,511,0ffh,0ffh,0,%off
	endif
	endif
	endif

buffer	equ	$

*****************************************************************
*								*
* Signon message output during cold boot.			*
*								*
*****************************************************************

prompt:	db	80h, clear		;Clean buffer and screen
	db	acr,alf,acr,alf,acr,alf
	db	'Morrow Designs '
	db	'0'+msize/10		;CP/M memory size
	db	'0'+(msize mod 10)
	db	'K CP/M '		;CP/M version number
	db	cpmrev/10+'0'
	db	'.'
	db	(cpmrev mod 10)+'0'
	db	(revnum/10)+'A'-1

	if	(maxhd ne 0) or (maxmw ne 0)
	db	' + '

	if	maxhd ne 0
	if	m10 ne 0
	db	'M10'
	endif
	if	m20 ne 0
	db	'M20'
	endif
	if	m26 ne 0
	db	'M26'
	endif

	if	maxmw ne 0
	db	' + '
	endif
	endif

	if	maxmw ne 0
	if	m5 ne 0
	db	'Seagate M5'
	endif
	if	m16a ne 0
	db	'Ampex M16'
	endif
	if	m16c ne 0
	db	'CMI M16'
	endif
	if	m19 ne 0
	db	'CMI M19'
	endif
	if	m40 ne 0
	db	'Quantum M40'
	endif
	endif

	endif

	db	acr,alf
	db	0		;End of message

*****************************************************************
*								*
* Cboot is the cold boot loader. All of CP/M has been loaded in	*
* when control is passed here.					*
*								*
*****************************************************************

cboot:	lxi	sp,tpa		;Set up stack

	xra	a		;Clear cold boot flag
	sta	cwflg
	sta	group		;Clear group select byte

	mvi	a,intioby
	sta	iobyte

	lxi	d,badmap	;Clear out bad map
	stax	d
	lxi	h,badmap+1
	lxi	b,9*badsiz	;32 map entries
	call	movbyt
	mvi	m,0ffh		;End marker

	if	contyp ne 0	;Do not call tinit for PROM's
	call	tinit		;Initialize the terminal
	endif

	if	lsttyp ne 0	;Do not call linit for PROM's
	call	linit		;Initialize the list device
	endif

	lxi	h,prompt	;Prep for sending signon message
	call	message		;Send the prompt
	xra	a		;Select disk A
	sta	cpmdrv
	sta	cdisk

	lxi	h,bios+3	;Patch cold boot to warm code
	shld	bios+1
	jmp	gocpm

	if	contyp eq 2	;Multi I/O, Decision I

*****************************************************************
*								*
* Terminal initilization routine.  This routine reads the sense	*
* switch on the WB-14 and sets the speed accordingly.		*
*								*
*****************************************************************

tinit:	lda	group		;Get group byte
	ori	congrp		;Select console device
	out	grpsel

	mvi	a,dtrenb+rtsenb	;Enable DTR and RTS outputs to terminal
	out	mcr
	in	rbr		;Clear reciever buffers
	in	rbr
	xra	a
	out	lsr		;Clear status
	out	ier		;Set no interrupts

	if	not multr3	;Multi I/O has no sense switches
	lda	group		;Get group byte
	out	grpsel		;Select group zero to read sense switch
	in	sensesw		;Get sense switch.
	ani	0e0h		;Mask in upper three bits
	rlc
	rlc
	rlc			;Move into lower 3 bits
	cpi	7		;check for sense = 7
	push	psw		;Save value
	lda	group		;Get group byte
	ori	congrp		;Reselect serial port group
	out	grpsel
	pop	psw

	jz	valid		;Do default rate

	lxi	h,btab		;Pointer to baud rate table
	add	a		;Table of words so double
	mov	e,a		;Make a 16 bit number into (de)
	mvi	d,0
	dad	d		;Get a pointer into baud rate table
	mov	e,m		;Get lower byte of word
	inx	h		;Point to high byte of word
	mov	d,m		;Get upper byte. (de) now has divisor
	jmp	setit		;Set baud rate.

btab:	dw	1047		;110 Baud	000
	dw	384		;300		001
	dw	96		;1200		010
	dw	48		;2400		011
	dw	24		;4800		100
	dw	12		;9600		101
	dw	6		;19200		110
	else
	jmp	valid		;Skip over table
	endif

*****************************************************************
*								*
* The following is a list of valid baud rates.  The current	*
* baud rate is checked on cold boot.  If it is not in the	*
* vtab table then the baud rate will be set from the defcon	*
* word found below the Cbios jump table.  If the user		*
* happens to have a weird baud rate that is not in this		*
* table or is looking for a way to save space then entries	*
* can be added or deleted from the table.			*
*								*
*****************************************************************

vtab:	dw	2304		;50 baud
	dw	1536		;75
	dw	1047		;110
	dw	857		;134.5
	dw	768		;150
	dw	384		;300
	dw	192		;600
	dw	96		;1200
	dw	64		;1800
	dw	58		;2000
	dw	48		;2400
	dw	32		;3600
	dw	24		;4800
	dw	16		;7200
	dw	12		;9600
	dw	6		;19200

svtab	equ	($-vtab)/2	;Length of the vtab table

*****************************************************************
*								*
* Valid checks if the divisor latch is a reasonable value.	*
* If the value seems off then it will get the default baud	*
* rate from defcon and jump to setit.				*
*								*
*****************************************************************

valid:	mvi	a,dlab+wls0+wls1+stb
	out	lcr		;Access divisor latch
	in	dll		;Get lower divisor value
	mov	e,a
	in	dlm		;Get upper divisor value
	mov	d,a
	mvi	a,wls1+wls0+stb
	out	lcr
	lxi	h,vtab		;Valid baud rate table
	mvi	c,svtab		;Length of the baud rate table
vloop:	mov	a,e
	cmp	m		;Check low byte
	jnz	vskip		;First byte is bad
	inx	h
	mov	a,d
	cmp	m		;Check high byte
	jz	done		;Baud rate is OK...  Do cleanup
	dcx	h
vskip:	inx	h		;Skip to next entry
	inx	h
	dcr	c		;Bump entry counter
	jnz	vloop

nvalid:	lhld	defcon		;Get default baud rate
	xchg

setit:	mvi	a,dlab+wls1+wls0+stb	;Enable divisor access latch
	out	lcr		;Set the baud rate in (de)
	mov	a,d
	out	dlm		;Set upper divisor
	mov	a,e
	out	dll		;Set lower divisor
	mvi	a,wls1+wls0+stb
	out	lcr

done:	xra	a		;Clear status register
	out	lsr
	ret

	endif			;Multi I/O, Decision I

	if	contyp eq 3	;2D/B console initialization

tinit:	call	fdtstat		;Clean input buffer
	rnz			;All empty
	call	fdcin
	jmp	tinit

	endif			;2D/B console

	if	contyp eq 4
tinit:	call	dminit		;See if controller present
	rc			;No controller, return
	lxi	d,dmaci		;Console initialization command sequence
	lxi	h,dmchan
	lxi	b,10		;Command length
	call	movbyt
	dcx	h
	xra	a		;Clear serial input status
	sta	serin+1
	jmp	docmd2		;Do stuff and return

dmaci:	db	writem		;Zot monitor disable flag
	dw	tinit		;Any non-zero byte will do
	db	0
	dw	1		;One byte
	dw	13f5h		;Magical place in monitor
	db	senabl		;Enable serial input
	db	1

	endif

	if	(lsttyp ge 2) and (lsttyp le 5)	;Serial Multi I/O list drivers

linit:	lda	group		;Get group byte
	ori	lstgrp		;Select list device
	out	grpsel
	mvi	a,dlab		;Access divisor latch
	out	lcr
	lhld	deflst		;Get LST: baud rate divisor
	mov	a,h
	out	dlm		;Set upper baud rate
	mov	a,l
	out	dll
	mvi	a,stb+wls0+wls1
	out	lcr
	in	rbr		;Clear input buffer
	xra	a
	out	ier		;No interupts
	ret

	endif

	if	lsttyp eq 6	;Multi I/O parallel, Centronics

linit:	lda	group		;Get group byte
	ori	denable		;Set driver enable bit
	sta	group
	out	grpsel		;Select group zero with	drivers enabled
	xra	a
	out	daisi1		;Zero out data
	mvi	a,d9+d10	;Set strobe high, init low
	out	daisi0
	mvi	a,10		;Wait about 50uS for printer to initilize
dloop:	dcr	a
	jnz	dloop
	mvi	a,d11+d9+d10
	out	daisi0
	ret

	endif

	if	lsttyp eq 7	;Diablo HyType II

*****************************************************************
*								*
* Initialize Diablo HyType printer.  If the printer fails	*
* to initialize then the output drivers will be turned off	*
* and any attempts to print will result in redirection to	*
* the console.							*
*								*
*****************************************************************

	if	multr3		;Multi I/O initialization
linit:	lda	group		;Get group byte
	ori	denable		;Add driver enable bit
	out	grpsel
	ori	rest		;Toggle restore high
	out	grpsel
	mvi	a,10		;Hold line up for 50uS
dloop:	dcr	a
	jnz	dloop
	lda	group
	out	grpsel		;Turn denable and restore off

	else			;Mother board initialization

linit:	lda	group		;Get group byte
	out	grpsel		;Select group zero
	mvi	a,pselect+rlift	;Set select line active, rlift not active
	out	clk
	mvi	a,0ffh
	out	daisi0
	mvi	a,0ffh-rest	;Strobe restore bit low
	out	daisi0
	mvi	a,10		;Wait about 50uS
dloop:	dcr	a
	jnz	dloop
	mvi	a,0ffh		;Raise restore back up
	out	daisi0
	endif

	xra	a
	out	daisi1		;Clear data buffers

	if	multr3		;Lift ribbon
	lda	group
	ori	denable
	out	grpsel		;Re-enable the drivers
	mvi	a,0ffh-rest	;Pull -ribbon lift down
	out	daisi0
	else
	mvi	a,pselect	;Re-enable drivers and lift ribbon
	out	clk
	endif

linit9:	lxi	h,hinc/cperi
	shld	hmi		;Save hmi = 120/(characters per inch)

	lxi	h,vinc/lperi
	shld	vmi		;Save vmi = 48/(lines per inch)

	lxi	h,0		;Other variables default to zero
	shld	vpos
	shld	dlvpos
	shld	hpos
	shld	dlhpos
	shld	lmar

	call	clrall		;Clear the TAB array

	xra	a
	sta	kludge		;Reset TAB clear byte
	sta	dirflg
	sta	grhflg

	ret

	endif

	db	0,0ffh,0

codelen	equ	($-bios)	;Length of Cbios code

	if	debug
dbgtmp	set	codelen		;Cbios code length   !   <debug>
	endif

	ds	512-($-buffer)	;Maximum size buffer for 512 byte sectors

	if	(maxfd ne 0) or (maxdm ne 0) or (maxmw ne 0)
	ds	512		;Additional space for 1k sector devices
	endif

*****************************************************************
*								*
* Each bad map entry consists of 9 bytes:			*
*	Logical drive number (1 byte)				*
* 	Track number of bad sector (2 bytes)			*
*	Sector number of bad sector (2 bytes)			*
*	Track number of alternate sector (2 bytes)		*
*	Sector number of alternate sector (2 bytes)		*
*								*
*****************************************************************

badmap:	ds	badsiz*9+1	;32 entries + end marker

dirbuf:	ds	128		;Directory buffer

*****************************************************************
*								*
* Allocation and checked directory table area			*
*								*
*****************************************************************

	if	maxhd ne 0
	if	hdpart ne 0	;Use non-standard partitioning

dn	set	0
	rept	maxhd		;Generate CKS and ALV tables
ldsk	set	0
	rept	hdlog
	if	ldsk eq 0			;On logical drive 0
dsm	set	(hdbls/hdlog)-(hdbls/hdtrks+1)	;   take off a tracks worth
	else
dsm	set	hdbls/hdlog
	endif
alv	set	(dsm/8)+1
	alloc	hd,%dn,%((hdbls/hdlog)/8)+1,0
ldsk	set	ldsk+1
dn	set	dn+1
	endm
	endm

	else			;Standard partitioning

dn	set	0
	rept	maxhd
	if	m26 ne 0
	alloc	hd,%dn,247,0
dn	set	dn+1
	alloc	hd,%dn,247,0
dn	set	dn+1
	alloc	hd,%dn,247,0
dn	set	dn+1
	endif

	if	m10 ne 0
	alloc	hd,%dn,159,0
dn	set	dn+1
	alloc	hd,%dn,161,0
dn	set	dn+1Š	endif

	if	m20 ne 0
	alloc	hd,%dn,252,0
dn	set	dn+1
	alloc	hd,%dn,252,0
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

	if	mwpart ne 0	;Use non-standard partitioning

dn	set	0
	rept	maxmw*mwlog
alv	set	((mwbls/mwlog)/8)+1
	alloc	mw,%dn,%alv,0
dn	set	dn+1
	endm

	else			;Use standard partitioning

dn	set	0
	rept	maxmw

blocks	set	mwbls
	rept	mwbls/2048	;Generate some 8 megabyte ALV's
	alloc	mw,%dn,257,0
blocks	set	blocks-2048
dn	set	dn+1
	endm
	if	blocks gt 256	;If there is any stuff left, then use it
alv	set	(blocks/8)+1
	alloc	mw,%dn,%alv,0
dn	set	dn+1
	endif
	endm
	endif
	endif
	end
