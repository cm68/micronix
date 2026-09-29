;****************************************************************
;*								*
;* 4k selector boot rom.  Lives at physical 0ff0000h, is copied	*
;* down to 0f000h by the monitor (see mon500.s copyrom), and	*
;* runs in task 0.						*
;*								*
;* Arms both the IDE board and the NCR 5380 SCSI at once and	*
;* lets them race: the first drive to answer ready wins, and	*
;* its sector 0 is read into task 1's 100h and entered.  Chatty,	*
;* so a dead boot names the device it could not find.		*
;*								*
;****************************************************************

	org	0F000h

;****************************************************************
;* Wunderbus console equates (the monitor already init'd it)	*
;****************************************************************

base	equ	048h
grpctl	equ	base+7		;I/O group select port
group1	equ	09h		;serial port 1
lcr	equ	base+3		;line control register
lsr	equ	base+5		;line status register
thr	equ	base		;transmitter data buffer
thre	equ	20h		;status line TBE
wls0	equ	01		;word length select bit 0
wls1	equ	02		;word length select bit 1
stb	equ	04		;stop bit count (2 stop bits)

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

scsidat	equ	40h		;data register (CSD), read and write
scsiicr	equ	41h		;initiator command register
scsimr	equ	42h		;mode register
scsistat equ	44h		;current bus status
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
gobuff	equ	01b0h		;ram stub for the task switch

;****************************************************************
;* entry: arm both controllers, then let them race		*
;****************************************************************

entry:
	ld	hl,strset	;"probing ide+scsi"
	call	uputs
	call	idesetup	;reset the ide drive
	call	scsisetup	;reset the bus and select target 0
	ld	de,0		;race timeout
race:	call	rdstat		;ide status
	and	40h		;DRDY
	jr	nz,idewin
	in	a,(scsistat)	;scsi status
	and	scsi_bsy	;target answered
	jr	nz,scsiwin
	dec	de
	ld	a,d
	or	e
	jr	nz,race
	jr	no_boot
idewin:	ld	hl,stride	;"ide"
	call	uputs
	call	ideboot		;ret only on a late failure
	jr	no_boot
scsiwin: ld	hl,strscsi	;"scsi"
	call	uputs
	call	ncrboot		;reads the sector and enters it
no_boot: ld	hl,strfail	;"no boot device"
	call	uputs
hang:	halt
	jr	hang

;****************************************************************
;* ide reset: bring the drive up so it can answer DRDY		*
;****************************************************************

idesetup:
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
	ret

;****************************************************************
;* ide boot: DRDY is already set.  Load the task file, wait for	*
;* the sector, read it into task 1's 100h, and enter it.		*
;****************************************************************

ideboot:
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
	ret				;sector never came
idebrd:	xor	a
	ld	(mapram + 2),a		;task 0 seg 1 -> physical page 0
	ld	hl,01100h		;= 100h to task 1
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
	ld	de,0100h		;the sector just read
	ld	a,01h			;run it as task 1
	jp	switch

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
;* scsi reset + select: arm the bus so target 0 can answer BSY	*
;****************************************************************

scsisetup:
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
	ret

;****************************************************************
;* scsi boot: BSY is already set.  Send READ(10), read sector 0	*
;* into task 1's 100h, and enter it.				*
;****************************************************************

ncrboot:
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
	xor	a			;task 0 seg 1 -> physical page 0
	ld	(mapram + 2),a
	ld	hl,01100h		;= 100h to task 1
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
	ld	de,0100h
	ld	a,1
	jp	switch

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

uout:	ld	a,group1
	out	(grpctl),a
	ld	a,wls0+wls1+stb
	out	(lcr),a
uout1:	in	a,(lsr)
	and	thre
	jr	z,uout1
	ld	a,c
	out	(thr),a
	ret

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
	ld	(hl),02h
	inc	hl
	ld	(hl),04h
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
	jp	gobuff

;****************************************************************
;* strings							*
;****************************************************************

strset:	db	'probing ide+scsi',0dh,0ah,0
stride:	db	'ide',0dh,0ah,0
strscsi: db	'scsi',0dh,0ah,0
strfail: db	'no boot device',0dh,0ah,0

	end
