        ; subttl	'(c) 1981 Morrow Designs'
	; title   'MPZ-80  MON3.75-M FIRMWARE'
	.z80

;****************************************************************
;*								*
;* Decision one CPU firmware, for the Morrow Designs / Thinker	*
;* Toys Decision one computer.	The monitor routine looks for	*
;* the power on jump addresses on CPU switches which determine 	*
;* address to begin execution (top 5 switches).	 I/O is through	*
;* the Wunderbus I/O motherboard UART 1.  Base address of the	*
;* I/O is assumed to be standard (beginning at port 48H).	*
;* If top five switches are 'On', a hard disk is assumed to be	*
;* the disk device and Boothd is executed.  Enter diagnostic	*
;* mode by grounding pin 13 of header at 12C.			*
;*								*
;* Revised 12/28/82	-	Fixed PIC initialization	*	
;*								*
;* Revised 8/27/82	-	M16 home and load constants	*
;*				changed.  group equ changed to 	*
;*				add int en bit high.		*
;*								*
;* Bobby Dale Gifford and Bob Groppo        			*
;* 10/20/81							*
;*								*
;****************************************************************

	org	0		;Local Ram in task zero
nop	equ	0
jmpop	equ	0c3h		;Jump instruction op-code
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
group2	equ	0Ah		;serial	port 2
group3	equ	0Bh		;serial port 3
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
ltim	equ	08h		;level triggered mode
adi4	equ	04h		;call address intervals = 4
adi8	equ	00h		;call address intervals = 8
sngl	equ	02		;sole system PIC
ic4	equ	01h		;icw4 access bit
lovect	equ	0		;call vectors begin at 0
hivect	equ	0		;call vectors begin at 0
normal	equ	0		;Master/Reg. nest/unbuffered/no AEOI/8085					; -normal setting of OCW4 for Morrow Software
eoi	equ	20h		;non-specific EOI constant
ivalu	equ	init OR ltim OR adi4 OR sngl OR ic4 OR lovect
;****************************************************************
;*								*
;*	HDC Winchester controller equates			*
;*								*
;****************************************************************

revnum	equ	13		
ioaddr	equ	120Q
contrl	equ	ioaddr
status	equ	ioaddr
data	equ	ioaddr+3
functn	equ	ioaddr+2
commd	equ	ioaddr+1
secstat equ	ioaddr+1
dread	equ	1
sector	equ	1
opdone	equ	2
complt	equ	4
header	equ	10Q
drenbl	equ	5
dskrun	equ	7
ready	equ	40Q
system	equ	200Q
stepo	equ	370Q
drivea	equ	374Q
trk0	equ	1

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
secsiz	equ	  7		;1024 byte sectors for CPM
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

;****************************************************************
;*								*
;* Task save areas. Each of the tasks 1-15 have one of these	*
;* structures associated with it.  When tskbase returns, HL	*
;* will point to u.ir for that task + 0 offset.        		*
;*								*
;*             offset						*
;*	u.ir    +0	Interrupt register	(1 byte)	*
;*	u.ix	+1	Index registers		(2 bytes)	*
;*	u.iy	+3				(2 bytes)	*
;*	u.abc	+5	Alternate registers	(2 bytes)	*
;*	u.ade	+7				(2 bytes)	*
;*	u.ahl	+9				(2 bytes)	*
;*	u.aaf	+B				(2 bytes)	*
;*	u.pc	+D	Task PC			(2 bytes)	*
;*	u.bc	+F	Task registers		(2 bytes)	*
;*	u.de	+11				(2 bytes)	*
;*	u.hl	+13				(2 bytes)	*
;*	u.af	+15				(2 bytes)	*
;*	u.sp	+17	Stack pointer		(2 bytes)	*
;*	u.mask	+19	Mask register contents.	(1 bytes)	*
;*								*
;****************************************************************

tasksiz	equ	26
tasks:	ds	tasksiz		;Task 1
	ds	tasksiz		;Task 2
	ds	tasksiz		;Task 3
	ds	tasksiz		;Task 4
	ds	tasksiz		;Task 5
	ds	tasksiz		;Task 6
	ds	tasksiz		;Task 7
	ds	tasksiz		;Task 8
	ds	tasksiz		;Task 9
	ds	tasksiz		;Task 10
	ds	tasksiz		;Task 11
	ds	tasksiz		;Task 12
	ds	tasksiz		;Task 13
	ds	tasksiz		;Task 14
	ds	tasksiz		;Task 15


u.save:	db	0		;Start of user save area
u.ir:	db	0		;Temporary user interrupt reg. save
	dw	0,0,0,0,0,0	;ix,iy,bc',de',hl',af'
u.pc:	dw	0		;Temporary user pc storage
	dw	0,0,0		;bc,de,hl
u.af:	dw	0		;Temporary user af storage
u.sp:   dw      0               ;temporary user stack storage
u.mask:	dw	0		;temporary mask storage
gobuff	equ	$
	ds	14		;task boot buffer
ersav	equ	$	
      	ds	200h-(ersav-ram);Fill out the ram
stack	equ	$		;End of local ram

;****************************************************************
;*								*
;* The following map is used to hold an image of the current	*
;* memory map for all tasks.					*
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
;* only after a reset but not accessible by any other tasks.	*
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
;* of the other switches.  If S1 - S5 are all 'ON' a MORROW	*
;* hard disk is assumed and the 'Boothd' program is executed.	*
;* If pin 2 of 15D is lifted, the diag nostic mode is entered.	*
;*								*
;****************************************************************


; Check all the readable registers


regrd:  out	(0ffh),a		;sync
	ld	hl,trpadd
	ld	a,(hl)			;read trap address reg @ 400h
	inc	hl			
	ld	a,(hl)			;read keyboard reg @ 401h
	inc	hl
	ld	a,(hl)			;read switch reg @ 402h
	inc	hl
	ld	a,(hl)			;read trap status reg @ 403h
	jr	getsw



; Check all the writable registers

regwr:	xor	a			;loop till switch not 00
regwr1:	out	(0ffh),a		;sync
	ld	(mask),a		;write to the mask register
	ld	(dspcol),a		;write to the display column register
	ld	(dspseg),a		;write to the display segment reg.
	cpl	
	cp	0ffh
	jr	z,regwr1
	jr	getsw


; Check the Map RAMs

tmap:   xor	a			;write to map ram / protection ram
	ld	hl,mapram
	out	(0ffh),a		;sync
	ld	(hl),a			;write location 600,0
	inc	hl
	ld	(hl),a			;write 601,0
	cpl
	ld	(hl),a			;write 601,0ffh
	dec	hl
	ld	(hl),a			;write 600,0ffh
	ld	hl,mapram + 01feh
	ld	(hl),a			;write 7fe,0ff
	inc	hl
	ld	(hl),a			;write 7ff,0ff
	cpl
	ld	(hl),a			;write 7ff,00
	dec	hl
	ld	(hl),a			;write 7fe,00
	jr	getsw


; Check the R/W RAMs


tram:	ld	hl,0000h		;write to read/write ram
	out	(0ffh),a
tram1:	xor	a
	ld	(hl),a			;write a 00 to ram 
	cp	(hl)			;read it back
	cpl
	ld	(hl),a			;write an ffh to ram 
	cp	(hl)			;read it back
	bit	0,h
	jr	nz,getsw
	ld	hl,03ffh
	jr      tram1			;write to 3ffh a ffh

; Check the Floating Point Processor


tfpp:   xor	a			;check FPP
	out	(0ffh),a		;sync
	ld	(0c00h),a		;write a 00 to location C00h
	ld	(0c08h),a		;write a 00 to location C08h
	ld	a,(0c00h)		;read c00h


getsw:	ld	a,(keybd)		
	bit	1,a
reset:	jp	z,reset0		;go to the montior if  low
	ld	a,(switch)
	bit	2,a
	jp	z,reset0		;go to the monitor if S6 is on
	and	070h			;strip insignificant bits
	rrc	a			;4 byte offset
	rrc	a
	ld	hl,jtable		;point to beginning of table
	add	a,l
	ld	l,a
	jp	(hl)

; Check the S-100 bus addr and data lines


tbus:   ld	a,0f0h
	ld	(task),a		;force upper task bits high
	ld	a,0ffh			;init the T0 map
	ld	(61eh),a
	ld	a,03
	ld	(61fh),a
	ld	(603h),a
	xor	a
	ld	(602h),a
	out	(0ffh),a		;sync
	ld	(0ffffh),a		;write - bus addresses A0-23 are high
	ld	(task),a		;upper task bits low
	ld	(1000h),a		;write - bus addresses A0-23 are low
	ld	a,0f0h
	ld	(task),a		;force upper task bits high
	ld	a,(0ffffh)		;read  - bus addresses A0-23 are high
	xor	a
	ld	(task),a		;force upper task bits low
	ld	a,(1000h)		;read  - bus addresses A0-23 are low
	jr	getsw


ntbus:  ld	a,0A0h
	ld	(task),a		;force upper task bits high
        ld	a,0aah			;init the T0 map
	ld	(61eh),a
	ld	a,03
	ld	(61fh),a
	ld	(603h),a
	ld	a,55h
	ld	(602h),a
	out	(0ffh),a		;sync
	ld	a,0aah
	ld	(0faaah),a		;write - bus addresses A0-23 = AAAAAA
	ld	a,50h
	ld	(task),a		;upper task bits low = 5
	ld	a,55h
	ld	(1555h),a		;write - bus addresses A0-23 are low
	ld	a,0A0h
	ld	(task),a		;force upper task bits high
	ld	a,(0faaah)		;read  - bus addresses A0-23 are high
	ld	a,050h
	ld	(task),a		;force upper task bits low
	ld	a,(1555h)		;read  - bus addresses A0-23 are low
	jp	getsw

	
; Initialize the maps and jump vectors


reset0: call	reset1
	call	uartst
	call	setup
	jp	gobuff

reset1:	ld	hl,super		;initialize 'super' to the monitor...
settle:	dec	hl			;wait for the hardware to settle down
	ld	a,l
	or	h
	jr	nz,settle
	ld	(hl),jmpop		;- this will be overwritten by the
	inc	hl			;- supervisor but all traps in the 
	ld	(hl),012h		;- meantime will fall into the monitor.
	inc 	hl
	ld	(hl),08h

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
	ld	(map + 2),a		;update the image map
	ld	(ersav),a		;null the error save byte
	ld	a,hstrap		;initialize the mask register
	ld	(mask),a		; -to trap on halts and stops
	ld	(cmask),a

;	Following code checks for ram in system

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
	cp	0			;boot hard disk if switches are all on
	jp	z,boothd
	cp	08h			;If switch 5 is off others are on
	jp	z,nuboot		; - boot DMA controller
	cp	10h			;If switches 4 is off, others on
	jr	z,djdma			; - boot the DJ-DMA floppy device

check:	ld	a,(switch)		;test monitor switch
	bit  	2,a
	ld	a,1			;normal task number
	ld	(15h),de		;initialize the t1 pc save area
	ld	(ctask),a
	jr	nz,nutask		;jump if boot desired

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
	ld	a,ivalu			;sngl,level,4 byte address
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

home:	ld	hl,-1  			;seek to home
        ld      (chan + 1),hl           ; - with ffff steps
	ld	a,noop
	ld	(statis - 1),a		;null command 
	ld	a,1
	ld	(statis),a		;set up the status byte
	call	cloop
	
rdata: 	ld	de,chan			;destination
	ld	bc,endrd - rdtbl	;byte count
	ld	hl,rdtbl		;source
	ldir				;move the read sector command
	call	cloop
	ld	de,0100h		;point to beginning of DMA boot prog.
	jp	check


; cloop1 = 9.6 us

			
cloop:	ld	c,020h
cloop0:	out	(attn),a
	ld	de,0000h
cloop1: ld	a,(statis)		;check drive status
	cp	0ffh			;an FF means command completed
	ret	z
	dec	de			;wait for controller to respond
	ld	a,e
	or	d
	jr	nz,cloop1		;give it time to respond
					;end cloop1

;	Fall through to here on any error

	ld	a,(statis)
	cp	01h
	jr	nz,cloop2	
	dec	c			;give it 10 tries if not rdy error
	jr	nz,cloop0		; - about 20 seconds

cloop2:	pop	de			;re-align the stack pointer
	ld	b,a
	ld	c,'H'
	ld	a,(cmmd)
	ld	d,a

;****************************************************************
;*								*
;* Enter here if DISK controllers don't respond correctly.	*
;* Routine alters gobuff to point to the monitor cout routine.	*	
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
	ld	(15h),de		;save the pointer in t1 pc
	jp	nutask			
	

	
;****************************************************************
;*								*
;* Hard Disk Boot program for Decision 1 EPROM.			*
;* For M26, M10, and M20.					*
;*						11/4/81  BJG	*
;****************************************************************

hdclop:	ld	de,0000h
hdlop1: in	a,(status)
	and	b
	ret	nz
	dec	de
	ld	a,d
	or	e
	jr	nz,hdlop1
	jr	hdcerr

wait:	ld	h,010h
wait0:	ld	de,0
wait1:	in	a,(status)
	and	b
	ret	z
	dec	de
	ld	a,d
	or	e
	jr	nz,wait1
	dec	h
	jr	nz,wait0
	pop	hl			;re-align the stack pointer
hdcerr: ld	c,'D'			; D for HDCA error flag
	in	a,(status)		;get the primary status
	ld	b,a
	in	a,(secstat)		;get the secondary status
	ld	d,a
	jr	derror

boothd:	ld	a,drivea		;select
	out	(functn),a		;    drive A
	ld	a,drenbl		;turn on drive
	out	(contrl),a		;    command register

rloop:	ld	b,ready
	call	wait
	ld	a,dskrun		;enable the
	out	(contrl),a		;    controller

waitz:	in	a,(status)		;test for heads at track 0
	rra
	jr	nc,sdone
	ld	a,stepo			;execute
	out	(functn),a		;    the
	ld	a,drivea		;    step out
	out	(functn),a		;    command

waitc:  ld	b,complt
	call	hdclop
	jr	waitz

sdone:	in	a,(status)		;get an image
	ld	c,a			;    of the status reg

iwait1: in	a,(status)		;wait for
	sub	c			;    the index pulse
	jr	z,iwait1		;    to arrive

iwait2: in	a,(status)		;wait for the
	sub	c			;    next index pulse
	jr	nz,iwait2		;test for head settle

iwait3:	in	a,(status)
	sub	c
	jr	z,iwait3
	ld	a,header		;reset the
	out	(commd),a		;    buffer pointer
	xor	a			;    to header area
	out	(data),a		;head 0
	out	(data),a		;track 0
	inc	a			;sector 1
	out	(data),a		    
	ld	a,system		;system key
	out	(data),a
	ld	a,dread			;issue a
	out	(commd),a		;    read command

waitd:	ld	b,opdone
	call	hdclop
	in	a,(data)		;low order byte of
	ld	l,a			;    bootstrap address
	ld	e,a
	in	a,(data)		;high order byte of
	ld	h,a			;    bootstrap address
	ld	d,a
	and	0f0h			;check for 1st segment of task0
	jr	z,dxloop
	xor	a
	ld	(mapram + 2),a		;T0 map is as normal with no window


lloop:	in	a,(data)		;load
	ld	(de),a			;    the
	inc	e			;    bootstrap
	jr	nz,lloop
	ld	d,h			;save the boot addr for later
	ld	e,l
	ld	a,01
	jp	check

dxloop: ld	a,d
	and	0fh			;strip the segment #
	or	010h			; force the load into seg 0 task 1
	ld	d,a
	jr	lloop


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

bootbl: db	10h			;direction out
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


; Dispatch table for the on-board diagnostic routines

jtable	equ 	$
	jp	regrd			;test all the readable registers
	db	0
	jp	regwr			;check all the writable registers but
 					; -task register
	db	0
	jp	tmap			;check map rams
	db	0
	jp	tram			;check read/write ram
	db 	0
	jp	tfpp			;check fpp
	db	0
	jp	tbus			;check bus read/write addresses
	db	0
	jp	ntbus			;R/W bus with 055h and 0aah
	db	0
	jr	start			;yet to be defined
	
	
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
start:	ld	sp,stack
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
tskbse:	jp	tskbase			;Return task base pointer
nmap:	jp	putmap			;set up new allocation vector, access 
gotsk:	jp	gotask			;switch to new task
getmap:	jp	gtmap			;get the old allocation vector, access
dupmap:	jp	dpmap			;copy user vector into T0 window
what:	jp	monitor			;debugger/monitor called 'MON'
restr: 	jp	restor			;restore task 0 map to normal condition
otask:	jp	oldtask			;jumps to last task before trap
 	

	
trappd:	ld	de,-15			;back up the users pc
	ld 	hl,(u.pc)
	add	hl,de
	ld	(u.pc),hl		;save the users original pc
	ex 	Af,Af'			;save auxilary registers
	exx
	push	Af
	push	hl
	push	de
	push	bc
	push	iy
	push	ix
	ld	a,i			;get interupt register
	push	af			;save it
	ld	a,(ctask)		;get current task number
	ld	sp,stack
	call    tskbse			;get base of task save area
	push	hl			;save the bottom of task save area
	ex	de,hl			;de = bottom task_ save
	ld	hl,u.ir			;hl = bottom of temp_ save
	ld      bc,tasksiz		;repeat count
	push	bc			;save for register print routine
	ldir   				;move registers from temporary store
					;to task_ store area
	ld	hl,(stats)		;get status bits
	push	hl
	bit	2,l			;was the trap a halt?
	jp	nz,super
	call	dupmap			;copy users map into task 0
	ld	hl,(u.pc)		;hl has users adjusted pc 
	ld 	a,(hl)			;a = adjusted pc contents
	cp	nxtbyte			;was it an 76 in next byte??
	jp      nz,super		;jump if not an 76 to supervisor
	inc	hl			;HL point to address after 2nd halt
	jp	(hl)			;jump to byte following the halt 76 
					; - but now in task 0...

           

;****************************************************************
;*								*
;*   Tskbase returns with H & L pointing to the first address	*
;*   in the user's task save area.  User is designated by 	*
;*   register A upon entry.					*	
;*								*
;****************************************************************



tskbase:	
	ld	hl,tasks-tasksiz	;no task zero save area
	ld	de,tasksiz		;size of save area
	and 	0fh			;mask off upper bits
	jr	nz,tsklp		;task 0 and 1 share the same area
	inc	a
tsklp:	add     hl,de			
	dec	a			
	jr	nz,tsklp		
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

oldtask:
	ld	a,(ctask)
gotask:	ld	(ctask),a		;save new task number
	call	tskbase 		;get address of task save area
	ld	de,u.ir			;base of save area
	ld	sp,u.save
	ld	bc,tasksiz		;repeat count
	ldir				;move the registers
	pop	Af			;get back the interupt register
	ld	i,a			;restore it
	pop	ix			;restore auxilliary registers
	pop	iy
	pop	bc
	pop	de
	pop	hl
	pop	af
	ex      Af,Af'
	exx
	pop	hl			;get pc register
	ld	(user+1),hl		;put in jump instruction
	pop	bc			;restore primary registers
	ld	a,jmpop			;lay down the jump instruction
	ld	(user),a		;
	ld	a,(u.mask)		;get back the old mask for that task
	ld	(cmask),a
	ld	hl,(ctask)		;get the task/mask for swapping
	ld	(task),hl		;begin count, eigth instruction
					; is in the user's space
	pop	de			;(1) restore primary registers
	pop	hl			;(2)
	pop	af			;(3)
	ld      sp,(u.sp)		;(4) (5)
	jp	user			;(6)
					; 7th instruction is @ 003 task 0
	
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

gtmap:	call	tsmod		;get task and segment numbers
	ex	de,hl		;save calculated offset in de
	ld	hl,map		;point to beginning of map ram image
	add	hl,de		;add offset to get desired map
	ld	b,(HL)		;get old allocation vector
	inc	hl		;offset to access map
	ld	c,(HL)		;get old access priviledges
	ret 	
		


;********************************************************
;*							*
;*	This routine copies the most recently trapped	*
;*	user's map into task0 actual map and gives T0   *
;*      unlimited privilidges with this memory. The	*
;*      T0 image map is not altered.			*
;*							*
;********************************************************

dpmap:	ld	a,(ctask)
	rlc	a
	rlc	a
	rlc	a
	rlc	a
dpmap0: call    gtmap    		;return with values in b,c
	ld	c,03h			;unlimied access priviledges
	push	af
	and	0fh			;force task # to be 0
	call	tsmod
	ld	de,mapram		;point to T0 actual map
	add	hl,de
	ld	(hl),b			;new access vector
	inc	hl
	ld	(hl),c			;new priviledges
	pop	af
	inc	a			;next map location
	ld	c,a
	and	0fh			
	ret	z			;do it 16 times for all segments
	ld	a,c
	jr	dpmap0

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

putmap:	call	tsmod			;get task and sec #
	ex 	de,hl			;save calculated offset in de
	ld	hl,mapram		;point to beginning of ram map
	call 	putmxx			
	ld	hl,map			;point to beginning of image map
putmxx: add	hl,de			;add offset to selected map
	ld	(hl),b			;write the allocation vector
	inc	hl			;point to access attribute ram
	ld	(hl),c			;write new access atributes
	ret

tsmod: 	ld	l,a			;get task / seg number
	ld	h,0
	add	hl,hl 			;multiply times two
	ret
	
;********************************************************	
;*							*
;*  The following routines make up the debugging tool	*
;*  called UPMON1.  It is a modified version of the 	*
;*  code in the lower half of the EPROM.       		*	
;*							*
;********************************************************


monitor:
	call	ucrlf
	pop	hl			;print the stats and trap address
	call	uladr			;print the task and mask
	pop 	de			;get task size
	dec	de			
	pop	hl			;get the bottom of task save area
	push	hl
	add	hl,de
	ex	de,hl
	pop	hl
	ld	bc,ustart
	push	bc
	jr	udi0			;print out the registers

cold:	ld	a,(ersav)		;retrieve the error byte if any
	ld	c,a
	call	ucout1			;print it
	ld	hl,(ersav + 1)		;retrieve disk command
	call	uladr

ustart:	ld	sp,stack
	LD	DE,USTART		;monitor begins here
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
	JR	USTART
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
	JR	Z,usuo0
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
	LD	HL,(STACK)
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

uretrn:	cp	'C'
	jr	z,uretr1
	cp	'c'	
	jr	nz,ucontr
uretr1:	ld	a,(ctask)
	call	tskbase
	ld	de,mskofst
	add	hl,de
	ld	a,(hl)
	or	08h
	ld	(hl),a
	jp	otask


;	Return to trapped task, execute next instruction and trap back

ucontr:	cp	'U'
	jr	z,ucont1
	cp	'u'
	jr	nz,uboot
ucont1:	ld	a,(ctask)
	call	tskbase
	ld	de,mskofst
	add	hl,de
	ld	a,(hl)
	and	0f6h			;force mask for stop and run enble low
	ld	(hl),a
	jp	otask	

;	Jump to the cpu switch address into task specified by CTASK

uboot:	cp	'B'
	jr	z,uboot1
	cp	'b'
	jp	nz,uerror
uboot1:	ld	a,(ctask)
	call	tskbase
	push	hl
	ld	a,(switch)
	and	0f8h
	cp	0			;check for a HDCA hard disk boot
	jp	z,uboot2
	cp	08h			;check for DMA hard disk boot
	jp	z,uboot2
	cp	010h			;check for DJ-DMA
	jp	z,uboot2
	ld	de,pcofst
	add	hl,de
	ld	(hl),0
	inc	hl
	ld	(hl),a
uboot2: ld	a,(cmask)
	pop	hl
	ld	de,mskofst
	add	hl,de
	ld	(hl),a
	jp	oldtask



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
;


ecode1	equ	$
	ds	3f0h-(ecode1-rom1)

;********************************************************
;*   							*
;*   The following piece of code is where the user	*
;*   task begins execution whenever a trap occurs.	*
;*   The users registers and sp are saved in the 	*
;*   temporary users store area.			*
;*							*
;********************************************************

	nop				;must be a nop to void instruction
	ld 	(u.sp),sp		;save the users stack pointer
	ld	sp,u.sp			;set sp to the temporary save area
	push	Af			;save the users registers in temp area
	push	hl
	push	de
	push	bc
	call 	svtrap			;go to supervisor trap, pc is save


	halt				;to allow halts in task 0
erom1	equ	$						
	ds	400h-(erom1-rom1)
	


fpp0:	ds	8
fpp1:	ds	1

	end
