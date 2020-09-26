*****************************************************************
* Diablo 1610 simulator for the Morrow Designs / Thinker Toys	*
* Mult I/O board. The simulator makes the parallel Hityp II	*
* look like a serial 1610.					*
* This interface is designed to work with the INSTALL.COM	*
* program which is available from Morrow Designs / Thinker	*
* Toys. For an explanation of how this works consult the	*
* INSTALL documentation.					*
* Bobby Dale Gifford.						*
* Revised 6/25/81 dmg						*
*****************************************************************

int	equ	1	; 1 for conditional assembly of interrupt driven code.

*****************************************************************
* Special character equates.					*
*****************************************************************

acr	equ	0dh		;Carriage Return
alf	equ	0ah		;Line feed
aetx	equ	3		;ETX character
aack	equ	6		;ACK character
aesc	equ	1bh		;Escape character
abs	equ	8		;Back Space
aht	equ	9		;Horizontal tab
aff	equ	0ch		;Form Feed
abel	equ	7		;Bell
asp	equ	20h		;Space
anul	equ	0		;Null
adel	equ	7fh		;Delete
ars	equ	1eh		;RS character
aus	equ	1fh		;US character
avt	equ	0bh		;Vertical tab

*****************************************************************
* The following equates are for the Mult I/O board.		*
*****************************************************************

grp	equ	7		;Group select port offset
clk	equ	2		;WB14 printer select port
pic0	equ	4		;Interrupt controller port 0
pic1	equ	5		;Interrupt controller port 1
daisy0	equ	0		;Daisy wheel port 0
daisy1	equ	1		;Daisy wheel port 1
imask	equ	40h		;Interrupt enable mask

d4	equ	10h		;8259 data bit 4
ltim	equ	8		;Level triggered interrupt mode
addi	equ	4		;Address interval
sngl	equ	2		;Single 8259
eoi6	equ	66h		;End of interrupt 6

rstbit	equ	080h		;Restore Bit
data11	equ	4		;Data bits on daisy port
data12	equ	8
data910	equ	3
crstrd	equ	1020h		;Carriage ready
pfstrd	equ	810h		;Paper feed ready
pwstrd	equ	2040h		;Print wheel ready

cperi	equ	10		;Default to 10 characters per inch
lperi	equ	6		;Default lines per inch
hinc	equ	120		;Horizontal increments per inch
vinc	equ	48		;Vertical increments per inch
numtabs	equ	160		;Number of horizontal tabs
maxchrs	equ	1024		;Maximum number of printer characters to queue
maxrgt	equ	1584		;Maximum carriage position

*****************************************************************
* Below is a standard CP/M Cbios jump table as required by	*
* INSTALL.							*
*****************************************************************

	jmp	$		;No change in the cold boot

owboot:	jmp	nwboot		;New warm boot routine

oconst:	if	int
	jmp	nconst		;New console status
	else
	jmp	$		;No change
	endif

oconin:	if	int
	jmp	nconin		;New console input
	else
	jmp	$		;No change
	endif

	jmp	$		;No change in the console output

olist:	jmp	nlist		;New list device output

	jmp	$		;No change in the punch device output
	jmp	$		;No change in the reader device output
	jmp	$		;No change in the home routine

osel:	if	int
	jmp	nsel		;New seldsk routine
	else
	jmp	$		;No change
	endif

	jmp	$		;No change in the settrk
	jmp	$		;No change in the setsec
	jmp	$		;No change in the setdma

ord:	if	int
	jmp	nrd		;New read
	else
	jmp	$		;No change
	endif

owr:	if	int
	jmp	nwr		;New write
	else
	jmp	$		;No change
	endif

olstst:	jmp	nlstst		;New list device status

	jmp	$		;No change in the sectran

*****************************************************************
* The following routines are for handshaking with the printer	*
* they can be used directly or by the CBIOS of CP/M.		*
*****************************************************************

	nop			;Marker

rest:	jmp	restor		;Initialization procedure

	if	int

lst:	jmp	lstdev		;Printer character output
				;	character in reg C
hndxof:	jmp	xonoff		;Printer busy test XON/XOFF
				;	returns with:
				;		A = 1 queue full
				;		A = 0 queue not full or empty
				;		A = 0ffh queue empty
hndetx:	jmp	etxack		;ETX and ACK software handshake
				;	returns with:
				;		A = 0 No ACK to transmit
				;		A = 0ffh ACK transmitted
	endif

*****************************************************************
* Dynamic data locations used by the simulator.			*
*****************************************************************

				;		   (made for electric pencil)
base:	db	48h		;Default Mult I/O board base address
				;	Can be changed with -bxx.
dfrmln:	dw	110		;Default forms length 10 times the forms
				;	length switch. Can be changed
				;	with -fxx.
dspace:	dw	cperi		;Default characters per inch.
				;	Can be changed with -cxx.
dlines:	dw	lperi		;Default lines per inch.
				;	Can be changed with -lxx.
autolf:	db	0		;Default to no Auto line feed.
				;	Can be changed with -ax.
hmi:	dw	0		;Horizontal motion index. Set by RESTORE
				;	and escape sequences.
vmi:	dw	0		;Vertical motion index. Set by RESTORE
				;	and escape sequences.
vpos:	dw	0		;Vertical position. Set by platen motion
dlvpos:	dw	0		;Delta vpos. Set by platen motion
hpos:	dw	0		;Horizontal position. Set by carriage motion
dlhpos:	dw	0		;Delta hpos. Set by carriage motion
lmar:	dw	0		;Left margin
dirflg:	db	0		;Direction flag
grhflg:	db	0		;Graphics mode flag
escflg:	db	0		;Escape sequence in progress flag
scstuf	dw	80h		;Scan buffer data

tabstp:	ds	numtabs		;Tab stops array

	ds	30		;Stack space
stack	equ	$

	if	int

queue:	ds	maxchrs		;Circular Queue of printer characters
quetop:	dw	queue		;Queue top pointer
quebot:	dw	queue		;Queue bottom pointer
hlsave:	dw	0		;Used by interrupt routine
afsave:	db	0		;Used by interrupt routine

hndflg:	db	0		;Handshake in progress flag
etxflg:	db	0		;Used for ETX/ACK handshake
ackxon:	db	2		;Default handshake is XON/XOFF
				;	Can be changed with -Hx.
				;	Possible handshakes are:
				;		0 = none
				;		1 = ETX/ACK
				;		2 = XON/XOFF
				;		3 = ETX/ACK through console

*
* The following data only needs to be included if the 8259
* has not been initialized.
*
	dw	0,0,0,0
	dw	0,0,0,0
	dw	0,0,0,0
	dw	0,0,0,0
table:	jmp	noint		;No interrupt
	db	0
	jmp	noint
	db	0
	jmp	noint
	db	0
	jmp	noint
	db	0
	jmp	noint
	db	0
	jmp	noint
	db	0
	jmp	pwint
	db	0
	jmp	noint
	db	0

	endif

*****************************************************************
* New Boot routine, examine the command line put at 80H by	*
* install.							*
*****************************************************************

nwboot:	mvi	a,1		;Is this a second warm boot ?
wbflg	equ	$-1
	ana	a
	mvi	a,0		;Reset the warm boot flag
	sta	wbflg
	jz	owboot		;Don't reset if second warm boot
	jmp	skpdsh

cloop:	call	scan
	jz	nomore
	cpi	'-'		;Check for flag
	jnz	cloop
skpdsh:	call	scan
	jz	nomore

	if	int
	cpi	'H'		;New handshake routine -Hx
	cz	newh
	endif

	cpi	'B'		;New I/O base -Bxx
	cz	newb
	cpi	'F'		;New forms length -Fxx
	cz	newf
	cpi	'C'		;New characters per inch -Cxx
	cz	newc
	cpi	'L'		;New lines per inch -Lxx
	cz	newl
	cpi	'A'		;New auto line feed -Ax
	cz	newa
	jmp	cloop


nomore:	call	rest		;Reset the printer
	jmp	owboot		;Go to the warm boot

scan:	push	h		;Return the next character in the command
	lhld	scstuf		;Pointer to next char
	mov	a,m		;Get next char
	ana	a		;Test error return
	jz	noupdt		;No update
	inx	h		;Update pointer
	shld	scstuf		;Save new pointer
noupdt:	pop	h		;Restore registers
	ret

	if	int
newh:	call	scan		;End of command ?
	rz
	cpi	'1'
	jc	zret		;Invalid ?
	cpi	'3'+1
	jnc	zret
	sui	'0'
	sta	ackxon		;Set new handshake option
	endif

zret:	mvi	a,0
	ret

newb:	call	scan		;End of command ?
	rz
	call	okhex		;Valid hex character ?
	jc	zret
	ral	!	ral	!	ral	!	ral
	mov	b,a
	call	scan
	rz
	call	okhex		;Valid hex character ?
	jc	zret
	ora	b
	mov	b,a
	ani	7		;Check if divisible by 8
	jnz	zret
	mov	a,b
	sta	base		;New I/O base
	jmp	zret

newf:	call	gettwo		;New default forms length
	jc	zret
	lxi	d,10		;Set to ten times the forms length
	call	hltde
	shld	dfrmln
	jmp	zret

gettwo:	call	scan		;Get two decimal digits
	jz	nogd		;No digits
	call	ok09		;Check for 0-9
	jc	nogd
	add	a		;Multiply by 10
	mov	b,a
	add	a
	add	a
	add	b
	mov	b,a
	call	scan		;Get next character
	jz	nogd		;No character
	call	ok09		;Check if 0-9
	jc	nogd		;No good
	add	b		;Add into result
	mov	l,a
	mvi	h,0		;Make it a 16 bit number
	ret
nogd:	stc			;Error return
	ret

newc:	call	scan		;Change the default characters per inch
	rz
	cpi	'1'		;Must be 10 or 12
	jnz	zret
	call	scan
	rz			;Only one character
	cpi	'0'
	mvi	l,10		;It was ten
	jz	newcok
	cpi	'2'
	mvi	l,12		;It was 12
	jnz	zret
newcok:	mvi	h,0		;Make 16 bit integer
	shld	dspace
	jmp	zret

newl:	call	gettwo		;New lines per inch
	jc	zret		;Error reading digits 
	shld	dlines
	jmp	zret

newa:	call	scan		;New auto line feed
	cpi	'1'		;Must be 0 or 1
	jz	newaok
	cpi	'0'
	jnz	zret
newaok:	sui	'0'		;Set the auto flag
	sta	autolf
	jmp	zret

okhex:	call	ok09		;Check first if 0-9
	rnc			;Yes
okaf:	cpi	'A'		;Check if less than 'A'
	rc
	cpi	'F'+1		;Check if greater than 'F'
	cmc
	rc
	sui	'A'+10		;Make into binary
	ret

ok09:	cpi	'0'		;Check for 0-9
	rc			;Less than '0'
	cpi	'9'+1		;Check if greater than '9'
	cmc
	rc
	sui	'0'		;Turn into binary
	ret

*****************************************************************
* New select disk routine, disable interrupts.			*
*****************************************************************

	if	int

nsel:	di
	call	osel		;Execute old disk select
	ei
	ret

nrd:	di			;Execute old disk read
	call	ord
	ei
	ret

nwr:	di	
	call	owr		;Execute old disk write
	ei
	ret

*****************************************************************
* New console status routine, used with ETX/ACK handshake.	*
*****************************************************************

nconst:	lda	ackxon
	cpi	3
	jnz	oconst
	call	oconst		;Check old console status
	ana	a
	rnz
	jmp	hndetx		;Check ETX handshake

*****************************************************************
* New console input routine, used with ETX/ACK handshake.	*
*****************************************************************

nconin:	lda	ackxon		;Determine the type of handshake
	cpi	3
	jnz	oconin		;None, do old conin
	call	oconst
	ana	a
	jnz	oconin
	call	hndetx
	ana	a
	mvi	a,aack
	jz	nconin
	ret

	endif

*****************************************************************
* List is the New list device output. As implemented, it uses	*
* an XON/XOFF or ETX/ACK protocal.				*
*****************************************************************

nlist:	if	not	int

	jmp	diablo

	else

	lda	ackxon
	dcr	a
	jm	lst
	dcr	a
	jm	lstetx
	dcr	a
	jm	lstxon
	jmp	lst

lstetx:	push	b		;Save the character
	call	lst		;Print the character
	pop	b
	mov	a,c		;Check if it was a carriage return
	cpi	acr
	rnz
	mvi	c,aetx		;Send an ETX
	call	lst
wetx:	call	hndetx		;Check if ACK
	ana	a
	rnz
	jmp	wetx

lstxon:	push	b		;Save char to print
	call	hndxof		;Check XOFF
	cpi	1		;Is it full ?
	cz	wxoff
	pop	b		;Recover char to print
	jmp	lst

wxoff:	call	hndxof		;Check XON
	cpi	0ffh
	jnz	wxoff
	ret

	endif

*****************************************************************
* New list device status routine. Returns 0ffh if the printer	*
* can except another character, otherwise it returns 0.		*
*****************************************************************

nlstst:	if	not	int

	call	group0
	lxi	d,pwstrd
	lda	base
	call	input
	ana	d
	mvi	a,0
	rz
	cma
	ret

	else

	call	hndxof		;Check # of characters in queue
	cpi	1
	mvi	a,0
	rz			;Can not except another char
	cma
	ret

*****************************************************************
* Xonoff status. Checks if there are any characters in the	*
* printers character queue. Returns with reg A = 1 if the	*
* character queue is within 10 characters of being full, or	*
* returns with reg A = 0ffh if the character queue is within	*
* 10 characters from being empty, otherwise returns 0.		*
* This can be used to implement the XON and XOFF protocal.	*
*****************************************************************

xonoff:	call	quesiz		;Get number of characters in queue
	xchg
	lxi	h,maxchrs-10
	call	hlcde
	mvi	a,1
	rc
	lxi	h,10
	call	hlcde
	mvi	a,0
	rc
	cma
	ret

*****************************************************************
* ETX/ACK handshake routine.					*
*****************************************************************

etxack:	di
	lda	etxflg
	mov	b,a
	xra	a
	sta	etxflg
	ei
	mov	a,b
	ret

*****************************************************************
* Quesiz returns the number of characters in the queue in HL.	*
*****************************************************************

quesiz:	di
	lhld	quetop		;Get pointer to top of queue
	xchg
	lhld	quebot		;Get pointer to bottom of queue
	ei
	call	hlcde		;Compare HL with DE
	jnc	hlmde		;Subtract DE from HL
	xchg
	call	hlmde
	xchg
	lxi	h,maxchrs
	jmp	hlmde

	endif

*****************************************************************
* Lstdev just puts characters in the printer queue. characters	*
* are removed from the queue by the print wheel interrupt	*
* service routine.						*
*****************************************************************

	if	int

lstdev:	di				;Disabled while manipulating queue
	lhld	quebot			;Get pointer to next slot
	mov	m,c			;Insert the character
	inx	h			;Point to next slot
	shld	quebot
	lxi	d,queue+maxchrs		;Address of first byte beyond queue
	call	hlcde			;Compare HL with DE
	jnz	lstdon			;No match, don't wrap around
	lxi	h,queue			;First address in queue
	shld	quebot

lstdon:	call	group0
	lda	base
	adi	pic1			;8259 mask register
	call	input			;Get current mask contents
	ani	0ffh-imask		;Turn on print wheel interrupt
	mov	b,a
	lda	base
	adi	pic1
	call	output
	ei
	ret

	endif

*****************************************************************
* Restore routine. Restore should be executed to reset the	*
* printer into a known state, and initialize all the ram	*
* dynamic data locations.					*
* Restore assumes that the 8259 interrupt controller on the	*
* Mult I/O board has already been initialized.			*
*****************************************************************

restor:	if	int
	di			;No interrupts
	lxi	d,table
	lxi	h,table
	mov	a,l
	ani	0e0h		;Form 32 byte boundry
	mov	l,a
	push	h
	mvi	c,32
sloop:	ldax	d
	mov	m,a
	inx	h
	inx	d
	dcr	c
	jnz	sloop
	pop	h

	call	group0
	mov	a,l
	ori	d4+ltim+addi+sngl
	mov	b,a
	lda	base
	adi	pic0
	call	output
	mov	b,h
	lda	base
	adi	pic1
	call	output
	mvi	b,0ffh
	lda	base
	adi	pic1
	call	output
*
* End of 8259 initialization
*
	endif



select:	call	group0
	lda	base
	adi	clk
	mvi	b,80h		;Set select line active.
	call	output

	lda	base		;Get base I/O port
	mvi	b,0ffh-rstbit	;Low bit on restore, others high
	call	output		;Output data in register B

	lda	base		;Base I/O port
	mvi	b,-1		;Output Restore bit high
	call	output		;Output data in register B

	lhld	dspace		;Characters per inch
	xchg			;DE = characters per inch
	lxi	h,hinc		;HL = maximum increments per inch
	call	hldde		;Divide Hl by DE
	shld	hmi		;Save hmi = 120/(characters per inch)
	lhld	dlines		;Lines per inch
	xchg			;DE = lines per inch
	lxi	h,vinc		;HL = MAximum increments per inch
	call	hldde		;Divide HL by DE
	shld	vmi		;Save vmi = 48/(lines per inch)
	lxi	h,0		;Other variables default to zero
	shld	vpos
	shld	dlvpos
	shld	hpos
	shld	dlhpos
	shld	lmar

	xra	a
	sta	dirflg
	sta	grhflg

	if	int
	sta	escflg
	sta	hndflg

	lxi	h,queue		;Zero the command queue
	shld	quetop
	shld	quebot

	mvi	b,eoi6		;Specific end of interrupt 6
	lda	base
	adi	pic0
	call	output
	lda	base		;Get the interrupt mask bits
	adi	pic1
	call	input
	ani	0ffh-imask	;Enable the daisy port interrupt
	mov	b,a
	lda	base
	adi	pic1
	call	output		;Output the daisy port interrupt mask low

	ei			;Ok for interrupts now

	endif

*****************************************************************
* Clear all tab stops.						*
*****************************************************************

notabs:	lxi	h,tabstp		;Beginning of tab stop array
	lxi	d,numtabs		;Number of tab stops
notblp:	mvi	m,0			;Reset the tab
	inx	h			;Next tab stop
	dcx	d			;Update repeat count
	mov	a,e			;Test for zero
	ora	d
	jnz	notblp			;Continue zeroing
	ret

*****************************************************************
* Noint should never be executed. If it is then just die.	*
*****************************************************************

	if	int
noint:	jmp	noint			;Die in jump self
	endif

*****************************************************************
* Pwint is the interrupt service routine for the Hityp II.	*
* Remember: interrupts are disabled.				*
*****************************************************************

pwint:	if	int
	sta	afsave		;Save the acumulator
	shld	hlsave		;Save HL
	ral			;Get the carry into register A
	lxi	h,0
	dad	sp		;Get the Stack pointer
	lxi	sp,stack	;Set up new stack
	push	h		;Save old stack pointer
	rar			;Restore the carry
	lda	afsave		;Get original contents of acumulator
	push	psw		;Save acc
	lhld	hlsave		;Get original contents of HL
	push	h		;Save HL
	push	b		;Save BC
	push	d		;Save DE
	call	group0		;Select group 0
	lhld	quebot		;Get bottom of queue
	xchg
	lhld	quetop		;Get top of queue
	call	hlcde		;Is there anything in the queue ?
	jz	empty		;No, queue is empty
	mov	c,m		;Get the next character
	inx	h		;Bump queue pointer
	shld	quetop		;Save the adjusted queue top
	lxi	d,queue+maxchrs	;Address of byte past queue
	call	hlcde		;Need to wrap ?
	jnz	pwdon
	lxi	h,queue		;Adjust queue top
	shld	quetop

pwdon:	call	diablo		;Process the character

intret:	call	group0
	mvi	b,eoi6		;End of interrupt service routine
	lda	base
	adi	pic0
	call	output
	pop	d		;Restore DE
	pop	b		;Restore BC
	pop	h		;Get original HL
	pop	psw		;Restore PSW
	shld	hlsave		;Save HL
	pop	h		;Get original SP
	sphl			;Restore original SP
	lhld	hlsave		;Restore HL
	ei			;Turn interrupts back on
	ret			;Go back

*****************************************************************
* Empty turns off the print wheel interrupt mask bit if the	*
* character queue is empty when an interrupt occurs.		*
*****************************************************************

empty:	call	paper		;Print any remaining motion
	call	carrg
	call	group0
	lda	base		;Base of Mult I/O
	adi	pic1		;Get the interrupt mask register
	call	input		;Read the current mask
	ori	imask		;Turn on the bit
	mov	b,a		;Data into B
	lda	base		;Put the mask back
	adi	pic1
	call	output
	jmp	intret
	ret

	endif

*****************************************************************
* Diablo does all of the character decoding, escape sequences	*
* forward, backward, etc. The list of escape sequences, and	*
* special characters recognized is:				*
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
*****************************************************************

diablo:	mov	a,c		;Get the character to print
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
	lxi	h,level1	;Single character escae sequences
	cpi	aesc
	mov	a,c		;Scan for char in A
	jz	lookup		;Execute single level escape sequence
	lxi	h,level2	;Two character escape sequence
	lda	escflg

*****************************************************************
* Lookup scans the table pointed at by HL looking for a match	*
* of the character in register A.				*
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
* Each of the following tables contains entries of the form:	*
*	1 byte character to match				*
*	2 bytes of address to execute				*
* terminated by a first byte of 0.				*
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
* The following routines execute escape sequences, etc.		*
*****************************************************************

settwo:
doaesc:	mov	a,c		;Get the escape character
	sta	escflg
func0:	ret

doaetx:	if	int
	mvi	a,0ffh		;Set the handshake flag
	sta	etxflg
	endif
	ret

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
	lda	autolf		;In Auto line feed mode ?
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

clrall:	call	notabs		;Clear all horizontal tabs
func2:
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

setlmar: lhld	hpos		;Get current position
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

abshtab: mov	e,c		;Absolute horizontal tab
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

absvtab: mov	e,c		;Absolute vertical tab
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

sethtab: call	tabcol		;Set horizontal tab
	mvi	m,1
	jmp	func1

tabcol:	lhld	hpos		;Compute address of current character col
	xchg
	lhld	dlhpos
	dad	d		;Get logical position
	xchg
	lhld	hmi		;And divide by hmi to get character column
	xchg
	call	hldde
	lxi	d,tabstp
	dad	d		;Index into the tab stop array
	ret

clrhtab: call	tabcol		;Clear horizontal tab
	mvi	m,0
	jmp	func1

doaht:	call	tabcol		;Get current tab column
	lxi	d,tabstp+numtabs
tablop:	inx	h		;Start with next position
	call	hlcde
	jnc	tofar		;Past last tab
	mov	a,m		;Get value of current column
	ana	a		;Test if it is set
	jz	tablop
	lxi	d,tabstp	;Subtract off array address
	call	hlmde
	xchg
	jmp	newdlh
tofar:	lhld	hpos
	xchg
	lxi	h,maxrgt
	call	hlmde
	shld	dlhpos
	ret

doaff:	lhld	dfrmln		;Multiply forms length by 48
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
	jmp	paper

group0:	lda	base
	adi	grp
	mvi	b,0

*****************************************************************
* Output the data in register B to the port in register A.	*
*****************************************************************

output:	sta	outnum		;Put port number in the instruction
	mov	a,b		;Data to register A.
	out	0		;Self modified to port number
outnum	equ	$-1
	ret

*****************************************************************
* Input from the port in register A.				*
*****************************************************************

input:	sta	innum		;Put port number in the instruction
	in	0		;Self modified port number
innum	equ	$-1
	ret

*****************************************************************
* Neghl forms the twos complement of HL.			*
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
* Hlmde subtracts DE from HL and returns.			*
*****************************************************************

hlmde:	xchg
	call	neghl
	xchg
	dad	d
	ret

*****************************************************************
* Hlcde compares HL with DE. On return the Z flag is set if	*
* they are equal, the Carry flag is set if HL is less than DE.	*
*****************************************************************

hlcde:	mov	a,h
	cmp	d
	rnz
	mov	a,l
	cmp	e
	ret

*****************************************************************
* Divide the number in HL by the number in DE.	Return the 	*
* quotient in HL and the remainder in DE.			*
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
* Multiply the contents of HL by the contents of DE.		*
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
* The routines below actually interface to the printer,		*
* causing paper feed, carriage, and print wheel motion.		*
*****************************************************************

carrg:	lhld	dlhpos		;Check for any accumulated motion
	mov	a,h
	ora	l
	rz
	lhld	hpos		;Check for to much motion
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
	mvi	c,data11
posh:	xchg
	lxi	h,0
	shld	dlhpos		;Reset the horizontal increment
	xchg
	mov	a,l
	ani	1
	jz	nohhlf		;No half spaces
	mov	a,c
	ori	data12
	mov	c,a
nohhlf:	call	divid2
	mov	a,h
	ani	data910
	ora	c
	mov	h,a
	lxi	d,crstrd
	jmp	cmnd

paper:	lhld	dlvpos		;Check for any paper motion
	mov	a,h
	ora	l
	rz			;No motion
	mov	a,h
	ana	a
	mvi	c,0
	jp	posv
	call	neghl
	mvi	c,data11
posv:	mov	a,h
	ani	data910
	ora	c
	mov	h,a
	push	h		;Save paper motion
	lhld	vpos
	xchg
	lhld	dlvpos		;Get logical position
	dad	d
	push	h		;Save for now
	lhld	dfrmln		;Get default form length
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
	call	paper
	pop	h
	lxi	d,pwstrd

cmnd:	call	group0
	lda	base
	call	input
	ana	d
	jz	cmnd
	mov	a,l
	cma
	mov	l,a
	mov	a,h
	ani	data910+data11+data12
	cma
	mov	h,a
	lda	base
	adi	daisy1
	mov	b,l
	call	output
	lda	base
	mov	b,h
	call	output
	mov	a,h
	xra	e
	mov	b,a
	lda	base
	call	output
	mov	b,h
	lda	base
	jmp	output

	end
