;
; djboot1 - the first level floppy boot for the DJ/DMA
;
; micronix/stand/boot/djboot1.s
;
; The monitor's DJ-DMA boot reads one sector to 0100 and enters task 1
; there.  That sector is this, and its job is to bring in the rest of
; the second level from the floppy - the same second level (mwboot.com)
; the hard disk boot loads, since it is that which reads the kernel off
; the hard disk.  This is the floppy half of the pair, and it differs
; from mwboot1.s only in how the sectors are read.
;
; The DJ-DMA is not the HD-DMA.  The HD-DMA takes a sector, a DMA
; address and an opcode in a command block at 0080 and the host polls a
; status byte; the DJ-DMA has its own z80 - the firmware is DJDMA25 -
; that runs a channel program out of the host's memory.  The host writes
; a SETDMA / SREAD / HALT command sequence at the default channel
; address (0050), pulses the attention port (00ef) and polls the status
; byte the controller writes back.  The channel program is fixed; only
; the DMA address and the sector number change per read.
;
; Everything else is mwboot1.s: move ourselves out of the way, read the
; second level to STAGE, then unpack it where its object header says it
; goes.
;
; assembled by asz, linked at BASE, and unwrapped, exactly as mwboot1.s
; is - see the Makefile.
;

CHAN	equ	0050h		; the default channel command address
ATTN	equ	00efh		; pulse to start the channel
SETDMA	equ	023h		; set the 24 bit DMA address, 4 bytes
SREAD	equ	020h		; read one sector, 5 bytes
HALT	equ	025h		; end the channel program, 2 bytes
OKSTAT	equ	040h		; the good status a command writes back

STAGE	equ	08000h		; where the sectors land before relocation
NSEC	equ	8		; 4k of it, which stand/README asks it to fit
TRIES	equ	10		; per sector

BASE	equ	0100h		; where the rom reads us, and where we link
RELOC	equ	0c000h		; out of the way of what we are loading
RBIAS	equ	RELOC-BASE	; add to one of our addresses for its copy

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

start:
	ld	hl,BASE
	ld	de,RELOC
	ld	bc,finish-BASE
	ldir
	jp	run+RBIAS

;
; Read sectors 1 through NSEC of this track into STAGE upward.  Sector 0
; is this code, so the second level starts at 1, and a track holds 10
; sectors - the whole of the second level is on the one track.
;
run:
;
; A stack of our own, at the top of memory for the same reason mwboot1.s
; puts it there: the second level inherits it while it loads the kernel,
; and the kernel fills everything from 0ff0 up.
;
	ld	sp,0		; ie 10000h - first push lands at fffe

	ld	a,GRP1		; the eight ports at 0048 are the uart now
	out	(GSEL),a
	ld	hl,msign+RBIAS
	call	puts+RBIAS

;
; The channel program is fixed but for the DMA address and the sector
; number.  Write the invariant part once:
;
;	0050	SETDMA
;	0051	<dma low>	(set per sector)
;	0052	<dma high>	(set per sector)
;	0053	00		(dma high byte, always 0)
;	0054	SREAD
;	0055	00		(cylinder 0)
;	0056	<sector>	(set per sector)
;	0057	00		(drive 0)
;	0058	<status>	(written by the controller)
;	0059	HALT
;	005a	<status>	(written by the controller)
;
	ld	a,SETDMA
	ld	(CHAN),a
	xor	a
	ld	(CHAN+3),a	; dma high byte
	ld	a,SREAD
	ld	(CHAN+4),a
	xor	a
	ld	(CHAN+5),a	; cylinder
	ld	(CHAN+7),a	; drive
	ld	a,HALT
	ld	(CHAN+9),a

	ld	hl,STAGE	; where the next sector goes
	ld	b,NSEC		; how many are left
	ld	c,1		; and which one this is

onesec:
	ld	a,l
	ld	(CHAN+1),a	; dma low
	ld	a,h
	ld	(CHAN+2),a	; dma high
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
pwait:
	in	a,(LSR)
	and	LSRTXE
	jr	z,pwait
	ld	a,c
	out	(TXB),a
	jr	puts

msign:	.db	"djboot:",13,10,0
mread:	.db	"read",13,10,0
mmagic:	.db	"magic",13,10,0

finish:
