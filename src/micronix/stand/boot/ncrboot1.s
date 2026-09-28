;
; ncrboot1 - the first level hard disk boot for the NCR 5380 SCSI card
;
; micronix/stand/boot/ncrboot1.s
;
; The monitor's ncrboot reads one sector - LBA 0, the drive's first -
; to 0100 and enters task 1 there.  That sector is this, and its job is
; to bring in the sectors after it: the second level, which knows the v6
; filesystem and finds a kernel.
;
; mkfs -i puts both of us in a file that owns cylinder 0, so this is
; block 0 of that file and the second level is block 1 onward.  Nothing
; else on the disk is involved and no filesystem is read here - that is
; the second level's job, and this exists only because the rom will read
; one sector and no more.
;
;
; the same loader as ideboot1, with a different read
; -------------------------------------------------
;
; Everything except the read is ideboot1.s: the relocation to RELOC, the
; stack, the unsigned object header, the bss clear, the entry, and the
; complaint.  The two files are meant to be read side by side, and the
; places they agree are the places neither controller has an opinion.
;
; What differs is the read.  The IDE card's registers are behind an 8255
; and every sector there loads a task file; the 5380 is driven one byte
; at a time through a REQ/ACK handshake, and every sector here is its own
; command - select the target, send a ten byte READ(10), take 512 bytes
; of data, take the status and message, and the bus goes free again.
; The byte engine (ncrout, ncrin) and the selection (ncrsel) are the
; rom's ncrboot subroutines, which were tested before this file existed;
; what this adds is the loop over sectors 1 through NSEC.
;
; A target is addressed by LBA - the whole disk as one run of 512-byte
; sectors numbered from 0 - so, like the IDE card, it needs no geometry
; and this loader has none.  The sectors are 1 through NSEC because this
; one is 0 and the rom has already read it.
;
; It is a loader, not a sector copier, for the reason ideboot1.s gives:
; the image on the disk is the linker's output unmodified, with its
; header, and this reads the header and does what it says.  Low memory is
; empty at this point, so the sectors are read to STAGE first and
; unpacked from there.
;
; The code moves itself to RELOC before loading anything, for the same
; reason: so what it loads may land wherever the header says.  After the
; move it uses jr for every branch of its own and adds RBIAS to every
; absolute address it builds.
;
; There is no sign-on.  The rom's ncrboot has already selected the
; console and set the line before it read this sector, so the uart below
; is ready, and the second level prints its own banner; the space a
; sign-on would take is the one byte this file cannot spare.
;

;
; The card's ports, at 40h through 47h.  The register number is the low
; three bits, straight into the chip; the four that matter are the data
; register, the initiator command register, the mode register and the
; current bus status.  The bits are the rom's ncrboot's, and the wire
; they drive is sys/ncr.c's.
;
scsidat	equ	40h		;data register (CSD), read and write
scsiicr	equ	41h		;initiator command: RST ACK BSY SEL DATA
scsistat equ	44h		;current bus status: BSY REQ

icr_ack	equ	10h
icr_bsy	equ	08h
icr_sel	equ	04h
icr_data equ	01h

scsi_bsy equ	40h
scsi_req equ	20h

STAGE	equ	08000h		; where the sectors land before relocation
NSEC	equ	8		; 4k of it, which stand/README asks it to fit
SECLEN	equ	512
CDBLEN	equ	10

BASE	equ	0100h		; where the rom reads us, and where we link
RELOC	equ	0c000h		; out of the way of what we are loading
RBIAS	equ	RELOC-BASE	; add to one of our addresses for its copy

;
; The MULTIO's first uart, for complaining with.  Output only, and no
; setup: the monitor has had this port since reset, so the group is
; selected and the line is right.
;
TXB	equ	048h		; transmit holding register
LSR	equ	04dh		; line status
LSRTXE	equ	020h		; holding register empty

;
; Whitesmith's object header, from include/obj.h.  Sixteen bytes, and
; the file is text then data straight after it.
;
OBJECT	equ	099h		; ident: Whitesmith's standard
HDRLEN	equ	16
O_IDENT	equ	0
O_TEXT	equ	4		; text segment length
O_DATA	equ	6		; data segment length
O_BSS	equ	8		; bss length, which is not in the file
O_TOFF	equ	12		; where the text wants to be
O_DOFF	equ	14		; and the data

	.text

start:
	ld	hl,BASE
	ld	de,RELOC
	ld	bc,finish-BASE
	ldir
	jp	run+RBIAS

;
; Read sectors 1 through NSEC into STAGE upward, each its own command.
;
run:
;
; A stack of our own, at the top of memory.  The second level inherits it
; while it loads the kernel, and the kernel is about to fill everything
; from 0100 up.
;
	ld	sp,0		; ie 10000h - first push lands at fffe

;
; hl is the destination and stays it: the data loop below leaves it
; exactly one sector on, which is where the next one goes.
;
	ld	hl,STAGE

sector:
	call	ncrsel+RBIAS

;
; The command phase: ten bytes.  hl is the destination, so it is set
; aside while hl walks the command block.  The block's fourth byte is
; the LBA, patched per sector like ideboot1's task file byte.
;
	push	hl
	ld	hl,ncrcdb+RBIAS
	ld	b,CDBLEN
cdbloop:
	ld	a,(hl)
	call	ncrout+RBIAS
	inc	hl
	djnz	cdbloop
	pop	hl

;
; The data phase: SECLEN bytes to hl, one handshake each.  A refused
; command still runs its data phase - see sys/ncr.c - so the status byte
; that reports the refusal is read after the data either way.
;
	ld	de,SECLEN
dataloop:
	call	ncrin+RBIAS
	ld	(hl),a
	inc	hl
	dec	de
	ld	a,d
	or	e
	jr	nz,dataloop

	call	ncrin+RBIAS	;the status byte (ignored)
	call	ncrin+RBIAS	;the message byte (ignored)

;
; The next sector, and the LBA with it.  The byte counting the sectors
; and the byte naming the next one are the same byte: lba starts at 1,
; one past it is 2, and after NSEC sectors it holds 1+NSEC.
;
	ld	a,(ncrlba+RBIAS)
	inc	a
	ld	(ncrlba+RBIAS),a
	cp	1+NSEC
	jr	nz,sector

;
; Unpack what arrived.  Refuse anything without the right ident rather
; than jumping into it.
;
	ld	a,(STAGE+O_IDENT)
	cp	OBJECT
	jr	nz,badmag

	ld	hl,STAGE+HDRLEN	; text, straight after the header
	ld	de,(STAGE+O_TOFF)
	ld	bc,(STAGE+O_TEXT)
	ldir			; hl is left at the data, in the file

	ld	de,(STAGE+O_DOFF)
	ld	bc,(STAGE+O_DATA)
	ldir			; de is left just past it, in memory

;
; And clear bss, which the header counts and the file does not contain.
;
	ld	bc,(STAGE+O_BSS)
	ld	a,b
	or	c
	jr	z,enter		; none of it
	ld	h,d
	ld	l,e		; hl = de, the usual fill
	inc	de
	dec	bc
	ld	(hl),0
	ld	a,b
	or	c
	jr	z,enter		; only the one byte
	ldir

enter:
	ld	hl,(STAGE+O_TOFF)
	jp	(hl)

;
; What we read is not a program.  Print and stop; puts below does not
; return, so there is no call and no return address to leave behind.
;
badmag:
	ld	hl,mmagic+RBIAS
	jr	puts

;
; Output only, and no setup.  Wait for the holding register, write a
; byte, repeat until the nul, then stop for good.
;
puts:
	ld	a,(hl)
	or	a
	jr	z,hang
	ld	c,a
	inc	hl
pwait:
	in	a,(LSR)
	and	LSRTXE
	jr	z,pwait
	ld	a,c
	out	(TXB),a
	jr	puts
hang:
	jr	hang

;
; Call target 0 and wait for it to answer.  No arbitration, one
; initiator: the id bits go on the data lines, SEL says a selection is
; happening, BSY claims the bus, and the target answers by raising BSY of
; its own, which is when the id bits come off.
;
; The bus has to be free first, and that is the first read below rather
; than a poll.  A command ends when the message byte's ACK lets the
; target drop BSY, and the read that sees it is the next one after the
; read that saw REQ low - so the bus is not free until one more status
; read, and one is all it takes.  The rom's ncrboot leaves the same
; one-read debt, and this pays it.
;
ncrsel:
	in	a,(scsistat)		; the one read that frees the bus
	ld	a,81h			; host id 7 and target 0 on the data lines
	out	(scsidat),a
	ld	a,icr_data
	out	(scsiicr),a
	ld	a,icr_data+icr_sel
	out	(scsiicr),a
	ld	a,icr_data+icr_sel+icr_bsy
	out	(scsiicr),a
	ld	b,0
ncrwait: in	a,(scsistat)		; wait for the target to answer
	and	scsi_bsy
	jr	nz,ncrsel1
	djnz	ncrwait
ncrfail: jr	hang			; nobody is home
ncrsel1: ld	a,icr_sel+icr_bsy	; the id bits come off
	out	(scsiicr),a
	ld	a,icr_bsy		; then SEL
	out	(scsiicr),a
	xor	a			; then our BSY; the target keeps its own
	out	(scsiicr),a
	ret

;
; One byte out, one REQ/ACK round trip: wait for REQ, drive the byte,
; pulse ACK, wait for REQ to drop.  The byte rides in c while a is the
; command register's.
;
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

;
; One byte in, the same round trip read side up: wait for REQ, read the
; byte, pulse ACK, wait for REQ to drop, hand the byte back in a.
;
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

;
; The READ(10) command block: opcode, a zero high LBA, and the byte
; patched in place as it is walked.  ncrlba is the LBA's low byte and
; starts at 1 because sector 0 is this code; the block count is one.
;
ncrcdb:	.db	28h,0,0
ncrlba:	.db	1,0,0,0,1,0,0

mmagic:	.db	"mag",0

finish:
