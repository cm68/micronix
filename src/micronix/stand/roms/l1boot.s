;****************************************************************
;*								*
;* Level 1 boot.  A 4k rom living at physical 0ff0000h that	*
;* mon500's ramcode copies down to 0f000h and enters in task 0.	*
;*								*
;* Level 0 is the monitor in its socket; level 1 is this, whose	*
;* job is to bring in 512 bytes from sector 0 of a disk; that	*
;* 512 bytes is level 2 and it loads level 3, the filesystem-	*
;* aware loader that offers a menu of kernels.			*
;*								*
;* It knows one thing per kind of disk - how to ask the card	*
;* whether a drive is there, and how to move block 0 off it -	*
;* and it walks those in table order twice: once to find every	*
;* disk carrying a label, and once to boot the one chosen.	*
;*								*
;****************************************************************

	org	0F000h

;****************************************************************
;* Console equates.  The monitor does not carry code for a uart	*
;* that may not be there; it finds the console at reset, programs	*
;* the chip, and builds the driver for it into on-board ram at	*
;* conram, which the level 1 rom also has mapped in task 0.	*
;****************************************************************

conram	equ	0132h		;the console found at reset
concon	equ	conram		; - which one answered: 0, 1 or 2
condrv	equ	conram + 2	; - the driver built for it
csend	equ	0		;+0: send the character in C
crecv	equ	3		;+3: wait for a character into A
credy	equ	6		;+6: is one waiting?  Z set if not

;****************************************************************
;* IDE board equates (8255 PPI on an ATA drive)			*
;****************************************************************

idepa	equ	30h		;port A: drive data, low byte
idepb	equ	31h		;port B: drive data, high byte
idepc	equ	32h		;port C: register number, strobes
idepctl equ	33h		;8255 mode word
idepdrv equ	34h		;drive select latch
idepnrm equ	92h		;A and B in, C out
idepwrt equ	80h		;A, B and C all out
idecs0	equ	08h		;chip select
idewr	equ	20h		;/WR
iderd	equ	40h		;/RD
iderst	equ	80h		;/RST
iderdc	equ	20h		;read sector(s), with retry

;****************************************************************
;* NCR 5380 SCSI equates						*
;****************************************************************

scsidat	equ	20h		;data register (CSD), read and write
scsiicr	equ	21h		;initiator command register
scsimr	equ	22h		;mode register
scsistat equ	24h		;current bus status
icr_rst	equ	80h
icr_ack	equ	10h
icr_bsy	equ	08h
icr_sel	equ	04h
icr_data equ	01h
scsi_bsy equ	40h
scsi_req equ	20h
scsi_mon equ	04h

;****************************************************************
;* on-board registers						*
;****************************************************************

mapram	equ	0600h		;actual map ram (write only)
map	equ	0200h		;map ram image (the readable copy)
task	equ	0402h		;the task register: write to switch
gobuff	equ	01b0h		;ram stub for the task switch
ctask	equ	0006h		;current task, read by the trap handler

;****************************************************************
;* the disk label, in the boot sector's second half		*
;* (include/sys/dlabel.h).  It is the drive speaking, and the	*
;* only thing in it level 1 cares about is that it is there at	*
;* all: the magic, and a geometry that is not empty.  The	*
;* version is deliberately not checked - the kernel does not	*
;* check it either, and a labelled disk boots whatever it says.	*
;****************************************************************

dl_off	equ	256		;into block 0: the second half
dl_trk	equ	dl_off + 6	;d_tracks, a word
dl_hds	equ	dl_off + 8	;d_heads
dl_spt	equ	dl_off + 10	;d_spt, blocks per track
dl_mfg	equ	dl_off + 88	;d_mfg, sixteen bytes
dl_mod	equ	dl_off + 104	;d_model, sixteen bytes

;****************************************************************
;* entry: the byte mon500 jumps to.  Its ramcode copies this 4k	*
;* down to physical 0f000h and jumps there in task 0, with the	*
;* map back to identity - so this must be the first thing in	*
;* the image, and nothing may sit ahead of it.  The monitor's	*
;* halt trap is live, which is what makes a halt here reach the	*
;* monitor rather than run off into nothing.			*
;*								*
;* Pass 1 finds every disk that answers with a label on it and	*
;* prints them; pass 2 boots the one chosen.			*
;****************************************************************

entry:
	ld	hl,strset	;"level 1 boot"
	call	uputs
	call	enumer		;pass 1: fill cand[] and print it
	ld	a,(ncand)
	or	a
	jp	nz,bootit
	ld	hl,strnone	;"no boot device"
	call	uputs
hang:	halt
	jr	hang

;****************************************************************
;* The controller table: one row per kind of disk the machine	*
;* can boot from, in the order they are probed, six bytes a row	*
;* - the name the menu prints, the routine that says whether a	*
;* drive is there, and the routine that moves block 0 to	*
;* (dest).  A row with a zero name ends the table.		*
;*								*
;* A probe answers with carry set if a drive is there and ready	*
;* to be read; a read answers with carry set if it delivered	*
;* the block.  Both leave the row cursor alone.			*
;****************************************************************

ctable:
	dw	cide, ideprobe, ideread
	dw	cscsi, ncrprobe, ncrread
	dw	0

;****************************************************************
;* pass 1: walk the table and keep every disk carrying a label.	*
;* Each survivor is remembered by the row it came from - so	*
;* pass 2 can run that row's own read again - and printed with	*
;* what it says about itself, while its block 0 is still in the	*
;* staging buffer to be read.					*
;****************************************************************

enumer:
	ld	hl,ctable
	ld	(cur),hl
	xor	a
	ld	(ncand),a
	ld	hl,cand
	ld	(chead),hl
enuml:	ld	hl,(cur)
	call	loadrow		;the row's three pointers
	ld	a,(cname)	;a zero name ends the table
	or	a
	ret	z
	ld	a,(cname+1)
	or	a
	ret	z
	call	rowatch
	jr	nc,enumnt
	ld	hl,(chead)	;remember the row, not the disk:
	ld	bc,(cur)	; pass 2 re-runs it, so a drive
	ld	(hl),c		; that vanished is caught then
	inc	hl
	ld	(hl),b
	inc	hl
	ld	(chead),hl
	ld	a,(ncand)
	inc	a
	ld	(ncand),a
	call	putnum		;"1) "
	ld	hl,(cname)
	call	uputs
	ld	hl,strcolon	;": "
	call	uputs
	call	putid		;what it calls itself, or its shape
	ld	hl,strcomma	;", "
	call	uputs
	call	putmb		;how big, in megabytes
	ld	hl,strmb	;" MB"
	call	uputs
	ld	hl,strnl
	call	uputs
enumnt:	ld	hl,(cur)
	ld	de,6		;one row further on
	add	hl,de
	ld	(cur),hl
	jr	enuml

;****************************************************************
;* one row: probe it, and if a drive answers, read block 0 into	*
;* the staging buffer and look for a label.  Carry set means	*
;* the row is a candidate.					*
;****************************************************************

rowatch:
	ld	hl,(cprobe)
	call	jphl
	jr	nc,rowno	;nothing answered
	ld	hl,stg
	ld	(dest),hl
	ld	hl,(cread)
	call	jphl
	jr	nc,rowno	;the drive stopped answering
	ld	hl,stg + dl_off
	ld	de,dlmagic
	ld	b,4
rowmg:	ld	a,(de)
	cp	(hl)
	jr	nz,rowno
	inc	hl
	inc	de
	djnz	rowmg
	ld	hl,stg + dl_trk
	call	nzw		;d_tracks
	jr	z,rowno
	call	nzw		;d_heads
	jr	z,rowno
	call	nzw		;d_spt
	jr	z,rowno
	scf			;it is a disk
	ret
rowno:	or	a		;carry clear: no candidate
	ret

;	the word at (hl) is not zero, and hl moves past it
nzw:	ld	a,(hl)
	inc	hl
	or	(hl)
	inc	hl
	ret

;	hl -> a table row: its three pointers into cname/cprobe/cread
loadrow:
	ld	de,cname
	ld	bc,6
	ldir
	ret

;	print the candidate number in A in decimal, then ") "
putnum:	ld	hl,decbuf + 4
	ld	(hl),')'
	inc	hl
	ld	(hl),' '
	inc	hl
	ld	(hl),0
	ld	hl,decbuf + 4	;the digits go below this
	ld	c,a
pnlp:	ld	a,c		;peel a digit off the right
	ld	b,0
pnsub:	sub	10
	jr	c,pndig
	inc	b
	jr	pnsub
pndig:	add	a,10		;the remainder
	add	a,'0'
	dec	hl
	ld	(hl),a
	ld	c,b
	ld	a,b
	or	a
	jr	nz,pnlp
	call	uputs
	ret

;****************************************************************
;* identify the disk whose block 0 is in staging.  A labelled	*
;* drive need not name itself: d_mfg and d_model hold whatever	*
;* the formatter was told, and it may have been told nothing.	*
;* A disk with no name is identified by its shape instead, and	*
;* 306x6x17 says what "Quantum Q540" says - and is always there,	*
;* because a geometry of zero is what made this a disk at all.	*
;****************************************************************

putid:	ld	hl,stg + dl_mod
	ld	a,(hl)
	or	a
	jr	z,pidgeo	;no name: go by the geometry
	ld	hl,stg + dl_mfg
	call	str16		;the maker, when the label has one
	jr	z,pidmod
	ld	hl,strsp
	call	uputs
pidmod:	ld	hl,stg + dl_mod
	call	str16
	ret

pidgeo:	ld	hl,stg + dl_trk
	call	putdec16	;d_tracks
	ld	c,'x'
	call	uout
	ld	hl,stg + dl_hds
	call	putdec16	;d_heads
	ld	c,'x'
	call	uout
	ld	hl,stg + dl_spt
	jp	putdec16	;d_spt

;	the field of sixteen bytes at hl, printed up to its nul.
;	The drive need not have terminated it, so it is bounded
;	here.  Z is set when the field was empty - which is decided
;	by its first byte and not by the nul that ends every field,
;	and is how a disk that does not name itself is told from
;	one that does.
str16:	ld	a,(hl)
	or	a
	ret	z		;the first byte is the terminator
	ld	b,16
st16l:	ld	a,(hl)
	or	a
	jr	z,st16d
	ld	c,a
	call	uout
	inc	hl
	djnz	st16l
st16d:	or	1		;something was printed
	ret

;	the sixteen bit word at hl, in decimal
putdec16:
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	;	fall into putdec

;	print de in decimal.  A digit comes off the right by subtracting
;	ten until the value goes below zero and adding the ten back;
;	the digits go into the buffer backwards and the count of
;	subtractions is what is left to print.
putdec:	ld	hl,numbuf + 7
	ld	(hl),0		;the number ends here and grows down
pdloop:	ld	bc,0		;the quotient, counted up
pdsub:	ld	a,e
	sub	10
	ld	e,a
	ld	a,d
	sbc	a,0
	ld	d,a
	jr	c,pddig		;below zero: this digit is done
	inc	bc
	jr	pdsub
pddig:	ld	a,e
	add	a,10		;the ten that took it below, back
	add	a,'0'
	dec	hl
	ld	(hl),a
	ld	a,b		;whatever is left to print
	or	c
	jr	nz,pdmore
	jp	uputs		;nothing left: hl is the whole number
pdmore:	ld	d,b
	ld	e,c
	jr	pdloop

;****************************************************************
;* how big the disk is, in the same words the kernel's own	*
;* drive probe uses it: tracks * heads * sectors per track, in	*
;* 512 byte blocks, halved eleven times to megabytes.		*
;*								*
;* Heads and sectors per track are read as bytes, because a	*
;* byte is what the kernel's view of a geometry makes them, and	*
;* the megabytes come off the low word of the product - which	*
;* is the truncation the kernel's own cast to an int does, so	*
;* that a disk big enough to reach it reads the same in both	*
;* places.  That takes more than 65535 of them, and nothing	*
;* these cards can drive is anywhere near it.			*
;****************************************************************

putmb:	ld	hl,stg + dl_hds
	ld	e,(hl)
	ld	d,0		;d_heads
	ld	hl,stg + dl_spt
	ld	l,(hl)
	ld	h,0		;d_spt
	call	dmul		;heads * sectors per track
	ld	(hs),hl		;neither is over a byte wide, so
				; this is still one word
	ld	hl,stg + dl_trk
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		;d_tracks
	ex	de,hl
	ld	de,(hs)
	call	dmul		;tracks * heads * sectors per track
	ld	a,11
pmbsh:	srl	b
	rr	c		;the thirty two bit product, halved
	rr	h		; eleven times
	rr	l
	dec	a
	jr	nz,pmbsh
	ex	de,hl
	jp	putdec

;****************************************************************
;* dmul: hl = hl * de, the thirty two bit product landing in	*
;* bc:hl with bc the high word.  The multiplier is parked in	*
;* scratch, since the loop wants all four registers: two for the	*
;* product, two for the multiplicand.  The counter is pushed.	*
;*								*
;* The product shifts up and the multiplier shifts up with it,	*
;* the bit coming out of the multiplier's top being the one to	*
;* add on.  Both operands stay sixteen bits that way - shifting	*
;* the multiplicand instead would lose it above 64k, which is	*
;* most disks.							*
;****************************************************************

dmul:	ld	(mulr),de	;the multiplier
	ex	de,hl		;de is the multiplicand
	ld	hl,0		;and bc:hl the product
	ld	bc,0
	ld	a,16
dmull:	push	af
	add	hl,hl		;the product shifts up first, to
	rl	c		; make room for this round
	rl	b
	ld	a,(mulr)	;and the multiplier shifts up, its
	sla	a		; top bit coming out to be tested
	ld	(mulr),a
	ld	a,(mulr + 1)
	rl	a
	ld	(mulr + 1),a
	jr	nc,dmuln	;that bit was a zero
	add	hl,de		;the product takes the multiplicand
	jr	nc,dmuln
	inc	bc
dmuln:	pop	af
	dec	a
	jr	nz,dmull
	ret

;****************************************************************
;* ask which disk to boot, and answer with its number, 1 to	*
;* ncand.  The entries are already on the screen; this waits	*
;* for a number and takes the first candidate if nothing comes,	*
;* so a machine left alone boots from the first disk that	*
;* answered, and a machine with one disk is never asked.	*
;*								*
;* Input is the monitor's own console driver, the one the menu	*
;* was printed through, so this answers on whichever console	*
;* mon500 found.  The driver does not echo - it is a character	*
;* driver with no idea what a line is - so a key that is taken	*
;* is echoed here, and one that is not is echoed and the	*
;* question asked again.					*
;****************************************************************

tmo	equ	12000		;a delay loop's worth, not a clock:
				; an idle machine waits about five
				; seconds before choosing for itself

menu:	ld	a,(ncand)
	cp	1
	ret	z		;one disk is not a choice

menask:	ld	hl,strpick	;"boot: "
	call	uputs
	ld	de,tmo		;the machine's whole patience,
				; renewed if the answer made no sense
menw:	ld	c,0		;the delay between two looks at
menwl:	dec	c		; the keyboard
	jr	nz,menwl
	push	de
	call	condrv + credy	;Z set if nothing has arrived
	pop	de
	jr	nz,mengot
	dec	de
	ld	a,d
	or	e
	jr	nz,menw
	ld	a,1		;nothing came: the first candidate
	jr	menend

mengot:	call	condrv + crecv	;the key, into a
	push	af
	ld	c,a
	call	uout		;echo it, since the driver will not
	pop	af
	cp	0dh		;a bare return takes the default
	jr	z,menone
	cp	0ah
	jr	z,menone
	sub	'0'		;a number, and one that is there
	jr	c,menbad
	jr	z,menbad	;and not zero
	ld	c,a
	ld	a,(ncand)
	cp	c
	jr	c,menbad
	ld	a,c
menend:	push	af		;leave the cursor on a line of its own
	ld	hl,strnl
	call	uputs
	pop	af
	ret

menone:	ld	a,1
	jr	menend
menbad:	ld	hl,strnl	;said something that is not a disk
	call	uputs
	jr	menask		;so ask again

;****************************************************************
;* pass 2: ask which disk, then re-probe that row and read its	*
;* block 0 into task 1's 100h, through the alias window - task	*
;* 0's segment 1 pointed at bus page 0, so the rom's 1100h	*
;* writes the 100h task 1 will run from.			*
;****************************************************************

bootit:
	call	menu		;which disk?  a number in a
	dec	a
	add	a,a		;a candidate is a row, two bytes
	ld	e,a		; apiece
	ld	d,0
	ld	hl,cand
	add	hl,de
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	ex	de,hl		;hl is the chosen row
	call	loadrow
	ld	hl,(cprobe)
	call	jphl
	jp	nc,bootng	;the drive answered twice and then not
	;	The transfer lands on physical 100h-2ffh, which is
	;	task 0's own stack - the monitor's, growing down from
	;	the map image at 200h.  Move the stack into this rom
	;	page, where 512 bytes cannot reach it, before the
	;	transfer starts.  1100h and this page are both in
	;	task 0: one through the window, one through segment 15.
	ld	sp,stktop
	xor	a
	ld	(mapram + 2),a		;seg 1 -> page 0
	ld	hl,01100h		;= 100h to task 1
	ld	(dest),hl
	ld	hl,(cread)
	call	jphl
	;	Both the register and the readable image of it: the map ram
	;	is write only, so 200h is the only way anything afterwards
	;	can see what task 0 is mapped to.
	ld	a,1			;seg 1 -> page 1, back to identity
	ld	(mapram + 2),a
	ld	(map + 2),a
	ld	a,2			;and seg 2 -> page 2
	ld	(mapram + 4),a
	ld	(map + 4),a
	ld	de,0100h		;the sector just read
	ld	a,1			;run it as task 1
	jp	switch
bootng:	ld	hl,strgone	;"boot disk stopped answering"
	call	uputs
	jp	hang

;****************************************************************
;* ide: reset the drive and wait for it to answer ready		*
;****************************************************************

ideprobe:
	ld	a,idepnrm		;8255 to read mode
	out	(idepctl),a
	ld	a,iderst		;/RST asserted
	out	(idepc),a
	ld	b,0
idebol:	in	a,(idepa)		;the read keeps the delay real
	djnz	idebol
	xor	a
	out	(idepc),a		;/RST released
	out	(idepdrv),a		;unit 0
	ld	de,0
ideprb:	call	rdstat			;register 7: status
	and	40h			;DRDY
	jr	nz,idepry
	dec	de
	ld	a,d
	or	e
	jr	nz,ideprb
	or	a			;no drive: carry clear
	ret
idepry:	scf
	ret

;****************************************************************
;* ide: DRDY is already set.  Load the task file, wait for the	*
;* sector, and move it to (dest).				*
;****************************************************************

ideread:
	ld	a,idepwrt
	out	(idepctl),a
	ld	hl,buildtf
	ld	c,1
	ld	b,7
idebtf:	ld	a,(hl)
	call	putreg
	inc	hl
	inc	c
	djnz	idebtf
	ld	a,idepnrm
	out	(idepctl),a
	ld	de,0
idebw2:	call	rdstat
	and	08h			;DRQ
	jr	nz,idebrd
	dec	de
	ld	a,d
	or	e
	jr	nz,idebw2
	or	a			;the sector never came
	ret
idebrd:	ld	hl,(dest)
	ld	b,0			;256 words
ideblp:	ld	a,idecs0
	out	(idepc),a
	or	iderd
	out	(idepc),a
	in	a,(idepa)
	ld	(hl),a
	inc	hl
	in	a,(idepb)
	ld	(hl),a
	inc	hl
	djnz	ideblp
	xor	a
	out	(idepc),a
	scf
	ret

rdstat:	ld	a,idecs0|7		;register 7: status/command
	out	(idepc),a
	or	iderd
	out	(idepc),a
	in	a,(idepa)
	ret

putreg:	out	(idepa),a		;the byte onto the data lines
	ld	a,c
	or	idecs0
	out	(idepc),a		;register selected
	or	idewr
	out	(idepc),a		;/WR asserted
	ld	a,c
	or	idecs0
	out	(idepc),a		;/WR released
	ret

; The task file: register 1 through register 7

buildtf:
	db	0			;1 features
	db	1			;2 sector count
	db	0			;3 LBA 0-7
	db	0			;4 LBA 8-15
	db	0			;5 LBA 16-23
	db	0e0h			;6 drive/head LBA unit 0
	db	iderdc			;7 command read sector(s)

;****************************************************************
;* scsi: reset the bus and select target 0			*
;****************************************************************

ncrprobe:
	ld	a,icr_rst		;reset the bus
	out	(scsiicr),a
	xor	a
	out	(scsiicr),a
	ld	a,scsi_mon
	out	(scsimr),a
	ld	a,81h			;host id 7, target 0
	out	(scsidat),a
	ld	a,icr_data
	out	(scsiicr),a
	ld	a,icr_data+icr_sel
	out	(scsiicr),a
	ld	a,icr_data+icr_sel+icr_bsy
	out	(scsiicr),a
	ld	de,0
ncrprb:	in	a,(scsistat)
	and	scsi_bsy		;the target answered
	jr	nz,ncrpry
	dec	de
	ld	a,d
	or	e
	jr	nz,ncrprb
	or	a			;no target: carry clear
	ret
ncrpry:	scf
	ret

;****************************************************************
;* scsi: BSY is already set.  Finish the select, send READ(10),	*
;* and move the sector to (dest).				*
;****************************************************************

ncrread:
	ld	a,icr_sel+icr_bsy	;the id bits come off
	out	(scsiicr),a
	ld	a,icr_bsy		;then SEL
	out	(scsiicr),a
	xor	a			;then our BSY; the target keeps its own
	out	(scsiicr),a
	ld	hl,ncrcdb		;the ten-byte READ(10)
	ld	b,10
ncrloop: ld	a,(hl)
	call	ncrout
	inc	hl
	djnz	ncrloop
	ld	hl,(dest)
	ld	de,0200h		;512 bytes
ncrdata: call	ncrin
	ld	(hl),a
	inc	hl
	dec	de
	ld	a,d
	or	e
	jr	nz,ncrdata
	call	ncrin			;status byte (ignored)
	call	ncrin			;message byte (ignored)
	scf
	ret

; output a byte in A: wait REQ, drive, pulse ACK, wait REQ to drop
ncrout:	ld	c,a
ncrout0: in	a,(scsistat)
	and	scsi_req
	jr	z,ncrout0
	ld	a,c
	out	(scsidat),a
	ld	a,icr_data
	out	(scsiicr),a
	ld	a,icr_data+icr_ack
	out	(scsiicr),a
	ld	a,icr_data
	out	(scsiicr),a
ncrout1: in	a,(scsistat)
	and	scsi_req
	jr	nz,ncrout1
	ret

; input a byte into A: wait REQ, read, pulse ACK, wait REQ to drop
ncrin:	in	a,(scsistat)
	and	scsi_req
	jr	z,ncrin
	in	a,(scsidat)
	ld	c,a
	ld	a,icr_ack
	out	(scsiicr),a
	xor	a
	out	(scsiicr),a
ncrin1:	in	a,(scsistat)
	and	scsi_req
	jr	nz,ncrin1
	ld	a,c
	ret

; the READ(10) command block: opcode, then a zero LBA, one block
ncrcdb:	db	28h,0,0,0,0,0,0,1,0,0

;****************************************************************
;* console: print the 0-terminated string at hl			*
;****************************************************************

uputs:	ld	a,(hl)
	or	a
	ret	z
	ld	c,a
	call	uout
	inc	hl
	jr	uputs

; The character is already in C, which is what the driver wants, so this
; is a jump and not a call.  The driver destroys A and the flags - both
; already dead here - and nothing else, so no caller saves anything.
uout:	jp	condrv + csend

;****************************************************************
;* switch to the task in A, entering the address in DE, via a	*
;* stub written to on-board ram (the rom goes away on the task	*
;* register write).						*
;****************************************************************

switch:	ld	hl,gobuff
	ld	(hl),03eh		;ld a,imm
	inc	hl
	ld	(hl),a			;task
	inc	hl
	ld	(hl),032h		;ld (task),a
	inc	hl
	ld	(hl),task & 0ffh
	inc	hl
	ld	(hl),task >> 8
	inc	hl
	ld	(hl),00h		;6 nops
	inc	hl
	ld	(hl),00h
	inc	hl
	ld	(hl),00h
	inc	hl
	ld	(hl),00h
	inc	hl
	ld	(hl),00h
	inc	hl
	ld	(hl),00h
	inc	hl
	ld	(hl),0c3h		;jp
	inc	hl
	ld	(hl),e			;addr low
	inc	hl
	ld	(hl),d			;addr high
	ld	(ctask),a		;the trap handler reads this to know
					; the trap came from task 1, not 0
	jp	gobuff

jphl:	jp	(hl)			;through a table slot

;****************************************************************
;* Scratch, in this rom's own page.  The page is ram at physical	*
;* 0f000h and task 0 reaches it through segment 15, so it needs	*
;* no map games - and it is off the s-100 bus, unlike bus page	*
;* 0, which is where task 1's 100h lives and what the alias	*
;* window is for.						*
;****************************************************************

stg:	ds	512		;block 0, on its way to the label
	ds	64		;the stack the final transfer runs on
stktop:	ds	0
decbuf:	ds	8		;the candidate number, as text
numbuf:	ds	8		;a decimal number, at most five digits
hs:	ds	2		;heads * sectors per track, on the way
mulr:	ds	2		;the multiplier, shifting right
cur:	ds	2		;the table cursor
chead:	ds	2		;where the next candidate goes
ncand:	ds	1		;how many answered
cand:	ds	2 * 8		;their table rows, in order
dest:	ds	2		;where a read routine puts block 0
cname:	ds	2		;the row being walked: name, probe,
cprobe:	ds	2		; and read - one block, so loadrow
cread:	ds	2		; can copy the three with one ldir

;****************************************************************
;* strings							*
;****************************************************************

strset:	db	'level 1 boot',0dh,0ah,0
strnone: db	'no boot device',0dh,0ah,0
strgone: db	'boot disk stopped answering',0dh,0ah,0
strnl:	db	0dh,0ah,0
strcolon: db	': ',0
strcomma: db	', ',0
strsp:	db	' ',0
strmb:	db	' MB',0
strpick: db	'boot: ',0
dlmagic: db	'MWDL'			;DL_MAGIC
; The names are the ones /dev uses for these drives (dev/devlist), so
; the menu and the device nodes agree.  The digit is the unit, and each
; row above probes one of them.
cide:	db	'ide0',0
cscsi:	db	'scsi0',0

	; the image is copied down as a flat 4k, so it must fit
	assert	$ <= 0F000h + 4096

	end
