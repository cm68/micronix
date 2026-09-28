;
; djboot1 - levels 0 and 1 of the floppy boot for the DJ/DMA
;
; micronix/stand/boot/djboot1.s
;
; Both levels, in one file, because on this controller they are one
; stage: the rom hands the DJ-DMA off to its own firmware, and what the
; firmware leaves in memory is 128 bytes - not enough for a level 1, and
; nothing else arrives.  So the first 128 bytes here are a program whose
; whole job is to fetch and enter the rest of this file, and the rest of
; this file is mwboot1.s: move ourselves out of the way, read the second
; level to STAGE, unpack it where its object header says it goes.
;
;	level 0	the first 128 bytes, at 0080, entered by the firmware
;	level 1	the rest, at 0100, entered by level 0's jp
;	level 2	djboot.com, read to STAGE by level 1 and unpacked
;
; The other three ways in have no level 0 of their own, because they do
; not need one: boothd, nuboot and ideboot each put a whole 512 byte
; sector at 0100 and jump, so the rom is the level 0.  This is the only
; monitor that cannot, and the 128 bytes is why.
;
; Level 0's 128 bytes are exactly what the firmware copies.  hwsim/d1's
; djdma_init shows it plainly - imd_read of track 0's first sector, then
; copyout(secbuf, 0x80, 0x80) - and DJ/DMA manual section 7 says the
; same ("80 hex bytes are loaded from the first sector on track 0 of the
; disk, and the cpu is sent to 000080").  So level 0 is what the medium
; calls its first sector and the body goes in the one after it, which is
; where mkbootimg -d lays them - level 0's first 128 bytes, and the rest
; of this file in the next sector.
;
; Level 0 prints nothing.  A diagnostic would have to set the uart up,
; and what the monitor still guarantees at this point is not known; the
; signon below arrives a moment later anyway, and 128 bytes is not the
; place to find out.
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
; assembled by asz, linked at BASE0 and unwrapped, exactly as mwboot1.s
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

BASE0	equ	0080h		; where the firmware leaves level 0
HEADLEN	equ	128		; and how much of it it leaves

;
; Where the medium numbers its sectors from - the last two bytes of
; level 0, which mkbootimg -d writes.  It is the one thing this file
; cannot work out for itself: the label that carries it is at byte 256
; of the sector the firmware copied only the first HEADLEN of, and the
; number is not a property of the geometry.  A soft sectored medium,
; which is what both eight inch diskettes are, numbers its sectors from
; one; a hard sectored one, which is Morrow's five inch format, from
; zero.  Read it rather than assume it and everything after follows:
; level 1 is one sector on from the medium's first, and level 2 one
; further still.  See sys/dlabel.h's d_firstsec and sys/dj.c, where the
; kernel takes the same answer from the drive itself (ORG1).
;
; The marker beside it is mkbootimg's: it writes the number only where
; it finds the marker intact, so a level 0 that outgrew HEADLEN and
; moved both fails the build rather than having a byte of its own code
; overwritten.
;
FSEC	equ	BASE0+126	; 00feh, the first of level 0's last two bytes
FSECMAG	equ	0a5h		; the marker after it that says so

BASE	equ	0100h		; where level 0 reads level 1, and where it links
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

;
; Level 0.  The firmware has left the first HEADLEN bytes of this file
; at BASE0 and jumped here.  All it does is ask the channel for level 1
; - the rest of this file, in the sector after the medium's first - and
; jump into it.  No stack, no output, no per-read loop: level 1 has all
; three, and there is only one read to make.
;
; The channel program's invariant half goes down first, exactly as it
; does in level 1's read loop (which is where these port writes are
; explained).  Level 0 differs from that loop in two ways: the DMA
; address is the constant BASE and the sector number is the one FSEC
; holds plus one, and a failure has no end.  Level 1 counts ten tries
; and then prints; level 0 kicks the channel again, forever, because a
; boot that cannot read its own second half has nothing left to try and
; nowhere to say so - setting the uart up would cost bytes this does
; not have.
;
level0:
	ld	a,SETDMA
	ld	(CHAN),a
	xor	a
	ld	(CHAN+3),a	; dma high byte
	ld	(CHAN+5),a	; cylinder 0
	ld	(CHAN+7),a	; drive 0
	ld	a,SREAD
	ld	(CHAN+4),a
	ld	a,HALT
	ld	(CHAN+9),a

	ld	hl,BASE		; level 1 goes where its labels expect
	ld	a,l
	ld	(CHAN+1),a	; dma low
	ld	a,h
	ld	(CHAN+2),a	; dma high
	ld	a,(FSEC)	; the medium's first sector number
	inc	a		; level 1 is the sector after it
	ld	(CHAN+6),a	; so read that one

l0retry:
	xor	a
	ld	(CHAN+8),a	; say we are waiting for it
	out	(ATTN),a	; run the channel
l0poll:
	ld	a,(CHAN+8)	; the read's status byte
	or	a
	jr	z,l0poll	; not written yet
	cp	OKSTAT
	jr	nz,l0retry	; 040h, so it worked
	jp	level1

;
; The pad that makes level 0 exactly HEADLEN bytes, and the two bytes at
; the end of it that are not padding at all: the first sector number
; FSEC names and its marker.  The length is the one invariant in the
; file: level 1's labels are the linker's, counted from BASE0, so they
; only come out at BASE - where level 0 puts its block - if level 0 is
; exactly the length the firmware copies and no more.
;
; The head above assembles to 61 bytes - the instruction count comes to
; 62, but the assembler shortens the trailing jp to a jr, it being in
; range - so the pad is 65 and the two bytes after it end it at 128.
; The Makefile checks that byte HEADLEN of djboot1.bin is level 1's
; first opcode, so a miscount here fails the build rather than quietly
; running level 1 at the wrong address - and mkbootimg, which writes the
; number, refuses the image if the marker has moved, so a miscount
; cannot have the number land on an instruction either.
;
	.ds	65
	.db	0		; the first sector number, for mkbootimg
	.db	FSECMAG		; and the marker saying it may write there

;
; Level 1.  Level 0 left us at BASE; move the whole of ourselves up to
; RELOC, out of the way of what we are about to load, and carry on
; there.  Every address from run down is used with RBIAS added, which is
; why this copy and this jump are the last unrelocated ones in the file.
;
level1:
	ld	hl,BASE
	ld	de,RELOC
	ld	bc,finish-BASE
	ldir
	jp	run+RBIAS

;
; Read NSEC sectors of this track, beginning two on from the medium's
; first, into STAGE upward.  Level 0 was the medium's first sector and
; this code the one after it, so the second level has the eight behind
; that.  Nothing here counts to the end of the track: the eight are all
; the second level needs, and a track holds fifteen sectors on the
; eight inch media and ten on the five inch one.
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
	ld	a,(FSEC)	; the medium's first sector number
	inc	a		; level 1 was the sector after it
	inc	a		; so level 2 starts one after that
	ld	c,a		; and this read wants that one

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
