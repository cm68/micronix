;
; djboot1 - the floppy's level 2 sector, for the DJ/DMA
;
; micronix/stand/boot/djboot1.s
;
; One sector, linked at BASE and entered there by the level 1 rom
; (stand/roms/l1boot.s), which read the diskette's block 0 - this code
; and the label behind it - into the window and ran the controller's
; channel program itself.
;
;	level 2	this sector, the front half of the medium's block 0
;	level 3	djboot.com, at cylinder 1, read by this and unpacked
;
; The card's own firmware used to read the floppy in two pieces.  A
; bootstrap - manual section 7 - halted the cpu, planted a handshake
; routine at 0038 and, when the cpu ran it, copied the first 128 bytes
; of track 0 to 0080 and sent the cpu there.  That is switched off: the
; card is strapped for non-bootstrap operation (J2 A to B) and the
; level 1 rom is the external means that boots it.  So nothing is
; planted in low memory, nothing takes the bus away from the cpu, and
; the floppy's block 0 is a boot block like every other medium's.
;
; The label is in the half of this sector behind the code, because the
; rom read a whole 512 byte block: struct dlabel, include/sys/dlabel.h.
; It says where the medium's filesystem begins, and the two numbers
; below say how to read the rest of it.  Nothing else on a diskette
; describes the medium at all.
;
; Block 0 is four sectors on an eight inch drive and one on a five inch
; one.  Cylinder 0 head 0 of an eight inch diskette is single density -
; fm, 26 sectors of 128 bytes, numbered from one - and the block is its
; first four; every cylinder above it is double density with 512 byte
; sectors.  So the rom reads that block as four reads and hands over
; 512 bytes, and this level neither knows nor cares which way it came.
;
; Level 3 is a cylinder up for that same reason.  On an eight inch
; drive the rest of cylinder 0 is 128 byte sectors, which is no place
; for a four kilobyte program, and every medium's boot area owns
; cylinders 0 and 1 anyway - the label's toff is 2 on all of them.  So
; mkbootimg lays an image out as block 0, filled to the end of cylinder
; 0, and then level 3: one rule for every medium, and the first sector
; of the second cylinder in all of them.
;
; The DJ-DMA is not the HD-DMA.  The HD-DMA takes a sector, a DMA
; address and an opcode in a command block at 0080 and the host polls a
; status byte; the DJ-DMA has its own z80 - the firmware is DJDMA25 -
; that runs a channel program out of the host's memory.  The host
; writes a SETDMA / SREAD / HALT command sequence at the default
; channel address (0050), pulses the attention port (00ef) and polls
; the status byte the controller writes back.  This one does not write
; that sequence; the rom's is still there, one read away from what this
; needs, and the drive byte in it is the only record of which drive
; this diskette came out of.  See run.
;
; assembled by asz, linked at BASE and unwrapped, exactly as mwboot1.s
; is - see the makefile.
;

CHAN	equ	0050h		; the default channel command address
ATTN	equ	00efh		; pulse to start the channel
OKSTAT	equ	040h		; the good status a command writes back

STAGE	equ	08000h		; where the sectors land before relocation
TRIES	equ	10		; per sector

BASE	equ	0100h		; where the level 1 rom reads this block
RELOC	equ	0c000h		; out of the way of what we are loading
RBIAS	equ	RELOC-BASE	; add to one of our addresses for its copy

;
; The numbers that are the medium's and not ours, out of the label in the
; second half of this very sector (include/sys/dlabel.h).  The level 1 rom
; read the whole block here - 512 bytes beginning at BASE, which is this
; code and then the label - so the label needs no read of its own.  It is
; the only place they can come from: where a medium numbers its sectors
; from is not derivable from the geometry, and neither is how many blocks
; a sector is from the block count alone.
;
; firstsec is one on the soft sectored media - both eight inch diskettes
; and one of the five inch ones - and zero on the hard sectored five inch
; medium.  It is also the first sector of every track, so it is the one
; level 3 begins at.  A drive reports it (sys/dj.c's ORG1); the loader
; has no drive to ask, so the medium says it.  d_spt is blocks (512
; bytes) per track, and the two together are how djio.c works out the
; sector size - this level's rule for it below is the same one.
;
DL_OFF	equ	BASE+256	; DL_OFFSET into the sector: the label
DL_SPT	equ	DL_OFF+10	; d_spt, blocks per track
DL_1ST	equ	DL_OFF+12	; d_firstsec, where its sectors are numbered from

;
; The MULTIO's first uart, for complaining with.  Output only, and no
; setup: the monitor has been talking to this port since reset.  The
; same ports and the same group select as mwboot1.s.
;
GSEL	equ	04fh		; group select
GRP1	equ	1		; the serial ports
TXB	equ	048h		; transmit holding register
LSR	equ	04dh		; line status
LSRTXE	equ	020h		; holding register empty
S1DATA	equ	07ch		; the SS1 2651's data
S1STAT	equ	07dh		; its status
S1TXRDY	equ	001h		; transmitter will take a byte

;
; Whitesmith's object header, from include/obj.h.  Sixteen bytes, text
; then data straight after it.
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

;
; Level 2.  The rom left us at BASE; move the whole of ourselves up to
; RELOC, out of the way of what we are about to load, and carry on
; there.  Every address from run down is used with RBIAS added, which is
; why this copy and this jump are the last unrelocated ones in the file.
;
level2:
	ld	a,c			;the console id the rom passed down
	ld	hl,BASE
	ld	de,RELOC
	ld	bc,finish-BASE
	ldir
	ld	(conid+RBIAS),a		;remember it in the relocated copy
	jp	run+RBIAS

;
; Read the level 3 program into STAGE upward, from the first sector of
; cylinder 1.  Four kilobytes of it, which stand/README asks it to fit -
; eight 512 byte sectors or four 1024 byte ones, so the count and the
; stride both come from the medium's blocks per sector, and the sector
; read count is what makes the two agree: eight blocks and four sectors
; are the same four kilobytes of medium.
;
run:
;
; A stack of our own, at the top of memory for the same reason mwboot1.s
; puts it there: the level 3 program inherits it while it loads the
; kernel, and the kernel fills everything from 0ff0 up.
;
	ld	sp,0		; ie 10000h - first push lands at fffe

	ld	hl,msign+RBIAS	; puts selects the console's port itself
	call	puts+RBIAS

;
; Hand the block on.  What the rom read into BASE is this sector and the
; label behind it, and this level is about to be written over: level 3 is
; linked at BASE and unpacks its text there.  So the whole block goes up
; to 1000h first, which is boot.c's buf0 - free until level 3 begins, and
; where its driver wants the label anyway.
;
; It saves level 3 from having to ask the medium for the block's label
; again, which is not a block it can depend on the shape of: on an eight
; inch drive block 0 is four 128 byte sectors of an fm track rather than
; one sector, and the rom has already worked that out, read it, found the
; magic in it and booted the drive it came from.  Nobody needs to do it
; twice, and level 3 has no room for the second way of doing it.  See
; djio.c's reset().
;
	ld	hl,BASE		; the block the rom read: this sector,
	ld	de,1000h	; and the label behind it, to boot.c's buf0
	ld	bc,512
	ldir

;
; How many blocks a sector is, from the label.  The rule is djio.c's - a
; soft sectored medium with ten or sixteen blocks to a track is 1024
; bytes, and every other medium this tree builds for is 512 - and it has
; to be the same rule here, because the count and the stride below are
; what make the two agree about where a block ends up.  D is the answer,
; less one: 0 for 512, 1 for 1024.
;
	ld	d,0		; 512 bytes: one block to a sector
	ld	a,(DL_SPT)
	cp	10
	jr	z,is1k
	cp	16
	jr	nz,sized
is1k:	ld	a,(DL_1ST)	; ... and only if it numbers from one
	dec	a
	jr	nz,sized
	ld	d,1		; 1024 bytes: two blocks to a sector

sized:
;
; The channel program.  The rom left it at CHAN, with the drive it read
; us from in it, and it is one read away from what this level needs:
; only the dma address and the sector number change from one read to the
; next, and only the cylinder changes once.  So there is no program to
; write here - this patches the rom's.
;
;	0050	SETDMA
;	0051	<dma low>	(patched per sector)
;	0052	<dma high>	(patched per sector)
;	0053	00		(the bank above: a flat address)
;	0054	SREAD
;	0055	01		(patched, once: cylinder 1)
;	0056	<sector>	(patched per sector)
;	0057	<drive>		(the rom's: the drive we came from)
;	0058	<status>	(written by the controller)
;	0059	HALT
;	005a	<status>	(written by the controller)
;
; The drive byte is the one byte that must not be touched.  It is how
; this level knows which drive the diskette came out of, with nothing
; having had to tell it - the rom asked all eight drives and this is the
; one that answered - and the level 3 driver reads it the same way, out
; of the same program, since nothing here overwrites it and nothing
; there writes it either.  See stand/roms/l1boot.s's DJ row.
;
	ld	a,1
	ld	(CHAN+5),a	; cylinder 1, the low one level 3 is on

	ld	hl,STAGE	; where the next sector goes
	ld	a,(DL_1ST)	; the first sector of a track, which is
	ld	c,a		; where mkbootimg put level 3
	ld	a,d
	or	a		; 0 for 512
	jr	nz,is1kb
	ld	b,8		; 512: eight sectors, 4K
	jr	onesec
is1kb:
	ld	b,4		; 1024: four sectors, 4K

onesec:
	ld	(CHAN+1),hl	; dma low, then high
	ld	a,c
	ld	(CHAN+6),a	; sector
	ld	e,TRIES

try:
	xor	a
	ld	(CHAN+8),a	; say we are waiting for it
	out	(ATTN),a	; run the channel
poll:
	ld	a,(CHAN+8)	; the read's status byte
	or	a
	jr	z,poll		; not written yet
	cp	OKSTAT
	jr	z,good		; 040h, so it worked
	dec	e
	jr	nz,try

;
; Out of retries.  Say so and stop.
;
stuck:
	ld	hl,mread+RBIAS
	jr	moan

good:
	inc	h		; on by 512, and l stays 0
	inc	h
	ld	a,d
	or	a		; 0 for 512
	jr	z,noadd
	inc	h		; 1024: on by another 512
	inc	h
noadd:
	inc	c
	djnz	onesec

;
; Unpack what arrived.  Refuse anything without the right ident rather
; than jumping into it, exactly as mwboot1.s does.
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
	ld	a,(conid+RBIAS)	;the console id, handed on in C
	ld	c,a
	jp	(hl)

;
; What we read is not a program.
;
badmag:
	ld	hl,mmagic+RBIAS

;
; Complain and stop.
;
moan:
	call	puts+RBIAS
hang:
	jr	hang

;
; Output only, and no setup: the monitor has had this port since reset.
;
puts:
	ld	a,(hl)
	or	a
	ret	z
	ld	c,a
	inc	hl
	ld	a,(conid+RBIAS)	;which console the monitor found
	or	a
	jr	nz,pss1		;the SS1's 2651
	ld	a,GRP1		;the MultIO: select its uart
	out	(GSEL),a
pwait:
	in	a,(LSR)
	and	LSRTXE
	jr	z,pwait
	ld	a,c
	out	(TXB),a
	jr	puts
pss1:
	in	a,(S1STAT)		;the 2651: wait until it will take a byte
	and	S1TXRDY
	jr	z,pss1
	ld	a,c
	out	(S1DATA),a
	jr	puts

conid:	.db	0		;the console the monitor found: 0 MultIO, 1 SS1

msign:	.db	"djboot:",13,10,0
mread:	.db	"read",13,10,0
mmagic:	.db	"magic",13,10,0

finish:
