	title		'-=< Mult I/O Test >=-'

*		Revised - 6/1/81

*****************************************
*		equates			*
*****************************************

*	switches for conditional assembly.	*
rev0	equ	1	;multi i/o rev 0.
rev3	equ	0	;multi i/o rev 3.
wb14	equ	0	;wunderbuss 14 slot motherboard multi i/o.

* i/o port definitions *
base	equ	10h	;base of master multi i/o board.
tbase	equ	48h	;base of multi i/o under test.

* group definitions *
group0	equ	0	;when sent to base+grp, 1 of the 4
group1	equ	1	;groups of 7 i/o ports is switched into
group2	equ	2	;the i/o space. 0=par, clk, pic, banks.
group3	equ	3	;1=serial1 - 2=serial2 - 3=serial3.

* allocation of ports to groups *
daisy	equ	group0	;2 parallel outputs and 1 input (level 6).
clock	equ	group0	;real time clock and interrupt timer (level 7).
pic	equ	group0	;programmable interrupt controller.
banks	equ	group0	;bank selection port.
pario	equ	group0	;wb14 parallel i/o port.

serprt1	equ	group1	;serial port #1, int. level 3.
serprt2	equ	group2	;serial port #2, int. level 4.
serprt3	equ	group3	;serial port #3, int. level 5.

* group 0 port definition *
daisy0	equ	0	;parallel output latch #0 - parallel input port.
daisy1	equ	1	;parallel output latch #1.
	if	rev0
clk	equ	2	;clock port.
clkr	equ	3	;clock int. reset.
bank	equ	2	;bank select port.
ribbon	equ	2	;ribbon lift output (1 bit).
	endif
	if	wb14
status	equ	1	;parallel port flags
sense	equ	1	;motherboard sense switch input.
clk	equ	2	;clock and timer.
select	equ	2	;printer select enable.
par	equ	3	;northstar parallel port emulator.
pcont	equ	6	;parallel port control port.
ribbon	equ	2	;ribbon lift output (1 bit).
	endif
pic0	equ	4	;interrupt initialization and control port #0.
pic1	equ	5	;interrupt initialization and control port #1.

* groups 1, 2, 3 port definition *
rbr	equ	0	;receiver buffer register.
thr	equ	0	;transmitter holding register.
ier	equ	1	;interrupt enable register.
dll	equ	0	;baud rate divisor latch (least significant).
dlm	equ	1	;baud rate divisor latch (most significant).
iir	equ	2	;interrupt identification register.
lcr	equ	3	;line control register.
mcr	equ	4	;modem control register.
lsr	equ	5	;line status register.
msr	equ	6	;modem status register.
grp	equ	7	;group selection port.

*********************************************************
*		memory address definitions		*
*********************************************************

bdos	equ	0005	;system call entry point.
wboot	equ	0000	;warm start entry.
tpa	equ	100h	;transient program area.

*********************************************************
*		data word definitions			*
*********************************************************

zero	equ	0000

*********************************************************
*		data byte definitions			*
*********************************************************

imask	equ	0	;pic interrupt mask byte.
baud0	equ	3	;least significant baud rate divisor.
baud1	equ	0	;most significant baud rate divisor.
icw1	equ	1eh	;int. init. control word.
ocw1	equ	imask	;int. operation control word (mask).
cr	equ	0dh	;carriage return.
lf	equ	0ah	;line feed.
t64	equ	10h	;64hz timed pulse.
const	equ	11	;console status bdos call.
readcon	equ	10	;read console buffer bdos call.
clkcl	equ	8	;clock cl bit.
clkclk	equ	2	;clock clk bit.
cstb	equ	20h	;clock stb bit.
shft	equ	4	;clock shift bits.
rclk	equ	0ch	;read clock
wclk	equ	8	;write clock.
clen	equ	80h	;command length byte.
cbuff	equ	81h	;command buffer string.
reghld	equ	0	;clock register hold command.
jswch	equ	8	;jig parallel port switch.
bell	equ	7	;[^g]
esc	equ	1bh	;escape.
auto	equ	255	;repititions of auto test.

*************************************************
*		bit definitions			*
*************************************************

* [ier] interrupt enable register. *
edri	equ	1	;enable data ready interrupt.
etbei	equ	2	;enable txmtr holding register empty interrupt.
elsi	equ	4	;enable line status register interrupt.
emsi	equ	8	;enable modem status interrupt.
uienbl	equ	0fh	;uart interrupt enable byte.
* bits 0-3 low disables all uart interrupts. *

* [iir] interrupt identification register. *
intpend	equ	1	;interrupt pending (active low ).
iid1	equ	2	;interrupt identification bit 1.
iid2	equ	4	;interrupt identification bit 2.

* interrupt identification register status definitions / priority	*
*	iir = 6		-	line status register int. first.
*	iir = 4		-	data ready int. second.
*	iir = 2		-	txmtr holding register empty int. third.
*	iir = 0		-	modem status int. fourth.

* [lcr] line control register of serial data transmission protocol. *
wls0	equ	1	;word length select bit 0.
wls1	equ	2	;word length select bit 1.
stb	equ	4	;stop bit select.
pen	equ	8	;parity enable.
eps	equ	10h	;even parity select.
spar	equ	20h	;stick parity.
sbrk	equ	40h	;set break.
dlab	equ	80h	;baud rate divisor latch access bit.

* [mcr] modem control register of eia line drivers. *
dtr	equ	1	;data terminal ready.
rts	equ	2	;request to send.
out1	equ	4	;user defined output (if loop, out1 = ri)
out2	equ	8	;user defined output (if loop, out2 = dcd)
loop	equ	10h	;ace diagnostic. sets all communication to half duplex.

* [lsr] line status register of serial data reception. *
dr	equ	1	;data ready.
oe	equ	2	;overrun error.
pe	equ	4	;parity error.
fe	equ	8	;framing error.
bi	equ	10h	;break interrupt.
thre	equ	20h	;transmitter holding register empty.
tsre	equ	40h	;transmitter shift register empty.

* [msr] modem status register of eia line receivers. *
dcts	equ	1	;delta clear to send.
ddsr	equ	2	;delta data set ready.
teri	equ	4	;trailing edge ring indicator.
ddcd	equ	8	;delta data carrier detect.
cts	equ	10h	;clear to send.
dsr	equ	20h	;data set ready.
ri	equ	40h	;ring indicator.
dcd	equ	80h	;data carrier detect.

*********************************
*	end of equates		*
*********************************

	org	tpa	;originate code at the transient program area.

*************************************************
*		first level routines		*
*************************************************

entry:	dw	zero	;hot patch area for jump, call, restart, etc.
	dw	zero	;these four bytes used for program development only.

* initialization *
main: 	lhld bdos+1	;relocate the stack to the very end of the tpa.
	sphl
	lxi d,msg	;initial sign-on,
	call pmsg	;sent to console.
	call clrbrd0	;initialize the master multi i/o.
	call clrbrd1	;initialize the multi i/o under test.
	call init1	;clear interrupts, store restart table, etc.

* command delegation *
mloop:	call getkey	;wait for a command. returns w/char. in 'a'.
	call pcr
	cpi '0'	!	jz wboot	;exit to cp/m.
	cpi '1'	!	cz sertst1	;test serial port #1.
	cpi '2'	!	cz sertst2	;test serial port #2.
	cpi '3'	!	cz sertst3	;test serial port #3.
	cpi '4'	!	cz partst	;test the parallel ports.
	cpi '5'	!	cz clkset	;test the clock and int. timer.
	cpi '6'	! 	cz ramtst	;test the ram and eprom.
	cpi '7'	!	cz intrupt	;test the interrupt structure.
	cpi '8'	!	cz baudset	;set all serial port baud rates.
	cpi '9'	!	cz autotst	;run all tests automatically.
	cpi cr	!	jz main		;re-initialize.

prompt:	call pcr
	lxi d,msg0	
	call pmsg	;prompt next command.

	jmp mloop	;get next command.

sertst1:lxi d,msg1	
	call pmsg	;print serial test message,
	mvi e,'1'	;and the port#.
	call pchar !	call pcr
	mvi a,serprt1	;pass the port# to the test.
	jmp sertest

sertst2:lxi d,msg1
	call pmsg	;print serial test message,
	mvi e,'2'	;and the port #.
	call pchar !	call pcr
	mvi a,serprt2	;pass the port# to the test.
	jmp sertest

sertst3:lxi d,msg1
	call pmsg	;print serial test message,
	mvi e,'3'	;and the port #.
	call pchar !	call pcr
	mvi a,serprt3	;pass the port# to the test.
	jmp sertest

* parallel port tests *
partst:	lxi d,msg4
	call pmsg	;print the parallel test message.
	mvi a,daisy
	out base+grp	;set access to the master's parallel ports.
	out tbase+grp	;set access to the test board's parallel ports.

	if wb14
	mvi a,80h	;set the select line high.
	out tbase+select
	endif

	mvi a,0
	out base+daisy1	;channel test daisy0 to mast daisy0 via jig.
	mov d,a			;save jig switch setting.
	lda autoflg
	mov c,a		;autotest rep. count.
	jmp partest

******************

clkset:	call skipb
	jnz sett
display:call displl
	call gchar
cauto:	nop
	rnz
	mvi e,cr
	call pchar
	jmp display

********************

ramtst:	call pcr

	if	wb14
	lxi d,msg5
	call pmsg
	ret

	else
	lxi d,msg22
	call pmsg
	jmp ramtest
	endif

******************

intrupt:lxi d,msg2
	call pmsg
	jmp intrpt

*******************
	
baudset:lxi d,msg15
	call pmsg
	jmp set1

******************

autotst:mvi a,auto
	sta autoflg
	mvi a,0c8h	;this is a 'return if zero' opcode. 
	sta sauto	!	sta pauto	!	sta cauto
	call pcr
	call sertst1	!	call pcr
	call sertst2	!	call pcr
	call sertst3	!	call pcr
	call partst	!	call pcr	!	call pcr
	call clkset	!	call pcr
	call ramtst	!	call pcr
	xra a
	sta cauto !	sta pauto !	sta sauto !	sta autoflg
	ret

*************************************************
*		second level routines		*
*************************************************

clrbrd0:mvi a,serprt1
	sta clrprt0
	call masinit	;clear master serial #1.
	mvi a,serprt2
	sta clrprt0
	call masinit	;clear master serial #2.
	mvi a,serprt3
	sta clrprt0
	call masinit	;clear master serial #3.
	ret

clrbrd1:mvi a,serprt1
	sta clrprt1
	call tstinit	;clear test serial #1.
	mvi a,serprt2
	sta clrprt1
	call tstinit	;clear test serial #2.
	mvi a,serprt3
	sta clrprt1
	call tstinit	;clear test serial #3.
	ret


sertest:		;this is the serial test routine for all three ports.
	sta minprt	;on entry, 'a' contains the port # to be tested.
			;the status, input and output routines are set to
	sta moutprt	;access the appropriate port by these eight 'store a's.
			;this method of modifying the routines themselves is
	sta tinprt	;also used in the initialization process.
			;the bytes that these 8 addresses point to are
	sta toutprt	;initially 0, and must be set before they are used.

	lda autoflg
	mov d,a
	mvi b,0			;clear byte to be sent, received and compared.
sloop:	inr b		;set next iteration.
	mov c,b		;the output routine sends register 'c'
	call mout		;through the jig and into the test board,
	call tin		;to be received in 'a' and passed 
	cmp b
	jnz err1a
	mov c,a				;to the test board's output routine,
	call tout		;to be sent through the jig into the master
	call min		;which is received in 'a' and compared
	cmp b		;with what was sent originally.
	jnz err1b				;flag error if no match found.
	cpi 0			;check for 256 bytes sent and received.
	jnz sloop	;send the next if not.
	dcr d
sauto:	nop
 	mvi e,'.'
	call pchar	;print a dot for every 256 bytes tested good.
	call gchar
	jz sloop	;check if keydown, continue if not.
	cpi esc
	rz		;escape.
	cpi ' '
	cz pause	;pause.
	jmp sloop		;ignore all other keys.

*************

partest:inr a		;set next iteration.
	mov b,a			;save byte to be sent, received and compared. 

	if	wb14
	mov a,d	!	ori 80h	!	mov d,a
	mov a,b
	out tbase+pario
	in tbase+pario
	cmp b
	jnz err2
	mvi e,'-'
	call pchar
	mov a,d	!	xri 80h	!	mov d,a
	endif

	mov a,b
	out base+daisy0		;'a' is sent out master port #0.
	in tbase+daisy0		;and received by the test input port.
	cma			;the data is sent inverted so get it strait.
	cmp b
	jnz err2
	mov a,d
	ora a
	jz latch0			;jump if switch =0, fall if switch =1. 
	mov a,b
	out tbase+daisy1		;switch =1, so send out test port #1.
	jmp swap

latch0:	mov a,b			;get test data back.
	out tbase+daisy0		;switch =0, so send out test port #0
swap:	in base+daisy0	;received by master input port.
	cma		;re-invert the test data.
	cmp b						;compare with original.
	jnz err2			;exit to err message if not the same.
	cpi 0
	jnz partest	;display a '-' or '=' for every 256 good passes.
	mov a,d			;recall jig switch setting.
	ora a
	jnz pass	;jump if switch =1, fall if switch =0.
	mvi a,jswch
	out base+daisy1		;throw the jig switch high (1).
	mov d,a		;store the setting.
	mvi e,'a'	;indicate that 256 bytes have passed test port #1.
	jmp passed

pass:	xra a
	out base+daisy1		;throw the jig switch low (0).
	mov d,a		;store the setting.
	mvi e,'b'	;indicate that 256 bytes have passed test port #0.
passed:	call pchar	;actually print 'a' or 'b'.
	dcr c
pauto:	nop
	call gchar	;check for keydown.
	jz partest		;continue if no key found.
	cpi esc
	rz		;escape.
	cpi ' '
	cz pause	;pause.
	jmp partest		;ignore all other keys.

*******************

ramtest:if	rev0 or rev3
	call mpoint
	mvi b,0ffh
stst1:	call sto
	jnz stst1

	call mpoint
rtst1:	call recall
	push psw
	cpi 0ffh
	jnz err11
	pop psw
	jnz rtst1

	call mpoint
	mvi b,00
stst0:	call sto
	jnz stst0

	call mpoint
rtst0:	call recall
	push psw
	cpi 0
	jnz err11
	pop psw
	jnz rtst0
	lxi d,msg23
	call pmsg
	ret

mpoint:	lxi h,0f000h
	lxi d,400h
	ret

sto:	mov m,b
	inx h
	dcx d
	xra a
	ora d
	ora e
	ret

recall:	mov b,m
	inx h
	dcx d
	xra a
	ora d
	ora e
	mov a,b
	ret
	endif

**********

intrpt:
task:	di
	call gchar
	jz enable
	cpi esc	!	rz
	cpi '0'	!	cz vector0
	cpi '1'	!	cz vector1
	cpi '2'	!	cz vector2
	cpi '3'	!	cz vector3
	cpi '4'	!	cz vector4
	cpi '5' !	cz vector5
	cpi '6'	!	cz vector6
	cpi '7'	!	cz vector7
	cpi ' '	!	cz pause
enable:	ei
	nop
	nop
	nop
	jmp task


int0:	mvi e,'0'
	jmp printc

int1:	mvi e,'1'
	jmp printc

int2:	mvi e,'2'
	jmp printc

int3:	mvi e,'3'
	jnc printc
	mvi a,serprt1
	jmp id

int4:	mvi e,'4'
	jnc printc
	mvi a,serprt2
	jmp id

int5:	mvi e,'5'
	jnc printc
	mvi a,serprt3
	jmp id

int6:	mvi e,'6'
	jnc printc
	jmp pwint

int7:	push psw
	mvi a,clock
	out tbase+grp
	if	wb14
	in tbase+sense
	ani 0ch
	adi 0d0h
	out tbase+clk
	xri 20h
	out tbase+clk
	endif
	mvi e,'7'
	in tbase+clkr	
	pop psw
	jmp printc

id:	out tbase+grp
	in tbase+iir
	push psw
	ani 1
	jnz err12
	pop psw
	ani 6
	cz err9
	cpi 6	!	cz err8
	cpi 4	!	jz newch
	cpi 2	!	jz newch
	jmp printn

newch:	in tbase+rbr
	out tbase+thr
	jmp printn

pwint:	lxi d,msg25
	call pmsg
	push psw
	mvi a,40h	;mask off print wheel ready int.
	out tbase+pic1
	pop psw
	mvi e,' '
	jmp printn

printc:	jc printn
	push d
	lxi d,msg3
 	call pmsg
	pop d

printn:	call pchar
	mvi e,' '
	call pchar
	mvi a,pic
	out tbase+grp
	mvi a,20h
	out tbase+pic0
	xra a
	ret

*******************

set1:	call getkey
	call toupper
	call pcr
	sui 41h
	jm err7
	cpi 16h
	jp err7
	cpi 12h
	push psw
	cp tpset
	pop psw
	jp reclear
	rlc
 	lxi d,zero
	mov e,a
	lxi h,btabl
	dad d
	mov a,m
	sta mbaudl
	sta tbaudl
	inx h
	mov a,m
	sta mbaudh
	sta tbaudh
reclear:call clrbrd0
	call clrbrd1
	ret

tpset:	cpi 12h	!	cz tp32
	cpi 13h	!	cz tp64
	cpi 14h	!	cz tp256
	cpi 15h	!	cz tp2048
	sta tprate0
	sta tprate1
	ret

tp32:	mvi a,0dch
	ret
tp64:	mvi a,0d0h
	ret
tp256:	mvi a,0d4h
	ret
tp2048:	mvi a,0d8h
	ret

*************************************************
*		third level routines		*
*************************************************

* initialization *
init1:	di
	mvi a,0ffh
	out 0feh	;compupro cpu inte port.

	lxi d,table
	lxi h,table and 0ffe0h
 	push h
	mvi c,32
tloop:	ldax d
	mov m,a
	inx h
	inx d
	dcr c
	jnz tloop

	mvi a,pic
	out tbase+grp
	pop h
	push h
	mov a,l
	ori icw1
	out tbase+pic0
	mov a,h
	out tbase+pic1
	mvi a,ocw1
	out tbase+pic1

	mvi a,0c3h
	mvi b,7
	lxi d,0008
	lxi h,0008
c3	mov m,a
	dad d
	dcr b
	jnz c3

	mvi a,7
	pop h
	push psw
	lxi b,0009
	lxi d,0006
	dad d
	xchg
loc	ldax d
	stax b
	inx d
	inx b
	ldax d
	stax b
	mvi a,7
	add c
	mov c,a
	mvi a,3
	add e
	mov e,a
	pop psw
	dcr a
	push psw
	jnz loc
	pop psw
	ret

*******************

masinit:
clrprt0	equ	$+1
	mvi a,zero
	out base+grp
	mvi a,uienbl
	out base+ier
	mvi a,dlab
	out base+lcr
mbaudl	equ	$+1
	mvi a,baud0
	out base+dll
mbaudh	equ	$+1
	mvi a,baud1
	out base+dlm
	mvi a,wls0+wls1+stb
	out base+lcr

	mvi a,clock
	out base+clk
tprate0	equ	$+1
	mvi a,0d0h	;(64hz) or 14h(256hz) or 18h(2048hz)