;
; mwboot1 - the first level hard disk boot for the HD/DMA
;
; micronix/stand/mwboot1.s
;
; The monitor's nuboot reads one sector - cylinder 0, head 0, sector 0 -
; to 0100 and enters task 1 there.  That sector is this, and its whole
; job is to bring in the sector after it: mwboot.com, which is linked at
; 0100 and knows the v6 filesystem.
;
; installboot puts both of us in a file that owns cylinder 0, so this is
; block 0 of that file and mwboot.com is block 1 onward.  Nothing else on
; the disk is involved and no filesystem is read here - that is mwboot's
; job, and this exists only because the rom will read one sector and no
; more.
;
; Almost nothing needs setting up.  The rom has just used the channel at
; 0080 to read this very sector, so the controller has its constants, the
; drive is selected, the head is 0 and the heads are at cylinder 0 - all
; of it correct and none of it ours to redo.  Only three things change
; per sector: which sector, where it goes, and the opcode.
;
; The code moves itself out of 0100 before loading anything, because
; mwboot.com is linked to run at 0100 and would otherwise be read over
; the top of the loop reading it.  After the move it uses jr for every
; branch of its own, so the copy runs correctly wherever it lands, and
; keeps its counters in registers so there is nothing to write to.
;

CHAN	equ	0080h		; the channel the rom left set up
DMA	equ	CHAN+4		; 24 bit address, low two bytes here
XDMA	equ	CHAN+6		; and the high byte here
SECTOR	equ	CHAN+10		; byte3 in struct hddma_cmd
OPCODE	equ	CHAN+11		; 0 is read sector
STATUS	equ	CHAN+12		; 0 busy, 0ffh done, anything else an error
ATTN	equ	055h		; kick the controller

LOAD	equ	0100h		; where mwboot.com is linked and entered
NSEC	equ	8		; 4k of it, which stand/README asks it to fit
TRIES	equ	10		; per sector, as mwio.c retries

RELOC	equ	0c000h		; out of the way of what we are loading

	org	0100h

start:
	ld	hl,start
	ld	de,RELOC
	ld	bc,finish-start
	ldir
	jp	RELOC+(run-start)

;
; Read sectors 1 through NSEC of this track into LOAD upward.  Sector 0
; is this code, so the second level starts at 1, and a track holds 17
; sectors on every drive here - the whole of the second level is on the
; one track and the head never moves.
;
run:
	ld	hl,LOAD		; where the next sector goes
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
; somebody watching the front panel or the simulator can see it.
;
stuck:
	jr	stuck

good:
	inc	h		; on by 512, and l stays 0
	inc	h
	inc	c
	djnz	onesec
	jp	LOAD

finish:

	end
