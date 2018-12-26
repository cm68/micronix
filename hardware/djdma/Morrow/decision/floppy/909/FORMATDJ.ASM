	title	djdma/format.asm	01-29-82

	.z80

channl	equ	50h		;Start channel address
index	equ	10h		;Delta index status bit
wproct	equ	40h		;Write protected status bit
dready	equ	80h		;Drive ready status bit
home	equ	0a0h		;Internal home-disk routine address
seek	equ	0a3h		;	  seek-track
sdrive	equ	0a6h		;	  set-drive
hsync	equ	0a9h		;	  header-sync
diskd	equ	4001h		;Disk data port
status	equ	4003h		;Status port
contrl	equ	4007h		;Control port

crlfs	equ	0d0ah		;Carraige return / line feed sequence
acr	equ	0dh		;A carraige return character
alf	equ	0ah		;A line feed character
aesc	equ	1bh		;An escape character
adel	equ	7fh		;A delete character
retries	equ	3		;Disk retries before giving up on verify

bdos	equ	5		;Bdos entry address
wcon	equ	2		;Write console function
direct	equ	6		;Direct console I/O
	page	63
start:	ld	sp,ecode+30h		;Initialize the stack pointer
	ld	hl,"0 "			;Initialize track number to 0
	ld	(ftrack+1),hl
	ld	hl,qfmess		;Ask about type of format
	call	getc
	jp	z,ibmst			;Formatting IBM
	cp	1
	jp	z,nstart		;Formatting North Star
	jp	0			;Return to CP/M
ibmst:	ld	hl,1030h		;Initalize command addresss
	ld	(dotcmd+1),hl
	ld	hl,sdadvt
	ld	(atcmd+1),hl
	ld	hl,slcmd+1		;Set 8 inch drive as 0-3
	ld	(hl),0	
	dec	hl
	ld	c,4
	call	lcmd
	ld	hl,drmess		;Get drive number
	call	getcc
	ld	(single+1),a		;Store the drive number in code
	ld	(drive),a		;   and save in a safe place
	ld	a,26			;26 Sectors per track for single
	ld	(nspt),a
	ld	hl,26*128		;Load number of bytes per track
	ld	(trksiz),hl
	ld	a,0e5h			;We format with e5's
	ld	(verval),a
	ld	hl,dnmess		;Get density
	call	getcc
	ld	(densty),a		;Save for later use
	jp	z,side			;Skip sector size if single density
	ld	hl,slmess		;Select sector length message
	call	getcc
	ld	d,0			;Form offset into sector table
	ld	e,a
	ld	hl,tsize		;Get track size
	add	hl,de
	ld	h,(hl)			;Get high byte of size
	ld	l,0
	ld	(trksiz),hl		;Save track size
	inc	a			;Adjust for sector length code
	ld	(dlcode-ddfmt+double),a	;Store in format code
	ld	hl,sptabl
	add	hl,de
	ld	a,(hl)			;Fetch number of sectors
	ld	(dlast-ddfmt+double),a	;Store in format code
	dec	a
	ld	(nspt),a		;   and in the verify section
	ld	a,20h			;Sector length code is 80,100, or 0
dcnst:	add	a,a
	dec	e			;Decrement the sector type
	jp	p,dcnst			;Test for cycle done
	ld	(dsize-ddfmt+double),a	;Store 1/4 length in format code
side:	ld	a,(drive)		;Get drive #
	add	a,"0"			;Make ASCII decimal
	ld	(wdriv8),a		;Save in wait message
	ld	hl,wdmes8		;Wait for a disk
	call	putm
	ld	hl,wdmess
	call	getcc
retryf:	ld	hl,sscmd+1		;Test various drive parameters
	ld	a,(drive)		;Load drive # to sense
	ld	(hl),a
	dec	hl
	ld	c,7			;Offset to status
	call	lcmd
	ld	hl,nrmess		;Not ready message
	cp	82h			;Test for not ready
	jp	z,nready		;Skip if not ready
	ld	a,(sscmd+4)		;fetch status port value
	ld	b,a			;Save for a while
	ld	hl,wpmess		;Write protected message
	and	40h			;Mask in write protected bit
	jp	nz,nready		;Skip if write protected
	ld	a,b			;Restore full status port value
	and	4			;Mask in 'sides' bit
	rrca				;Shift to bit 0
	rrca
	ld	(ddsbit-ddfmt+double),a	;Store in format code double density
	ld	(sdsbit-sdfmt+single),a	;Store in format code single density
	rrca				;Zap around to bit 7
	ld	(nside),a		;And save side flag
	ld	hl,ftmess		;Formatting ... message
	call	putm
	ld	a,1			;Load track counter
	ld	(ctrack),a
	ld	hl,lsdcmd		;Load single density code command
	ld	c,9			;Offset to halt status
	call	lcmd			;Load the code
	ld	hl,dotcmd		;Format track 0 command
	ld	c,5			;Offset to status
	call	lcmd			;Execute the command
	jp	z,proced		;Zero => no error
	ld	hl,nrmess		;Drive not ready message
	cp	82h			;Drive not ready error code
	jp	z,nready		;Test for drive not ready
	ld	hl,wpmess		;Well...  the write tab fell off...
nready:	call	getcc			;Send the message
	jp	z,start			;Zero => start the program over
	jp	retryf			;Go back and do the command over
proced:	ld	hl,sdrdy		;Adjusted execution address of format
	ld	a,(densty)
	or	a			;Test for double density
	jp	z,contue		;Make no adjustments for single density
	ld	hl,lddcmd		;Load double density format command
	ld	c,9			;Offset to halt status
	call	lcmd			;Load the code into controller
	ld	hl,ddadvt		;Advance track execute address
	ld	(atcmd+1),hl		;Update the command execute address
	ld	hl,1030h		;Format execute address
contue:	ld	(dotcmd+1),hl		;Update track format execute address
	ld	hl,atcmd		;Advance track command
	ld	c,5			;Offset to status
	call	lcmd			;Load the command and execute
	cp	77			;Last track value (77 decimal)
	jp	nz,fmtrck		;Zero => formatting done
	ld	a,77			;77 tracks on an 8 inch drive
	ld	(ntrack),a
	jp	verify			;Verify disk
fmtrck:	ld	hl,ftrack		;Pointer to track #
	call	ptrack			;Print and update track number
	ld	hl,dotcmd		;Format a track command
	ld	c,5			;Offset to status
	call	lcmd			;Load and execute the command
	jp	z,contue+3		;Loop back for more tracks
	ld	hl,femess		;Drive has become not ready
	call	putm
	jp	start			;Stop the formatting
	page
;
;	The following routine set up and execute the North Star formatter
;
nstart:	ld	hl,1030h
	ld	(nscmex+1),hl
	ld	a,20h
	ld	(data-nsfmt+nsform),a
	ld	(cpdata-nsfmt+nsform),a
	ld	(verval),a
	xor	a
	ld	(track-nsfmt+nsform),a
	ld	a,10			;Load number of sectors per track
	ld	(nspt),a
	ld	hl,10*512		;Load number of bytes per track
	ld	(trksiz),hl
	ld	hl,slcmd+1		;Set 5 1/4 inch drive as 0-3
	ld	(hl),4
	dec	hl
	ld	c,4
	call	lcmd
	ld	hl,drmess		;Get drive number
	call	getcc
	ld	(nsform+1),a
	ld	(drive),a
	ld	hl,ntmess		;Get number of tracks
	call	getcc
	ld	d,0
	ld	e,a
	ld	hl,nstrak
	add	hl,de
	ld	a,(hl)
	ld	(strack-nsfmt+nsform),a
	ld	(ntrack),a
	push	de
	ld	hl,dnmess		;Get density
	call	getcc
	pop	de
	ld	b,051h
	jp	z,nstore
	push	af
	rrca
	add	a,e
	ld	e,a
	pop	af
	ld	b,0d1h
nstore:	ld	(den1-nsfmt+nsform),a
	ld	a,b
	ld	(den2-nsfmt+nsform),a
	push	de
	ld	hl,simess		;Get number of sides
	call	getcc
	pop	de
	ld	(dflag-nsfmt+nsform),a
	rrca
	ld	(nside),a		;Set up side flag
	rlca
	jp	z,nsdatc
	rlca
	rlca
	add	a,e
	ld	e,a
nsdatc:	push	de
	ld	hl,nsmess		;Ask about North Star vs CP/M format
	call	getcc
	pop	de
	jp	z,nsload
	ld	a,e
	and	80h
	ld	a,10h
	jp	z,nstord
	ld	hl,nstype-80h
	add	hl,de
	ld	a,(hl)
nstord:	ld	(cpdata-nsfmt+nsform),a
	ld	a,0e5h
	ld	(data-nsfmt+nsform),a
	ld	(verval),a
nsload:	ld	a,(drive)		;Get drive #
	add	a,"0"			;Make ASCII decimal
	ld	(wdriv5),a		;Save in wait message
	ld	hl,wdmes5		;Wait for a disk
	call	putm
	ld	hl,wdmess
	call	getcc
nsrtry:	ld	hl,nscmlm
	ld	c,9
	call	lcmd
	ld	hl,nscmex
	ld	c,5
	call	lcmd
	jp	z,nsproc
	ld	hl,nrmess
	cp	82h
	jp	z,$+6
	ld	hl,wpmess		;Tell about fault
	call	getcc
	jp	z,start
	jp	nsrtry
nsproc:	ld	hl,ftmess		;Formatting ... message
	call	putm
	xor	a			;Initialize track counter
	ld	(ctrack),a
	ld	hl,entry
	ld	(nscmex+1),hl
nscont:	ld	hl,ftrack		;Print track #
	call	ptrack
	ld	hl,nscmat
	ld	c,5
	call	lcmd
	ld	b,a
	ld	a,(strack-nsfmt+nsform)
	cp	b
	jp	z,verify		;Go verify disk
	ld	hl,nscmex
	ld	c,5
	call	lcmd
	jp	z,nscont
	ld	hl,femess		;Drive not ready
	call	putm
	jp	start
	page
;
;	Verify the disk
;
verify:	ld	hl,vtmess		;Send verifying ... message
	call	putm
	ld	hl,sacmd		;Set the DMA address
	ld	c,5			;Halt status offset
	call	lcmd
	xor	a			;Initialize track counters
	ld	(ctrack),a
	ld	(rtcmd+1),a
	ld	a,(drive)		;Initialize the drive number
	ld	(rtcmd+3),a
vertrk:	ld	hl,vtrack		;Print track #
	call	ptrack
	ld	a,(nside)
	ld	(rtcmd+2),a		;Load sides flag
	ld	a,retries		;Initialize retry counters
	ld	(retrys),a		;Soft error counter
	ld	(retryh),a		;Hard error counter
versid:	ld	hl,rtcmd		;Verify a side
	ld	c,9			;Status offset
	call	lcmd			;Read a track
	jp	nz,verfat		;Fatal drive error
	ld	hl,sectab		;Lets check out sectors
	ld	a,(nspt)		;Load number of sectors to check
	ld	c,a
	ld	a,40h			;'Ok' code
verspt:	cp	(hl)
	jp	nz,vererr		;Sector does not verify
	inc	hl
	dec	c
	jp	nz,verspt
	ld	hl,rtcmd+2		;Side ok, check for other side
	ld	a,(hl)			;Get side flag
	ld	(hl),0			;Clear flag just in case ...
	or	a			;Test flag
	jp	nz,versid		;Verify other side
	dec	hl			;Bump to track count
	inc	(hl)			;Bump track number
	ld	a,(ntrack)		;Test for end of disk
	cp	(hl)
	jp	nz,vertrk		;Verify next track
	ld	hl,vdmess		;Verify done
	call	putm
	jp	start			;Restart
verfat:	ld	hl,retryh		;Bump hard retry count
	dec	(hl)
	jp	nz,versid
	ld	hl,vemess		;Verify error
	jp	errpr
vererr:	ld	hl,retrys		;Bump hard retry count
	dec	(hl)
	jp	nz,versid
	ld	hl,vsmess		;Verify error
errpr:	call	putm			;Tell user about it
	jp	start			;    and quit
;
;	Execute controller command.
;	hl -> Start of command sequence
;	c  -> Offset pointer to status byte of sequence
;	a  <- Status return (z flag set if = 40h)
;
;	The command sequence must end with a halt.  If any status
;	is returned from the regular part of the command then this
;	status must immediatly precede the halt.  The value in the
;	c register added to hl should produce a pointer to the
;	halt status.
;
lcmd:	ld	a,26h			;Branch channel command
	ld	(channl),a
	ld	(channl+1),hl		;Channel address
	xor	a
	ld	(channl+3),a		;Extended address
	ld	b,0
	add	hl,bc			;Offset to status byte
	ld	(hl),a			;Clear halt status
	out	(0efh),a		;Start controller
waitc:	or	(hl)			;Wait for halt complete
	jp	z,waitc
	dec	hl			;Back up to command status
	dec	hl
	ld	a,(hl)			;Load status
	cp	40h			;Comper to Ok status
	ret
;
;	Print a track #, do fancy backspacing, ect.
;
ptrack:	ld	(ptrk),hl		;Save ASCII pointer
	ld	a,(ctrack)		;Load current track
	push	af
	ld	hl,(ptrk)		;Place to deposite track #
	call	decim3			;Figure track #
	pop	af
	inc	a			;Bump to next track
	ld	(ctrack),a
	ld	bc,300h			;b = fore count, b = back count
	ld	hl,(ptrk)		;Pointer to current track #
	ld	de,otrack		;Pointer to old track #
	ex	de,hl			;Set pointers the way we want them
ptrkcp:	ld	a,(de)			;Get a digit
	cp	(hl)			;Compare against old number
	jp	nz,ptrkdn		;Skip if done
	inc	c			;Bump offset
	inc	de			;     current pointer
	inc	hl			;     old pointer
	dec	b			;     counter
	jp	nz,ptrkcp		;Compare next digit
	ret				;Numbers were the same
ptrkdn:	push	hl			;Save pointer to first dirty digit
ptrklp:	ld	(hl),a			;Copy dirty string over
	inc	de
	ld	a,(de)
	inc	hl
	dec	b
	jp	nz,ptrklp
	ld	hl,bsmess		;Back space
	add	hl,bc			;Offset neccesary amount
	call	putm
	pop	hl			;Retreive string pointer
	call	putm			;And print
	ld	c,6
	ld	e,0ffh
	call	5
	ret
;
;	Put 'a' in (hl) in ASCII decimal form with leading spaces
;
decim3:	ld	c,"0"			;Initialize 'leader' flag
	ld	d,100			;Hundreds
	call	decfig
	ld	d,10			;Tens
	call	decfig
decim1:	ld	c,0			;Force leading 0
	ld	d,1			;Ones
	jp	decfig

decfig:	ld	e,"0"-1
declop:	inc	e
	sub	d
	jp	nc,declop
	add	a,d
	push	af
	ld	a,e
	cp	c			;Handle leading space (sometimes)
	jp	nz,decok
	ld	e," "			;Load space
	jp	decokk
decok:	ld	c," "
decokk:	ld	(hl),e			;Save digit
	inc	hl			;Bump to next position
	pop	af
	ret
;
;	Print a null terminated text to the terminal
;
putm:	ld	a,(hl)			;Get current byte of message
	or	a			;Test for end of message
	ret	z			;Return at end of message
	push	hl			;Save the character pointer
	call	putc			;Output the character
	pop	hl			;Recover the character pointer
	inc	hl			;Advance the character pointer
	jp	putm			;Go get the next character
;
;	Print a character to the terminal
;
putc:	push	af
	push	bc
	push	de
	push	hl
	ld	e,a
	ld	c,wcon
	call	bdos
	pop	hl
	pop	de
	pop	bc
	pop	af
	ret
;
;	Call getc.  Parse ESC and DEL codes.  If these codes
;	were typed then return to start.
;
getcc:	call	getc			;Get code
	jp	m,start			;Restart if ESC or DEL
	ret				;Return regular status
;
;	This routine prints a prompt and then accepts an input character.
;	This input character is compared to a 'reply list'.  If the
;	character is found then an associated 'reply string' is echoed
;	and a value associated with that character is returned.
;
;	The routine is called with a pointer to a string in hl and
;	returns a value in (a) and (zero).  The format for the string
;	follows:
;
;		db	"initial prompt string"
;		db	0			;Null terminator
;		db	2			;Number of 'reply values'
;		db	"a", "A", acr		;A list of characters to be
;						;  with the coresponding reply
;						;  'value.'
;		db	83h			;The reply value.  Parity must
;						;  be set.
;		db	"Reply echo string for a, A, or acr."
;		db	0			;Null terminator
;		db	"d", "D"		;A list of characters to be
;						;  with the coresponding reply
;						;  'value.'
;		db	85h			;The reply value.  Parity must
;						;  be set.
;		db	"Reply echo string for d or D."
;		db	0			;Null terminator
;
;	This structure will return a 3 if an 'a', 'A', or acr is typed
;	and a 5 if a 'd' or 'D' is typed.  The routine prints a CRLF
;	before the initial prompt string and a CRLF after the reply
;	string.  If an ESC or DEL is typed then a code of 80h will be
;	returned and the minus flag will be set.
;
getc:	push	hl
	ld	hl,crlf			;Print an initial CRLF
	call	putm
	pop	hl
	call	putm			;Print prompt
	inc	hl			;Bump to number of valid replies
	ld	c,(hl)
	ld	a,c
	ld	(valid),a		;Save reply count
	ld	(string),hl		;Save string pointer
gtchk:	push	bc			;Save reply count
	push	hl			;Save string pointer
gtwait:	ld	c,direct		;Direct console I/O
	ld	e,255			;We want input!
	call	bdos
	or	a			;Test for no character typed
	jp	z,gtwait		;Wait for a character
	pop	hl
	pop	bc
	cp	3			;Check for control C
	jp	z,0			;    exit to CP/M
	cp	aesc			;Escape and delete get special coverage
	jp	z,gtspec
	cp	adel
	jp	z,gtspec
	ld	b,a			;Save user reply
gtscan:	inc	hl			;Bump to reply string
	ld	a,(hl)			;Load reply character
	or	a			;Test for end of reply string
	jp	m,gtflsh		;Not in this reply list, flush string
	cp	b			;Compare to user reply
	jp	nz,gtscan		;No match, continue scan
gtdone:	inc	hl			;Look for reply value
	ld	a,(hl)
	or	a
	jp	p,gtdone
	push	af			;Save value
	inc	hl			;Bump to response message
	call	putm
	ld	hl,crlf			;Print a trailing CRLF
	call	putm
	pop	af			;Restore reply value
	and	7fh			;Clear parity bit
	ret
gtflsh:	inc	hl			;Look for and of message (null)
	ld	a,(hl)
	or	a
	jp	nz,gtflsh
	dec	c			;Bump reply count
	jp	nz,gtscan		;Continue scan
	ld	a,(valid)		;Reinitialize reply count
	ld	c,a
	ld	hl,(string)		;Reinitialize string pointer
	jp	gtchk			;User guessed wrong, let us try again
gtspec:	ld	a,80h			;Special flag
	or	a			;Zap flags
	ret
	page
slcmd:	db	02eh			;Set/get logical drive settings
	db	0			;Logical to set
	db	0			;Logical drives returned
	db	25h
	db	0

lddcmd:	db	0a1h			;Write controller memory command
	dw	double			;Main memory address pointer
	db	0
	dw	single-double		;Byte count
	dw	1030h			;Controller memory address pointer
	db	25h			;Controller halt command
	db	0			;Halt command status byte

lsdcmd:	db	0a1h			;Write controller memory for single
	dw	single
	db	0
	dw	nsform-single
	dw	1030h
	db	25h
	db	0

dotcmd:	db	0a2h			;Execute controller routine command
	dw	1030h			;Format a track address
	db	0			;Execute command status
	db	25h			;Halt command
	db	0			;Status byte

atcmd:	db	0a2h
	dw	sdadvt			;Advance the track value address
	db	0
	db	25h
	db	0

sscmd:	db	022h			;Sense drive status
	db	0			;Drive #
	db	0			;Drive characteristic byte
	db	0			;Sector size byte
	db	0			;Status port byte
	db	0			;Completion status
	db	25h
	db	0

sacmd:	db	023h			;Set DMA address
	dw	buffer
	db	0
	db	25h
	db	0

rtcmd:	db	029h			;Read track command
	db	0			;Track #
	db	0			;Side #
	db	0			;Drive #
	dw	sectab			;Sactor table
	db	0
	db	0			;Status
	db	25h
	db	0

nscmlm:	db	0a1h			;Load controller memory
	dw	nsform
	db	0
	dw	ecode-nsform
	dw	1030h
	db	25h
	db	0

nscmex:	db	0a2h			;Execute controller memory
	dw	1030h
	db	0
	db	25h
	db	0

nscmat:	db	0a2h			;Advance track command (internal)
	dw	advtrk
	db	0
	db	25h
	db	0

nstrak:	db	35			;Track count table
	db	40
	db	80

nstype:	db	90h
	db	0a0h
	db	0c0h
	db	0
	db	0f0h
	db	0d0h
	db	0e0h

tsize:	db	26*256/100h		;Number of pages per track
	db	15*512/100h
	db	8*1024/100h

sptabl:	db	27			;26 sectors per track (256 bytes)
	db	16			;15 sectors per track (512 bytes)
	db	9			;8 sectors per track (1024 bytes)
	page
qfmess:	dw	crlfs
	db	"Floppy disk format command for the Morrow Designs", acr, alf
	db	"Disk Jockey DMA (DJDMA) floppy disk controller.", acr, alf
	dw	crlfs
	db	"Type an ESC or DEL, at any time, to return to the main menu."
	dw	crlfs
	dw	crlfs
	db	"Select: (I) IBM 3740 compatable 8 inch format.", acr, alf
	db	"	(N) North Star compatable 5 1/4 inch format.", acr, alf
	db	"	    <RETURN> back to CP/M.", acr, alf
	dw	crlfs
	db	"Select (I, N, or <CR>): "
	db	0
	db	3
	db	"i", "I", 80h
	db	"IBM 3740 format"
	db	0
	db	"n", "N", 81h
	db	"North Star format"
	db	0
	db	alf, acr, 82h
	db	"Returning to CP/M"
	db	0

drmess:	db	"Select a drive ( 0, 1, 2, or 3 ): "
	db	0
	db	4
	db	"0", acr, 80h
	db	"Preparing to format drive 0"
	db	0
	db	"1", 81h
	db	"Preparing to format drive 1"
	db	0
	db	"2", 82h
	db	"Preparing to format drive 2"
	db	0
	db	"3", 83h
	db	"Preparing to format drive 3"
	db	0

dnmess:	db	"Select: <D> double density", acr, alf
	db	"	 S  single density", acr, alf
	db	"	"
	db	0
	db	2
	db	"d", "D", acr, 81h
	db	"    Double density selected."
	db	0
	db	"s", "S", 80h
	db	"    Single density selected."
	db	0

ntmess:	db	"Select the number of tracks ( 0=35, 1=40, 2=80 ): "
	db	0
	db	3
	db	"0", acr, 80h
	db	"35 track drive"
	db	0
	db	"1", 81h
	db	"40 track drive"
	db	0
	db	"2", 82h
	db	"80 track drive"
	db	0

slmess:	db	"Select the sector length ( 0=256, 1=512, 2=1024 ): "
	db	0
	db	3
	db	"0", 80h
	db	"256 byte sectors"
	db	0
	db	"1", 81h
	db	"512 byte sectors"
	db	0
	db	"2", acr, 82h
	db	"1024 byte sectors"
	db	0

nsmess:	db	"Select:  N  North star or", acr, alf
	db	"	<C> CP/M data compatibility.", acr, alf
	db	"	"
	db	0
	db	2
	db	"n", "N", 80h
	db	"    North Star data format."
	db	0
	db	"c", "C", acr, 81h
	db	"    CP/M data format."
	db	0

simess:	db	"Select: <S> single sided or", acr, alf
	db	"	 D  double sided media.", acr, alf
	db	"	"
	db	0
	db	2
	db	"s", "S", acr, 80h
	db	"    Single sided media selected."
	db	0
	db	"d", "D", 81h
	db	"    Double sided media selected."
	db	0

nrmess:	db	"Drive not ready - (R)estart program, or (C)ycle: "
	db	0
	db	2
	db	"r", "R", 80h
	db	"Restarting program"
	db	0
	db	"c", "C", acr, 81h
	db	"Cycling"
	db	0

femess:	dw	crlfs
	db	"Drive has become 'not ready' during formatting"
	db	0

wpmess:	db	"Write protected - (R)estart program, or (C)ycle: "
	db	0
	db	2
	db	"r", "R", 80h
	db	"Restarting program"
	db	0
	db	"c", "C", acr, 81h
	db	"Cycling"
	db	0

wdmes5:	dw	crlfs, crlfs
	db	"Insert a write enabled diskette in 5 1/4 inch drive "
wdriv5:	db	"0."
	db	0

wdmes8:	dw	crlfs, crlfs
	db	"Insert a write enabled diskette in 8 inch drive "
wdriv8:	db	"0."
	db	0

wdmess:	db	"Close the drive door and then press <RETURN> "
	db	0
	db	1
	db	alf, acr, 80h
	db	0

ftmess:	dw	crlfs
	db	"Formatting track:"
ftrack:	db	"  0"			;ASCII track number
	db	0

vtmess:	dw	crlfs
	db	"Verifying track: "
vtrack:	db	"  0"
	db	0

bsmess:	db	8, 8, 8			;Back terminal 3 spaces
	db	0

otrack:	db	"  0"			;Old ASCII track number
	db	0

vdmess:	dw	crlfs, crlfs
	db	"Verify done."
	dw	crlfs
	db	0

vemess:	dw	crlfs, crlfs
	db	"Fatal verify error, probable bad diskette."
	dw	crlfs
	db	0

vsmess:	dw	crlfs, crlfs
	db	"Sector verify error"
	dw	crlfs
	db	0

crlf:	dw	crlfs
	db	0
	page
drive:	db	0			;# of drive being formatted
densty:	db	0			;Density flag for current drive
nspt:	db	0			;Number of sectors per track
trksiz:	dw	0			;Size of a track (bytes)
verval:	db	0			;Byte to verify with
nside:	db	0			;Number of sides flag
ntrack:	db	0			;Number of tracks on current drive
ctrack:	db	0			;Current track number
ptrk:	dw	0			;Pointer to ASCII track #

retryh:	db	0			;Retry counter, hard
retrys:	db	0			;Retry counter, soft
valid:	db	0			;Temp save for number of valid replies
string:	dw	0			;Temp pointer to parse trees

sectab:	db	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	;Sector status table
	db	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	db	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	page
;
;	The following routines are the actual formatting routines.
;	These routines are loaded into controller RAM at 1030h
;	and executed as needed.  Currently there are 3 routines:
;
;	IBM 8 inch single density.
;	IBM 8 inch double density.
;	North Star 5 1/4 inch multi desity.
;

;
;	IBM 8 inch single density formatter routine
;
double	equ	$
	.phase	1030h
ddfmt:	ld	hl,status
	bit	7,(hl)		;Check that the drive is ready
nrexit:	ld	a,82h		;Drive not ready error code
	ret	z		;Error exit
	bit	6,(hl)		;Test for write protected
	ld	a,90h		;Write protected error code
	ret	nz		;Error exit
	ld	(ix+0bh),0	;Reset index counter
	ld	a,(dtrck)	;Get the new track value
	cp	(iy+1)		;Compare with current track
	push	af		;Save the track
	call	nz,seek		;Move the head(s) if needed
	ld	hl,diskd	;Pointer to disk shift register
	ld	de,contrl	;Pointer to control port
	pop	af		;Recover the tack
	cp	2bh		;Compare with track 43
	ld	a,4		;No write precompensation
	jr	c,loadpc	;Carry => track is less than 43
	ld	a,14h		;Write precompensation bit set
loadpc:	ld	(precmp),a	;Setup the write precompensation byte
	sbc	a,a		;Push carry bit throughout accumulator
	or	0feh		;Low current bit now set
	and	(iy+2)		;Merge with drive pattern
	or	2		;Select side 0
	ld	(iy+2),a	;Restore drive pattern
	or	0ch		;Turn off step command
	ld	(4005h),a	;Update the drive register
	ld	b,50h		;Preamble length
ddlbl1:	ld	a,(status)
	and	index		;Look for index pulse
	jr	nz,ddlbl1	;Wait for no index pulse present
ddlbl2:	ld	a,(status)
	and	index
	jr	z,ddlbl2	;Wait for leading edge of new indes pulse
	ld	a,90h		;Control byte - normal write/no crc
	ld	(de),a		;Initialize control port
	ld	a,0
precmp	equ	$-1		;Write precompensation & controller start
	ld	(4006h),a	;Start the controller
ddlbl3:	ld	(hl),4eh
	djnz	ddlbl3		;Write the preamble
	ld	b,0ch		;Zero preamble length
ddlbl4:	ld	(hl),0
	djnz	ddlbl4		;Write the zero preamble
	ld	a,80h		;Control byte for 16 bit write
	ld	(de),a		;Change mode
	ld	(hl),52h	;First half of c2
	ld	(hl),24h	;Second half of c2
	ld	(hl),52h	;Another c2
	ld	(hl),24h
	ld	(hl),52h	;The third c2
	ld	a,90h		;Control byte 8 bit write
	ld	(de),a		;Change mode
	ld	(hl),24h	;Finish the sync bytes
	ld	(hl),0fch	;Index mark
	ld	b,32h		;Postamble length
ddlbl5:	ld	(hl),4eh
	djnz	ddlbl5		;Write the postamble

dmloop:	ld	b,0ch		;Zero preamble length
ddlbl6:	ld	(hl),0
	djnz	ddlbl6		;Write the preamble
	ld	a,81h		;16 bit write mode w/crc
	ld	(de),a		;Change mode
	ld	(hl),44h	;First half of a1
	ld	(hl),89h	;Second half of a1
	ld	(hl),44h	;Second a1
	ld	(hl),89h
	ld	(hl),44h	;Third a1
	ld	a,91h		;8 bit write mode w/crc
	ld	(de),a		;Change mode
	ld	(hl),89h	;Finish sync bytes
	ld	(hl),0feh	;Sector header id byte
	ld	(hl),0		;Write the track number
dtrck	equ	$-1
	ld	(hl),0		;Write the side
dside	equ	$-1
	ld	(hl),1		;Write the sector number
dsect	equ	$-1
	ld	(hl),1		;Sector length code
dlcode	equ	$-1
	ld	a,0a1h		;Mode to write crc bytes
	ld	(de),a		;Change mode
	ld	(hl),a
	ld	(hl),a		;Write the crc bytes
	ld	a,90h		;Reset crc generator
	ld	(de),a		;Change mode
	ld	b,16h		;4e postamble length
ddlbl7:	ld	(hl),4eh
	djnz	ddlbl7		;Write the postamble
	ld	b,0ch		;Data field preamble
ddlbl8:	ld	(hl),0
	djnz	ddlbl8		;Write the preamble
	ld	a,81h		;16 bit write w/crc
	ld	(de),a		;Change mode
	ld	(hl),44h	;First half of a1
	ld	(hl),89h	;Second half of a1
	ld	(hl),44h	;Second a1
	ld	(hl),89h
	ld	(hl),44h	;Third a1
	ld	a,91h		;8 bit write w/crc
	ld	(de),a		;Change mode
	ld	(hl),89h	;Finish the 3 sync bytes
	ld	(hl),0fbh	;Data header id byte
	ld	b,40h		;Sector length divided by four
dsize	equ	$-1
ddlbl9:	ld	(hl),0e5h	;Empty sector data byte
	ld	(hl),0e5h
	ld	(hl),0e5h
	ld	(hl),0e5h	;Write four fill bytes
	djnz	ddlbl9		;Test for data field write done
	ld	a,0a1h		;Crc control byte
	ld	(de),a		;Change mode
	ld	(hl),a		;Write the crc bytes
	ld	(hl),a
	ld	a,90h		;Turn off the crc generator
	ld	(de),a		;Change mode
	ld	a,(dsect)	;Get the sector number
	inc	a
	cp	1bh		;Test for last sector +1
dlast	equ	$-1
	ld	(hl),4eh	;First byte of postamble
	jr	nz,$+4		;Zero => all sectors written
	ld	a,1
	ld	(dsect),a	;Update the sector number
	ld	b,35h		;Postamble length less one
ddlbla:	ld	(hl),4eh
	djnz	ddlbla		;Write the postamble
	jr	nz,dmloop
	ld	(hl),4eh	;First fill byte
	ld	b,0		;Double sided bit test
ddsbit	equ	$-1
	ld	a,(dside)
	xor	b		;Conditionally switch the side byte
	ld	(dside),a	;Update the side byte
	ld	(hl),4eh	;Second fill byte
	ld	b,4fh		;Preamble length less one
	ex	Af,Af'		;Save the double sided status
dlblb:	ld	(hl),4eh	;Write a fill byte
	ld	a,(status)
	and	index		;Wait for the index pulse
	jr	z,dlblb
	ex	Af,Af'		;Recover the double sided status
	jr	z,ddlblc	;Zero => track write is done
	ld	a,(iy+2)	;Drive pattern
	or	0ch		;Turn off the step command
	and	0fdh		;Change read/write heads
	ld	(4005h),a	;Update the command register
	ld	(hl),4eh	;First preamble byte
	jp	ddlbl3		;Format the other side
ddlblc:	ld	(hl),4eh	;Trailing fill byte
	ld	(hl),4eh	;Trailing fill byte
	ld	(hl),4eh	;Trailing fill byte
	xor	a
	ld	(de),a		;Turn off the write gate
	ld	a,6
	ld	(4006h),a	;Turn off the controller
	ld	a,40h		;Status code
	ret
ddadvt:	ld	a,(dtrck)	;Get the current track value
	inc	a		;Increment
	ld	(dtrck),a	;Restore the new value
	ret			;Return with current track value
	.dephase
	page
;
;	IBM 8 inch double density formatter routine
;
single	equ	$
	.phase	1030h
sdfmt:	ld	a,0		;Second byte filled with proper drive number
	call	sdrive		;Select the new drive
	ret	nz		;Return if wrong value
	ld	a,(iy+2)	;Get the drive pattern
	or	0fh		;Side 0 and no step command
	ld	(4005h),a	;Update drive control register
	ld	hl,0		;Delay for the head load
sdwait:	dec	hl
	ld	a,h
	or	l
	jr	nz,sdwait
	ld	(ix+0bh),a	;Reset the index counter
sdtrk0:	call	home		;Calibrate the head(s)
	bit	5,(hl)		;Test for track zero
	jr	z,snrext
sdrdy:	ld	hl,status
	bit	7,(hl)		;Test for the drive ready
snrext:	ld	a,82h		;Drive not ready code
	ret	z		;Error exit
	bit	6,(hl)		;Write protect bit
	ld	a,90h		;Write protect error code
	ret	nz
	ld	(ix+0bh),0	;Reset the index counter
	ld	a,(strck)	;Get the new track
	cp	(iy+1)		;Compare with current track
	call	nz,seek		;Do track seek if necessary
	ld	hl,diskd	;Controller data register
	ld	de,contrl	;Control register
	ld	b,28h		;Preamble length
sdlbl1:	ld	a,(status)
	and	index	
	jr	nz,sdlbl1	;Wait for no index pulse
sdlbl2:	ld	a,(status)
	and	index
	jr	z,sdlbl2	;Wait for leading edge of new index pulse
	ld	a,90h		;Clear the crc register & turn on write gate
	ld	(de),a		;Change modes
	ld	a,44h		;Single density & start bit
	ld	(4006h),a	;Start the controller
sdlbl3:	ld	(hl),0ffh
	djnz	sdlbl3		;Write the preamble
	ld	a,80h		;16 bit write mode
	ld	(de),a		;Change modes
	ld	b,0ch		;Zero preamble length
sdlbl4:	ld	(hl),0aah	;Half a zero cell
	djnz	sdlbl4		;Write the zero preamble
	ld	(hl),0f7h	;First half of fc
	ld	a,90h		;8 bit write mode
	ld	(de),a		;Change modes
	ld	(hl),7ah	;Second half of fc
	ld	b,1ah		;Postamble length
sdlbl5:	ld	(hl),0ffh
	djnz	sdlbl5		;Write the postamble

smloop:	ld	a,80h		;16 bit write mode
	ld	(de),a		;Change modes
	ld	b,0ch		;Sector header preamble length
sdlbl6:	ld	(hl),0aah	;Half a zero cell
	djnz	sdlbl6		;Write the preamble
	ld	a,81h		;Enable crc & 16 bit write
	ld	(de),a		;Change modes
	ld	(hl),0f5h	;First half of fe
	ld	a,91h		;Enable crc & 8 bit write
	ld	(de),a		;Change modes
	ld	(hl),7eh	;Second half of fe
	ld	(hl),0		;Write the track
strck	equ	$-1
	lD	(hl),0		;Write the side byte
sside	equ	$-1
	ld	(hl),1		;Write the sector number
ssect	equ	$-1
	ld	(hl),0		;Write the sector length code
	ld	a,0a1h
	ld	(de),a		;Change modes
	ld	(hl),a
	ld	(hl),a		;Write the crc bytes
	ld	a,90h		;Reset the crc
	ld	(de),a		;Change modes
	ld	b,0bh		;Sector header postamble length
sdlbl7:	ld	(hl),0ffh
	djnz	sdlbl7		;Write the postamble
	ld	a,80h		;16 bit write mode
	ld	(de),a		;Change modes
	ld	b,0ch		;Data field preamble length
sdlbl8:	ld	(hl),0aah	;Half a zero cell
	djnz	sdlbl8		;Write the preamble
	ld	a,81h		;Enable crc & 16 bit write
	ld	(de),a		;Change modes
	ld	(hl),0f5h	;First half of fb
	ld	a,91h		;8 bit write
	ld	(de),a		;Change modes
	ld	(hl),6fh	;Second half of fb
	ld	b,80h		;Sector data field length
sdlbl9:	ld	(hl),0e5h
	djnz	sdlbl9		;Write the data field
	ld	a,0a1h
	ld	(de),a		;Change modes
	ld	(hl),a
	ld	(hl),a		;Write the crc bytes
	ld	a,90h		;Reset the crc
	ld	(de),a		;Change modes
	ld	a,(ssect)	;Get the current sector
	inc	a		;Advance
	cp	1bh		;Compare with 27
	ld	(hl),0ffh	;First postamble byte
	jr	nz,$+4		;Zero => all sectors written
	ld	a,1
	ld	(ssect),a	;Update the sector
	ld	b,1ah		;Postamble length less one
sdlbla:	ld	(hl),0ffh
	djnz	sdlbla		;Write the postamble
	jr	nz,smloop	;Test for more sectors to format
	ld	(hl),0ffh	;First fill byte
	ld	b,0		;Side bit
sdsbit	equ	$-1
	ld	a,(sside)	;Get the current side
	xor	b		;Conditionally switch side bits
	ld	(sside),a	;Update the side byte
	ld	(hl),0ffh	;Write second fill byte
	ld	b,19h		;Preamble length less one
	ex	Af,Af'		;Save the double sided status
sdlblb:	ld	(hl),0ffh	;Write a fill byte
	ld	a,(status)
	and	index
	jr	z,sdlblb	;Wait for the index hole
	ex	Af,Af'		;Recover the double sided status
	jr	z,sdlblc	;Zero => single sided
	ld	a,(iy+2)	;Get the drive pattern
	or	0ch		;Turn off the step command
	and	0fdh		;Turn on head one
	ld	(4005h),a	;Update drive control register
	ld	(hl),0ffh	;Write first preamble byte
	jp	sdlbl3		;Go format the other side
sdlblc:	ld	(hl),0ffh	;Trailing byte
	xor	a
	ld	(de),a		;Turn off write gate
	ld	a,6
	ld	(4006h),a	;Turn off the controller
	ld	a,40h		;Status code
	ret
sdadvt:	ld	a,(strck)	;Get the current track
	inc	a		;Advance track value
	ld	(strck),a	;Update the track value
	ret			;Return with track value
	.dephase
	page
;
;	North Star multi desity formatter routine
;
nsform	equ	$
	.phase	1030h
nsfmt:	ld	a,0
	call	sdrive
	ret	nz
	ld	(ix+0bh),0
	ld	a,(iy+2)
	or	0eh
	ld	(4004h),a
	call	hsync
nsexit:	ld	a,82h
	ret	z
track0:	call	home
	bit	5,(hl)
	jr	z,nsexit
entry:	ld	(ix+0bh),0
	ld	a,(track)
	cp	(iy+1)
	call	nz,seek
	ld	a,(4003h)
	and	40h
	ld	a,90h
	ret	nz
	ld	(ix+0ah),80h
wsect0:	call	hsync
	jr	z,nsexit
	xor	a
	cp	(ix+0ah)
	jr	nz,wsect0
	ld	a,90h
	ld	(contrl),a
	ld	hl,diskd
	ld	c,0
	ld	(ix+9),c
	ld	b,11h
	ld	a,0
den1	equ	$-1
	rra
	ld	a,64h
	jr	nc,cstart
	ld	a,18h
strack	equ	$-1
	rra
	add	a,5
	cp	(iy+1)
	sbc	a,a
	and	10h
	or	24h
	ld	b,20h
cstart:	ld	(4006h),a

zerow:	ld	(hl),0
	ex	(sp),hl
	ex	(sp),hl
	djnz	zerow
	ld	a,(den1)
	or	a
	jr	z,lasts
	ld	(hl),0fbh
	ex	(sp),hl
	ex	(sp),hl
lasts:	ld	(hl),0fbh
	ld	b,5ch
	ld	e,20h
data	equ	$-1
	ld	d,20h
cpdata	equ	$-1
	xor	a
d1loop:	ex	(sp),hl
	ex	(sp),hl
	ld	(hl),e
	xor	e
	rlca
	djnz	d1loop
	ld	b,51h
den2	equ	$-1
	ex	(sp),hl
	ex	(sp),hl
	ld	(hl),d
	xor	d
	rlca
	ex	Af,Af'
	ld	a,e
	ld	(cpdata),a
	ex	Af,Af'
	ex	(sp),hl
	ex	(sp),hl
	ld	(hl),e
	xor	e
	rlca
d2loop:	ex	(sp),hl
	ex	(sp),hl
	ld	(hl),e
	xor	e
	rlca
	ex	(sp),hl
	ex	(sp),hl
	ld	(hl),e
	xor	e
	rlca
	djnz	d2loop
	ex	(sp),hl
	ex	(sp),hl
	ld	(hl),a
	ld	a,(den1)
	or	a
	ld	b,11h
	jr	z,$+4
	ld	b,20h
iloop:	ex	(sp),hl
	ex	(sp),hl
	ld	(hl),e
	ld	a,(status)
	and	index
	jr	z,iloop
	inc	c
	ld	a,0ah
	cp	c
	jr	nz,zerow
	ld	c,0
	ld	a,(nsdsid)
	xor	0
dflag	equ	$-1
	ld	(nsdsid),a
	jr	z,ftdone
	ld	a,(iy+2)
	or	0eh
	and	0fdh
	ld	(4004h),a
	jr	zerow
ftdone:	ld	(contrl),a	;Turn off write gate
	ld	a,40h
	ret
advtrk:	ld	a,(track)	;Get the current track
	inc	a		;Advance track value
	ld	(track),a	;Update the track value
	ret			;Return with track value
track:	0
nsdsid:	0
	.dephase
ecode	equ	$		;End of code marker, stack follows
	ds	30h		;Room for the stack
buffer	equ	$		;Track read buffer

	end
