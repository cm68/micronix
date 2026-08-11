;
; mwboot1 - the first level hard disk boot for the HD/DMA
;
; micronix/stand/mwboot1.s
;
; The monitor's nuboot reads one sector - cylinder 0, head 0, sector 0 -
; to 0100 and enters task 1 there.  That sector is this, and its job is
; to bring in the sector after it: the second level, which knows the v6
; filesystem and finds a kernel.
;
; mkfs -i puts both of us in a file that owns cylinder 0, so this is
; block 0 of that file and the second level is block 1 onward.  Nothing
; else on the disk is involved and no filesystem is read here - that is
; the second level's job, and this exists only because the rom will read
; one sector and no more.
;
;
; a loader, not a sector copier
; -----------------------------
;
; This used to read the second level straight to 0100 and jump there,
; which meant the thing on the disk had to be a flat image already
; running at 0100 - so building it meant stripping the object header on
; the host and knowing, out here, where the text began and how long it
; was.  That is knowledge in the wrong place, and getting it wrong
; produces a boot that reads perfectly and executes the middle of a
; function.
;
; So the image on the disk is now the linker's output, unmodified, and
; this reads the header and does what it says: text to its own address,
; data to its own address, bss cleared, entry where the header puts it.
; Replacing the second level is
;
;	cat mwboot1.bin mwboot.com >bootimg
;
; with a pad, and nothing else has to agree about anything.
;
; The sectors are read to STAGE first, because a header displaces
; everything after it - you cannot read a headered image directly to
; 0100 and have the text land at 0100, and the copy has to come from
; somewhere this code is not sitting in.  Low memory is empty at this
; point: the kernel is not loaded and the second level has not started.
;
; It also means a second level linked somewhere other than 0100 works
; without changing this, which the flat version could not do.
;
; Almost nothing needs setting up before the read.  The rom has just
; used the channel at 0080 to read this very sector, so the controller
; has its constants, the drive is selected, the head is 0 and the heads
; are at cylinder 0 - all of it correct and none of it ours to redo.
; Only three things change per sector: which sector, where it goes, and
; the opcode.
;
; The code moves itself to RELOC before loading anything, so that what
; it is loading may land wherever the header says without arriving on
; top of the loader.  After the move it uses jr for every branch of its
; own, so the copy runs correctly wherever it lands.
;
;
; assembled by asz, linked at BASE, and unwrapped
; ----------------------------------------------
;
; asz has no org: it writes a relocatable object and the linker decides
; where the text goes, so BASE below is the one place that says 0100 and
; the Makefile passes the same number to wsld.  The 16 byte header the
; linker writes is then cut off, because the rom reads a sector and
; jumps to the first byte of it - there is nothing there to read a
; header, which is the whole reason the second level needs this one.
;
; Two addresses of ours therefore appear in two forms.  A label is where
; the linker put it, which is where it lies in the sector we were read
; into; adding RBIAS gives the same byte in the copy at RELOC.  Every
; reference the moved code makes to itself is written that way, and the
; branches it takes are jr, which need no adjustment at all.
;

CHAN	equ	0080h		; the channel the rom left set up
DMA	equ	CHAN+4		; 24 bit address, low two bytes here
XDMA	equ	CHAN+6		; and the high byte here
SECTOR	equ	CHAN+10		; byte3 in struct hddma_cmd
OPCODE	equ	CHAN+11		; 0 is read sector
STATUS	equ	CHAN+12		; 0 busy, 0ffh done, anything else an error
ATTN	equ	055h		; kick the controller

STAGE	equ	08000h		; where the sectors land before relocation
NSEC	equ	8		; 4k of it, which stand/README asks it to fit
TRIES	equ	10		; per sector, as mwio.c retries

BASE	equ	0100h		; where the rom reads us, and where we link
RELOC	equ	0c000h		; out of the way of what we are loading
RBIAS	equ	RELOC-BASE	; add to one of our addresses for its copy

;
; The MULTIO's first uart, for complaining with.  Output only, and no
; setup: the monitor has been talking to this port since reset - it
; printed before it read us in - so the baud rate and the line are
; already right and all that is left is to wait for the holding register
; and write a byte.  A few dozen bytes, which we have.
;
; Which group is selected decides what the eight ports at 0048 mean, so
; the group is set before use.  Only the failure paths do this, so the
; ordinary boot touches nothing on the way past.
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
; Read sectors 1 through NSEC of this track into STAGE upward.  Sector 0
; is this code, so the second level starts at 1, and a track holds 17
; sectors on every drive here - the whole of the second level is on the
; one track and the head never moves.
;
run:
;
; A stack of our own before anything calls anything.  The rom left one
; somewhere and the second level's startup will pop from it regardless,
; but the first level had no stack dependency at all until the signon
; introduced one, and it costs three bytes not to inherit a guess.
;
; At the top of memory, and it has to be up there.  The second level
; inherits this stack and keeps using it while it loads the kernel, and
; the kernel is 117 blocks starting at 0ff0 - it ends around 0f9f0.  A
; stack at RELOC, which is where this was, is in the middle of that: the
; load ran over it about seven eighths of the way through and the loader
; died holding a return address that was now kernel image.  It got as far
; as reading every block it meant to and then never reached its own
; "Entering".
;
; Below is no good either.  0000 to 0fff is the rom and I/O page for the
; supervisor, and the loader already fills 0100 to 0fba, so there is no
; room down there for a stack and nowhere else for the loader to be.
; Above the kernel is what is left.
;
	ld	sp,0		; ie 10000h - first push lands at fffe

	ld	a,GRP1		; the eight ports at 0048 are the uart now
	out	(GSEL),a
	ld	hl,msign+RBIAS
	call	puts+RBIAS

	ld	hl,STAGE	; where the next sector goes
	ld	b,NSEC		; how many are left
	ld	c,1		; and which one this is

onesec:
	ld	(DMA),hl
	xor	a
	ld	(XDMA),a
	ld	a,c
	ld	(SECTOR),a
	ld	e,TRIES

try:
	xor	a
	ld	(OPCODE),a	; read
	ld	(STATUS),a	; and say we are waiting for it
	out	(ATTN),a
poll:
	ld	a,(STATUS)
	or	a
	jr	z,poll		; still busy
	inc	a
	jr	z,good		; 0ffh, so it worked
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
; than jumping into it: a disk whose boot area was never written reads
; as zeros, and zeros are a nop slide into whatever follows.
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
; written in expects its globals zeroed and says so - "int inumber = 1"
; beside "struct dsknod *inode INIT" - so this is not optional.
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
; Complain and stop.  There is nowhere to go from here: the console
; belongs to whatever we failed to load, and the only thing left is to
; say which of the two things went wrong and wait for somebody.
;
moan:
	call	puts+RBIAS
hang:
	jr	hang

;
; Output only, and no setup: the monitor has had this port since reset,
; so the line is already right.  Wait for the holding register, write a
; byte, repeat until the nul.
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

msign:	.db	"mwboot:",13,10,0
mread:	.db	"read",13,10,0
mmagic:	.db	"magic",13,10,0

finish:
