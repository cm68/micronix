;
; hdcaboot1 - the first level hard disk boot for the HDCA
;
; micronix/stand/boot/hdcaboot1.s
;
; The monitor reads one sector - cylinder 0, head 0, sector 0 - to 0100
; and enters task 1 there.  That sector is this, and its job is to bring
; in the sectors after it: the second level, which knows the v6
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
; complaints.  What differs is that this one cannot assume a primed
; controller the way mwboot1 can: the HDCA has no channel command block
; in memory, so every sector here writes its own header - head, track,
; sector, key - through the data port and issues its own read, then
; pulls the sector out one byte at a time.
;
; Almost nothing needs setting up first.  The rom has just read this
; very sector through the same controller, so the arm is at cylinder 0
; and the whole second level is on that one track - only the sector
; number and the destination change per read, and no seek is ever
; needed.
;
; The code moves itself to RELOC before loading anything, so that what
; it is loading may land wherever the header says without arriving on
; top of the loader.  After the move it uses jr for every branch of its
; own, so the copy runs correctly wherever it lands, and adds RBIAS to
; every absolute address it builds.
;

;
; The HDCA's four ports, one address read and written.  See sys/hdca.h.
;
CONTROL	equ	050h		; write: front-end, run, clock, writes
CMD	equ	051h		; write: ram pointer / read / write
FUNC	equ	052h		; write: drive, head, step, direction
DATA	equ	053h		; write/read: the controller's RAM
PSR	equ	050h		; read: primary status
SSR	equ	051h		; read: secondary status
INTCLR	equ	052h		; read: acknowledge seek-done

;
; The command byte written to CMD.  RAMDATA and RAMHDR reset the RAM
; pointer to the data or the header area; READSEC reads the sector named
; by the header.  A read stores the sector at offset +2 with its last
; two bytes wrapped to the front, so two slack bytes are read and
; discarded before the 512 that follow.
;
CMD_RAMDATA	equ	0
CMD_READSEC	equ	1
CMD_RAMHDR	equ	8

;
; The control byte: front-end enabled, running, on the drive clock, and
; writes on.  The head byte is the head number inverted, shifted to the
; FUNC register's high nibble.
;
CONTROL_BITS	equ	00fh
HEAD0		equ	0f0h	; head 0

PSR_OPDONE	equ	002h	; primary status: the read is done

STAGE	equ	08000h		; where the sectors land before relocation
NSEC	equ	8		; 4k of it, which stand/README asks it to fit

BASE	equ	0100h		; where the rom reads us, and where we link
RELOC	equ	0c000h		; out of the way of what we are loading
RBIAS	equ	RELOC-BASE	; add to one of our addresses for its copy

;
; The MULTIO's first uart, for complaining with.  Output only, and no
; setup: the monitor has been talking to this port since reset - it
; printed before it read us in - so the baud rate and the line are
; already right.
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
; A stack of our own before anything calls anything, at the top of
; memory for the reason mwboot1.s gives: the second level inherits it
; while it loads the kernel, and the kernel is about to fill everything
; from 0100 up.
;
	ld	sp,0		; ie 10000h - first push lands at fffe

	ld	a,GRP1		; the eight ports at 0048 are the uart now
	out	(GSEL),a
	ld	hl,msign+RBIAS
	call	puts+RBIAS

;
; Enable the controller and select drive 0, head 0.  Nothing else needs
; doing: the rom has just read sector 0 through this controller, so the
; arm is on cylinder 0 and the head is 0.
;
	ld	a,CONTROL_BITS
	out	(CONTROL),a
	ld	a,HEAD0
	out	(FUNC),a

;
; hl is the destination and stays it: the transfer below leaves it
; exactly one sector on, which is where the next one goes.
;
	ld	hl,STAGE
	ld	c,2		; the second level: one past sector 1, block 0

onesec:
;
; Write the header the read keys on: head 0, track 0, this sector, key
; zero.  The RAM pointer is reset to the header area first, then the
; four bytes go through the data port.
;
	ld	a,CMD_RAMHDR
	out	(CMD),a
	xor	a		; head 0
	out	(DATA),a
	xor	a		; track 0
	out	(DATA),a
	ld	a,c		; the sector
	out	(DATA),a
	xor	a		; key 0
	out	(DATA),a

;
; Issue the read and wait for the controller to say it is done.
;
	ld	a,CMD_READSEC
	out	(CMD),a
poll:
	in	a,(PSR)
	and	PSR_OPDONE
	jr	z,poll

;
; Pull the sector out, a byte at a time.  Two slack bytes first - a read
; leaves the sector at offset +2 with its last two bytes wrapped to the
; front - then the 512 that follow are the sector in order.  Two loops
; of 256, because djnz counts a zero-loaded b down.
;
	ld	a,CMD_RAMDATA
	out	(CMD),a
	in	a,(DATA)
	in	a,(DATA)
	ld	b,0		; 256 bytes
xfer:
	in	a,(DATA)
	ld	(hl),a
	inc	hl
	djnz	xfer
	ld	b,0		; and the other 256
xfer2:
	in	a,(DATA)
	ld	(hl),a
	inc	hl
	djnz	xfer2

;
; The next sector.  The byte naming it starts at 1, one past it is 2,
; and after NSEC sectors it holds 1+NSEC, which is what ends the loop.
;
	inc	c
	ld	a,c
	cp	2+NSEC
	jr	nz,onesec

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
; de already points at the first byte of it.
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
; belongs to whatever we failed to load.
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

msign:	.db	"hdcaboot:",13,10,0
mmagic:	.db	"magic",13,10,0

finish:
