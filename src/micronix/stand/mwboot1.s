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
;	cat mwboot1.cim mwboot.com >bootimg
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

RELOC	equ	0c000h		; out of the way of what we are loading

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

	org	0100h

start:
	ld	hl,start
	ld	de,RELOC
	ld	bc,finish-start
	ldir
	jp	RELOC+(run-start)

;
; Read sectors 1 through NSEC of this track into STAGE upward.  Sector 0
; is this code, so the second level starts at 1, and a track holds 17
; sectors on every drive here - the whole of the second level is on the
; one track and the head never moves.
;
run:
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
; Out of retries.  There is nowhere to go and nothing to say it with -
; the console belongs to whatever we failed to load - so stop where
; somebody watching the front panel or the simulator can see it.  The
; two stops below are deliberately different addresses: which one the pc
; is sitting in says whether the disk would not read or what it read was
; not a program.
;
stuck:
	jr	stuck

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
; What we read is not a program.  Same silence as above, a different
; address.
;
badmag:
	jr	badmag

finish:

	end
