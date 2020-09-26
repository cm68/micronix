
*****************************************************************
*								*
* Single Drive copy program. This program allows pip (or any	*
* other program) to copy or move data between diskettes by 	*
* prompting for a disk change operation on drive A. Will allow	*
* back ups to be made with only one drive.			*
*								*
* Written for Thinker Toys by Bobby Dale Gifford.		*
* 12/17/79							*
*								*
*	The program SINGLE tricks CP/M into thinking there are	*
* two drives when in actuality only one drive is present in the	*
* system. To execute the program give the command:		*
*	SINGLE S						*
* followed by a carriage return. The messge:			*
*	SINGLE INSTALLED					*
* should appear, followed by the CP/M prompt. At this time,	*
* if an access to drive B is made, a prompt will appear		*
* requesting you to insert a diskette of the proper density	*
* into drive A. Type a return. CP/M will now access drive A	*
* thinking it is actually drive B. When CP/M return to drive	*
* A, another prompt will be given to you requesting that you	*
* reinsert the original drive A diskette. This procedure will	*
* continue until the command:					*
*	SINGLE E						*
* is given followed by a carriage return. A message should	*
* appear saying:						*
*	SINGLE REMOVED						*
* followed by the CP/M prompt.					*
*      The program is intended to allow backing up diskettes	*
* for those users with only one drive. To back up a diskette	*
* follow these these procedures.				*
*								*
*	1) Execute SINGLE S.					*
*	2) Use PIP to copy all or a selected group of files	*
*	   (actually any program should run).			*
*	3) Follow the prompts for inserting diskettes ( Suggest *
*	   you write protect the original diskette (Tab OFF)).	*
*	5) When the CP/M prompt reappears, ececute SINGLE E.	*
*								*
*****************************************************************

	title	'*** Single Drive Utility For CP/M Ver 2.2 ***'

acr	equ	0ah		;Carriage return
alf	equ	0dh		;Line feed
wboot	equ	0		;CP/M warm boot
tpa	equ	100h		;CP/M transient program area
tbuff	equ	80h		;Command line buffer
ccpsize	equ	800h		;Size of CP/M ccp
aspc	equ	' '		;Space
entry	equ	5		;CP/M BDOS entry location
seljmp	equ	30h		;Offset to jump to select drive

*****************************************************************
*								*
* Main reconfiguration program.					*
*								*
*****************************************************************

	org	tpa

start	lda	tbuff		;Check if any command given
	mov	b,a		;Put counter in B
	lxi	h,tbuff+1	;Beginning of command line
loop	dcr	b		;Check for end of command line
	jm	exit		;If minus then error exit without change
	mov	a,m		;Get charactor typed on command line
	inx	h		;Bump scanning pointer
	cpi	aspc		;Skip spaces
	jz	loop		;Continue if space
	cpi	'S'		;Check for "Start" command
	jz	insert		;Insert the code if true
	cpi	'E'		;Check for "End" command
	jz	remove		;Remove the code if true

*****************************************************************
*								*
* Exit by printing an error message. No change made.		*
*								*
*****************************************************************

exit	lxi	d,what		;No change message
exit1	call	list		;Entry point for different message on exit
	jmp	wboot		;Do a warm boot on exit

*****************************************************************
*								*
* List a message on the CP/M console device.			*
*								*
*****************************************************************

list	mvi	c,9		;Print buffer command
	jmp	entry		;Enter CP/M

*****************************************************************
*								*
* Insert the code for the new selection routine into the ram	*
* below the ccp.						*
*								*
*****************************************************************

insert	lhld	entry+1		;Get pointer to base of BDOS
	lxi	d,-(ccpsize+6+100h)
	dad	d
	push	h		;Save pointer to relocated single
	lxi	d,heart		;Pointer to code to relocate
	lxi	b,map		;Pointer to relocation map
relo	ldax	b		;Get a relocation byte
	ana	a		;Test if byte needs relocated
	jm	putjmp		;If minus then all done
	mov	m,h		;Assume relocation
	jnz	incre		;Move this byte directly
	ldax	d
	mov	m,a
incre	inx	h		;Bump relocation address
	inx	d		;Bump source
	inx	b		;Bump relocation map
	jmp	relo

putjmp	lhld	wboot+1		;Get pointer to CBIOS
	lxi	d,seljmp	;Offset to drive select jump
	dad	d		;Form pointer to drive select jump
	lxi	d,badmsg	;Message pointer
	mov	a,m		;Get the jump opcode
	cpi	(jmp)		;Is it a jump instruction ?
	jnz	exit1		;Exit if not a jump
	inx	h
	mov	a,m		;Get first byte of jump address
	cpi	1bh		;Is it a jump with correct offset ?
	jnz	exit1
	inx	h		;Get page address of controller
	mov	a,m
	pop	h		;Restore pointer to relocated code
	push	h
	push	psw		;Save page address of controller
	inx	h
	lda	entry+1
	mov	m,a		;Insert first byte of new bdos location
	inx	h
	lda	entry+2
	mov	m,a		;Insert second byte of new bdos location
	xra	a
	sta	entry+1		;Modify entry to jump to relocated code
	mov	a,h
	sta	entry+2
	pop	psw		;Restore controller page address
	lxi	d,3
	dad	d
	mov	m,a		;Form second jump (home)
	dad	d
	mov	m,a		;Form third jump (seek)
	dad	d
	mov	m,a		;Form fourth jump (status)
	lda	wboot+2		;Get page address of CBIOS
	dad	d
	mov	m,a		;Form fifth jump (oldinp)
	dad	d
	mov	m,a		;Form sixth jump (oldout)
	lhld	wboot+1		;Get pointer to CBIOS
	lxi	d,seljmp+1	;Offset to select drive jump
	dad	d
	pop	d		;Restore pointer to relocated code
	xchg
	lxi	b,newsel-heart
	dad	b
	xchg
	mov	m,e
	inx	h
	mov	m,d
	lxi	d,insmsg
	jmp	list

*****************************************************************
*								*
* Remove does the opposite of insert. It modifies the address	*
* to be in the original state.					*
*								*
*****************************************************************

remove	lhld	wboot+1		;Pointer to CBIOS
	lxi	d,seljmp	;Offset to select drive
	dad	d		;Form offset to select drive
	lxi	d,badmsg	;Assume error
	mov	a,m		;Get the first byte of the jump
	cpi	(jmp)		;Is it a jump opcode ?
	jnz	exit1		;Error if not
	inx	h
	mov	a,m		;Should point to newsel
	cpi	newsel-heart
	jnz	exit1		;Error if not
	inx	h		;Point to high byte of address
	mov	d,m
	mvi	e,5		;Form address to controller page
	ldax	d		;Get the byte of controller address
	mov	m,a		;Reconstruct controller address
	dcx	h
	mvi	m,1bh		;Offset into controller rom to drive select
	lxi	d,remmsg
	jmp	exit1

*****************************************************************
*								*
* Unknown command error message.				*
*								*
*****************************************************************

what	db	acr,alf
	db	'Unknown command - NO change made.$'

*****************************************************************
*								*
* Successful insertion message.					*
*								*
*****************************************************************

insmsg	db	acr,alf
	db	'Single installed.$'

*****************************************************************
*								*
* Seccessful removal message.					*
*								*
*****************************************************************

remmsg	db	acr,alf
	db	'Single removed.$'

*****************************************************************
*								*
* Bad Cbios format message.					*
*								*
*****************************************************************

badmsg	db	acr,alf
	db	'Bad Cbios format found - NO change made.$'

*****************************************************************
*								*
* Byte for byte map of code to relocate. A zero represents no	*
* relocation, a 1 represents a relocateable byte, and a 0ff	*
* represents the end of the map.				*
*								*
*****************************************************************

map	db	0,0,0		;jmp
	db	0,0,0		;jmp
	db	0,0,0		;jmp
	db	0,0,0		;jmp
	db	0,0,0		;jmp
	db	0,0,0		;jmp
	db	0,0,1		;lxi
	db	0,0,0		;shld
	db	0,0		;mvi
	db	0		;mov
	db	0,0,1		;sta
	db	0		;cmp
	db	0		;rz
	db	0,0,1		;lxi
	db	0		;ana
	db	0,0,1		;jz
	db	0,0,1		;lxi
	db	0,0,1		;call
	db	0,0,1		;call
	db	0,0,1		;lxi
	db	0,0,1		;call
	db	0,0,1		;call
	db	0		;push
	db	0,0,1		;call
	db	0		;pop
	db	0,0,1		;lda
	db	0		;ana
	db	0,0,1		;lxi
	db	0,0,1		;jz
	db	0,0,1		;lxi
	db	0		;mov
	db	0,0,1		;lda
	db	0		;ana
	db	0,0		;mvi
	db	0,0,1		;jz
	db	0,0		;mvi
	db	0,0,1		;jmp
	db	0		;mov
	db	0		;mov
	db	0		;ana
	db	0		;rz
	db	0		;inx
	db	0		;push
	db	0,0,1		;call
	db	0		;pop
	db	0,0,1		;jmp
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0,0,0,0,0
	db	0ffh

	ds	100h-($ mod 100h)

*****************************************************************
*								*
* The rest of the program is relocated to execute with the	*
* current CP/M being executed under.				*
*								*
*****************************************************************

heart	equ	$		;Beginning of code to relocate

diff	set	tpa-$

oldbdos	jmp	0		;New jump to bdos
home	jmp	09h		;Controller track 0 seek
seek	jmp	0ch		;Controller seek routine
status	jmp	27h		;Controller status routine
oldinp	jmp	09h		;Jump installed
oldout	jmp	0ch		;Jump installed

newsel	lxi	h,oldbdos+diff	;Insert new bdos jump in case of warm boot
	shld	entry+1
	mvi	b,0		;Get last selected drive
olddrv	equ	$-1
	mov	a,c		;Put new drive into A
	sta	olddrv+diff	;Update last selected drive
	cmp	b		;Check if same drive
	rz			;Done if same
	lxi	h,insamsg+diff	;Insert A message
	ana	a		;Test for drive A
	jz	next+diff	;Take jump if selecting A
	lxi	h,insbmsg+diff	;Else get insert B message
next	call	mesg+diff	;Print message
	call	oldinp+diff	;Get response
	lxi	h,crlf+diff	;Echo carriage return and line feed
	call	mesg+diff
	call	status+diff	;Get status of currently selected drive
	push	b		;Save track
	call	home+diff	;Home drive
	pop	b		;Restore track
	lda	olddrv+diff	;Get drive to select
	ana	a		;Test for drive A
	lxi	h,trkb+diff	;Point to place to save track for B
	jz	next0+diff	;Take jump if selecting drive A
	lxi	h,trka+diff	;Place to save track for A
next0	mov	m,c		;Save track number
	lda	olddrv+diff	;Get drive to select
	ana	a		;Test for drive A
trka	equ	$+1		;Storage for track on drive A
	mvi	c,0		;Pick up track for A
	jz	seek+diff	;If selecting A then take the jump
trkb	equ	$+1		;Pick up track for drive B
	mvi	c,0
	jmp	seek+diff	;Seek to proper track

*****************************************************************
*								*
* Mesg prints a message on the console.				*
*								*
*****************************************************************

mesg	mov	c,m		;Get char to print
	mov	a,c		;Put copy into A
	ana	a		;Test for end
	rz			;Return if done
	inx	h		;Bump pointer to message
	push	h		;Save pointer
	call	oldout+diff	;Print the char
	pop	h		;Restore the pointer
	jmp	mesg+diff	;Continue printing

*****************************************************************
*								*
* Messages output by the program.				*
*								*
*****************************************************************

insamsg	db	acr,alf,acr,alf
	db	'Insert "A" (return) : '
	db	0
insbmsg	db	acr,alf
	db	'Insert "B" (return) : '
	db	0
crlf	db	acr,alf,0

	end
