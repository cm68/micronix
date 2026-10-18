;****************************************************************
;*								*
;* Level 1 boot.  A 4k rom living at physical 0ff0000h that	*
;* mon500's ramcode copies down to 0f000h and enters in task 0.	*
;*								*
;* Level 0 is the monitor in its socket; level 1 is this, whose	*
;* job is to bring in 512 bytes from sector 0 of a disk; that	*
;* 512 bytes is level 2 and it loads level 3, the filesystem-	*
;* aware loader that offers a menu of kernels.			*
;*								*
;* It knows one thing per kind of disk - how to ask the card	*
;* whether a drive is there, and how to move block 0 off it -	*
;* and it walks those in table order twice: once to find every	*
;* disk carrying a label, and once to boot the one chosen.	*
;*								*
;****************************************************************

	org	0F000h

;****************************************************************
;* Console equates.  The monitor does not carry code for a uart	*
;* that may not be there; it finds the console at reset, programs	*
;* the chip, and builds the driver for it into on-board ram at	*
;* conram, which the level 1 rom also has mapped in task 0.	*
;****************************************************************

conram	equ	0132h		;the console found at reset
concon	equ	conram		; - which one answered: 0, 1 or 2
condrv	equ	conram + 2	; - the driver built for it
csend	equ	0		;+0: send the character in C
crecv	equ	3		;+3: wait for a character into A
credy	equ	6		;+6: is one waiting?  Z set if not

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

scsidat	equ	20h		;data register (CSD), read and write
scsiicr	equ	21h		;initiator command register
scsimr	equ	22h		;mode register
scsistat equ	24h		;current bus status
icr_rst	equ	80h
icr_ack	equ	10h
icr_bsy	equ	08h
icr_sel	equ	04h
icr_data equ	01h
scsi_bsy equ	40h
scsi_req equ	20h
scsi_mon equ	04h

;****************************************************************
;* The alias window: task 0's segment 1 points at bus page 0.	*
;* That is where the level 2 boot lives in task 1, at 100h, and	*
;* it is the only way to reach a controller that dmAs to a	*
;* physical address, because this rom's own page is not on the	*
;* s-100 bus.  A row that dmAs stages its sector in bus page 0	*
;* and copies the block out of the window, and every address it	*
;* hands a controller is a bus address and not the one the	*
;* window shows it at.						*
;****************************************************************

win	equ	01000h		;bus 0000h, through segment 1
wbase	equ	0200h		;bus 0200h: where a sector is staged
wbuf	equ	win + wbase	; - and where the window shows it

;****************************************************************
;* HD-DMA (MW) equates.  Not a programmed-io board: the host	*
;* leaves a command block in memory and points the controller	*
;* at it with three bytes at 0050h, and the board dmAs to a	*
;* physical address and writes a completion status back into	*
;* the block.  The block is the one the monitor's own disk boot	*
;* used, at 0080h in bus page 0.  See mon447.s's nuboot, which	*
;* is this sequence in the monitor's own rom.			*
;****************************************************************

mwrst	equ	54h		;reset to the controller
mwattn	equ	55h		;attention: run the command
mwgood	equ	0ffh		;a completed command's status
mwconst	equ	4		;load the drive's constants
mwnoop	equ	6		;no op, used to recalibrate
mwstepo	equ	10h		;step toward track 0
mwstpdl	equ	01eh		;step rate, in 15 microsecond units
mwhdsl	equ	0c8h		;head settle
mwsecsz	equ	3		;512 byte sectors
mwhd0	equ	07ch		;drive 0, head 0, high current
mwldhd	equ	03ch		;drive 0, head 0, loading constants
mwbase	equ	080h		;bus 0080h: the command block
mwcca	equ	win + 050h	;bus 0050h: where the controller reads
				; the block's address from, on reset
mwchan	equ	win + mwbase	;the block, through the window
mwstep	equ	mwchan + 1	;the step count
mwdma	equ	mwchan + 4	;the dma address
mwcmnd	equ	mwchan + 11	;the opcode
mwstat	equ	mwchan + 12	;the status the controller writes back

;****************************************************************
;* HDCA equates.  Port mapped: four registers carry everything,	*
;* with no command block in memory, no dma address and no	*
;* status byte to poll there - the host reads the sector through	*
;* the data port, out of the controller's own sector ram.	*
;* See mon447.s's boothd and stand/boot/hdcaio.c.		*
;****************************************************************

hdcont	equ	050h		;write: control / read: primary status
hdcmd	equ	051h		;write: command / read: secondary status
hdfunc	equ	052h		;write: drive, head, step, direction
hddata	equ	053h		;write/read: the controller's ram
hdctl	equ	00fh		;front end on, running, on the drive
				; clock, writes enabled
hdrest	equ	0fch		;drive 0, head 0, step line parked:
				; the head field is inverted, so
				; head 0 is f0h and step and down are
				; 04h and 08h
hdnrdy	equ	020h		;primary status: the drive is not ready
hdopdn	equ	002h		;primary status: the operation finished
hdnout	equ	008h		;primary status: it timed out
hdnfault equ	010h		;primary status: no fault
hdcrc	equ	002h		;secondary status: a data crc error
hdramhd	equ	008h		;command: ram pointer to the header area
hdrsec	equ	001h		;command: read one sector
hdramdt	equ	000h		;command: ram pointer to the data area

;****************************************************************
;* DJ-DMA equates.  The controller has a z80 of its own that	*
;* runs a channel program out of the host's memory: the host	*
;* writes a setdma / sread / halt sequence at the default	*
;* channel address, pulses the attention port, and polls the	*
;* status byte the controller writes back into the sequence.	*
;* See stand/boot/djio.c.					*
;****************************************************************

djattn	equ	0efh		;attention: run the channel program
djsdma	equ	023h		;set the 24 bit dma address
djsread	equ	020h		;read one sector
djsense	equ	022h		;sense drive status: the drive's
				; characteristics, its sector length
				; and its status, three bytes written
				; after the command
djhlt	equ	025h		;end the channel program
djlset	equ	02eh		;set the logical drive numbering
djfive	equ	004h		; its parameter: bit 2 makes the five
				; inch port logical 0 to 3
dj5drv	equ	4		;and the five inch port's first drive,
				; which is the number of that port's
				; first minus the port: the drive
				; numbering is which port it is on
djok	equ	040h		;a command's good status
djhard	equ	002h		;characteristics byte: the medium is
				; hard sectored, so its sectors are
				; numbered from zero
djslc	equ	003h		;sector length code: 128 bytes, and
				; one doubling per step of the code
djchan	equ	win + 050h	;bus 0050h: the default channel address
				; +0 setdma  +1,+2 address  +3 high byte
				; +4 sread   +5 cylinder  +6 sector and
				; head  +7 drive  +8 status
				; +9 halt    +10 status
				; Drive is 0 to 3 for the eight inch
				; drives and 4 to 7 for the five inch
				; ones - the port the drive hangs off.
				; The program is still here, drive byte
				; and all, when level 2 runs, which is
				; how level 2 knows which drive it came
				; from.  See stand/boot/djboot1.s.
				;
				; Asking the drive what it is puts a
				; three byte sense program here
				; instead - see djask - and the read
				; program is copied back in before every
				; read, so the two never share it.
djsec	equ	djchan + 6		;the sector, and the head above it
djstat	equ	djchan + 8		;the status the controller writes

;****************************************************************
;* CompuPro DISK 1 equates.  An 8272 floppy controller - the	*
;* chip owns the head, finds the sector and moves it itself,	*
;* to the address the board's dma register holds - behind the	*
;* board's own glue.  See include/sys/disk1.h and sys/disk1.c.	*
;****************************************************************

d1msr	equ	0c0h		;read: the chip's main status register
d1dsel	equ	0c0h		;write: the drive select register
d1data	equ	0c1h		;the chip's data register, both ways
d1stat	equ	0c2h		;read: the drive status register
d1dma	equ	0c2h		;write: the dma address, msb first
d1motor	equ	0c3h		;write: the motors and the boot eprom
d1rqm	equ	080h		;main status: the data register is ready
d1dio	equ	040h		; ... and it is the chip's byte
d1int	equ	080h		;drive status: a command has finished
d1boot	equ	001h		;motor register: bit 0 down disables the
				; board's boot eprom for good, so it is
				; written 1 on every pass
d1m0	equ	010h		;and this is drive 0's motor
d1recal	equ	007h		;step out to track 0.  No result phase
d1sensei equ	008h		; - the two byte answer to it is this
d1rdcmd	equ	046h		;read one sector, double density
d1n	equ	2		;the sector size code for 512 bytes
d1eot	equ	1		;and one sector a track is all it asks
d1gpl	equ	01bh		;the gap length that size goes with
d1st0m	equ	0e0h		;status 0: the interrupt code and the
				; seek end bit
d1se	equ	020h		; ... which a finished recalibrate sets
d1errm	equ	0d8h		;status 0: a finish that was not normal
d1err1	equ	0b7h		;status 1: every error but the head
d1err2	equ	063h		;status 2: every error but the mark

;****************************************************************
;* CompuPro DISK 2 equates.  A hard disk board that is not	*
;* intelligent: no dma engine of its own - the transfer goes	*
;* through the SELECTOR CHANNEL, a board of its own that takes	*
;* the bus and generates the address - and no seek, so the	*
;* host steps the head one pulse per cylinder.  See		*
;* include/sys/disk2.h and sys/disk2.c.				*
;****************************************************************

d2ctl	equ	0c8h		;write: control / read: status
d2data	equ	0c9h		;write: the named register / read: step
d2sel	equ	0e0h		;the selector channel's port
d2attn	equ	080h		;control: attention, which marks the
				; board busy and makes its status
				; transparent - and the status bit that
				; falls when a command finishes
d2run	equ	040h		;control: run the command
d2op2	equ	020h		;control: the opcode's top bit, and the
				; direction the head steps while it is
				; not running
d2trk0	equ	001h		;status: the drive is at track 0
d2errm	equ	07ch		;status: every failure bit there is
d2rdcmd	equ	008h		;the read operation
d2null	equ	000h		;and the one that only waits
d2regdrv equ	000h		;data register select: the drive and
				; head - the drive is the high nibble,
				; bit 4 being drive 0, and the head the
				; low one
d2regcyl equ	008h		;and the three header match bytes
d2reghed equ	010h
d2regsec equ	018h
d2unit	equ	010h		;the drive register: drive 0, head 0
d2wrmem	equ	000h		;selector channel mode: the transfer
				; writes memory, which is what a disk
				; read does
d2scid	equ	006h		; ... and the low nibble is the
				; complement of this board's dma
				; priority, which is jumpered to 9

;****************************************************************
;* CompuPro DISK 3 equates.  An intelligent controller: an 8085	*
;* runs the drive and the host's whole part is a sixteen byte	*
;* iopb in bus memory, which the board fetches itself, and an	*
;* attention to make it run.  The answer is the status byte the	*
;* board writes back into the iopb.  See include/sys/disk3.h	*
;* and sys/disk3.c.						*
;****************************************************************

d3attn	equ	090h		;the board's one port, write only
d3rst	equ	0ffh		;hold the 8085 in reset.  Anything
				; else written is an attention
d3noop	equ	000h		;do nothing: the presence tickle
d3global equ	002h		;set the mode, the retry count and
				; how many drives there are
d3specify equ	003h		;load a drive's parameter table, which
				; is what a dma address must point at
				; when this one runs
d3rwcmd	equ	008h		;read or write sectors
d3abs	equ	0ffh		;GLOBAL's mode: sectors are absolute
				; numbers, which is the only mode that
				; need not know the drive's geometry
d3rdcmd	equ	001h		;R/W's first argument: read
d3ok	equ	0ffh		;the status a command that worked leaves
d3bus	equ	0010h		;bus 0010h: the iopb.  The board fetches
				; it, so it must be bus memory
d3pbus	equ	0020h		;bus 0020h: the parameter table, which
				; is a second address it fetches
d3lnkb	equ	win + 005dh	;bus 005dh: the link bytes the first
				; fetch after a reset leaves behind
d3iopb	equ	win + d3bus	;those two, through the window
d3param	equ	win + d3pbus
d3cmnd	equ	d3iopb + 0	;the opcode
d3stat	equ	d3iopb + 1	;the status the board writes back
d3arg	equ	d3iopb + 3	;arg1, and the six after it
d3data	equ	d3iopb + 10	;the dma address, three bytes
d3link	equ	d3iopb + 13	;and the next iopb, which is this one

;****************************************************************
;* on-board registers						*
;****************************************************************

mapram	equ	0600h		;the map ram, which is write only: this
				; rom sets a segment and never reads a
				; map back.  200h is the monitor's own
				; readable image of it and its stack, and
				; the boot transfer lands on both.  Every
				; write here needs its twin at map + n:
				; the kernel reads IMAGE0 (0200h) as its
				; own window table (sys/mem.s), so an
				; image left behind is a page the kernel
				; will put back, and the monitor's own
				; "restore the maps" pairs the two
map	equ	0200h		;the readable image of the map ram
task	equ	0402h		;the task register: write to switch
gobuff	equ	01b0h		;ram stub for the task switch
ctask	equ	0006h		;current task, read by the trap handler

;****************************************************************
;* the disk label, in the boot sector's second half		*
;* (include/sys/dlabel.h).  It is the drive speaking, and the	*
;* only thing in it level 1 cares about is that it is there at	*
;* all: the magic, and a geometry that is not empty.  The	*
;* version is deliberately not checked - the kernel does not	*
;* check it either, and a labelled disk boots whatever it says.	*
;****************************************************************

dl_off	equ	256		;into block 0: the second half
dl_trk	equ	dl_off + 6	;d_tracks, a word
dl_hds	equ	dl_off + 8	;d_heads
dl_spt	equ	dl_off + 10	;d_spt, blocks per track
dl_mfg	equ	dl_off + 88	;d_mfg, sixteen bytes
dl_mod	equ	dl_off + 104	;d_model, sixteen bytes

;****************************************************************
;* entry: the byte mon500 jumps to.  Its ramcode copies this 4k	*
;* down to physical 0f000h and jumps there in task 0, with the	*
;* map back to identity - so this must be the first thing in	*
;* the image, and nothing may sit ahead of it.  The monitor's	*
;* halt trap is live, which is what makes a halt here reach the	*
;* monitor rather than run off into nothing.			*
;*								*
;* Pass 1 finds every disk that answers with a label on it and	*
;* prints them; pass 2 boots the one chosen.			*
;****************************************************************

entry:
	ld	hl,strset	;"level 1 boot"
	call	uputs
	call	enumer		;pass 1: fill cand[] and print it
	ld	a,(ncand)
	or	a
	jp	nz,bootit
	ld	hl,strnone	;"no boot device"
	call	uputs
hang:	halt
	jr	hang

;****************************************************************
;* The controller table: one row per kind of disk the machine	*
;* can boot from, in the order they are probed, six bytes a row	*
;* - the name the menu prints, the routine that says whether a	*
;* drive is there, and the routine that moves block 0 to	*
;* (dest).  A row with a zero name ends the table.		*
;*								*
;* A probe answers with carry set if a drive is there and ready	*
;* to be read; a read answers with carry set if it delivered	*
;* the block to (dest) - the staging buffer in pass 1, where	*
;* task 1 runs it in pass 2.  Both leave the row cursor alone.	*
;****************************************************************

ctable:
	dw	chd0a, mwprobe, mwread
	dw	chda, hdaprobe, hdaread
	dw	cide, ideprobe, ideread
	dw	cscsi, ncrprobe, ncrread
	dw	cfla, djprobe, djread
	dw	cd1a, d1probe, d1read
	dw	cd2a, d2probe, d2read
	dw	cd3a, d3probe, d3read
	dw	0

;****************************************************************
;* pass 1: walk the table and keep every disk carrying a label.	*
;* Each survivor is remembered by the row it came from - so	*
;* pass 2 can run that row's own read again - and printed with	*
;* what it says about itself, while its block 0 is still in the	*
;* staging buffer to be read.					*
;****************************************************************

enumer:
	ld	hl,ctable
	ld	(cur),hl
	xor	a
	ld	(ncand),a
	ld	hl,cand
	ld	(chead),hl
enuml:	ld	hl,(cur)
	call	loadrow		;the row's three pointers
	ld	a,(cname)	;a zero name ends the table
	or	a
	ret	z
	ld	a,(cname+1)
	or	a
	ret	z
	call	rowatch
	jr	nc,enumnt
	ld	hl,(chead)	;remember the row, not the disk:
	ld	bc,(cur)	; pass 2 re-runs it, so a drive
	ld	(hl),c		; that vanished is caught then
	inc	hl
	ld	(hl),b
	inc	hl
	ld	(chead),hl
	ld	a,(ncand)
	inc	a
	ld	(ncand),a
	call	putnum		;"1) "
	ld	hl,(cname)
	call	uputs
	ld	hl,strcolon	;": "
	call	uputs
	ld	a,(legac)	;a medium with no label has no shape
	or	a		; to print and no name to give
	jr	nz,enumlg
	call	putid		;what it calls itself, or its shape
	ld	hl,strcomma	;", "
	call	uputs
	call	putmb		;how big, in megabytes
	ld	hl,strmb	;" MB"
	call	uputs
enumle:	ld	hl,strnl
	call	uputs
enumnt:	ld	hl,(cur)
	ld	de,6		;one row further on
	add	hl,de
	ld	(cur),hl
	jr	enuml
enumlg:	ld	hl,strlegac	;no label: the medium's own sector 0
	call	uputs		; is the program, and is a legacy
	jr	enumle		; boot sector's

;****************************************************************
;* one row: probe it, and if a drive answers, read block 0 into	*
;* the staging buffer and look for a label.  Carry set means	*
;* the row is a candidate.					*
;****************************************************************

rowatch:
	ld	hl,(cprobe)
	call	jphl
	jr	nc,rowno	;nothing answered
	ld	hl,stg
	ld	(dest),hl
	;	A read that fails without moving anything leaves the
	;	block the last row left there, and what the last row
	;	left there is a block 0 with a label in it.  So the
	;	magic is spoiled first, and the match below can only
	;	have come from this row's own read.
	ld	hl,stg + dl_off
	ld	(hl),0
	inc	hl
	ld	(hl),0
	inc	hl
	ld	(hl),0
	inc	hl
	ld	(hl),0
	xor	a		;and this is pass 1: the block is
	ld	(pass2),a	; staged for the label, not for a
	ld	(legac),a	; task.  Nothing has claimed to be a
				; legacy boot sector yet, and a row
				; that reads one says so below
	ld	hl,(cread)
	call	jphl
	jr	nc,rowno	;the drive stopped answering
	ld	hl,stg + dl_off
	ld	de,dlmagic
	ld	b,4
rowmg:	ld	a,(de)
	cp	(hl)
	jr	nz,rowlg
	inc	hl
	inc	de
	djnz	rowmg
	ld	hl,stg + dl_trk
	call	nzw		;d_tracks
	jr	z,rowno
	call	nzw		;d_heads
	jr	z,rowno
	call	nzw		;d_spt
	jr	z,rowno
rowok:	scf			;it is a disk
	ret
	;	A block with no label in it is not ours, but it is not
	;	nothing either: a row that read one says so, and what
	;	it read is the medium's own sector 0 - the program,
	;	which that row put where the program wants to run.
rowlg:	ld	a,(legac)
	or	a
	jr	nz,rowok
rowno:	or	a		;carry clear: no candidate
	ret

;	the word at (hl) is not zero, and hl moves past it
nzw:	ld	a,(hl)
	inc	hl
	or	(hl)
	inc	hl
	ret

;	hl -> a table row: its three pointers into cname/cprobe/cread
loadrow:
	ld	de,cname
	ld	bc,6
	ldir
	ret

;	print the candidate number in A in decimal, then ") "
putnum:	ld	hl,decbuf + 4
	ld	(hl),')'
	inc	hl
	ld	(hl),' '
	inc	hl
	ld	(hl),0
	ld	hl,decbuf + 4	;the digits go below this
	ld	c,a
pnlp:	ld	a,c		;peel a digit off the right
	ld	b,0
pnsub:	sub	10
	jr	c,pndig
	inc	b
	jr	pnsub
pndig:	add	a,10		;the remainder
	add	a,'0'
	dec	hl
	ld	(hl),a
	ld	c,b
	ld	a,b
	or	a
	jr	nz,pnlp
	call	uputs
	ret

;****************************************************************
;* identify the disk whose block 0 is in staging.  A labelled	*
;* drive need not name itself: d_mfg and d_model hold whatever	*
;* the formatter was told, and it may have been told nothing.	*
;* A disk with no name is identified by its shape instead, and	*
;* 306x6x17 says what "Quantum Q540" says - and is always there,	*
;* because a geometry of zero is what made this a disk at all.	*
;****************************************************************

putid:	ld	hl,stg + dl_mod
	ld	a,(hl)
	or	a
	jr	z,pidgeo	;no name: go by the geometry
	ld	hl,stg + dl_mfg
	call	str16		;the maker, when the label has one
	jr	z,pidmod
	ld	hl,strsp
	call	uputs
pidmod:	ld	hl,stg + dl_mod
	call	str16
	ret

pidgeo:	ld	hl,stg + dl_trk
	call	putdec16	;d_tracks
	ld	c,'x'
	call	uout
	ld	hl,stg + dl_hds
	call	putdec16	;d_heads
	ld	c,'x'
	call	uout
	ld	hl,stg + dl_spt
	jp	putdec16	;d_spt

;	the field of sixteen bytes at hl, printed up to its nul.
;	The drive need not have terminated it, so it is bounded
;	here.  Z is set when the field was empty - which is decided
;	by its first byte and not by the nul that ends every field,
;	and is how a disk that does not name itself is told from
;	one that does.
str16:	ld	a,(hl)
	or	a
	ret	z		;the first byte is the terminator
	ld	b,16
st16l:	ld	a,(hl)
	or	a
	jr	z,st16d
	ld	c,a
	call	uout
	inc	hl
	djnz	st16l
st16d:	or	1		;something was printed
	ret

;	the sixteen bit word at hl, in decimal
putdec16:
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	;	fall into putdec

;	print de in decimal.  A digit comes off the right by subtracting
;	ten until the value goes below zero and adding the ten back;
;	the digits go into the buffer backwards and the count of
;	subtractions is what is left to print.
putdec:	ld	hl,numbuf + 7
	ld	(hl),0		;the number ends here and grows down
pdloop:	ld	bc,0		;the quotient, counted up
pdsub:	ld	a,e
	sub	10
	ld	e,a
	ld	a,d
	sbc	a,0
	ld	d,a
	jr	c,pddig		;below zero: this digit is done
	inc	bc
	jr	pdsub
pddig:	ld	a,e
	add	a,10		;the ten that took it below, back
	add	a,'0'
	dec	hl
	ld	(hl),a
	ld	a,b		;whatever is left to print
	or	c
	jr	nz,pdmore
	jp	uputs		;nothing left: hl is the whole number
pdmore:	ld	d,b
	ld	e,c
	jr	pdloop

;****************************************************************
;* how big the disk is, in the same words the kernel's own	*
;* drive probe uses it: tracks * heads * sectors per track, in	*
;* 512 byte blocks, halved eleven times to megabytes.		*
;*								*
;* Heads and sectors per track are read as bytes, because a	*
;* byte is what the kernel's view of a geometry makes them, and	*
;* the megabytes come off the low word of the product - which	*
;* is the truncation the kernel's own cast to an int does, so	*
;* that a disk big enough to reach it reads the same in both	*
;* places.  That takes more than 65535 of them, and nothing	*
;* these cards can drive is anywhere near it.			*
;****************************************************************

putmb:	ld	hl,stg + dl_hds
	ld	e,(hl)
	ld	d,0		;d_heads
	ld	hl,stg + dl_spt
	ld	l,(hl)
	ld	h,0		;d_spt
	call	dmul		;heads * sectors per track
	ld	(hs),hl		;neither is over a byte wide, so
				; this is still one word
	ld	hl,stg + dl_trk
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		;d_tracks
	ex	de,hl
	ld	de,(hs)
	call	dmul		;tracks * heads * sectors per track
	ld	a,11
pmbsh:	srl	b
	rr	c		;the thirty two bit product, halved
	rr	h		; eleven times
	rr	l
	dec	a
	jr	nz,pmbsh
	ex	de,hl
	jp	putdec

;****************************************************************
;* dmul: hl = hl * de, the thirty two bit product landing in	*
;* bc:hl with bc the high word.  The multiplier is parked in	*
;* scratch, since the loop wants all four registers: two for the	*
;* product, two for the multiplicand.  The counter is pushed.	*
;*								*
;* The product shifts up and the multiplier shifts up with it,	*
;* the bit coming out of the multiplier's top being the one to	*
;* add on.  Both operands stay sixteen bits that way - shifting	*
;* the multiplicand instead would lose it above 64k, which is	*
;* most disks.							*
;****************************************************************

dmul:	ld	(mulr),de	;the multiplier
	ex	de,hl		;de is the multiplicand
	ld	hl,0		;and bc:hl the product
	ld	bc,0
	ld	a,16
dmull:	push	af
	add	hl,hl		;the product shifts up first, to
	rl	c		; make room for this round
	rl	b
	ld	a,(mulr)	;and the multiplier shifts up, its
	sla	a		; top bit coming out to be tested
	ld	(mulr),a
	ld	a,(mulr + 1)
	rl	a
	ld	(mulr + 1),a
	jr	nc,dmuln	;that bit was a zero
	add	hl,de		;the product takes the multiplicand
	jr	nc,dmuln
	inc	bc
dmuln:	pop	af
	dec	a
	jr	nz,dmull
	ret

;****************************************************************
;* ask which disk to boot, and answer with its number, 1 to	*
;* ncand.  The entries are already on the screen; this waits	*
;* for a number and takes the first candidate if nothing comes,	*
;* so a machine left alone boots from the first disk that	*
;* answered, and a machine with one disk is never asked.	*
;*								*
;* Input is the monitor's own console driver, the one the menu	*
;* was printed through, so this answers on whichever console	*
;* mon500 found.  The driver does not echo - it is a character	*
;* driver with no idea what a line is - so a key that is taken	*
;* is echoed here, and one that is not is echoed and the	*
;* question asked again.					*
;****************************************************************

tmo	equ	12000		;a delay loop's worth, not a clock:
				; an idle machine waits about five
				; seconds before choosing for itself

menu:	ld	a,(ncand)
	cp	1
	ret	z		;one disk is not a choice

menask:	ld	hl,strpick	;"boot: "
	call	uputs
	ld	de,tmo		;the machine's whole patience,
				; renewed if the answer made no sense
menw:	ld	c,0		;the delay between two looks at
menwl:	dec	c		; the keyboard
	jr	nz,menwl
	push	de
	call	condrv + credy	;Z set if nothing has arrived
	pop	de
	jr	nz,mengot
	dec	de
	ld	a,d
	or	e
	jr	nz,menw
	ld	a,1		;nothing came: the first candidate
	jr	menend

mengot:	call	condrv + crecv	;the key, into a
	push	af
	ld	c,a
	call	uout		;echo it, since the driver will not
	pop	af
	cp	0dh		;a bare return takes the default
	jr	z,menone
	cp	0ah
	jr	z,menone
	sub	'0'		;a number, and one that is there
	jr	c,menbad
	jr	z,menbad	;and not zero
	ld	c,a
	ld	a,(ncand)
	cp	c
	jr	c,menbad
	ld	a,c
menend:	push	af		;leave the cursor on a line of its own
	ld	hl,strnl
	call	uputs
	pop	af
	ret

menone:	ld	a,1
	jr	menend
menbad:	ld	hl,strnl	;said something that is not a disk
	call	uputs
	jr	menask		;so ask again

;****************************************************************
;* pass 2: ask which disk, then re-probe that row and read its	*
;* block 0 into bus page 0 through the alias window - task 0's	*
;* segment 1 points at bus page 0, so the rom's 1100h writes	*
;* the 100h task 1 will run from - and enter it there.		*
;****************************************************************

bootit:
	call	menu		;which disk?  a number in a
	dec	a
	add	a,a		;a candidate is a row, two bytes
	ld	e,a		; apiece
	ld	d,0
	ld	hl,cand
	add	hl,de
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	ex	de,hl		;hl is the chosen row
	call	loadrow
	ld	hl,(cprobe)
	call	jphl
	jp	nc,bootng	;the drive answered twice and then not
	;	The transfer lands on physical 100h-2ffh, which is
	;	task 0's own stack - the monitor's, growing down from
	;	the map image at 200h.  Move the stack into this rom
	;	page, where 512 bytes cannot reach it, before the
	;	transfer starts.  1100h and this page are both in
	;	task 0: one through the window, one through segment 15.
	ld	sp,stktop
	xor	a
	ld	(mapram + 2),a		;seg 1 -> page 0
	ld	(map + 2),a
	ld	hl,win + 0100h		;where a block with no opinion of
	ld	(dest),hl		; its own goes: 100h to task 1
	ld	a,1			;this is the pass with a task 1 to
	ld	(pass2),a		; put a block in, which is what a
					; legacy boot sector needs to know
	ld	hl,(cread)
	call	jphl
	jp	nc,bootng	;it answered a moment ago and now not:
				; the window holds whatever was there
				; before, and entering that is worse
				; than saying so
	ld	a,1			;seg 1 -> page 1, back to identity
	ld	(mapram + 2),a
	ld	(map + 2),a
	ld	a,2			;and seg 2 -> page 2
	ld	(mapram + 4),a
	ld	(map + 4),a
	;	And enter it where the row left it.  The block is in the
	;	window and the window is bus page 0, so the offset into it
	;	is the address task 1 sees - win being 1000h, that is the
	;	low three nibbles of the high byte, and 100h for every row
	;	here, since every medium's block 0 is the program that
	;	runs.  A medium that said otherwise would move (dest), and
	;	nothing but this line would have to know - and one does:
	;	the floppy row moves it to 0080h when the block has no
	;	label, because a medium with no label is a legacy boot
	;	sector and its sector 0 was written to run at 0080h,
	;	which is where the controller's own bootstrap puts it,
	;	and where it then is.
	ld	de,(dest)
	ld	a,d
	and	0fh
	ld	d,a
	ld	a,1			;run it as task 1
	jp	switch
bootng:	ld	hl,strgone	;"boot disk stopped answering"
	call	uputs
	jp	hang

;****************************************************************
;* ide: reset the drive and wait for it to answer ready		*
;****************************************************************

ideprobe:
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
	ld	de,0
ideprb:	call	rdstat			;register 7: status
	and	40h			;DRDY
	jr	nz,idepry
	dec	de
	ld	a,d
	or	e
	jr	nz,ideprb
	or	a			;no drive: carry clear
	ret
idepry:	scf
	ret

;****************************************************************
;* ide: DRDY is already set.  Load the task file, wait for the	*
;* sector, and move it to (dest).				*
;****************************************************************

ideread:
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
	or	a			;the sector never came
	ret
idebrd:	ld	hl,(dest)
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
	scf
	ret

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
;* scsi: reset the bus and select target 0			*
;****************************************************************

ncrprobe:
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
	ld	de,0
ncrprb:	in	a,(scsistat)
	and	scsi_bsy		;the target answered
	jr	nz,ncrpry
	dec	de
	ld	a,d
	or	e
	jr	nz,ncrprb
	or	a			;no target: carry clear
	ret
ncrpry:	scf
	ret

;****************************************************************
;* scsi: BSY is already set.  Finish the select, send READ(10),	*
;* and move the sector to (dest).				*
;****************************************************************

ncrread:
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
	ld	hl,(dest)
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
	scf
	ret

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
;* hd0a: the HD-DMA, which the monitor calls the mw controller.	*
;* The host leaves a command block in memory and points the	*
;* controller at it with three bytes at 0050h, and the board	*
;* dmAs to a physical address and writes a completion status	*
;* back into the block.  Both the block and the transfer are in	*
;* bus page 0, reached through the window: the controller	*
;* cannot see this rom's page, which is not on the bus.		*
;*								*
;* The block's link field points back at the block, so the	*
;* controller re-fetches it after every command and the host	*
;* only rewrites what changes between them.  The transfer goes	*
;* to the staging area, not to (dest) - (dest) is a task 0	*
;* address and may be this rom's own page - and the block is	*
;* copied out of the window afterwards.				*
;****************************************************************

mwprobe:
	call	mwopen			;the constants, and the home the
	ret				; monitor's own boot does with them

mwread:
	call	mwopen
	ret	nc
	ld	hl,-1			;home: all the steps there are,
	ld	(mwstep),hl		; toward track 0
	ld	a,mwnoop
	call	mwkick
	ret	nc
	ld	hl,mwrdtm		;the read: drive 0, head 0,
	ld	de,mwchan		; cylinder 0, sector 0
	ld	bc,mwrdend - mwrdtm
	ldir
	xor	a
	call	mwkick
	ret	nc
	ld	hl,wbuf			;the sector, as the window shows
	ld	de,(dest)		; it, out to where the row wants it
	ld	bc,0200h
	ldir
	scf
	ret

;	point the controller at the block, reset it so it reads the
;	address, and load the drive's constants.  Carry set if the
;	controller answered, which is the whole of the probe: a
;	board that is not there has nothing to write a status byte.
mwopen:	xor	a
	ld	(mapram + 2),a		;seg 1 -> bus page 0, the window
	ld	(map + 2),a
	ld	hl,mwcca
	ld	(hl),mwbase		;0050h holds the block's bus address
	inc	hl			; - little endian, and the third
	ld	(hl),0			; byte is the bank above it
	inc	hl
	ld	(hl),0
	ld	hl,mwtmpl		;the constants, into the block
	ld	de,mwchan
	ld	bc,mwtmend - mwtmpl
	ldir
	xor	a
	out	(mwrst),a		;the controller reads 0050h here
	ld	d,0
mwsdl:	dec	d			;and settles
	jr	nz,mwsdl
	ld	a,mwconst
	jp	mwkick

;	run the command in A and wait for it.  Carry set if it
;	completed.  Zero in the status byte is what says the
;	controller is still working on it, so the kick clears it.
mwkick:	ld	(mwcmnd),a
	xor	a
	ld	(mwstat),a
	out	(mwattn),a		;run it
	ld	hl,mwstat
	ld	de,0
mwkickw:ld	a,(hl)
	or	a
	jr	nz,mwkickd
	dec	de
	ld	a,d
	or	e
	jr	nz,mwkickw
	or	a			;the controller never answered
	ret
mwkickd:cp	mwgood
	jr	nz,mwkickb
	scf
	ret
mwkickb:or	a
	ret

;	Load the drive's constants, stepping toward track 0.
mwtmpl:	db	mwstepo		;step toward track 0
	db	0		;step count, low
	db	0		;step count, high
	db	mwldhd		;drive 0, head 0, loading constants
	db	0		;dma address, low
	db	0		;dma address, high
	db	0		;dma address, extended
	db	0		;argument 0
	db	mwstpdl		;argument 1: the step rate
	db	mwhdsl		;argument 2: the head settle
	db	mwsecsz		;argument 3: 512 byte sectors
	db	mwconst		;load the constants
	db	0		;the completion status
	db	mwbase		;the link: this block again, at its bus
	db	0		; address and not at the window's
	db	0
mwtmend	equ	$

;	Read one sector into the staging area.  No step and no seek:
;	the head is where the block before this one left it, which
;	for block 0 is track 0.  Thirteen bytes of the block, so the
;	link the constants left at +13 survives.
mwrdtm:	db	0		;no step
	db	0
	db	0
	db	mwhd0		;drive 0, head 0, high current
	db	wbase & 0ffh	;the dma address: the staging area in
	db	wbase >> 8	; bus page 0
	db	0		;and the bank above it
	db	0		;cylinder, low
	db	0		;cylinder, high
	db	0		;head 0
	db	0		;sector 0
	db	0		;read
	db	0		;the completion status
mwrdend	equ	$

;****************************************************************
;* hda: Morrow's HDCA winchester board.  Port mapped - four	*
;* registers carry everything, the host reads the sector through	*
;* the data port, and there is no command block in memory for a	*
;* dma board to fetch, so nothing here goes near the window.	*
;*								*
;* A read stores the sector at offset 2 of the controller's own	*
;* data ram, with its last two bytes wrapped to the front, so	*
;* two bytes are read and dropped before the 512 that are the	*
;* sector in order.  The header the read matches on goes into	*
;* the header area first, and the hdca's formatter numbers a	*
;* track's first sector 1, so block 0 is sector 1.		*
;****************************************************************

hdaprobe:
	ld	a,hdctl			;the front end, running, on the
	out	(hdcont),a		; drive clock, writes enabled
	ld	a,hdrest		;drive 0, head 0
	out	(hdfunc),a
	ld	de,0
hdaprl:	in	a,(hdcont)		;primary status
	and	hdnrdy			;which a ready drive clears
	jr	z,hdapry
	dec	de
	ld	a,d
	or	e
	jr	nz,hdaprl
	or	a			;no board, or nothing ready
	ret
hdapry:	scf
	ret

hdaread:
	ld	a,hdramhd		;the header the read matches on:
	out	(hdcmd),a		; the pointer to the header area
	xor	a
	out	(hddata),a		;head 0
	out	(hddata),a		;track 0
	ld	a,1
	out	(hddata),a		;sector 1, the first of the track
	xor	a
	out	(hddata),a		;the key
	ld	a,hdrsec
	out	(hdcmd),a		;read it
	ld	de,0
hdadrl:	in	a,(hdcont)		;primary status
	and	hdopdn			;the operation finished
	jr	nz,hdadrd
	dec	de
	ld	a,d
	or	e
	jr	nz,hdadrl
	or	a			;it never finished
	ret
hdadrd:	in	a,(hdcont)		;a timeout is a failure, and so
	ld	c,a			; is a drive that has faulted
	and	hdnout
	jr	nz,hdadbad
	ld	a,c
	and	hdnfault
	jr	z,hdadbad
	in	a,(hdcmd)		;and so is a data crc, from the
	and	hdcrc			; secondary status
	jr	nz,hdadbad
	ld	a,hdramdt		;back to the data area
	out	(hdcmd),a
	in	a,(hddata)		;the sector is stored two bytes
	in	a,(hddata)		; in, so drop the first two
	ld	hl,(dest)
	ld	b,0			;256 words
hdadlp:	in	a,(hddata)
	ld	(hl),a
	inc	hl
	djnz	hdadlp
	scf
	ret
hdadbad:or	a			;a read that faulted is no disk
	ret

;****************************************************************
;* fla: the DJ-DMA floppy controller, the one with a z80 of its	*
;* own running a channel program out of the host's memory.  The	*
;* program sits at the controller's default channel address, in	*
;* bus page 0, and the status byte the controller writes back	*
;* into it is at 0058h.  The transfer goes to the staging area	*
;* and is copied out of the window, for the same reason the	*
;* hd-dma's is: the board dmAs to a bus address.		*
;*								*
;* A diskette's block 0 is 512 bytes with a label in its second	*
;* half, and what it is made of is the medium's to say: which	*
;* sector of the medium the label lands in is the same thing as	*
;* how many sectors make the block.  An eight inch drive's	*
;* cylinder 0 head 0 is single density - fm, 26 sectors of 128	*
;* bytes numbered from one - so a block there is the first four	*
;* of them and the label is four sectors in.  A cylinder 0 with	*
;* 512 byte sectors carries the label in its first one, and the	*
;* block is that one sector.  The media this tree builds are	*
;* that shape on every track: 128 byte sectors hold too little	*
;* to be worth a filesystem, and a five inch medium is one	*
;* density and one sector size all the way down.			*
;*								*
;* Which shape a medium is is written down nowhere, and it is	*
;* what has to be known to find the label, so the block is read	*
;* by trying: one sector first, and if the label is not in it,	*
;* three more at 128 byte steps, which is the block an fm	*
;* cylinder 0 carries its label in.  The controller reads a	*
;* whole sector whatever this rom expects, so the first attempt	*
;* is the whole block on 512 byte sectors and only the block's	*
;* first sector on 128, and neither attempt reads a sector the	*
;* medium does not have.						*
;*								*
;* A medium numbers its sectors from 1 if it is soft sectored	*
;* and from 0 if it is hard sectored, and that the drive says:	*
;* the controller's sense command answers with the drive's	*
;* characteristics byte, the same bit the kernel reads for the	*
;* same reason (sys/dj.c).  So a hard sectored medium is read	*
;* from its sector 0 like any other.				*
;*								*
;* Which of the two a drive is comes from the drive number,	*
;* because that is the port it hangs off: the board's drives 0	*
;* to 3 are the eight inch ones and 4 to 7 the five inch ones	*
;* (DJDMA manual 2.6.7).  The number is the channel program's	*
;* own drive byte, and level 2 inherits it from here.		*
;****************************************************************

;	Which drive is the boot one.  All eight are asked for their
;	block 0 - a board with no diskette in any drive answers none
;	of them, since a drive with nothing in it cannot read a
;	sector.  A drive holding a labelled diskette is the one, and
;	the asking stops there.  A drive that answers with no label
;	in the block is holding a legacy boot sector, which boots on
;	its own terms: the first of those is kept in case no drive
;	has a label, and only then is it the one.  A label always
;	wins, since it is the medium that says how it is read.
djprobe:
	xor	a
	ld	(djdrv),a		;from drive 0
	ld	(djleg),a		;and no legacy diskette yet
djpdlp:	call	djblock
	jr	nc,djpdnx		;nothing in this drive
	ld	a,(legac)
	or	a
	jr	nz,djpleg		;it answered without a label
	scf				;a labelled diskette: this is it
	ret
djpleg:	ld	a,(djleg)		;an unlabelled one: remember the
	or	a			; first that answered, in case no
	jr	nz,djpdnx		; drive has a label.  Zero means
	ld	a,(djdrv)		; none, so it is stored one higher
	inc	a
	ld	(djleg),a
djpdnx:	ld	a,(djdrv)
	inc	a
	ld	(djdrv),a
	cp	8			;and that was all eight of them
	jr	c,djpdlp
	ld	a,(djleg)		;no label anywhere: the legacy one,
	or	a			; if there was one, is the boot one
	ret	z			; - and carry clear if there was not
	dec	a
	ld	(djdrv),a
	ld	a,1
	ld	(legac),a		;and it is a legacy boot sector, so
	scf				; the block says where it runs
	ret

;	the chosen drive's block 0, out to where it is wanted.  Pass 2
;	asks again rather than trusting what pass 1 read: the diskette
;	may have been changed, and a read that fails now must not enter
;	whatever the staging area still holds.
;
;	And a block with no label in it is not ours: it is the
;	medium's own boot sector, written to run where the controller
;	put it, which for a diskette is the bottom of the page.
;	Pass 1 stages it with the labelled ones; pass 2, the one
;	with a task 1 to load for, moves it down to where it says
;	it runs.
djread:
	call	djblock
	ret	nc
	ld	hl,wbuf			;the block is here: the staging
	ld	de,(dest)		; area, and it is wanted there
	ld	bc,0200h		;the block, when the block is ours
	ld	a,(legac)
	or	a
	jr	z,djrcp			;a labelled block goes where the
	ld	a,(pass2)		; row said.  A legacy one is the
	or	a			; medium's own boot sector, and
	jr	z,djrcp			; the controller's bootstrap is
					; what says where that runs: it
					; reads the lowest sector of
					; cylinder 0 to 0080h and jumps
					; there, so a medium with no
					; label wrote its sector 0 to run
					; at 0080h, and only in pass 2,
					; where that memory is task 1's
	ld	a,(djfirst)
	call	djone			;the block is one sector of the
	ret	nc			; medium here, so read that one
					; again: the shape that found no
					; label in it may have made the
					; block up out of four
	call	djrenum			;the medium is entered on the
					; numbering the controller's own
					; bootstrap leaves behind, so the
					; drive its boot sector asks for
					; by number is the one it came
					; from
	ld	a,(djdrv)		;and it is copied at the length
	cp	4			; the controller reads a sector
	jr	c,djrfm			; of there: one fm sector on an
	call	djlen			; eight inch drive, whose cylinder
	ld	b,h			; 0 is fm whatever the rest of the
	ld	c,l			; medium is, and the drive's own
	jr	djrsrc			; sector length on a five inch one,
					; which is one length all the way
					; down - all sixteen bits of it,
					; since a 512 byte sector's low
djrfm:	ld	bc,128			; byte is zero and the low half
					; alone says nothing
djrsrc:	ld	hl,wbuf			;and the block runs at 0080h, not
	ld	de,win + 80h		; where the staging address says:
	ld	(dest),de		; the controller's bootstrap read
					; the lowest sector of cylinder 0
					; to 0080h and jumped there, so a
					; medium with no label wrote its
					; sector 0 to run there, and only
					; in pass 2, where that memory is
					; task 1's.  Source and address
					; are both set here because djlen
					; answers in hl and djrenum runs
					; the channel, and neither leaves
					; the other's registers alone
djrcp:	ldir				;from the window down to it
	scf
	ret

;	Read block 0 of the diskette in (djdrv) into the staging area,
;	and say whether the drive answered: carry set means it did, and
;	(legac) says whether what came back has a label in it - a block
;	with one is ours and says how it is read, and one without is
;	the medium's own boot sector.
;
;	The block is 512 bytes with the label at 256, and one sector is
;	read first, from the number the drive says its sectors start at.
;	A medium of 512 byte sectors or more is answered by that one
;	read: the whole block is there and so is the label.  A medium
;	of 128 byte sectors - an eight inch drive's fm cylinder 0, and
;	any medium whose cylinder 0 is short of a block - is not, so
;	the three sectors after the first make up the rest of the block
;	and the label is four sectors in.  A label in either shape is
;	what says the shape was the right one, and a block with no label
;	in it is the medium's own: a program, which says where it runs,
;	or the fill of an unwritten track, which is neither.
djblock:
	call	djask			;the sector the drive's medium
	call	djopen			; starts at, and the window
	ld	hl,wbase + 128		;a read shorter than the block
	ld	de,wbase + 129		; leaves the rest of the staging
	ld	bc,383			; area as it was, which is the
	ld	(hl),0			; last drive's block, label and
	ldir				; all - so the label's own place
					; is cleared before the read that
					; looks for it there
	ld	a,(djfirst)
	call	djone			;one sector, the medium's first
	ret	nc			;the drive did not answer
	call	djlab			;the label, in that one sector
	ret	c			;it is there: a block of one
	ld	a,(djfirst)		;it is not: the medium's sectors
	inc	a			; are shorter than its block, so
	ld	b,a			; the block is four of them.  The
	ld	hl,wbase + 128		; first is already where it goes
	ld	c,3
	call	djfm			;read the other three
	ret	nc			;the drive stopped answering
	ld	hl,wbase
	call	djlab			;the label, in the whole block
	ret	c			;it is there: a block of four
	call	djcpm			;no label: a program or the fill
	ret	nc			; of an unwritten track, which is
	ld	a,1			; not to be entered.  A program
	ld	(legac),a		; is the medium's own boot sector
	scf
	ret

;	Read one sector into the staging area, from the sector in A,
;	and say whether the drive answered: carry set means it did.
djone:	ld	hl,wbase
	ld	(djadr),hl
	jp	djrun

;	The three sectors after the medium's first one, into the rest
;	of the block: the shape an eight inch drive's fm cylinder 0 has,
;	128 bytes to a sector, four to a block.  B is the next sector
;	number, HL where it goes and C how many are left.  Carry set
;	if every one of them answered.
djfm:	push	hl
	ld	(djadr),hl		;where this one goes
	ld	a,b
	call	djrun			;clobbers a, hl and de, not bc
	pop	hl
	ret	nc
	inc	b			;the next sector of the track
	ld	de,128
	add	hl,de			; and the next 128 bytes of the block
	dec	c
	jr	nz,djfm
	scf
	ret

;	Ask the drive what its medium is.  The controller answers with
;	three bytes after the command - the drive's characteristics, the
;	sector length it reads, and the drive's status - and the first of
;	them is the one that has to be known before a sector can be
;	named: its hard sectored bit says whether the medium numbers its
;	sectors from one or from zero.  The kernel asks the same question
;	and reads the same bit for the same reason; see sys/dj.c.
;
;	The sector length is kept as well.  A block is 512 bytes whatever
;	the medium's sectors are, so it is not what the block is read
;	at; it is what a legacy five inch block is copied at, since the
;	controller's own bootstrap reads one sector of that length to
;	0080h.
djask:	ld	hl,djstmp
	ld	de,djchan
	ld	bc,djstme - djstmp
	ldir
	ld	a,(djdrv)
	ld	(djchan + 1),a		;the drive to ask about
	ld	a,1
	ld	(djfirst),a		;sector 1, which is what a soft
	xor	a			; sectored medium numbers from -
	ld	(djsecl),a		; and what no answer at all
	ld	(djchan + 5),a		; leaves behind, since the read
	out	(djattn),a		; that follows is the one that
	ld	hl,djchan + 5		; reports whether there is a
	ld	de,0			; medium in there at all
djsnw:	ld	a,(hl)
	or	a
	jr	nz,djsnd		;the controller answered
	dec	de
	ld	a,d
	or	e
	jr	nz,djsnw
	ret				;it never did
djsnd:	cp	djok
	ret	nz			;an error is the read's to report
	ld	a,(djchan + 2)		;the characteristics byte: hard
	and	djhard			; sectored is numbered from zero
	jr	z,djsnl
	xor	a
	ld	(djfirst),a
djsnl:	ld	a,(djchan + 3)		;and the sector length, which is
	and	djslc			; the length the medium's own
	ld	(djsecl),a		; boot sector is written in
	ret

;	the window, and the channel program in it.  The program's
;	address, sector and drive bytes are written per read.
djopen:	xor	a
	ld	(mapram + 2),a		;seg 1 -> bus page 0, the window
	ld	(map + 2),a
	ld	hl,wbase		;where a sector goes unless the
	ld	(djadr),hl		; caller says otherwise
	ld	hl,djtmpl
	ld	de,djchan
	ld	bc,djtmend - djtmpl
	ldir
	ret

;	run the channel for the sector in A, to the dma address in
;	(djadr) and on the drive in (djdrv).  Carry set if the
;	controller answered with a good status, and then that sector
;	is at that address.
djrun:	ld	(djsec),a		;the sector on the track, head 0
	ld	hl,(djadr)
	ld	a,l
	ld	(djchan + 1),a
	ld	a,h
	ld	(djchan + 2),a		; - the address, low byte first
	ld	a,(djdrv)
	ld	(djchan + 7),a		;and the drive to read it from
	xor	a
	ld	(djchan + 3),a		;the bank above it: a flat
	ld	(djstat),a		; address.  Zero says we are
	out	(djattn),a		; waiting for the sector, and
	ld	hl,djstat		; running the channel is what
	ld	de,0			; makes the controller answer
djrunw:	ld	a,(hl)
	or	a
	jr	nz,djrund
	dec	de
	ld	a,d
	or	e
	jr	nz,djrunw
	or	a			;the controller never answered
	ret
djrund:	cp	djok
	jr	nz,djrunb
	scf
	ret
djrunb:	or	a
	ret

;	Put the drive numbering where the controller's own bootstrap
;	leaves it.  A legacy medium is written to run on the machine
;	that bootstrap makes: it runs LOGCAL (DJ49.MAC:898) after it
;	reads the sector, and the port that answered becomes logical 0
;	to 3, which is why a medium's own boot sector asks for logical
;	drive 0 - the drive it came from, whichever port that is.  A
;	boot through this rom is the same boot as far as the medium can
;	tell, so it is entered on the same numbering.
;
;	The command is a channel program at the default channel address
;	like any other, with the halt that ends the walk written in after
;	it - an unknown code does not advance the channel, so a program
;	that ends without one never ends.  The window djopen opened is
;	still the one it goes through.  It is a channel program and a
;	wait, so what it leaves alone is nothing: a, hl, de and bc all
;	belong to it.
djrenum:ld	hl,djnum
	ld	de,djchan
	ld	bc,djnumend - djnum
	ldir				;the command, at the channel
	ld	a,djfive		; address.  Its parameter is the
	ld	(djchan + 1),a		; port the medium came from: bit 2
	ld	a,(djdrv)		; set makes the five inch port
	cp	4			; logical 0 to 3, and clear makes the
	jr	nc,djrne		; eight inch one logical 0 to 3,
	xor	a			; which is all LOGCAL does
	ld	(djchan + 1),a
djrne:	xor	a
	ld	(djchan + 4),a		;the halt's status, cleared first:
	out	(djattn),a		; running the channel is what sets it
	ld	hl,djchan + 4
	ld	de,0
djrnw:	ld	a,(hl)
	or	a
	jr	nz,djrnx
	dec	de
	ld	a,d
	or	e
	jr	nz,djrnw
djrnx:	ret

;	the renumber program: set the numbering, then halt.
djnum:	db	djlset			;set the logical drive numbering
	db	0			; - the port, written in per call
	db	0			;its status
	db	djhlt			;end the channel program
	db	0			;its status
djnumend equ	$

;	the label, in the second half of the block in the staging area.
;	Carry set if it is there, and then the block is this shape; carry
;	clear if it is not, and then the read was still a good one and
;	the block may yet be another shape's.
djlab:	ld	hl,wbuf + dl_off
	ld	de,dlmagic
	ld	b,4
djlabl:	ld	a,(de)
	cp	(hl)
	jr	nz,djlabn
	inc	hl
	inc	de
	djnz	djlabl
	scf
	ret
djlabn:	xor	a
	ret

;	the sector length a drive reads, from the code the controller
;	reports: 128 bytes, doubled once per step of the code.  The
;	answer is in hl, and hl is not the caller's to keep: a caller
;	whose source is in hl sets it again after the call.
djlen:	ld	hl,128
	ld	a,(djsecl)
	and	djslc
	ret	z
	ld	b,a
djlenl:	add	hl,hl
	djnz	djlenl
	ret

;	A block with no label in it is a legacy boot sector only if it
;	has a program in it.  An unwritten track is one byte repeated -
;	the formatter's fill - and that is not something to enter, so
;	the drive is taken to have answered no.  Carry set if the first
;	128 bytes - the boot sector of any CP/M medium we boot, whatever
;	its sector size - hold more than one value.
djcpm:	ld	hl,wbuf
	ld	b,128
	ld	a,(hl)
djcpml:	inc	hl
	cp	(hl)
	jr	nz,djcpmg
	djnz	djcpml
	or	a			;all the same: not a program
	ret
djcpmg:	scf
	ret

;	the channel program: setdma, sread, halt.  The address, the
;	sector and the drive are written into it per read, and the
;	program stays here for level 2 to find.
djtmpl:	db	djsdma		;set the 24 bit dma address
	db	0		; - the address, set by the read
	db	0
	db	0		; - and the bank above it
	db	djsread		;read one sector
	db	0		;cylinder 0, the boot track
	db	0		;sector, set by the read
	db	0		;drive, set by the open
	db	0		;the status, cleared before each run
	db	djhlt		;end the channel program
	db	0		;its status
djtmend	equ	$

;	the sense program: sense drive status, halt.  The drive byte is
;	written in per ask, and the controller writes its three answer
;	bytes over the three spares after that - the characteristics
;	byte, the sector length and the drive's status - with the
;	command's own status after them, at +5.
djstmp:	db	djsense		;sense drive status
	db	0		;the drive, written in per ask
	db	0		;the characteristics byte
	db	0		;the sector length
	db	0		;the drive's status
	db	0		;the status, cleared before the run
	db	djhlt		;end the channel program
	db	0		;its status
djstme	equ	$

;****************************************************************
;* d1a: the CompuPro DISK 1A's 8 inch bank.  The 8272 owns the	*
;* head and moves the sector itself, to the address the board's	*
;* dma register holds - so the block is staged in bus page 0 and	*
;* copied out of the window, as the hd-dma's and the floppy's	*
;* are, and the command is written to the chip and its result	*
;* read back a byte at a time.					*
;*								*
;* Only drive 0 is probed: a row is one /dev name, and this one	*
;* is d1a.  The 5 1/4 inch bank is four more drives behind the	*
;* same chip, and would be four more rows - d1e to d1h -	*
;* differing only in the drive select register's data rate bit	*
;* and the unit the command names.				*
;****************************************************************

d1probe:
	call	d1open
	ld	a,d1recal
	call	d1put
	ret	nc
	xor	a
	call	d1put			;recalibrate unit 0, so the head
	ret	nc			; steps out to track 0
	call	d1wait			;and the chip answers when it
	ret	nc			; has arrived
	ld	a,d1sensei
	call	d1put			;sense interrupt status takes that
	ret	nc			; answer back, two bytes of it
	call	d1get
	ret	nc
	and	d1st0m			;a recalibrate that ended is the
	cp	d1se			; only answer this row will take
	jr	nz,d1prb
	call	d1get			;and the cylinder it landed on
	ret
d1prb:	call	d1get			;read that out too, so the chip is
	or	a			; not left holding a result
	ret

d1read:
	call	d1open
	xor	a			;the dma address, three bytes
	out	(d1dma),a		; most significant first: nothing
	ld	a,wbase >> 8		; above the first 64k, and the
	out	(d1dma),a		; staging area's bus address in
	ld	a,wbase & 0ffh		; the two below it
	out	(d1dma),a
	ld	hl,d1cmdt		;then the command, nine bytes
	ld	b,d1cmdlen		; through the data register
d1rdl:	ld	a,(hl)
	call	d1put
	ret	nc
	inc	hl
	djnz	d1rdl
	call	d1wait			;the chip reads the sector now
	ret	nc
	ld	hl,d1res		;and answers with seven bytes:
	ld	b,d1reslen		; three statuses and where it
d1resl:	call	d1get			; landed
	ret	nc
	ld	(hl),a
	inc	hl
	djnz	d1resl
	ld	a,(d1res)
	and	d1errm			;a finish that was not normal
	jr	nz,d1bad
	ld	a,(d1res + 1)
	and	d1err1
	jr	nz,d1bad
	ld	a,(d1res + 2)
	and	d1err2
	jr	nz,d1bad
	ld	hl,wbuf			;the block, as the window shows
	ld	de,(dest)		; it, out to where the row put it
	ld	bc,0200h
	ldir
	scf
	ret
d1bad:	or	a			;a read that faulted is no disk
	ret

;	the chip and the board around it, into a state this row can
;	command: drive 0's motor turning, the boot eprom kept alive
;	- a zero where it lives disables it until a system reset -
;	and the 8 inch bank's data rate
d1open:	ld	a,d1boot + d1m0
	out	(d1motor),a
	xor	a
	out	(d1dsel),a
	ret

;	write A to the chip, and say whether it took it
d1put:	push	bc
	push	af
	ld	c,d1rqm			;a byte goes when the register is
	call	d1pollw			; ready and the chip is not sending
	jr	nc,d1putf
	pop	af
	out	(d1data),a
	pop	bc
	scf
	ret
d1putf:	pop	af
	pop	bc
	or	a
	ret

;	read the chip's next byte into A, and say whether one came
d1get:	push	bc
	ld	c,d1rqm + d1dio		;and one comes when the register is
	call	d1pollw			; ready and the chip is sending
	jr	nc,d1getf
	in	a,(d1data)
	pop	bc
	scf
	ret
d1getf:	pop	bc
	or	a
	ret

;	wait for the chip's interrupt, which is how a command with no
;	result phase says it is done.  Carry set if it came.
d1wait:	ld	de,0
d1wtl:	in	a,(d1stat)
	and	d1int
	jr	nz,d1wty
	dec	de
	ld	a,d
	or	e
	jr	nz,d1wtl
	or	a			;the chip never answered
	ret
d1wty:	scf
	ret

;	wait for the two main status bits C names, and say whether
;	they came.  A chip that stops answering part way through a
;	command is a fault rather than a wait, so the loop is bounded.
d1pollw:ld	de,0
d1plw:	in	a,(d1msr)
	and	d1rqm + d1dio
	cp	c
	jr	z,d1plwy
	dec	de
	ld	a,d
	or	e
	jr	nz,d1plw
	or	a
	ret
d1plwy:	scf
	ret

;	a one sector read: the opcode with double density set, then the
;	unit and head, the cylinder, the head, the sector, the size
;	code, the last sector on the track, the gap length, and the
;	length - which the chip reads only when the size code is zero
d1cmdt:	db	d1rdcmd
	db	0			;unit 0, head 0
	db	0			;cylinder 0
	db	0			;head 0
	db	1			;sector 1, which is where both this
					; board and the hdca's formatter
					; put a track's first sector
	db	d1n
	db	d1eot
	db	d1gpl
	db	0ffh
d1cmdlen equ	$-d1cmdt
d1reslen equ	7

;****************************************************************
;* d2a: the CompuPro DISK 2.  A hard disk board that is not	*
;* intelligent: no dma engine of its own - the transfer goes	*
;* through the SELECTOR CHANNEL, a board of its own that takes	*
;* the bus and generates the address - and no seek, so the host	*
;* steps the head itself, one pulse per cylinder, and the	*
;* drive's track 0 line is the only position it has.  So this	*
;* row homes the drive before it reads.  Only drive 0 is		*
;* probed, so this row is d2a.					*
;****************************************************************

d2probe:
	call	d2home			;which selects the drive too
	ld	a,d2attn + d2run	;the null command only waits for a
	out	(d2ctl),a		; seek to complete and times out
	call	d2wait			; after two index pulses, so a
	ret	nc			; board with no drive never
	jp	d2done			; finishes and one whose drive
					; is not ready says so

d2read:
	call	d2home
	in	a,(d2sel)		;reading the channel disables it,
	xor	a			; which is what resets the count
	out	(d2sel),a		; of bytes it has taken.  Then
	ld	a,wbase >> 8		; the address, the extended byte
	out	(d2sel),a		; first and a flat 64k one, which
	ld	a,wbase & 0ffh		; is the staging area
	out	(d2sel),a
	ld	a,d2wrmem + d2scid	;and the mode: a disk read writes
	out	(d2sel),a		; memory, and the low nibble is
					; this board's priority
	ld	a,d2attn + d2regcyl
	out	(d2ctl),a
	xor	a
	out	(d2data),a		;cylinder 0
	ld	a,d2attn + d2reghed
	out	(d2ctl),a
	xor	a
	out	(d2data),a		;head 0
	ld	a,d2attn + d2regsec
	out	(d2ctl),a
	ld	a,1
	out	(d2data),a		;sector 1, the first of the track
	ld	a,d2attn + d2run + d2rdcmd
	out	(d2ctl),a		;and read it
	call	d2wait
	ret	nc
	call	d2done
	ret	nc
	ld	hl,wbuf			;the block, as the window shows
	ld	de,(dest)		; it, out to where the row put it
	ld	bc,0200h
	ldir
	scf
	ret

;	the drive register, and the head stepped out to track 0.  A
;	drive that never reports track 0 costs as many pulses as a
;	byte can count, and no more.
d2home:	call	d2open
	ld	a,d2attn + d2op2
	out	(d2ctl),a		;step out, toward track 0
	ld	b,0
d2hml:	in	a,(d2ctl)
	and	d2trk0
	jr	z,d2hmd			;the drive says it is there
	in	a,(d2data)		;one step pulse
	djnz	d2hml
d2hmd:	xor	a
	ret

d2open:	ld	a,d2attn + d2regdrv
	out	(d2ctl),a		;the drive and head register
	ld	a,d2unit		; - drive 0 in the high nibble,
	out	(d2data),a		; head 0 in the low
	ret

;	wait for the board to finish, which is its attention line
;	falling.  Carry set if it did.
d2wait:	ld	de,0
d2wtl:	in	a,(d2ctl)
	and	d2attn
	jr	z,d2wty
	dec	de
	ld	a,d
	or	e
	jr	nz,d2wtl
	or	a			;the board never finished
	ret
d2wty:	scf
	ret

;	the status it left, and whether every error bit is clear
d2done:	in	a,(d2ctl)
	and	d2errm
	jr	nz,d2bad
	scf
	ret
d2bad:	or	a			;a faulted transfer is no disk
	ret

;****************************************************************
;* d3a: the CompuPro DISK 3, an intelligent board with an 8085	*
;* of its own.  The host leaves a sixteen byte iopb in memory	*
;* and the board fetches it, so the block, the parameter table	*
;* it names and the sector it moves all have to be at addresses	*
;* on the s-100 bus - and the window is the only bus memory	*
;* this rom can write.  Every command is an attention, and the	*
;* answer is the status byte the board writes back into the	*
;* block.							*
;*								*
;* The first fetch after a reset comes from 0050h and only its	*
;* link bytes are kept, so bring up is that link, the reset,	*
;* and two attentions.  Only drive 0 is probed, so this row is	*
;* d3a.								*
;****************************************************************

d3probe:
	call	d3open			;the block, the reset, and the
	jp	d3fire			; attentions after it.  The block
					; is a no op, so that is the
					; whole tickle

d3read:
	call	d3open
	ld	a,d3global		;the mode: absolute sector
	ld	(d3cmnd),a		; numbers, which is the one mode
	ld	a,d3abs			; that need not know how the
	ld	(d3arg),a		; drive is built
	call	d3fire
	ret	nc
	ld	a,d3specify		;and the parameter table, which
	ld	(d3cmnd),a		; the board will not read a
	ld	hl,d3pbus		; sector without: it is what says
	call	d3fire			; how a sector is made
	ret	nc
	ld	a,d3rwcmd
	ld	(d3cmnd),a
	ld	a,d3rdcmd
	ld	(d3arg),a		;arg1: read
	xor	a
	ld	(d3arg + 1),a		;arg2 to arg5: absolute sector 0,
	ld	(d3arg + 2),a		; so cylinder 0, head 0 and
	ld	(d3arg + 3),a		; sector 0 whatever shape the
	ld	(d3arg + 4),a		; table calls the drive
	ld	a,1
	ld	(d3arg + 5),a		;arg6: one sector
	xor	a
	ld	(d3arg + 6),a		;arg7
	ld	hl,wbase
	call	d3fire
	ret	nc
	ld	hl,wbuf			;the block, as the window shows
	ld	de,(dest)		; it, out to where the row put it
	ld	bc,0200h
	ldir
	scf
	ret

;	bring the board up: the block, the parameter table, the link
;	bytes, and the two attentions that follow a reset.  The first
;	leaves the board holding the block's bus address, the second
;	runs the block there.
d3open:	xor	a
	ld	(mapram + 2),a		;seg 1 -> bus page 0, the window
	ld	(map + 2),a
	ld	hl,d3tpl
	ld	de,d3iopb
	ld	bc,d3tmplen
	ldir
	ld	hl,d3ptpl
	ld	de,d3param
	ld	bc,d3ptmplen
	ldir
	ld	a,d3bus & 0ffh		;the block's own bus address, in
	ld	(d3lnkb),a		; the link bytes the board reads
	xor	a			; after a reset
	ld	(d3lnkb + 1),a
	ld	(d3lnkb + 2),a
	ld	a,d3rst			;hold the 8085 down ...
	out	(d3attn),a
	xor	a
	out	(d3attn),a		; ... and let it go: this attention
	out	(d3attn),a		; fetches, and this one runs
	ret

;	run the block, with the dma address in HL.  Carry set if the
;	board finished it and the status it wrote back says it worked.
d3fire:	call	d3addr
	xor	a
	ld	(d3stat),a		;zero says we are waiting for it
	out	(d3attn),a
	ld	hl,d3stat
	ld	de,0
d3fw:	ld	a,(hl)
	or	a
	jr	nz,d3fwd
	dec	de
	ld	a,d
	or	e
	jr	nz,d3fw
	or	a			;the board never answered
	ret
d3fwd:	cp	d3ok
	jr	nz,d3fbad
	scf
	ret
d3fbad:	or	a			;it answered with an error
	ret

;	the block's dma address, from HL.  Three bytes and flat: a
;	bus address below 64k has nothing above it.
d3addr:	ld	a,l
	ld	(d3data),a
	ld	a,h
	ld	(d3data + 1),a
	xor	a
	ld	(d3data + 2),a
	ret

;	the block, and the parameter table.  The link is the block's
;	own bus address, so every attention after the first runs this
;	block again - which is what makes one attention a command.
;	The table's geometry is a placeholder: block 0 is cylinder 0,
;	head 0, sector 0 under any geometry, and the level 2 boot
;	loads the drive's real one from the label.
d3tpl:	db	d3noop			;the opcode
	db	0			;the status, cleared before
	db	0			; every run.  Drive 0
	db	0,0,0,0,0,0,0		;arg1 to arg7
	db	wbase & 0ffh		;the dma address: the staging
	db	wbase >> 8		; area, in bus page 0
	db	0
	db	d3bus & 0ffh		;the link: this block, at its bus
	db	d3bus >> 8		; address and not at the window's
	db	0
d3tmplen equ	$-d3tpl

d3ptpl:	dw	0			;step rate, in 100 microsecond
	dw	0			; units, and head settle
	dw	512			;bytes a sector
	dw	32			;sectors a track
	dw	4			;heads a cylinder
	dw	0			;cylinders: not this rom's
	dw	0			; business, and neither is
	dw	0			; what follows
	dw	0
	dw	0
	dw	0			;position, which must be zero
d3ptmplen equ	$-d3ptpl

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

; The character is already in C, which is what the driver wants, so this
; is a jump and not a call.  The driver destroys A and the flags - both
; already dead here - and nothing else, so no caller saves anything.
uout:	jp	condrv + csend

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
	ld	(hl),task & 0ffh
	inc	hl
	ld	(hl),task >> 8
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
	ld	(ctask),a		;the trap handler reads this to know
					; the trap came from task 1, not 0
	jp	gobuff

jphl:	jp	(hl)			;through a table slot

;****************************************************************
;* Scratch, in this rom's own page.  The page is ram at physical	*
;* 0f000h and task 0 reaches it through segment 15, so it needs	*
;* no map games - and it is off the s-100 bus, unlike bus page	*
;* 0, which is where task 1's 100h lives and what the alias	*
;* window is for.						*
;****************************************************************

stg:	ds	512		;block 0, on its way to the label
	ds	64		;the stack the final transfer runs on
stktop:	ds	0
decbuf:	ds	8		;the candidate number, as text
numbuf:	ds	8		;a decimal number, at most five digits
hs:	ds	2		;heads * sectors per track, on the way
mulr:	ds	2		;the multiplier, shifting right
cur:	ds	2		;the table cursor
chead:	ds	2		;where the next candidate goes
ncand:	ds	1		;how many answered
cand:	ds	2 * 8		;their table rows, in order
dest:	ds	2		;where a read routine puts block 0
djadr:	ds	2		;where the dj-dma's next sector goes
djdrv:	ds	1		;and the drive it comes from
djfirst: ds	1		;the number that drive's medium
				; numbers its first sector from: 1 for
				; a soft sectored one, 0 for a hard
djsecl:	ds	1		;and the sector length code the drive
				; reports, for a legacy block's copy
djleg:	ds	1		;the first drive that answered with
				; no label, one higher than its number
				; since zero means there was none
legac:	ds	1		;the block just read has no label in
				; it, so it is the medium's own sector
				; 0 and says where it runs
pass2:	ds	1		;pass 2 is the one with a task 1
cname:	ds	2		;the row being walked: name, probe,
cprobe:	ds	2		; and read - one block, so loadrow
cread:	ds	2		; can copy the three with one ldir
d1res:	ds	3		;the 8272's three status bytes

;****************************************************************
;* strings							*
;****************************************************************

strset:	db	'level 1 boot',0dh,0ah,0
strnone: db	'no boot device',0dh,0ah,0
strgone: db	'boot disk stopped answering',0dh,0ah,0
strnl:	db	0dh,0ah,0
strcolon: db	': ',0
strcomma: db	', ',0
strsp:	db	' ',0
strmb:	db	' MB',0
strpick: db	'boot: ',0
strlegac: db	'legacy boot sector',0
dlmagic: db	'MWDL'			;DL_MAGIC
; The names are the ones /dev uses for these drives (dev/devlist), so
; the menu and the device nodes agree.  The digit is the unit, and each
; row above probes one of them.
cide:	db	'ide0',0
cscsi:	db	'scsi0',0
chd0a:	db	'hd0a',0
chda:	db	'hda',0
cfla:	db	'fla',0
cd1a:	db	'd1a',0
cd2a:	db	'd2a',0
cd3a:	db	'd3a',0

	; the image is copied down as a flat 4k, so it must fit
	assert	$ <= 0F000h + 4096

	end
