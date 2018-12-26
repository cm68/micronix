*****************************************************************
*								*
* Installable printer and console drivers.			*
*								*
*  Date	   Author	Description				*
*								*
*  6 21 82 Marc		Reversed the sense of the console flag	*
*  4  2 82 Marc		Console install flag			*
*  4  2 82 Marc		Automatic determination of hardware	*
*  3 31 82 Marc		Initial coding				*
*								*
*****************************************************************

congrp	equ	1		;Serial console port (1 = p1, 2 = p2, 3 = p3)

lstgrp	equ	3		;Serial printer port (1 = p1, 2 = p2, 3 = p3)

*****************************************************************
*								*
* The folowing equates will define the Decision I mother	*
* board I/O or the Multi I/O environments.			*
*								*
*****************************************************************

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

; Define group select bits (base + 7)

s0	equ	01h		;Group number (0-3)
s1	equ	02h
smask	equ	03h
bank	equ	04h
enint	equ	08h		;Enable interrupts
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

*****************************************************************
*								*
* The following are internal driver equates. Most are misc.	*
* constants.							*
*								*
*****************************************************************

cpmbuf	equ	80h		;CPM buffer (command flag buffer)

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
* The jump table below must remain in the same order, the	*
* routines may be changed, but the function executed must be	*
* the same.							*
*								*
*****************************************************************

	jmp	$		;Cold boot entry point
owboot:	jmp	wboot		;Warm boot entry point
oconst:	jmp	const		;Console status routine
oconin:	jmp	conin		;Console input
oconout:jmp	conout		;Console output
olist:	jmp	list		;List device output
	jmp	$		;Punch device output
	jmp	$		;Reader device input
	jmp	$		;Home drive
	jmp	$		;Select disk
	jmp	$		;Set track
	jmp	$		;Set sector
	jmp	$		;Set DMA address
	jmp	$		;Read the disk
	jmp	$		;Write the disk
olistst:jmp	listst		;List device status
	jmp	$		;Sector translation
	jmp	$		;SINGLE.COM hookup

*****************************************************************
*								*
* The following word defines the default baud rate for the	*
* console device.						*
*								*
* The following is a list of possible baud rates and the	*
* value needed for the defcon word.				*
*								*
* Baud rate    defcon		Baud rate     defcon		*
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

defcon:	dw	12		;Default console baud rate

*****************************************************************
*								*
* The next byte is to make sure that the group select byte	*
* on the Mult I/O or Decsion I stays consistant throughout	*
* the driver.  Only the group bits themselves (bits 0 and 1)	*
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
* The next byte is a motherboard flag.  It is a has a non-	*
* zero value if a motherboard is present and is zero for a	*
* Multio.  This flag is used primarily by the parallel		*
* drivers since the parallel port assignments differ for	*
* the two boards.						*
*								*
*****************************************************************

mother:	db	0

wboot:	jmp	start		;Warm boot write through
	dw	owboot		;Warm boot after initialization

list:	jmp	olist		;List output
	dw	otlst	

listst:	jmp	olistst		;List status
	dw	stlst	

	db	0		;End of table marker

cninit:	jmp	noop		;Console initialization
	dw	cinit

const:	jmp	oconst		;Console status
	dw	cstty

conin:	jmp	oconin		;Console input
	dw	citty

conout:	jmp	oconout		;Console output
	dw	cotty

	db	0		;End of table marker

*****************************************************************
*								*
* This is a one-time-only warm boot routine.  It checks		*
* for the presence of a flag characters and, if present,	*
* installs the console driver.  Users who do not intend to	*
* use a Multio as console should type a flag argument '-'.	*	
*								*
*****************************************************************

start:	lxi	h,wboot		;Set up list device pointers
	call	setup
	lda	cpmbuf		;Test for flag
	ora	a
	lxi	h,cninit
	cz	setup		;If the flag is reset then set up more routines
	call	cninit		;Initialize console and list devices
	call	linit
	jmp	owboot		;Do regular warm boot

setup:	mov	a,m		;Test for end of table
	ora	a
	rz
	inx	h		;Bump to start of pointer
	mov	d,h		;Make 'from' pointer
	mov	e,l
	inx	d
	inx	d
	ldax	d		;Move low byte of pointer
	mov	m,a
	inx	d
	inx	h
	ldax	d		;Move high byte of pointer
	mov	m,a
	inx	h		;Bump to next table entry
	inx	h
	inx	h
	jmp	setup

noop:	ret

*****************************************************************
*								*
* Multi I/O or Decision I console driver			*
*								*
*****************************************************************

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

citty:	call	selcon		;Select console group

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

cotty:	call	selcon		;Select console

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

cstty:	call	selcon		;Select console

	in	lsr		;Read status register
	ani	dr
	rz			;No charactter ready
	mvi	a,0ffh		;Character ready
	ret

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

otlst:	lda	group		;Set printer initialized flag
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

cmnd:	call	selg0		;Select group 0

cmnd0:	in	daisy0
	ana	d
	jz	cmnd0
	mov	a,l		;Negate low data bits
	cma
	mov	l,a
	mov	a,h
	ani	d9+d10+d11+d12	;Mask in data bits only
	cma
	push	psw
	lda	mother
	ora	a
	jz	cmndmt
	pop	psw		;Toggle lines on a motherboard
	mov	h,a
	mov	a,l
	out	daisy1		;Output low bits
	mov	a,h
	out	daisy0		;Output high bits
	xra	e		;Slap strobe bits in
	out	daisy0
	mov	a,h		;And drop strobes back down
	out	daisy0
	ret

cmndmt:	pop	psw		;Toggle lines on a Multio
	ani	0ffh-rest	;Keep ribbon up on a Multio
	mov	h,a
	mov	a,l
	out	daisy0		;Output low bits
	mov	a,h
	out	daisy1		;Output high bits
	xra	e		;Slap strobe bits in
	out	daisy1
	mov	a,h		;And drop strobes back down
	out	daisy1
	ret

*****************************************************************
*								*
* New list device status routine. Returns 0ffh if the printer	*
* can except another character, otherwise it returns 0.		*
*								*
*****************************************************************

stlst:	lda	group		;Check printer initialized flag
	ani	denable
	rz			;0 = printer not initialized
	call	selg0		;Select group 0
	lxi	d,pwstrd
	in	daisy0
	ana	d
	mvi	a,0
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

*****************************************************************
*								*
* Console and list device initialization routines follow.	*
*								*
*****************************************************************

*****************************************************************
*								*
* Terminal initilization routine.  This routine reads the sense	*
* switch on the WB-14 and sets the speed accordingly.		*
*								*
*****************************************************************

cinit:	call	selcon		;Select console

	mvi	a,dtrenb+rtsenb	;Enable DTR and RTS outputs to terminal
	out	mcr
	in	rbr		;Clear reciever buffers
	in	rbr
	xra	a
	out	lsr		;Clear status
	out	ier		;Set no interrupts

	call	selg0		;Select group 0
	in	sensesw		;Get sense switches
	xri	0ffh		;Test for multio board
	sta	mother		;Save motherboard flag
	jz	valid		;Skip switch checkout for multio's
	xri	0ffh
	ani	0e0h		;Mask in upper three bits
	rlc
	rlc
	rlc			;Move into lower 3 bits
	cpi	7		;check for sense = 7
	push	psw		;Save value
	call	selcon		;Re-select console
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

*****************************************************************
*								*
* Initialize Diablo HyType printer.				*
*								*
*****************************************************************

linit:	call	selg0		;Select group 0
	in	sensesw		;Get sense switches
	xri	0ffh		;Test for multio board
	sta	mother		;Save motherboard flag 
	jnz	mthrin
				;Initialize a Multio
	lda	group		;Get group byte
	ani	0ffh-(denable+restor)	;Clear driver enable + restore
	sta	group
	ori	denable+restor	;Enable output drivers and pull restore down
	out	grpsel
	mvi	c,10		;Hold restore line down for 50uS
delop:	dcr	c
	jnz	delop
	ani	0ffh-(denable+restor)	;Clear driver enable + restore
	out	grpsel
	xra	a
	out	daisy0		;Clear data buffers
	lda	group
	ori	denable
	out	grpsel		;Re-enable the drivers
	mvi	a,0ffh-rest	;Lift ribbon
	out	daisy1
	jmp	hydone
				;Mother board initialization
mthrin:	mvi	a,pselect+rlift	;Set select line active, rlift not active
	out	clk
	mvi	a,0ffh
	out	daisy0
	mvi	a,0ffh-rest	;Strobe restore bit low
	out	daisy0
	mvi	a,10		;Wait about 50uS
dloop:	dcr	a
	jnz	dloop
	mvi	a,0ffh		;Raise restore back up
	out	daisy0
	xra	a
	out	daisy1		;Clear data buffers
	mvi	a,pselect	;Lift ribbon
	out	clk

hydone:	lxi	h,hinc/cperi
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

selg0:	lda	group		;Select group zero
	out	grpsel
	ret

selcon:	lda	group		;Select console group
	ori	congrp
	out	grpsel
	ret

sellst:	lda	group		;Select printer group
	ori	lstgrp
	out	grpsel
	ret

	db	0		;Ensure generation of a complete hex file

	end
