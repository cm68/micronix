

	; subttl	'(c) 1981 Morrow Designs'
	title   'MPZ-80  MON5.0 FIRMWARE'

	.z80
	aseg

;****************************************************************
;*								*
;* Decision one CPU firmware, for the Morrow Designs / Thinker	*
;* Toys Decision one computer.	The monitor routine looks for	*
;* the power on jump addresses on CPU switches which determine 	*
;* address to begin execution (top 5 switches).	 I/O is through	*
;* the Wunderbus I/O motherboard UART 1.  Base address of the	*
;* I/O is assumed to be standard (beginning at port 48H).	*
;* If top five switches are 'On', a hard disk is assumed to be	*
;* the disk device and Boothd is executed.			*
;*								*
;*  Revised  9/24/26    -	MON 5.0.  The HDCA (Morrow HDC	*
;*				Winchester) boot driver is	*
;*				replaced by one for the		*
;*				S100Computers MYIDE board, an	*
;*				8255 PPI wired to an ATA	*
;*				drive's pins.  The rest of the	*
;*				rom is MON 4.47 unchanged.	*
;*				MON 4.47 is still the rom to	*
;*				install in a machine with an	*
;*				HDCA; this one boots a machine	*
;*				with an IDE card.  See ideboot.	*
;*								*
;*  Revised  3/24/82    -       Restore maps before power on    *
;*				jump with switch 6 off for cold *
;*				boot of MOS.			*
;*								*
;*  Revised 12/28/82	-	Fixed PIC init routine		*
;*								*
;*  Revised 11/15/82    -      added nop to wrtask for correct  *
;* 			       delay count			* 
;*								*	
;*  Revised 8/27/82	-	M16 home and load constants	*
;*				changed. group0-3 equates now	*
;*				set int en bit high.		*	
;*								*
;* Bobby Dale Gifford and Bob Groppo        			*
;* 10/20/81							*
;*								*
;****************************************************************

	org	0		;Local Ram in task zero

tempstk equ	0200h		;temporary stack
nop	equ	0
gobuff	equ	01b0h		;location of task switch routine on 'RESET'
jmpop	equ	0c3h		;Jump unstruction op-code
callop	equ	0cdh		;z80 call instruction opcode
t1mask	equ	2bh		;unlimited mask... no traps enabled
t0mask	equ	2bh		;unlimited mask	
ssmode	equ	036h		;single step mode mask
hstrap	equ	1Ah		;allow traps on halts and stops, interrupts
				;- masked out in task 0 (temporary)
window	equ	01h		;task 0 window at location 10000
ACR	equ	0Dh		;carriage return
ALF	equ	0Ah		;line feed
ASP	equ	' '		;space
AFF	equ	0Ch		;form feed
BEl	equ	07H		;bell
BSP	equ	08H		;backspace
mskofst	equ	19h		;offset to get to user's mask reg. contents
spofst	equ	17h		;offset to stack pointer of user
afofst	equ	15h		;offset to user Af register
hlofst	equ	13h		;offset to user h,l register
deofst	equ	11h		;offset to d,e
bcofst	equ	0Fh		;offset to b,c
pcofst	equ	0Dh		;offset to the users pc register
nxtbyte	equ	076h		;byte after a halt

;****************************************************************
;*								*
;*		   Wunderbus I/O equates:			*
;*								*
;****************************************************************
	
base	equ	048h		;I/O base address of wunderbus ports
group0	equ	08h
group1	equ	09h		;serial port 1
group2	equ	0ah		;serial	port 2
group3	equ	0bh		;serial port 3
grpctl	equ	base+7		;I/O group select port

;	UART equates

dll	equ	base		;divisor latch lsb
dlm	equ	base+1		;divisor latch msb
ier	equ	base+1		;interupt enable register
lcr	equ	base+3		;line control register
mcr	equ	base+4		;modem control register
lsr	equ	base+5		;line status register
rbr	equ	base		;read data buffer
thr	equ	base		;transmitter data buffer
dlab	equ	80h		;divisor latch access bit
thre	equ	20h		;status line TBE
dr	equ	01		;line status DR bit
wls0	equ	01		;word length select bit 0
wls1	equ	02		;word length select bit 1 (for 8 bit word)
stb	equ	04		;stop bit count (2 stop bits)
imask	equ	00		;non interupt mode
loop	equ	010h		;UART loop mode

;	PIC equates

init	equ	010h		;bit high to initialize the PIC
icw1	equ	base + 4	;PIC initialization control word 1
icw2	equ	base + 5	;PIC initialization control word 2
icw3	equ	base + 5	;PIC initialization control word 3
icw4	equ	base + 5	;PIC initialization control word 4
ocw1	equ	base + 5	;PIC interrupt mask register
ocw2	equ	base + 4	;PIC EOI register
picmask equ	0ffh		;mask to turn all interrupts off
ltim	equ	018h		;ICW access + level triggered mode
adi4	equ	04h		;call address intervals = 4
adi8	equ	00h		;call address intervals = 8
sngl	equ	02		;sole system PIC
ic4	equ	01h		;icw4 access bit
lovect	equ	0		;call vectors begin at 0
hivect	equ	0		;call vectors begin at 0
normal	equ	0ch		;Master/Reg. nest/buffered/no AEOI/8085
				; -normal setting of OCW4 for Morrow Software
eoi	equ	20h		;non-specific EOI constant
ivalu	equ	init OR ltim OR adi4 OR sngl OR ic4 OR lovect

;****************************************************************
;*								*
;*	MYIDE board equates					*
;*								*
;* The board is an 8255 PPI (programmable peripheral interface:	*
;* three 8-bit ports, A, B and C, whose direction is set by a	*
;* mode word written to a fourth address) wired between the	*
;* S-100 bus and the pins of an ATA drive.  Ports A and B carry	*
;* the drive's sixteen data lines, port C its address and	*
;* control pins.  The drive's registers - the "task file": data,*
;* error/features, sector count, three of LBA, drive/head, and	*
;* status/command - are not mapped into the bus at all; port C	*
;* names one, port A carries the byte, and a strobe moves it.	*
;*								*
;****************************************************************

idepa	equ	30h		;port A: drive data, low byte
idepb	equ	31h		;port B: drive data, high byte
idepc	equ	32h		;port C: register number, control strobes
idepctl equ	33h		;8255 mode word: 92h read, 80h write
idepdrv equ	34h		;drive select latch, bit 0

;
; 8255 mode words.  Port C is an output either way; what changes
; is whether ports A and B are inputs (a register read, or a
; sector coming off the drive) or outputs (a register write, or a
; sector going to it).  Read mode is also what the board needs
; when a transfer is in progress, since the drive is driving.
;
idepnrm equ	92h		;A and B in, C out
idepwrt equ	80h		;A, B and C all out

;
; Port C bits.  A0, A1 and A2 reach the drive's address pins
; directly and select the task file register; the rest go through
; inverting buffers, so a set bit here asserts the line.
;
idecs0	equ	08h		;chip select - the board's own decode
idewr	equ	20h		;/WR
iderd	equ	40h		;/RD
iderst	equ	80h		;/RST

idecmd	equ	7		;status when read, command when written
idestat	equ	7
iderdc	equ	20h		;read sector(s), with retry

;****************************************************************
;*								*
;*	DJ-DMA Equates						*
;*								*
;****************************************************************

djstat	equ	104ah		;adjusted channel address of status byte

;****************************************************************
;*								*
;*  	     DMA Winchester Controller Equates			*
;*								*
;****************************************************************
cyl	equ	153		;number cylinders for Seagate ST-506
heads	equ	  4		;number heads for Seagate ST-506
stpdly	equ	 01eh		;15 msec for Seagate ST-506
hdsetl	equ	 0C8h		;20 msec for Seagate ST-506
secsiz	equ	  3		;512 byte sectors for Micronix
readat	equ	  0		;DMA controller read sector opcode
write	equ	  1		;DMA controller write sector opcode
rhead	equ	  2		;DMA controller read header opcode
format	equ	  3		;DMA controller format track opcode
const	equ	  4		;load drive constants command
sense	equ	  5		;return drive status command
noop	equ	  6		;command used when seeking
dmarst	equ	 54h		;DMA controller reset port
attn	equ	 55h		;DMA controller Attention port
stepout	equ	 10h		;Step direction to track 0
stepin	equ	  0		;Step direction away from track 0
track0	equ	  1		;track 0 status
wfault	equ	  2		;write fault condition from drive
dready	equ	  4		;drive ready status
sekcmp	equ	  8		;seek complete status
hdspt	equ	 17		;number of sectors per track
iopb	equ	1050h		;pointer to the channel
chan	equ	1080h		;actual channel
select	equ	chan + 3	;select byte in channel
dmaddr	equ	chan + 4	;24 bit dma address location
arg	equ	chan + 7	;beginning of four arguments to commands
cmmd	equ	chan + 11	;actual command location
statis	equ	chan + 12	;controller return status location 
link	equ	chan + 13	;link field address for next command
bootad	equ	1100h		;dma address for first sector from hddma
good	equ	0ffh		;good status result



;****************************************************************
;*								*
;* Decision One Ram variables, visible only to task 0.		*
;*								*
;****************************************************************

ram	equ	$			;Local RAM, visible only to task 0

;****************************************************************
;*								*
;* Supervisor entry point, this jump must be inserted into the	*
;* CPU's ram by the supervisor for subsequent entry to the	*
;* supervisor when traps occur.					*
;*								*
;****************************************************************

super:	jp	super			;Supervisor entry point
user:	jp	user			;(7) User entry point

ctask:	db	0			;Current task
cmask:	db	0			;Current mask contents
cstack:	dw	0			;temporary save stack
u.sp:	dw	0
u.pc:	dw	0
u.de:	dw	0
u.hl:	dw	0
u.af:	dw	0

begsav	equ	$
regsav:	dw	0			;address of beginning of reg save area
ersav	equ	regsav + 2	;temporary error save area
	ds	1Ch

monstk	equ	$			;monitor stack area

bounce:	ds	100h			;256-byte bounce buffer
;	The upper 4 bits of the task register gate straight to S-100 A20-A23,
;	a whole 1M bank at a time with no overlap, so once the supervisor banks
;	past 1M it can see none of its own low memory.  This buffer is the
;	non-moving staging area for reading boot roms that live above 1M: it
;	sits in on-board ram below 0x1000, off the S-100 bus, so it stays put
;	while a rom is copied through here a chunk at a time.

	ds	(200h - 01eh) - $	;fill to the initialized-stack frame
;********************************************************
;*							*
;*	Initialized stack at 200h contents		*
;*							*
;********************************************************

retstk: dw	0		;return address from call to monitor or super
tskmsk:	dw	0
t.pc:	dw	0
t.sp: 	dw	0		;users stack pointer
t.af:	dw	0		;primary A & f register save
t.bc:	dw	0		;primary b & c register save
t.de:	dw	0		;primary d & e register save
t.hl:	dw	0		;primary h & l register save
t.int:	dw	0		;interrupt register register save
t.ix:	dw	0		; ix register save
t.iy:	dw	0		; iy register save
t.af1:	dw	0		; alternate A & f register save
t.bc1:	dw	0		; alternate b & c register save
t.de1:  dw	0		; alternate d & e register save
t.hl1:  dw 	0		; alternate h & l register save




;****************************************************************
;*								*
;* The following map is used to hold an image of the current	*
;* memory map for all tasks.  this is because the actual map    *
;* ram does not support read operations				*
;*								*
;****************************************************************

map:	ds	200h			;Task Memory map image

;****************************************************************
;*								*
;* Decision One local I/O map, the following registers are	*
;* memory mapped into task 0, and are always visible to task	*
;* zero only.							*
;*								*
;****************************************************************

locio	equ	$			;Local I/O, visible only to task 0
trpadd:	ds	0			;Trapp address register (read)
dspseg:	ds	1			;Display segment register (write)
keybd:	ds	0			;Key board register (read)
dspcol:	ds	1			;Display column register (write)
switch:	ds	0			;CPU switch port (read)
task:	ds	1			;Task register (write)
stats:	ds	0			;Trap status register (read)
mask:	ds	1			;Task mask register (write)
elocio	equ	$			;End of local I/O

	ds	200h-(elocio-locio)	;Fill out local I/O

;****************************************************************
;*								*
;* The following ram is the actual memory map, it can only be	*
;* written into, so an image is kept in the local ram.		*
;*								*
;****************************************************************

mapram:	ds	200h			;Memory Map RAM, visible only to task 0

;****************************************************************
;*								*
;* Decision One prom routines, usable by the supervisor task	*
;* but not accessible by any other tasks.			*
;*								*
;****************************************************************

rom0	equ	$			;Local ROM, visible only to task 0
					;and is visible only during RESET

;****************************************************************
;*								*
;* Reset is executed only once. Currently, reset forms an	*
;* identity map for task zero to occupy the first 64K of main	*
;* memory, allows task 0 to have unlimited priviledges. Task1   *
;* occupies the first 64K, unlimited access and the traps are  	*
;* set for halts or a stop. All other task maps are initialized *
;* starting at bank 2 to bank 15. (e.g. task 15 has bank 15).	*
;* If swithches are set with S1 through S7 off and S8 on, the   *
;* power on jump address will be F800. If switch 6 is on, the	*
;* program will jump to the monitor regardless of the state	*
;* of the other switches.  If S1 - S5 are all 'ON' a hard disk	*
;* is assumed and the IDE boot program is executed.		*
;* If pin 13 of 12C is grounded, the diagnostic mode is entered.*
;*								*
;****************************************************************


;* Reset is executed only once, and the on-board diagnostics are gone,
;* so reset falls straight through to the boot path.  getsw stays as the
;* reset entry: start jumps here, and it hands off to reset0.
;

getsw:	jp	reset0
; Initialize the maps and jump vectors


reset0: call	reset1
	call	uartst
	call	setup
	jp	gobuff

reset1:	ld	hl,super		;initialize 'super' to the monitor...
settle:	dec	hl			;wait for hardware to settle down
	ld	a,l
	or	h
	jr	nz,settle
	ld	(hl),jmpop		;- this will be overwritten by the
	inc	hl			;- supervisor but all traps in the 
	ld	(hl),00h		;- meantime will fall into the monitor.
	inc 	hl
	ld	(hl),10h
	ld	hl,map
	ld	(cstack),hl		;initialize a temporary stack

reslop: xor	a
	ld	c,3			;New access priviledges
reslp2: ld	b,a			;New allocation = segment #
	call	rstmap			;Allocate it
	inc	a			;Next segment #
	and	0fh			;Check if all done
	jr	nz,reslp2		;Continue until done
        LD	A,10h			;write new task and segment
	LD	B,0			;TASK 1 gets first 64K of memory
reslp1: call    rstmap			;Give TASK 1 a full 64k of space
	inc	B			
	inc	A			
	cp      20h			
	jr	nz,reslp1						
fmap:	ld	b,a			;fill all the tasks' maps
	call	rstmap
	inc	a
	cp	0h
	jr	nz,fmap
	ret

	
setup:	xor	a
	ld	(mapram + 2),a		;a window for DMA device commands
	ld	(map + 2),a		;image map updated
	ld	(ersav),a		;null the error save byte
	ld	a,hstrap		;initialize the mask register
	ld	(mask),a		; -to trap on halts and stops
	ld	(cmask),a

;	Following code checks for presence of any ram in system

	ld	hl,0ffffh		;top of ram
ramchk:	ld	a,0f0h
	and	h
	jr	z,badram		;dont go below task0,seg1	
	ld	(hl),a			;check it with a 00h
	cp	(hl)			
	jr	z,nexchk
	dec	hl			;try the next location
	jr	ramchk
nexchk:	cpl	
	ld	(hl),a			;check it with an ff (might be ROM)
	cp	(hl)
	ld	(ersav + 1),hl		;store it away for printing
	jr	z,tstsw					
	dec	hl			;try next location
	jr	ramchk

badram: ld	hl,0badh
	ld	(ersav + 1),hl
	ld	a,'M'
	ld	(ersav),a
	jp 	allerr			;if no ram force entry to monitor

tstsw:	ld	a,(switch)		;get contents of switch
	and	0f8h			;Ignore irrelevent bits
	ld	d,a			;d & e contain jump address
	ld	e,0H
	cp	0			;all switches on -> the 4k selector rom
	jp	z,copyrom
	cp	08h			;If switch 5 is off others are on
	jp	z,nuboot		; - boot DMA controller
	cp	10h			;If switches 4 is off, others on
	jr	z,djdma			; - boot the DJ-DMA floppy device

check:	ld	a,(switch)		;test monitor switch
	bit  	2,a
	ld	a,1			;normal task number
	ld	(u.pc),de		;initialize the  pc save area
	ld	(ctask),a
	jr	z,montor		;jump if monitor desired
	ld	(mapram + 2),a
	ld	(map + 2),a		;restore the maps
	jr	nutask

montor:	xor	a			;monitor task number
	ld	de,cold			;monitor location

nutask: ld	hl,gobuff		;Write a routine to switch to new task
	ld	(hl),03eh		;- because when the task register is
	inc	hl			;- written into, the lower half of the
	ld	(hl),a			;- prom goes away.
	inc	hl
	ld	(hl),032h
	inc	hl
	ld	(hl),02h
	inc	hl
	ld	(hl),04h
	inc	hl
	ld	(hl),nop		;6 nops for countdown sequence
	inc	hl
	ld	(hl),nop
	inc	hl
	ld	(hl),nop
	inc	hl
	ld	(hl),nop
	inc	hl
	ld	(hl),nop
	inc	hl
	ld	(hl),nop
	inc	hl
	ld	(hl),0c3h		;the jump op code
	inc	hl
	ld	(hl),e
	inc	hl
	ld	(hl),d


;****************************************************************
;*								*
;*	Wunderbuss I/O and Mult I/O PIC initialization	rou-	*
;*	tine.  Interrupt vectors = restart locations.		*
;*								*
;****************************************************************

picset:	xor     a
	out	(grpctl),a
	ld	a,ivalu
	out	(icw1),a		;initialize the first word
	ld	a,hivect
	out	(icw2),a		;initialize the second word
	ld	a,normal
	out	(icw4),a		;initialize the forth word
	ld	a,picmask
	out	(ocw1),a		;mask all interrupts
	ld	a,eoi			;send PIC an End of Interrupt word
	out	(ocw2),a		;clear the master interrupt requests
	out	(ocw2),a		;clear the slaves interrupt requests
	ret


;****************************************************************
;*								*
;* DJ-DMA floppy disk boot routine (5.25 or 8 inch).		*
;*								*
;****************************************************************

djdma:	ld	h,10h			; wait byte for 1 minute 
djlop0:	ld	bc,0000h
djloop:	ld	a,(djstat)		;read the status back
	cp	040h
	ld	de,(djstat - 2)		;d & e point to cold boot loader
	jr	z,check			;if good status continue set gobuff
	cp	0ffh			; - else loop for good status
	jr	z,nstat			;if 0ffh then force to a zero
	dec	bc
	ld	a,b
	or	c
	jr	nz,djloop
	dec	h
	jr	nz,djlop0		;continue looping till a minute elapses

;	DJ-DMA not responding correctly

	ld	c,'F'
	ld	a,(djstat)
	ld	b,a			;save the error status
	ld	a,(djstat - 2)
	ld	d,a
	jr	derror			;go to error--controller not
					; - responding

nstat: 	xor	a
	ld	(djstat),a		;null status byte ... signal DJ-DMA
	jr	djloop

;****************************************************************
;*								*
;* Hard Disk Boot program for the DMA Winchester Controller.	*
;*								*
;****************************************************************

nuboot:	ld	bc,endboot - bootbl	;byte count
	ld	hl,bootbl		;source
	ld	de,chan			;destination
	ldir				;move the command
	ld	hl,iopb			;point to default channel addr
	ld	(hl),80h		;fill in the command channel address
	xor	a
	inc	hl			; -located at 50h to point to channel
	ld	(hl),a			; -at 80h.
	inc	hl
	ld	(hl),a
	out	(dmarst),a		;send the controller a reset
	ld	de,010h

hdrl:	dec	d			;wait for controller to process reset
	jr	nz,hdrl
	call	cloop
home:	ld	hl,-1			;seek to home
	ld	(chan + 1),hl		;- with ffff step pulses
	ld	a,noop
	ld	(statis - 1),a		;null the command byte
	ld	a,1
	ld	(statis),a		;initialize the status byte
	call	cloop
	
rdata: 	ld	de,chan			;destination
	ld	bc,endrd - rdtbl	;byte count
	ld	hl,rdtbl		;source
	ldir				;move the read sector command
	call	cloop
	ld	de,0100h		;point to beginning of DMA boot prog.
	jp	check

			
cloop:	ld	c,020h
cloop0:	out	(attn),a
	ld	de,0000h
cloop1: ld	a,(statis)		;check drive status
	cp	0ffh			;an FF means command completed
	ret	z
	dec	de			;wait for controller to respond
	ld	a,e
	or	d
	jr	nz,cloop1		;give it a couple seconds to respond

;	Fall through to here on any error

	ld	a,(statis)
	cp	01h
	jr	nz,cloop2	
	dec	c			;give it 10 tries if not rdy error
	jr	nz,cloop0		; - about 20 seconds

cloop2:	pop	de			;re-align the stack pointer
	ld	c,'H'			;save the device
	ld	b,a			;save the status
	ld	a,(cmmd)		;save the command
	ld	d,a

;****************************************************************
;*								*
;*  Enter here if DISK controllers don't respond correctly.	*
;*  Routine alters gobuff to point to the monitor cout routine.	*	
;*								*
;****************************************************************

derror:	
	ld	a,c
	ld	hl,ersav
	ld	(hl),a			;store c for later
	inc	hl
	ld	(hl),b			;error status
	inc	hl
	ld	(hl),d			;command causing error 

allerr:	xor	a
	ld	(ctask),a
	ld	de,cold			;pointer to error print
	ld	(u.pc),de		;save the pointer in t1 pc
	jp	nutask			
	

	
;****************************************************************
;*								*
;* Selector boot: copy the 4k selector rom down from bank 15	*
;* and run it.							*
;*								*
;* The selector lives at physical 0ff0000h - bank 15, page 0f0h	*
;* - far above the 1M the map alone can reach.  The task		*
;* register's upper 4 bits gate straight to S-100 A20-A23, so	*
;* banking there is a whole 1M switch with no overlap: once	*
;* banked, none of the machine's own low memory is visible.	*
;* Writing the task register also drops the boot rom, so the	*
;* loop that does the banking cannot live in rom - it is copied	*
;* into on-board ram, which stays put across both events, and	*
;* reads 256 bytes at a time through the bounce buffer.		*
;*								*
;* The 4k lands at physical 0f000h - bank 15's top segment - and	*
;* is entered in task 0, whose segment 2 the loop points there.	*
;*								*
;****************************************************************

copyrom:
	ld	hl,ramcode		;the copy loop, assembled at 132h
	ld	de,0132h
	ld	bc,ramend - ramcode
	ldir
	jp	0132h			;run it

;
; The copy loop, built in rom but run from on-board ram (132h).  It
; banks the task register to 15, reads 256 bytes of the selector into
; the bounce buffer, banks back to 0, and stores the bounce buffer to
; physical 0f000h.  Sixteen passes cover the 4k.
;
ramcode:
	.phase	132h
	ld	a,0f0h			;seg 1 -> page 0f0h, the rom
	ld	(mapram+2),a
	ld	(map+2),a
	ld	a,00fh			;seg 2 -> page 0fh, physical 0f000h
	ld	(mapram+4),a
	ld	(map+4),a
	exx
	ld	b,16			;loop counter lives in the alternate set
	exx
	ld	hl,1000h		;the rom, through seg 1

cploop:	ld	a,0f0h			;bank 15 (task 0 stays)
	call	bank
	push	hl			;save the source address
	ld	de,bounce		;read into the bounce buffer
	ld	bc,0100h
	ldir
	xor	a			;bank 0
	call	bank
	pop	hl			;source address back
	push	hl
	ld	de,1000h		;dest = source + 1000h
	add	hl,de
	ex	de,hl			;de = dest
	ld	hl,bounce
	ld	bc,0100h
	ldir
	pop	hl
	inc	h			;next 256
	exx
	dec	b			;b'-- (8-bit dec sets Z)
	exx
	jr	nz,cploop

	jp	0f000h			;enter the selector in task 0

;
; Write the task register from A and ride out the hardware swap delay.
; The seven nops cover it.  The caller and this code are both in
; on-board ram, so the setrom the write triggers does not move them.
;
bank:	ld	(task),a
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	ret

	.dephase
ramend:



;****************************************************************
;*								*
;* Rstmap writes the tasks memory allocation vectors. Upon	*
;* entry the registers must contain:				*
;*	a = task number / task segment number to update		*
;*          high nibble = task #    low nibble = segment #	*
;*	b = New allocation vector				*
;*	c = New allocation access				*
;*								*	
;* Routine calculates the expression  600+(Accumulator) x 2 	*
;* where accumulator contents are as listed above.  All arit-   * 	
;* hmetic and numbers are in Hex				*
;*  						 		*
;****************************************************************

rstmap:	ld	l,a             ;Get task and segment numbers       
        ld	h,0h    
	add	hl,hl           ;multiply times 2       
        ex	de,hl  		;save calculated offset in D,E
        ld	hl,mapram       ;point to beginning of ram map
        call    rstmxx            
	ld	hl,map		;point to image map at 200
rstmxx:	add	hl,de		;add offset to selected map
	ld	(hl),b          ;write the allocation vector to ram
	inc	hl		;point to access ram
	ld	(hl),c          ;write access attributes to ram
	ret

;********************************************************
;*							*
;*  The following code intitializes the I/O for		*
;*  the Decision 1 Motherboard and the Mult I/O.	*
;*  							*
;********************************************************

uartst:	ld	d,3			;start with uart 3
uarts0:	ld	a,d
	out   	(grpctl),a
	xor 	a
	out	(lsr),a			;clear line status register 
	out 	(ier),a			;initialialize interupt mask (off)
	dec	d
	jr	nz,uarts0
	out 	(grpctl),a		;select sense switch port
	in	a,(base+1)		
	rlca
	rlca
	rlca
	and	07h			;mask insignificant bits
	cp	07h			;all off?
	ld	d,0
	jr	z,default		;default if all off
	ld	hl,btable		;point to baud rate table
	add 	a,a
	ld	e,a
	add 	hl,de			;offset to selected baud rate
	ld	c,(hl)
	inc	hl
	ld	b,(hl)			;bc = baud rate divisor value (D)
	jr	setit

default:
	ld	bc,12			;default baud rate is 9600

setit:	inc	d
	ld	a,d
	out	(grpctl),a
	ld	a,dlab+wls1+wls0+stb
	out 	(lcr),a			;divisor access bit is on
	ld	a,b
	out 	(dlm),a			;load high divisor register
	ld	a,c
	out 	(dll),a			;load low divisor register
	ld	a,wls1+wls0+stb
	out 	(lcr),a			;divisor access bit is off
        ld	a,loop			;clear the shift register
	out	(mcr),a			; - by looping back.
	in	a,(rbr)			;clear receiver buffer
	xor	a
	out	(thr),a			;clear transmitter buffer
	call	begin0
	in	a,(rbr)
	xor	a
	out	(thr),a
	call	begin0			;two times to make sure
rduart:	in	a,(lsr)
	and	dr 			;check for data available
	jr	z,rduart
	in	a,(rbr)			;intitialize receiver buffer
	cp	0
	jr	z,contin		;jump if UARTS good

urterr:	push 	af
	ld	a,'U'
	ld	(ersav),a		;Uart error
	ld	a,d			
	ld	(ersav + 2),a		;Uart # saved
	pop	af
	ld	(ersav + 1),a		;bad character saved
	xor	a

contin:	out	(mcr),a
	ld	a,d
	cp	3			;initialize all three uarts
	ret	z
	jr	setit

begin0:	in	a,(lsr)
	and	thre
	jr	z,begin0
	ret


;  Baud rate selection table for Mult I/o or WB I/O

btable:	dw	1047			;110 baud		0 0 0
	dw	384			;300 baud		0 0 1
	dw	96			;1200 baud		0 1 0
	dw	48			;2400 baud		0 1 1
	dw	24			;4800 baud		1 0 0
	dw	12			;9600 baud		1 0 1
	dw	6			;19200 baud		1 1 0

; Load constants command for the DMA Winchester controller

bootbl: db	10h			;direction --> track 0
	db	0			;low steps
	db	0			;high steps
	db	03ch			;select drive 0
	db	0			;low dma address
	db	01			;high dma address
	db	0			;extended dma address
	db	0			;argument 0
	db	stpdly			;argument 1
	db	hdsetl			;argument 2
	db	secsiz			;argument 3
	db	const			;load constants opcode
	db	0			;clear status byte
	db	80h			;low link address
	db	0			;high link address
	db	0			;extended link address

endboot equ	$

; Read sector 1, head 0, cyl 0  command for the HD-DMA:

rdtbl:	db	0			;no seek
	db	0
	db	0
	db	07ch			;select drive 0, head 0 
	db	0			;dma address of 100h
	db	1
	db	0
	db	0			;low byte cylinder
	db	0			;high byte cylinder
	db	0			;head 0
	db	0			;sector 1
	db	0			;read command
	db	0			;clear status

endrd 	equ	$



;* The on-board diagnostics are gone, and their dispatch table with them.
ecode0  equ	$			;End of reset prom code

	ds	3f0h-(ecode0-rom0)	;Fill out the prom

;****************************************************************
;*								*
;* The following special piece of code is where the user task	*
;* begins executing when a reset trap occurs. 			*
;*								*
;****************************************************************

	ld	hl,djstat
	ld	a,0h
	ld	(mapram + 2),a		;t0 map points to t1 map seg 0
	inc	a
start:	ld	sp,map
	jp	getsw 		        ;power-on or reset jump
	nop				;Fill out the prom.
erom0   equ	$

	ds	400h-(erom0-rom0)


	.phase 800h  

;****************************************************************
;*								*
;*	This code is usuable by the supervisor task (task0)    	*
;*	but is not accessible to any other tasks. Any trap	*
;*      other than a reset will enable this half of the eprom	*
;*	as well.						*
;*								*
;****************************************************************


	rom1	equ	$

;****************************************************************
;*								*
;*          ===>>  J U M P   T A B L E   <<===			*
;*								*
;****************************************************************

svtrap:	jp	trappd			;trap routine, check out reason why
tskbse: jp	what			;vestigial
nmap:	jp	putmap			;set up new allocation vector, access 
gotsk:	jp	gotask			;switch to new task
getmap:	jp	gtmap			;get the old allocation vector, access
dupmap: jp	what			;vestigial
what:	jp	monitor			;debugger/monitor called 'MON'
restr: 	jp	restor			;restore task 0 map to normal condition
oldtask:
	jp	gotsk			;jumps to last task before trap
wrtask: jp	writsk			;writes value in CTASK to task register
wratsk:	jp	atask			;writes value in 'A' to  task register

	
trappd:	ld	de,-15			;back up the users pc
	pop	hl
	add	hl,de
	ld	(u.pc),hl
	ld	a,(ctask)
	cp	0
	jp	z,suptrap
	pop	de
	pop	hl
	pop	Af
	ld	sp,(cstack)		;set up reg_save stack in supervisor
alltrp:	ex 	Af,Af'			;save user's auxiliary registers
	exx
	push	hl
	push	de
	push	bc
	push	Af
	push	iy
	push	ix
	ld	a,i			;get interupt register
	push	af			;save it
	ex	Af,Af'
	exx
	push	hl			;- and save all user registers
	push	de
	push	bc
	push	Af
	ld	hl,(ctask)		;save the task and mask
	ld	de,(u.sp)		;get the user's stack pointer
	ld	bc,(u.pc)
	push	de			;save the user's stack pointer
	push	bc			;save the user's program counter
	push	hl			;save the current task and mask
	ld	(regsav),sp		;beginning address of reg save area
					; - saved here.

	;Stop switch calls the monitor - return will restore to "CTASK"

	ld	a,l			;get the trapped task #
	cp	0
	jr	z,gowhat		;if trap was in task 0, go to monitor
	ld	a,(stats)
	bit	4,a
	jr	nz,gosupr		;go to super unless it was a stop trap 

gowhat:	xor 	a
	ld	(ctask),a
	call	what
	jr	gotask

suptrap:
	pop	de
	pop	hl
	pop	Af
	ld	sp,(u.sp)
	jp	alltrp	

	;Call the supervisor - a return will restore the task in "CTASK"
gosupr:	xor	a
	ld	(ctask),a
	call	super		
	jr	gotask


;********************************************************
;*							*
;*  Writsk will take the value in CTASK and write it	*
;*  to the TASK register.  It then waits 6 instructions	*
;*  for the hardware delay and returns with traps set 	*
;*  and operation in the task selected.	 The 'A' re-	*
;*  gister is not preserved, all others untouched.	*
;*							*
;********************************************************

writsk:	ld	a,(ctask)
atask:	ld	(task),a        ;update the task register and count   
	nop			; - 7 instructions to delay for the 
	nop			; - hardware swap counter.
	nop
	nop
	nop
	nop
	nop
	ret


;********************************************************
;*							*
;*   Restore will restore Task 0's map with its old 	*
;*   values.  This assumes that if the map for task 0   *
;*   has been changed, that only the actual map had 	*
;*   been changed and that the image map was left in	*
;*   the condition before the change occurred.		*
;*							*
;********************************************************


restor:	ld 	hl,map			;point to beginning of map image
	ld	de,mapram		;point to beginning of actual map
	ld	bc,01Fh			;all of task 0 map 
	ldir

;****************************************************************
;*								*
;*	Gotask restores all the task's registers  and then	*
;*	switches to that task.                 			*
;*								*
;****************************************************************

gotask:	ld	hl,0			;init hl for normal entry
	add	hl,sp
ntask:	ld	sp,hl
	pop	hl
	ld	(ctask),hl		;get back CTASK and CMASK
	pop	hl
	ld	(u.pc),hl		;get back the user's pc
	pop	hl
	ld	(u.sp),hl		;get back the user's sp
	pop	Af			;get back the primary registers
	pop	bc
	pop	de
	pop	hl
	ex	Af,Af'
	exx
	pop	Af			;get back the interrupt register
	ld	i,a			;restore it
	pop	ix			;restore auxilliary registers
	pop	iy
	ld	hl,(u.pc)		
bcomnd:	ld	(user + 1),hl		;form jump to user's pc value @ user
	ld	a,jmpop
	ld	(user),a
	pop	Af			;restore the alternate registers
	pop	bc
	pop	de
	ld	hl,(ctask)
	ld	(task),hl		;write the new task and mask
	pop	hl
	ex	Af,Af'
	exx
	ld	sp,(u.sp)
	jp	user
	
					
;********************************************************
;*							*
;*  The following code will return with:		*
;*      Register A = task #/ segment #			*
;*	Register B = old allocation vector		*
;*	Register C = old access	priviledges		*
;*  Upon entry, it expects the A register to have	*
;*  the desired task# / segment #. Consider this to be	*
;*  opposite of the putmap routine.			*
;*							*
;********************************************************

gtmap:	ld	l,a		;get task and segment numbers
	ld	h,0h		
	add	hl,hl		;multiply times 2
	ex	de,hl		;save calculated offset in de
	ld	hl,map		;point to beginning of map ram image
	add	hl,de		;add offset to get desired map
	ld	b,(HL)		;get old allocation vector
	inc	hl		;offset to access map
	ld	c,(HL)		;get old access priviledges
	ret 	
		
			
;********************************************************
;*							*
;*  Putmap updates a task's allocation vectors and	*
;*  access atributes.  Upon entry, registers must	*
;*  contain:						*
;*	a = task # / task segment # to update		*
;*	    high nibble = task#, low nibble = segment#	*
;*	b = new allocation vector			*
;*	c = new access privilidges			*
;*							*
;*  Routine calculates the expression 600 + (a) X 2	*
;*  where a is as listed above.  All arithmetic and	*
;*  numbers are in Hex.					*
;*							*
;********************************************************

putmap:	ld	l,a			;get task and segment number
	ld	h,0h			;
	add	hl,hl			;multiply times 2
	ex 	de,hl			;save calculated offset in de
	ld	hl,mapram		;point to beginning of ram map
	call 	putmxx			
	ld	hl,map			;point to beginning of image map
putmxx: add	hl,de			;add offset to selected map
	ld	(hl),b			;write the allocation vector
	inc	hl			;point to access attribute ram
	ld	(hl),c			;write new access atributes
	ret

	
;********************************************************	
;*							*
;*  The following routines make up the debugging tool	*
;*  monitor  						*
;*							*
;********************************************************

monitor:
	ld	(monstk -2),sp		;save stack for 'u' & 'c' commands
	ld	iy,(monstk-2)
	ld	sp,monstk
	ld	hl,(regsav)		;get the stack location
	ld	de,27			;number of saved registers
	push	hl
	add	hl,de
	ex	de,hl
	pop	hl
	ld	bc,ustart
	push	bc
	jp	udi0			;print out the registers

cold:	ld	a,(ersav)		;retrieve the error byte if any
	ld	c,a
	call	ucout1			;print it
	ld	hl,(ersav + 1)		;retrieve disk command
	call	uladr

ustart:	ld	sp,monstk
	LD	DE,USTART
	PUSH	DE
	CALL	UCRLF
	LD	C,':'
	CALL	ucout1
USTAR0:	CALL	UTI
	OR	A
	JR	Z,USTAR0
	CP	'z'+1
	JP	NC,UERROR
	LD	C,002H
	CP	'D'
	JR	Z,udisp
	cp	'd'
	jr	nz,ufill

;
;	DISPLAY MEMORY XXXX TO XXXX
;
;
UDISP:	CALL	UEXLF
UDI0:	CALL	UCRLF
	CALL	ULADR
	LD	B,010H
UDI1:	CALL	UBLK
	LD	A,(HL)
	CALL	ULBYTE
	CALL	UHILOX
	DJNZ	UDI1
	JR	UDI0
;
;
;
;	FILL MEMORY XXXX TO XXXX WITH XX
;
;
UFILL:	CP	'F'
	JR	z,ufill0
	cp	'f'
	jr	nz,ugoto
ufill0:	CALL	UEXPR3
UFI0:	LD	(HL),C
	CALL	UHILO
	JR	NC,UFI0
	POP	DE
	JP	USTART
;
;
;	GOTO (EXECUTE) XXXX
;
;
UGOTO:	CP	'G'
	JR	Z,ugoto0
	cp	'g'
	jr	nz,umtest
ugoto0:	CALL	UEXPR1
	CALL	UCRLF
	POP	HL
	JP	(HL)
;
;
;	TEST MEMORY XXXX TO XXXX
;
;
UMTEST:	CP	'T'
	JR	Z,ut10
	cp	't'
	jr	nz,umove
ut10:	CALL	UEXLF
UT1:	LD	A,(HL)
	LD	B,A
	CPL
	LD	(HL),A
	XOR	(HL)
	JR	Z,UT2
	PUSH	DE
	LD	E,A
	CALL	UHLSP
	CALL	UQI1
	CALL	UCRLF
	POP	DE
UT2:	LD	(HL),B
	CALL	UHILOX
	JR	UT1
;
;
;	MOVE DATA FROM XXXX TO XXXX
;
;
UMOVE:	CP	'M'
	JR	Z,umvo0
	cp	'm'
	jr	nz,usubs
umvo0:	CALL	UEXPR3
UMV0:	LD	A,(HL)
	LD	(BC),A
	INC	BC
	CALL	UHILOX
	JR	UMV0
USTORE:	LD	(IX+00H),A
	INC	IX
	DEC	E
	RET
;
;
;	EXAMINE AND/OR REPLACE MEMORY DATA
;
;
USUBS:	CP	'S'
	JP	Z,usuo0
	cp	's'
	jp	nz,uhexn
usuo0:	CALL	UEXPR1
	CALL	UQCHK
	JP	C,UERROR
	POP	HL
USU0:	LD	A,(HL)
	CALL	ULBYTE
	LD	C,02DH
	CALL	UCOPCK
	RET	C
	JR	Z,USU1
	PUSH	HL
	LD	HL,0
	LD	C,001H
	CALL	UEX1
	POP	DE
	POP	HL
	LD	(HL),E
	LD	A,B
	CP	00DH
	RET	Z
USU1:	INC	HL
	CALL	UCRLF
	PUSH	HL
	CALL	ULADR
	CALL	UBLK
	POP	HL
	JR	USU0
;
;
UEXLF:	CALL 	UEXPR
	POP	DE
	POP	HL
;	CR/LF OUTPUT
;
;
UCRLF:	PUSH	HL
	PUSH	BC
	LD	C,0DH
	CALL	ucout1
	LD	C,0AH
	CALL	ucout1
	POP	BC
	POP	HL
	CALL	UCSTS
	OR	A
	RET	Z
;
;	CHECK FOR CONTROL CHARACTER
;
;
UCCHK:	CALL	ucon1
	AND	07FH
	CP	013H	;CONTROL-S
	JR	Z,UCCHK
	CP	003H	;CONTROL-C
	RET	NZ
UERROR:	CALL	UMEMSIZ
	LD	DE,UERROR
	PUSH	DE
	LD	C,'?'
	CALL	ucout1
	JP	USTART
UHLSP:	CALL	ULADR
;
;	PRINT SPACE CHARACTER
;
UBLK:	LD	C,020H


;********************************************************
;*							*
;*  Console I/O routines for the Wunderbus I/O.  These	*
;*  routines assume that the uart divisor latch has 	*
;*  previously set (either on power up or in routine 	*
;*  executed before a trap to this routine occurred.    *
;*  The character to output should be in the 'C' reg-	*
;*  ister, the character received is returned in the	*
;*  'A' register.  UCSTS returns with zero flag set	*
;*  when no character is waiting in the UART buffer,	*
;*  or with A = FFh if a character is waiting.		*
;*							*
;********************************************************	


ucout1:	call	uconinit
ucout2:	in	a,(lsr)			;get uart status
	and	thre
	jr	z,ucout2		;loop until tbe
	ld	a,c
	out	(thr),a			;output the data to uart
	ret

ucon1:	call	uconinit
ucon2:	in	a,(lsr)			;get uart status
	and	dr
	jr	z,ucon2			;wait until receive data available
	in	a,(rbr)			;read the uart data register
	and	07fh			;strip parity
	ret

ucsts:	call	uconinit		
	in	a,(lsr)			;read uart status
	and	dr
	ret	z			;return zero set if no character
	ld	a,0ffh
	ret				;return a = ffh if character waiting

uconinit:
	ld	a,group1
	out   	(grpctl),a		;set up for UART 1
	ld	a,wls0+wls1+stb		
	out 	(lcr),a			;8 bit word, 2 bit stop bits
	ret		



;	CONVERT HEX TO ASCII

UCONV:	AND	00fh
	ADD	A,090H
	DAA
	ADC	A,040H
	DAA
	LD	C,A
	RET
;
;	GET PARAMETERS 1,2,OR 3
;
UEXPR3:	INC	C
	CALL	UEXPR
	CALL	UCRLF
	POP	BC
	POP	DE
	POP	HL
	RET
UEXPR1:	LD	C,001H
UEXPR:	LD	HL,0
UEX0:	CALL	UTI
UEX1:	LD	B,A
	CALL	UNIBBLE
	JR	C,UEX2
	ADD	HL,HL
	ADD	HL,HL
	ADD	HL,HL
	ADD	HL,HL
	OR	L
	LD	L,A
	JR	UEX0
UEX2:	EX	(SP),HL
	PUSH	HL
	LD	A,B
	CALL	UQCHK
	JR	NC,UEX3
	DEC	C
	RET	Z
UEX3:	JP	NZ,UERROR
	DEC	C
	JR	NZ,UEXPR
	RET
UHILOX:	CALL	UHILO
	RET	NC
	POP	DE
	RET
UHILO:	INC	HL
	LD	A,H
	OR	L
	SCF
	RET	Z
	LD	A,E
	SUB	L
	LD	A,D
	SBC	A,H
	RET
;
;	HEXADECIMAL ARITHMETIC
;
UHEXN:	CP	'H'
	JR	Z,uhexd
	cp	'h'
	jr	nz,uport
uhexd:	CALL	UEXLF
	PUSH	HL
	ADD	HL,DE
	CALL	UHLSP
	POP	HL
	OR	A
	SBC	HL,DE
;
;	CONVERT HL REGISTER TO ASCII
;
ULADR:	LD	A,H
	CALL	ULBYTE
	LD	A,L
;
;	CONVERT A REGISTER TO ASCII
;
ULBYTE:	PUSH	AF
	RRCA
	RRCA
	RRCA
	RRCA
	CALL	UDBLC
	POP	AF
UDBLC:	CALL	UCONV
	JP	ucout1			;checked

UMEMSIZ:
	RET

UNIBBLE:
	cp	'a'			;is it less than lower case 'a'?
	jr	c,unibok		;take jump if so
	cp	'z'+1			;less than a lower case 'z'?
	ccf				;set carry and return if > 'z'
	ret	c
	sub	' '			;convert to upper case
unibok:	SUB	030H
	RET	C
	cp	017h
	ccf
	RET	C
	CP	00AH
	CCF
	RET	NC
	SUB	007H
	CP	00AH
	RET
UCOPCK:	CALL	ucout1
UPCHK:	CALL	UTI
;
;	CHARACTER CHECK
;
UQCHK:	CP	020H
	RET	Z
	CP	02CH
	RET	Z
	CP	00DH
	SCF
	RET	Z
	CCF
	RET
;
;	ECHO CONSOLE
;
UTI:	CALL	ucon1
	INC	A
	RET	Z
	DEC	A
	AND	07FH
	RET	Z
	CP	000H
	RET	Z
	CP	04EH
	RET	Z
	CP	06EH
	RET	Z
	PUSH	BC
	LD	C,A
	CALL	ucout1
	LD	A,C
	POP	BC
	RET

;
;	READ/WRITE TO I/O PORT
;
UPORT:	CP	'O'
	JR	Z,UQOUT
	CP 	'o'
	jr	z,uqout
	CP	'I'
	JR	Z,uin
	cp	'i'
	jr	z,uin
	JR	UVERIFY
UIN:	CALL	UEXPR1
	LD	C,0AH
	CALL	ucout1
	POP	BC
UQ0:	IN	E,(C)
UQI1:	LD	B,008H
	CALL	UBLK
UQI2:	SLA	E
	LD	A,018H
	ADC	A,A
	LD	C,A
	CALL	ucout1
	DJNZ	UQI2
	RET
UQOUT:	CALL	UEXPR
	POP	DE
	POP	BC
	OUT	(C),E
	RET
;
;
;
;
;	VERIFY MEMORY XXXX TO XXXX WITH XXXX
;
UVERIFY:
	CP	'V'
	JR	Z,uver0
	cp	'v'
	jr	nz,uretrn
uver0:	call 	uexpr3
UVERIO:	LD	A,(BC)
	CP	(HL)
	JR	Z,U..B
	PUSH	BC
	CALL	UCERR
	POP	BC
U..B:	INC	BC
	CALL	UHILOX
	JR	UVERIO



;	Return to task which just trapped with old pc and registers restored
;	All registers are saved!!

uretrn:	cp	'C'
	jr	z,uretr1
	cp	'c'	
	jr	nz,ucontr
uretr1:	ld	sp,iy			;get back the user save stack
	pop	de			; ... the return address
	pop	hl			; ... Ctask/Cmask
	ld	a,h
	or	08h			;set the run bit for 'run'
	ld	h,a
	push	hl			;restore the stack
	push	de
	ret				;return to user through 'gotask'


;	Return to trapped task, execute next instruction and trap back
;	All registers are saved but the mask is changed!!

ucontr:	cp	'U'
	jr	z,ucont1
	cp	'u'
	jr	nz,uboot
ucont1:	ld	sp,iy			;get back the user save stack
	pop	de			;... the return address
        pop	hl			;... CTASK/CMASK
	ld	a,h
	and	0f6h			;force mask for stop and run enble low
	ld	h,a
	push	hl			;restore the stack
	push	de
	ret				;return to user through 'gotask'

;	Jump to the cpu switch address into task specified by CTASK
;	No registers are preserved!!

uboot:	cp	'B'
	jr	z,uboot1
	cp	'b'
	jp	nz,uerror
uboot1:	ex	Af,Af'
	exx
	xor	a
	inc	a
	ld	(mapram + 2),a		;restore the actual map
	ld	(map + 2),a		;restore map image 
	ld	a,(switch)
	and	0f8h
	cp	0			;IDE Boot?
	jr	z,uboot2		
	cp	8			;HD-DMA Boot?
	jr	z,uboot2
	cp	10h			;DJ-DMA Boot?
	jr	z,uboot2
	ld	h,a			;Memory Address jump
	ld	l,0
	jp	bcomnd
uboot2: ld	hl,(u.pc)		;get the boot address for controllers
	jp	bcomnd


;	MEMORY MISMATCH PRINTOUT
;
UCERR:	LD	B,A
	CALL	UHLSP
	LD	A,(HL)
	CALL	ULBYTE
	CALL	UBLK
	LD	A,B
	CALL	ULBYTE
	JP	UCRLF


ecode1	equ	$
	ds	3edh - (ecode1-rom1)

serial:	dw	0ffffh			;Micronix serialization word
	db	0ffh

;********************************************************
;*   							*
;*   The following piece of code is where the user	*
;*   task begins execution whenever a trap occurs.	*
;*   The users registers and sp are saved in the 	*
;*   temporary users store area.			*
;*							*
;********************************************************

	nop				;must be nop to void a halt
	ld 	(u.sp),sp		;save the users stack pointer
	ld	sp,begsav	     	;set sp to the temporary save area
	push	af	
	push	hl
	push	de
	nop
	call	trappd			;go to supervisor trap via temp
	halt				;halt here allows T0 to halt


erom1	equ	$						
	ds	400h-(erom1-rom1)
	


fpp0:	ds	8
fpp1:	ds	1

	.dephase
	end
