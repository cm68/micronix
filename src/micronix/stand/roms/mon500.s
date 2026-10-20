

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
;
; The free window in the on-board ram, and mon5.0's two resident blocks
; in it.  It runs from 132h, the byte after the monitor's bounce buffer,
; to the task switch stub at gobuff, which the monitor writes on every
; task switch and the level 1 rom writes for itself.  Both blocks are
; laid out from the bottom up and neither may cross gobuff: the console
; first, because it is called for the life of the machine, then the copy
; loop that brings the level 1 rom down, which runs once at a boot.
;
conram	equ	0132h		;the console found at reset
concon	equ	conram		; - which one answered: 0, 1 or 2
condrv	equ	conram + 2	; - the driver built for it
ramhome	equ	0166h		;where the copy loop runs
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
ss1valu	equ	init OR ltim OR adi4 OR ic4	;the same for a cascaded pair

;****************************************************************
;*								*
;*	System Support 1 equates				*
;*								*
;* The CompuPro System Support 1 is one sixteen port board at 50h	*
;* carrying the two 8259 interrupt controllers, three timers, a	*
;* floating point unit, a clock/calendar and a serial channel.	*
;* Only the serial channel and the controllers matter here.	*
;*								*
;* The channel is a 2651, an older relative of the 8250 that does	*
;* not address its registers the same way and does not put its	*
;* ready bits in the same places.  It has no divisor latch and no	*
;* group select: its four registers are ordinary ports, and what	*
;* the 8250 says with a divisor and a line control register it	*
;* says with two mode registers written one after the other.	*
;*								*
;* Its controllers are a master and a slave, cascaded - the	*
;* master's IR7 is the slave - so where the Wunderbus's single	*
;* 8259 answers an ICW1 of ivalu, these two want the cascade bit	*
;* clear and an ICW3 each.					*
;*								*
;****************************************************************

;
;	The board's sixteen ports are a jumper block, and it is 07ch here
;	and not CompuPro's default 05ch: 050h through 055h are the HDCA's
;	and the HD-DMA's, so at the default the two cards answer on one
;	port and one of them is never heard again.  The four numbers here,
;	s1icw1 below, sys/ss1pic.c, sys/ss1_clock.c and the simulator's
;	SS1_BASE are one address said five ways.
;
s1data	equ	07ch		;data: send on a write, receive on a read
s1stat	equ	07dh		;status: bit 0 ready to send, bit 1 to read
s1mode	equ	07eh		;the two mode registers, first one first
s1cmd	equ	07fh		;command register
s1mr1	equ	04eh		;mode 1: asynchronous x16, 8 bits, no parity
s1mr2	equ	07fh		;mode 2: 19200 baud, top four bits as asked
s1cen	equ	005h		;command: transmitter and receiver on
s1txrdy	equ	001h		;status: the transmitter will take a byte
s1rxrdy	equ	002h		;status: a byte is waiting to be read
s1quiet	equ	028h		;status: no framing and no parity error -
				; a port with nothing on it reads ffh, which
				; is both, so a quiet status is what a real
				; board has and an empty socket cannot fake

s1icw1	equ	070h		;master: ICW1/OCW2 and the read registers
s1icw2	equ	071h		;master: ICW2/ICW3/ICW4 and the mask
s1sicw1	equ	072h		;the slave's same two ports
s1sicw2	equ	073h
s1slave	equ	007h		;ICW3 of the slave: its id, the cascade line
s1cas	equ	080h		;ICW3 of the master: the cascade is IR7
s1sbuf	equ	008h		;ICW4: buffered slave, 8080 mode, EOI
s1mbuf	equ	00ch		;ICW4: buffered master, same

;****************************************************************
;*								*
;*	Switch equates						*
;*								*
;* The switch register is negated: a switch that is on reads as a	*
;* clear bit.  Two of its eight bits are still meaningful.  Switch	*
;* 6 says enter the monitor instead of booting; switch 5 says look	*
;* for the System Support 1's serial port first.		*
;*								*
;****************************************************************

sw5	equ	008h		;switch 5 on: the System Support 1 first
sw6	equ	004h		;switch 6 on: the monitor, not a boot

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

;	Free ram runs from here to gobuff, and it is where mon5.0's two
;	resident blocks live: the console driver at conram, and the copy
;	loop above it at ramhome.  It is gobuff that ends the window,
;	not the trap save frame above it, because the monitor writes its
;	task switch stub at gobuff on every task switch and the level 1 rom
;	writes its own there.  The frame is the kernel's contract - it reads
;	the saved registers from 1e0h up (sys/uhdr.s, _oldstack, 32 bytes to
;	1ffh) - and above the frame sit the other two contracts this ram
;	carries: the map image at 200h, which stands in for the map
;	registers, which cannot be read back, and local i/o at 400h.

oldstk	equ	01e0h		;lowest word of the trap save frame

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
	call	ramdown		;resident copy loop
	call	findcon
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

;
;	The switch is binary now.  Switch 6 on asks for the monitor and
;	switch 6 off for a boot.  There is no jump address field left to
;	read and no disk driver in this rom to read it for: a boot means
;	the level 1 rom in its socket, and boot is the resident copy loop
;	that brings it down into ram and enters it.
;
;	Switch 5 picked the console back at findcon.  Nothing else on the
;	switch is read.

boot	equ	ramhome		;the resident copy loop

tstsw:	ld	a,(switch)
	and	sw6
	jp	nz,boot			;switch 6 off reads set: the level 1 rom

montor:	ld	a,1			;normal task number
	ld	(ctask),a
	xor	a			;monitor task number
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
;*	Interrupt controller initialization.  The console says	*
;*	which controllers the machine has: the Wunderbus board	*
;*	carries one 8259 behind its group select port, and the	*
;*	System Support 1 carries a cascaded pair of its own.	*
;*	Either way the vectors are the restart locations and	*
;*	every line is masked, because the monitor polls.	*
;*								*
;****************************************************************

picset:	ld	a,(concon)
	or	a
	jr	nz,picss1		;not a Wunderbus console

	xor     a
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

;	The System Support 1's pair.  The slave goes first so that it is
;	not signalling when the master arms.  Neither takes an EOI: no
;	line was ever unmasked, so none is pending.

picss1:	ld	a,ss1valu
	out	(s1sicw1),a		;initialize the slave
	ld	a,hivect
	out	(s1sicw2),a
	ld	a,s1slave
	out	(s1sicw2),a		;its id: the master's cascade line
	ld	a,s1sbuf
	out	(s1sicw2),a
	ld	a,picmask
	out	(s1sicw2),a		;mask all interrupts
	ld	a,ss1valu
	out	(s1icw1),a		;then the master
	ld	a,hivect
	out	(s1icw2),a
	ld	a,s1cas
	out	(s1icw2),a		;the cascade is IR7
	ld	a,s1mbuf
	out	(s1icw2),a
	ld	a,picmask
	out	(s1icw2),a		;mask all interrupts
	ret


;****************************************************************
;*								*
;*  Enter the monitor with whatever went wrong left in the	*
;*  error save area, which cold prints.  A machine with no ram	*
;*  worth the name arrives here as surely as one whose console	*
;*  answered nothing: there is nothing else left to do with it.	*
;*								*
;****************************************************************

allerr:	xor	a
	ld	(ctask),a
	ld	de,cold			;pointer to error print
	ld	(u.pc),de		;save the pointer in t1 pc
	jp	nutask			
	

	
;****************************************************************
;*								*
;* Level 0 boot: copy the 4k rom out of its socket and run it.	*
;* This is the whole of what a boot is - the monitor has no	*
;* disk driver, so the rom is what boots.			*
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

;	The copy loop is built here and copied into on-board ram
;	at reset, because it cannot stay in the rom: banking the task
;	register drops the reset half of the prom, and the monitor half
;	cannot call into this one even before that.  It runs from ramcode,
;	above the console driver, so both halves of the rom can reach it
;	and the console survives the boot.

ramdown:ld	hl,ramcode		;the copy loop, assembled at ramhome
	ld	de,ramhome
	ld	bc,ramend - ramcode
	ldir
	ret

;
; The copy loop, built in rom but run from on-board ram.  It banks the
; task register to 15, reads 256 bytes of the selector into the bounce
; buffer, banks back to 0, and stores the bounce buffer to physical
; 0f000h.  Sixteen passes cover the 4k, counted in h, which walks from
; 1000h to 2000h as the source does.
;
ramcode:
	.phase	ramhome
	ld	a,0f0h			;seg 1 -> page 0f0h, the rom
	ld	(mapram+2),a
	ld	a,00fh			;seg 2 -> page 0fh, physical 0f000h
	ld	(mapram+4),a
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
	inc	h			;next 256 of the source
	ld	a,h
	cp	020h			;sixteen passes: 1000h to 1fffh
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

;	The two resident blocks share the free window in the on-board ram,
;	which runs from 132h, the byte after the monitor's bounce buffer,
;	to gobuff at 1b0h, where the monitor writes its task switch stub
;	on every task switch - and the level 1 rom writes its own there.
;	Each block has a fixed home in the window, and neither may grow
;	over its neighbour or over gobuff, so the bounds are asserted here
;	rather than discovered on the machine: a console driver must end
;	below the copy loop, and the copy loop below gobuff.  The copy
;	loop's own home is the driver's ceiling, so a driver that grew past
;	it would be overwritten by the next boot.  The trap save frame at
;	oldstk is above all of this and out of reach here.

	assert	cdwend - cdwb <= ramhome - condrv
	assert	cdsend - cdsb <= ramhome - condrv
	assert	ramend - ramcode <= gobuff - ramhome



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

;********************************************************
;*							*
;*  The console.  A machine has one of two.  The	*
;*  Wunderbus i/o board carries an 8250, which is not	*
;*  always visible: it sits behind the group select	*
;*  port and is reached only after a group 1 write.	*
;*  The System Support 1 carries a 2651, whose		*
;*  registers are plain ports at 5ch through 5fh -	*
;*  nothing else in the machine is there, since the	*
;*  HDCA stops at 53h and the HD-DMA at 55h.		*
;*							*
;*  Both are programmed here for 19200 baud, 8 data	*
;*  bits, no parity and one stop bit.  The switch names	*
;*  which to look for first and the other is looked for	*
;*  after it, so a machine with one console is found	*
;*  either way and a machine with both is steered.	*
;*							*
;*  Neither driver can be a routine in the rom: the	*
;*  console is called from both halves of it, and a rom	*
;*  cannot hold code for a chip that may not be there.	*
;*  The driver is built into on-board ram at reset	*
;*  instead, and the monitor's console names are three	*
;*  addresses in it.  The level 1 rom runs in task 0	*
;*  with that ram mapped and calls the same three.	*
;*							*
;********************************************************

;	The console's home in the free window is conram, and the window's
;	layout is set out at the top of this file.

csend	equ	0		;its entry for sending the char in C
crecv	equ	3		;its entry for waiting for one into A
credy	equ	6		;its entry for asking whether one waits

;********************************************************
;*							*
;*  Find the console, program it, and build its	*
;*  driver.  A probe answers with Z set if its console	*
;*  is there and programs the chip either way, so a	*
;*  machine with both ends up with the one that was	*
;*  found.						*
;*							*
;********************************************************

findcon:ld	a,(switch)
	and	sw5
	jr	nz,fconw		;switch 5 off: Wunderbus first
	call	findss1
	jr	z,fconss
	call	findwb
	jr	z,fconwb
	jr	nocon

fconw:	call	findwb
	jr	z,fconwb
	call	findss1
	jr	z,fconss
	jr	nocon

fconwb:	xor	a			;0: the Wunderbus 8250 answered
	ld	hl,cdwb
	ld	bc,cdwend - cdwb
	jr	fconcp

fconss:	ld	a,1			;1: the System Support 1's 2651
	ld	hl,cdsb
	ld	bc,cdsend - cdsb

fconcp:	ld	(concon),a		;name what answered
	ld	de,condrv
	ldir				;and build its driver
	ret

nocon:	ld	a,2			;2: neither answered
	ld	(concon),a
	ld	a,'U'
	ld	(ersav),a		;the monitor's error byte, in case
hang:	halt				; - one runs.  There is no console
	jr	hang			; - to say this on, so the machine stops

;********************************************************
;*							*
;*  Probe for the Wunderbus 8250 and program it.  The	*
;*  divisor is fixed: 6 is 19200 baud from the board's	*
;*  1.8432 MHz clock.  The test is the loopback in the	*
;*  modem control register - with it on, a byte sent	*
;*  comes straight back - so a chip that is there hands	*
;*  back the zero it was sent and an empty socket hands	*
;*  back ffh.						*
;*							*
;********************************************************

findwb:	ld	a,group1
	out	(grpctl),a		;a group 1 write reaches the uart
	xor	a
	out	(lsr),a			;clear line status register
	out	(ier),a			;interrupt mask off
	ld	a,dlab+wls1+wls0
	out	(lcr),a			;divisor access on
	xor	a
	out	(dlm),a			;high divisor
	ld	a,6
	out	(dll),a			;low divisor: 19200 baud
	ld	a,wls1+wls0
	out	(lcr),a			;divisor access off, 8 data bits
	ld	a,loop
	out	(mcr),a			;loop back
	in	a,(rbr)			;drop what is in the receiver
	xor	a
	out	(thr),a			;send a zero
	call	wbwait
	in	a,(rbr)			;it should come back
	xor	a
	out	(thr),a
	call	wbwait
	in	a,(rbr)			;twice, to be sure
	push	af
	xor	a
	out	(mcr),a			;loop back off
	pop	af
	or	a			;Z set: the zero came back
	ret	z
	or	0ffh			;Z clear: there is no uart here
	ret

wbwait:	in	a,(lsr)			;a socket with nothing on it reads
	and	thre			; - ffh, which has this bit set, so
	jr	z,wbwait		; - this wait cannot hang on one
	ret

;********************************************************
;*							*
;*  Probe for the System Support 1's 2651 and program	*
;*  it.  The mode registers carry the baud rate and the	*
;*  format, so there is no divisor to load and no access	*
;*  bit to set: the two mode registers are written one	*
;*  after the other and the command turns both halves of	*
;*  the channel on.  A real chip then reports itself	*
;*  ready to send; an empty socket reads ffh, which is	*
;*  every error bit set at once.			*
;*							*
;********************************************************

findss1:ld	a,s1mr1
	out	(s1mode),a		;8 data bits, no parity, one stop
	ld	a,s1mr2
	out	(s1mode),a		;19200 baud
	ld	a,s1cen
	out	(s1cmd),a		;transmitter and receiver on
	in	a,(s1stat)
	ld	b,a			;keep the status
	and	s1quiet
	jr	nz,fs1no		;framing or parity error: not here
	ld	a,b
	and	s1txrdy
	jr	z,fs1no			;never came ready: not here
	xor	a			;Z set: the chip answered
	ret
fs1no:	or	0ffh			;Z clear: no 2651 here
	ret

;********************************************************
;*							*
;*  The Wunderbus console driver, built at condrv when	*
;*  the 8250 answers.  The three entry points come	*
;*  first, so both drivers offer the same three		*
;*  addresses and no caller asks which was built.	*
;*							*
;*  Every access selects group 1 first: the 8250 sits	*
;*  behind the group select port, and the monitor is not	*
;*  the only thing that writes that port.  The two poll	*
;*  loops jump back to the select, not to the read, so an	*
;*  interrupt that selects group 0 (the clock's, to clear	*
;*  its latch and EOI the PIC) cannot leave the loop	*
;*  reading the 8259's IMR at 0x4d as the line status.	*
;*							*
;********************************************************

cdwb:
	.phase	condrv
	jp	cdwout			;+0: send the character in C
	jp	cdwin			;+3: wait for a character into A
	jp	cdwst			;+6: is one waiting?

cdwout:	ld	a,group1
	out	(grpctl),a
cdwo1:	in	a,(lsr)			;wait for the transmitter to empty
	and	thre
	jr	z,cdwout		;re-select: an interrupt may have changed the group
	ld	a,c
	out	(thr),a
	ret

cdwin:	ld	a,group1
	out	(grpctl),a
cdwi1:	in	a,(lsr)			;wait for a character to arrive
	and	dr
	jr	z,cdwin			;re-select: an interrupt may have changed the group
	in	a,(rbr)
	and	07fh			;strip parity
	ret

cdwst:	ld	a,group1
	out	(grpctl),a
	in	a,(lsr)
	and	dr
	ret	z			;Z set: nothing waiting
	ld	a,0ffh
	ret
	.dephase
cdwend:

;********************************************************
;*							*
;*  The System Support 1 console driver.  There is no	*
;*  group select to set and no divisor to load - the	*
;*  baud rate went in with the mode registers - so all	*
;*  that is left before a byte is to poll the status.	*
;*							*
;********************************************************

cdsb:
	.phase	condrv
	jp	cdsout			;+0: send the character in C
	jp	cdsin			;+3: wait for a character into A
	jp	cdsst			;+6: is one waiting?

cdsout:	in	a,(s1stat)		;wait for the transmitter to empty
	and	s1txrdy
	jr	z,cdsout
	ld	a,c
	out	(s1data),a
	ret

cdsin:	in	a,(s1stat)		;wait for a character to arrive
	and	s1rxrdy
	jr	z,cdsin
	in	a,(s1data)
	and	07fh			;strip parity
	ret

cdsst:	in	a,(s1stat)
	and	s1rxrdy
	ret	z			;Z set: nothing waiting
	ld	a,0ffh
	ret
	.dephase
cdsend:

;	Both drivers are measured against the window at the copy loop, not
;	against the trap save frame: gobuff is nearer and the level 1 rom's
;	home above it is nearer still.



;* The on-board diagnostics are gone, and their dispatch table with them.
ecode0  equ	$			;End of reset prom code

	ds	3f0h-(ecode0-rom0)	;Fill out the prom

;****************************************************************
;*								*
;* The following special piece of code is where the user task	*
;* begins executing when a reset trap occurs. 			*
;*								*
;****************************************************************

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
;*  The console, as the rest of the monitor reaches it.	*
;*							*
;*  There is no routine here because there is no one	*
;*  console: a machine has the Wunderbus board's 8250 or	*
;*  the System Support 1's 2651, and this half of the	*
;*  rom has no room for code for both.  A driver for	*
;*  whichever was found is built into on-board ram at	*
;*  reset - findcon, in the other half - and these three	*
;*  names are its three entry points, which sit at the	*
;*  same addresses either way.				*
;*							*
;*  The character to output is in the 'C' register and	*
;*  the character received is returned in 'A'.  UCSTS	*
;*  returns with the zero flag set when no character is	*
;*  waiting and with A = FFh when one is.  That is what	*
;*  the driver keeps, so every call to them above is	*
;*  unchanged by which console answered.			*
;*							*
;********************************************************

ucout1	equ	condrv + csend
ucon1	equ	condrv + crecv
ucsts	equ	condrv + credy



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

;	'B' - boot.  There is one boot now and it is the level 1 rom:
;	the resident copy loop at ramhome brings it down from its socket
;	and enters it, which is what reset does for a machine with switch
;	6 off.  No registers are preserved: this does not return, and the
;	loop runs from on-board ram, which the rom going away cannot
;	reach.

uboot:	cp	'B'
	jr	z,uboot1
	cp	'b'
	jp	nz,uerror
uboot1:	jp	boot


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
