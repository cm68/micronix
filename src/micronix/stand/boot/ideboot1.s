;
; ideboot1 - the first level hard disk boot for the IDE card
;
; micronix/stand/boot/ideboot1.s
;
; The monitor's ideboot reads one sector - LBA 0, the drive's first -
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
; the same loader as mwboot1, with a different read
; ------------------------------------------------
;
; Everything except the read is mwboot1.s: the relocation to RELOC, the
; stack, the unsigned object header, the bss clear, the entry, and the
; complaints.  The two files are meant to be read side by side, and the
; places they agree are the places neither controller has an opinion.
;
; What differs is that this one cannot assume a primed controller.
; mwboot1 inherits everything: the rom's nuboot has just read this very
; sector through the HD-DMA's command block at 0080, so the drive is
; selected, the head is 0 and the heads are at cylinder 0, and a read is
; three stores into that block.  The IDE card has no such block - its
; registers are behind the 8255 - and there is no state worth inheriting
; past "unit 0", so every sector here loads the whole task file and
; issues its own command.  That is most of the extra size.
;
; The card, in the terms the rom's driver uses.  The MYIDE board puts an
; 8255 PPI between the S-100 bus and an ATA drive's pins, so the host
; cannot see the drive's registers directly.  Port C carries the register
; number on its low three bits and the strobes above them; port A carries
; the byte; port B is the high half of a 16-bit move.  The drive is
; addressed by LBA - the whole disk as one run of 512-byte sectors
; numbered from 0 - so it needs no geometry and this loader has none.
;
; The transfer is a word at a time because that is what the card moves:
; ports A and B are read inside one /RD, and a second read of port A
; under a strobe of its own would step the drive on and hand back the low
; half of the next word.  See sys/ide.c, which is the same protocol.
;
; It is a loader, not a sector copier, for the reason mwboot1.s gives at
; length: the image on the disk is the linker's output unmodified, with
; its header, and this reads the header and does what it says.  Low
; memory is empty at this point - the kernel is not loaded and the second
; level has not started - so the sectors are read to STAGE first and
; unpacked from there.
;
; The code moves itself to RELOC before loading anything, so that what it
; is loading may land wherever the header says without arriving on top of
; the loader.  After the move it uses jr for every branch of its own, so
; the copy runs correctly wherever it lands, and adds RBIAS to every
; absolute address it builds - including the two data bytes it patches.
;

;
; The MYIDE board's ports.  The range is free on this machine: 0048 and
; 004c are the Mult I/O console and 0050 the obsolete hdca.
;
IDEPA	equ	030h		; 8255 port A: drive data, low byte
IDEPB	equ	031h		; 8255 port B: drive data, high byte
IDEPC	equ	032h		; 8255 port C: register number, strobes
IDEPCTL	equ	033h		; 8255 mode word
IDEPDRV	equ	034h		; drive select latch, bit 0 selects unit 1

;
; 8255 mode words.  Port C is an output either way; what changes is
; whether ports A and B are inputs - a register read, or a sector coming
; off the drive - or outputs, a register write.
;
PPI_RD	equ	092h		; A and B in, C out
PPI_WR	equ	080h		; A, B and C all out

;
; Port C bits.  A0-A2 are the drive's address pins, so the register
; number goes straight on; the rest are inverted on the way to the drive,
; which is why a set bit here asserts the line.
;
C_CS0	equ	008h		; chip select, the board's own decode
C_WR	equ	020h		; /WR
C_RD	equ	040h		; /RD

;
; The status register, read at register 7 - the same address as the
; command, which is written - and bit 3 of it, which the drive raises
; when it has a word ready.
;
R_STAT	equ	7
DRQ	equ	008h

STAGE	equ	08000h		; where the sectors land before relocation
NSEC	equ	8		; 4k of it, which stand/README asks it to fit
TRIES	equ	10		; per sector, as ideio.c retries

BASE	equ	0100h		; where the rom reads us, and where we link
RELOC	equ	0c000h		; out of the way of what we are loading
RBIAS	equ	RELOC-BASE	; add to one of our addresses for its copy

;
; The MULTIO's first uart, for complaining with.  Output only, and no
; setup: the monitor has been talking to this port since reset - it
; printed before it read us in - so the baud rate and the line are
; already right and all that is left is to wait for the holding register
; and write a byte.
;
GSEL	equ	04fh		; group select
GRP1	equ	1		; the serial ports
TXB	equ	048h		; transmit holding register
LSR	equ	04dh		; line status
LSRTXE	equ	020h		; holding register empty

;
; Whitesmith's object header, from include/obj.h.  Sixteen bytes, and
; the file is text then data straight after it - the symbol table and
; the relocation bytes follow those and we neither read nor need them.
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
; Read sectors 1 through NSEC into STAGE upward.  Sector 0 is this code,
; so the second level starts at 1, and the whole of it is on one
; cylinder of any drive here.
;
run:
;
; A stack of our own before anything calls anything, at the top of memory
; for the reason mwboot1.s gives: the second level inherits it while it
; loads the kernel, and the kernel is about to fill everything from 0100
; up.  Nothing below is any use - it is all about to be loader or kernel.
;
	ld	sp,0		; ie 10000h - first push lands at fffe

	ld	a,GRP1		; the eight ports at 0048 are the uart now
	out	(GSEL),a
	ld	hl,msign+RBIAS
	call	puts+RBIAS

;
; hl is the destination and stays it: the transfer below leaves it
; exactly one sector on, which is where the next one goes.
;
	ld	hl,STAGE

sector:
;
; The task file.  Seven registers, in the order the drive wants them:
; features, sector count, the three LBA bytes, drive/head, command.
; tf holds them with the LBA placed already, so only its first byte is
; rewritten per sector.
;
; The PPI has to be in write mode to put a byte on port A, so it is
; switched once at each end of the run rather than once per register.
;
	ld	a,PPI_WR
	out	(IDEPCTL),a
	ld	de,tf+RBIAS
	ld	c,1		; register numbers run 1 through 7
	ld	b,7
wrtf:
	ld	a,(de)
	call	putreg+RBIAS
	inc	de
	inc	c
	djnz	wrtf
	ld	a,PPI_RD
	out	(IDEPCTL),a

;
; Wait for the drive to offer a word.  Busy and error need no test of
; their own: a drive that failed never raises DRQ, so the counter running
; out reaches the same complaint by the other road.
;
	ld	e,TRIES
try:
	call	rdstat+RBIAS
	and	DRQ
	jr	nz,gotdrq
	dec	e
	jr	nz,try
	ld	hl,mread+RBIAS
	jr	moan

;
; Move the sector, a 16-bit word at a time, both halves read inside the
; one /RD strobe.  The loop's first store clears /RD, so the strobe is
; pulsed rather than held from word to word.
;
gotdrq:
	ld	b,0		; 256 words to a sector
xfer:
	ld	a,C_CS0		; register 0, the data register
	out	(IDEPC),a
	or	C_RD
	out	(IDEPC),a
	in	a,(IDEPA)
	ld	(hl),a
	inc	hl
	in	a,(IDEPB)
	ld	(hl),a
	inc	hl
	djnz	xfer
	xor	a
	out	(IDEPC),a	;/RD released

;
; The next sector, and the LBA in the task file with it.  The byte
; counting the sectors and the byte naming the next one are the same
; byte: lba starts at 1, one past it is 2, and after NSEC sectors it
; holds 1+NSEC, which is what ends the loop.  One byte doing both is not
; only smaller than two, it cannot disagree with itself.
;
	ld	a,(lba+RBIAS)
	inc	a
	ld	(lba+RBIAS),a
	cp	1+NSEC
	jr	nz,sector

;
; Unpack what arrived.  Refuse anything without the right ident rather
; than jumping into it: a disk whose boot area was never written reads as
; zeros, and zeros are a nop slide into whatever follows.
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
; de already points at the first byte of it.  The C the second level is
; written in expects its globals zeroed and says so, so this is not
; optional.
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
; Write one task file register.  The register number arrives in c and the
; byte in a, and c comes back unchanged so the caller can walk it.  /CS0
; is held across the access and /WR is pulsed, which is how the drive
; expects to see it; the caller has already put the PPI in write mode.
;
putreg:
	out	(IDEPA),a	;the byte onto the data lines
	ld	a,c
	or	C_CS0
	out	(IDEPC),a	;register selected
	or	C_WR
	out	(IDEPC),a	;/WR asserted, the drive takes it
	ld	a,c
	or	C_CS0
	out	(IDEPC),a	;/WR released
	ret

;
; Read the status register.  Register 7 reads as status and writes as
; command; the direction picks which is meant.  The PPI is in read mode
; by the time this is reached.
;
; The register number and /CS0 are added, not ored.  This assembler has
; no | - it is not an operator, and an expression using one is not
; rejected: the left operand is taken and the operator and everything
; after it are dropped without a word of complaint.  C_CS0|R_STAT
; assembles to /CS0 alone, register 0, the data register; a read of that
; takes a word off the drive rather than reporting the status, so DRQ is
; never seen, the counter below runs out on the first sector, and the
; loader blames a drive that was answering correctly all along.
;
; A sum is the same as an or here, and + is an operator this assembler
; does evaluate.  The two are the same value because no bit of one is a
; bit of the other: the register number is the drive's A0-A2 and /CS0 is
; bit 3 of the same port C byte.
;
rdstat:
	ld	a,R_STAT+C_CS0
	out	(IDEPC),a
	or	C_RD
	out	(IDEPC),a
	in	a,(IDEPA)
	ret

;
; What we read is not a program.
;
badmag:
	ld	hl,mmagic+RBIAS

;
; Complain and stop.  There is nowhere to go from here: the console
; belongs to whatever we failed to load, and the only thing left is to
; say which of the two things went wrong and wait for somebody.
;
moan:
	call	puts+RBIAS
hang:
	jr	hang

;
; Output only, and no setup: the monitor has had this port since reset, so
; the line is already right.  Wait for the holding register, write a byte,
; repeat until the nul.
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

;
; The task file, and the byte patched in place as it is walked.  The
; layout is the drive's, not ours: register 1 first.  A sector count of
; one and a drive/head of e0h - bit 6 set is LBA mode, unit 0 - and the
; command 20h, read sector with retry, is the whole of what the drive is
; told.  lba is the first of the three LBA bytes and starts at 1 because
; sector 0 is this code.
;
tf:	.db	0		;features
	.db	1		;sector count
lba:	.db	1		;LBA 0-7
	.db	0		;LBA 8-15
	.db	0		;LBA 16-23
	.db	0e0h		;drive/head: LBA mode, unit 0
	.db	020h		;command: read sector(s), with retry

msign:	.db	"ideboot:",13,10,0
mread:	.db	"read",13,10,0
mmagic:	.db	"magic",13,10,0

finish:
