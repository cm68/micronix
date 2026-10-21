;
; =====================================================================
; sys/mio.s  --  Z80 translation of sys/mio.anat (asz dialect)
; =====================================================================
;
; ------- A-NATURAL SOURCE: declarations -------
; /*
;  * Morrow Designs multiple Mult I/O driver
;  * Len Edmondson
;  * February, 1982
;  *
;  * sys/mio.s
;  * Changed: <2022-01-04 11:03:49 curt>
;  */
;
; /* THESE CONSTANTS DEPEND ON # include "tty.h" */
;
;
; MODE	:= 4		/low byte of tty mode (offset into tty _str)
; STATE	:= 6		/tty state (offset into tty structure
; DEV	:= 8		/minor device number
;
; TTSIZE	:= 38		/* sizeof (struct tty) */
; TTSIZE2 := 76		/* X 2 */
; TTSIZE3 := 114		/* X 3 */
;
; public	miobase
; public	_mttys
; public	_ace_start
; public	_ace_stop
; public	_ace_putc
; public	_ace_set
; public	_console
; public	intrupt
; public	m1int
; public	m2int
; public	m3int
; public	clkint
;
; /* absolute I/O addresses on the Master Mult I/O */
;
;
; MBASE	:= 0x48		/base io address for master multiboard
; MUDATA	:= MBASE	 /port for uart data
; MLOBAUD := MBASE	 /port for low byte of baud rate
; MUABLE	:= MBASE[1]	 /port to enable uart interrupts
; MHIBAUD := MBASE[1]	 /port for high byte of baud rate
; MIIR	:= MBASE[2]	 /* interrupt identification _reg (master) */
; CLOCK	:= MBASE[2]	 /clock port
; CLKCLR	:= MBASE[2]	 /clear clock-_latch Use base[3] for multio rev < 3
; MREGSET := MBASE[3]	 /port for uart register selection
; MMCNTRL := MBASE[4]	 /modem control register
; ICNTRL	:= MBASE[4]	 /port to interrupt controller
; ICNTRL1 := MBASE[5]	 /another
; MOCW1	:= MBASE[5]	 /* Operation control word 1 */
; MUSTATUS := MBASE[5]	 /* Master Uart(ace) status */
; MMSTATUS := MBASE[6]	/* Master Modem status */
; MSELECT := MBASE[7]	/offset to select uart or _int controller
;
;
; /* offsets into mio I/O space */
;
; BASE	:= 0	     /base io address for master multiboard
; UDATA	:= 0
; LOBAUD	:= 0
; HIBAUD	:= 1
; UABLE	:= 1
; INDAISY := 0
; LODAISY := 0
; HIDAISY := 1
; MCNTRL	:= 4
; USTATUS := 5
; SELECT	:= 7
; REGSET	:= 3
; OCW1	:= 5
; OCW2	:= 4
; ICW1	:= 4
; ICW2	:= 5
; ICW3	:= 5
; ICW4	:= 5
;
; /*	masks and bits */
;
; /* Line status register. */
;
; ERROR := 030		/* Framing error or break interrupt. */
; 			/* Should occur on baud rate error */
;
; YES	:= 1
; NO	:= 0
; IRR	:= 0x0b
; EMPTY	:= 040		/transmit buffer empty
; CLR2SND := 020
; SHAKE	:= 0200		/do hardware handshaking
; CLKRATE := 034		/clock rate code
; 			/for 32 HZ, use rate = 034, strb = 074
; 			/for 64 HZ, use rate = 020, strb = 060
; CLKSTRB := 074		/rate | 040
; TIMSET	:= 010		/put new NEC clock chip in time-set mode
; TIMSTRB	:= 050		/strobe for above
;
; NORMAL	:= 7		/* uart data reg: 8 bits + 2 stop bits */
; BAUDREG := 0200		/set uart registers to accept new baud rate
; ININT	:= 4		/interrupt caused by input character ready
; OUTINT	:= 2		/interrupt caused by transmit buffer empty
; MODINT	:= 0		/modem status interrupt
; OENABLE := 013		/enable output, input, and modem interrupts
; ODSABLE := 011		/disable output, enable input and modem intrpts
; IDSABLE	:= 012		/enable output and modem, disable input intrpts
; IODSABLE := 000		/disable all interrupts
; BOUND32 := 0340		/mask off least significant 5 bits
; ICWORD1 := 037		/level triggered, 4 byte vectors, 1 controller
; ICWORD4 := 014
; ENDINT	:= 0240		/most recent interrupt is done, rotate priorities
;
; PENABLE := 050		/enable PIC and Paralell printer
; PICONLY := 010
;
; MACE1	:= 051
; MACE2	:= 052
; MACE3	:= 053
;
; INTSOFF := 0377		/mask off all interrupts
;
; /* ARMMASTER is the master pic _int mask */
; /* IR0/IR1 (hd-dma, djdma) stay masked until open; IR6 is the slave's
;  * cascade line, so the value is 0x03, not the old 0x43 (daisy). */
;
;
; ARMMASTER  := 003	/allow interrupts 2 - 7 (ttys, clock, cascade)
;
; CONNECT := 3		/data terminal ready, request to send
;
; /* parallel port daisy printer commands */
;
; RIBLIFT := 0200
; BACKWARDS := 040
; MOTION	 := 0200
; VERTICAL := 0100
; DIR	:= 4
; HALF	:= 8
; VSTROBE := 020		/* Vertical strobe - bit 4 */
; HSTROBE := 040		/* Horizontal strobe - bit 5 */
; PSTROBE := 0100		/* print wheel strobe - bit 6 */
; RSTROBE := 16		/* Mult I/O reset strobe - bit 4 */
; MRSTROBE := 0200	/* mother board restore strobe is different - bit 7 */
;
; cma := 0x2f
; invert := 0x2f
; return := ret
;
MODE=4
STATE=6
DEV=8
TTSIZE=38
TTSIZE2=76
TTSIZE3=114
	.globl	miobase, _ace_start, _ace_stop
	.globl	_ace_putc, _ace_set, _console, intrupt, m1int, m2int
	.globl	m3int, clkint, ss1_clkint, ss1_txint, ss1_rxint
	.extern	_di, _ei, dicount, _mttys, _ttyin, _ttyout
	.extern	_mumint, _ttyerror, _clock
MBASE=0x48
MUDATA=MBASE+0
MUABLE=MBASE+1
MIIR=MBASE+2
CLKCLR=MBASE+2
ICNTRL=MBASE+4
MUSTATUS=MBASE+5
MMSTATUS=MBASE+6
MSELECT=MBASE+7
UDATA=0
LOBAUD=0
HIBAUD=1
UABLE=1
REGSET=3
MCNTRL=4
USTATUS=5
SELECT=7
OCW1=5
OCW2=4
ERROR=0x18
EMPTY=0x20
CLR2SND=0x10
SHAKE=0x80
ININT=4
OUTINT=2
MODINT=0
OENABLE=0x0B
ODSABLE=0x09
IODSABLE=0
PENABLE=0x28
ENDINT=0xA0
MACE1=0x29
MACE2=0x2A
MACE3=0x2B
TTY_ACE0=4		; the first ACE's minor; 4-6 are ace 1-3
NORMAL=7
BAUDREG=0x80
CONNECT=3

; ------- A-NATURAL SOURCE: intrupt -------
; /save and restore registers around an interrupt
; intrupt:
;
; /* save registers */
;
; 	*sp <> hl	/stack = hl, hl points to address of routine
; 	sp <= de <= bc <= af
;
; 	dicount = (a = 1);	/* dicount = 1; */
;
; 	hl =a^ hl	/hl = address of routine
; 	call pchl	/do service routine
;
; 	dicount = (a ^ a);	/* dicount = 0; */
;
;
; 	call intrdismiss	/* EOI the PIC(s), sys/multio.c */
;
;
; /* restore registers */
;
; 	sp => af => bc => de => hl /* pop, pop, pop, pop ! */
;
; 	ei;
; 	return;
;
intrupt:
	ex	(sp),hl
	; Whitesmith's C only touched af/bc/de/hl, so the original save
	; list stopped there.  ccc also uses IX (struct pointer), IY (frame
	; ptr) and - through its lmul/ldiv/fexb runtime - the shadow set.
	; Save ix/iy and af'/bc'/de'/hl' exactly as the firmware trap code
	; (alltrp) does, or a handler clobbers whatever it interrupts.
	ex	af,af'
	exx
	push	hl
	push	de
	push	bc
	push	af
	exx
	ex	af,af'
	push	ix
	push	iy
	push	de
	push	bc
	push	af
	ld	a,1
	ld	(dicount),a
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	call	pchl
	xor	a
	ld	(dicount),a
	call	intrdismiss
	pop	af
	pop	bc
	pop	de
	pop	iy
	pop	ix
	ex	af,af'
	exx
	pop	af
	pop	bc
	pop	de
	pop	hl
	exx
	ex	af,af'
	pop	hl
	ei
	ret

; ------- A-NATURAL SOURCE: pchl -------
; /* pchl - indirect call */
;
;
;
; pchl:	jmp *hl;	/* a = hl; (*a) (); */
;
pchl:
	jp	(hl)

; ------- intrdismiss -------
; The interrupt-dismiss hook, called from intrupt after the handler
; returns where the inline EOI used to be.  The resident dispatcher
; (ovlntr, sys/probe.c) now supplies the EOI itself - pic_eoi(line) - so
; there is nothing left for this hook to do.  It stays as a call site so
; the inline EOI is not re-added by accident.
intrdismiss:
	ret

; ------- A-NATURAL SOURCE: badint -------
; /unimplimented interrupt
; badint:
; 	ret
;
badint:
	ret

; ------- A-NATURAL SOURCE: tick -------
; /hard disk interrupt
;
; /* clock interrupt */
;
; tick:
; 	/
; 	0;
; 	/
;
tick:
	.defb	0

; ------- A-NATURAL SOURCE: clkint -------
; clkint:			/clock interrupt
; 	/
; 	/* divide down the clock frequency by 4 */
;
; 	/* HERTZ will become 8 */
; 	/* if (++tick % 4 == 0) clock (); */
;
; 	a = tick + 1 -> tick & 3; cz _clock;
;
; 	a = PENABLE; out; MSELECT      /select the interrupt controller
;
; 	in; CLKCLR	/clear clock latch
;
; 	return;
; 	/
;
clkint:
	ld	a,(tick)
	inc	a
	ld	(tick),a
	and	3
	call	z,_clock
	ld	a,PENABLE
	out	(MSELECT),a
	in	a,(CLKCLR)
	ld	a,0x67		; specific EOI, master IR7 (the clock)
	out	(ICNTRL),a
	ret

; ------- ss1_clkint -------
; The SS1's clock interrupt: the 8253 heartbeat on the slave's IR1.  Like
; the MultIO's clock (int7), it is entered through intrupt, which saves
; the registers and re-enables interrupts on the way out.  The handler
; does the divide-by-four tick and the SS1's own EOI - a specific EOI to
; the slave (level 1) and to the master's cascade (level 7), where the
; MultIO clears a clock latch instead.
;
ss1_clkint:
	call	intrupt
	.defw	ss1_clk

ss1_clk:
	ld	a,(tick)
	inc	a
	ld	(tick),a
	and	3
	call	z,_clock
	ld	a,0x61		; OCW2 specific EOI, level 1 (the timer)
	out	(072h),a	; the slave 8259 command port
	ld	a,0x67		; OCW2 specific EOI, level 7 (the cascade)
	out	(070h),a	; the master 8259 command port
	ret

; ------- ss1_txint -------
; The 2651's transmitter-ready interrupt, the slave's IR6.  ttyout sends
; one queued character, or stops the transmitter when the queue is empty;
; the SS1 is minor 1 and lives in mttys[0].  EOI before the return: if the
; queue still has characters, TxRDY is asserted again and the next
; interrupt takes one more, which is how the queue drains.
ss1_txint:
	call	intrupt
	.defw	ss1_tx

ss1_tx:
	ld	hl,_mttys		; the SS1 tty, minor 1, slot 0
	call	_ttyout
	ld	a,0x66		; OCW2 specific EOI, level 6 (TxRDY)
	out	(072h),a	; the slave 8259 command port
	ld	a,0x67		; OCW2 specific EOI, level 7 (the cascade)
	out	(070h),a	; the master 8259 command port
	ret

; ------- ss1_rxint -------
; The 2651's receiver-ready interrupt, the slave's IR7.  Read the data
; register and hand the character to ttyin with the SS1 tty (minor 1,
; mttys[0]).
ss1_rxint:
	call	intrupt
	.defw	ss1_rx

ss1_rx:
	in	a,(07ch)	; the received character
	ld	c,a
	ld	b,0
	ld	hl,_mttys		; the SS1 tty, minor 1, slot 0
	push	hl
	ld	l,c
	ld	h,b		; the character, arg0
	call	_ttyin
	pop	af		; drop the tty argument
	ld	a,0x67		; OCW2 specific EOI, level 7 (RxRDY)
	out	(072h),a	; the slave 8259 command port
	ld	a,0x67		; OCW2 specific EOI, level 7 (the cascade)
	out	(070h),a	; the master 8259 command port
	ret

; ------- A-NATURAL SOURCE: ace -------
; /* a bit of data */
;
;
; 	ace:	 0	/ number of the current ACE being polled
;
ace:
	.defb	0

; ------- A-NATURAL SOURCE: miobase -------
; 	miobase: 0	/* base I/O address of current MIO board */
;
miobase:
	.defb	0

; ------- A-NATURAL SOURCE: m1int -------
; m1int:
; 	/
; 	hl = &_mttys; bc = TTSIZE; hl + bc;
; 	a = MACE1;
; 	jmp maint;
; 	/
;
m1int:
	ld	hl,_mttys
	ld	bc,TTSIZE
	add	hl,bc
	ld	a,MACE1
	ld	b,0x63		; specific EOI, master IR3 (Master ACE 1)
	call	ireoi
	jp	maint

; ------- A-NATURAL SOURCE: m2int -------
; m2int:
; 	/
; 	hl = &_mttys; bc = TTSIZE2; hl + bc;
; 	a = MACE2
; 	jmp maint;
; 	/
;
m2int:
	ld	hl,_mttys
	ld	bc,TTSIZE2
	add	hl,bc
	ld	a,MACE2
	ld	b,0x64		; specific EOI, master IR4 (Master ACE 2)
	call	ireoi
	jp	maint

; ------- A-NATURAL SOURCE: m3int -------
; m3int:
; 	/
; 	hl = &_mttys; bc = TTSIZE3; hl + bc;
; 	a = MACE3
; 	jmp maint;
; 	/
;
m3int:
	ld	hl,_mttys
	ld	bc,TTSIZE3
	add	hl,bc
	ld	a,MACE3
	ld	b,0x65		; specific EOI, master IR5 (Master ACE 3)
	call	ireoi
	jp	maint

; ------- ireoi -------
; Dismiss a resident (non-module) interrupt line on the MultIO's master
; 8259.  intrdismiss does nothing for these lines: the module lines find
; their EOI in ovlntr (sys/probe.c), and the SS1's handlers are entered
; through its own vectors and dismiss both of its PICs.  The clock, the
; three ACEs and the parallel/cascade strays are wired straight to the
; master, so the handler names its own level.
;
; a = the ACE group select the handler wants in A (preserved), b = the
; OCW2 specific-EOI byte, 0x60 | level.
ireoi:
	push	af
	ld	a,PENABLE
	out	(MSELECT),a
	ld	a,b
	out	(ICNTRL),a
	pop	af
	ret

; ------- A-NATURAL SOURCE: maint -------
; maint: /* Master ACE interrupt */
; 	/
; 	out; MSELECT	 /* select the group given above */
;
; 	sp <= hl;	/* pass tty structure pointer for C */
;
; 	in;
; 	a :: ININT;  jz main	/* Master ACE input int */
; 	a :: OUTINT; jz maout	/* Master ACE output int */
; 	a :: MODINT; jz mashake /* Master ACE status change */
;
; 	sp => af;
; 	ret;
; 	/
;
maint:
	out	(MSELECT),a
	ex	de,hl		; de = tty (save), hl = scratch
	in	a,(MIIR)
	cp	ININT
	jp	z,main
	cp	OUTINT
	jp	z,maout
	cp	MODINT
	jp	z,mashake
	ret

; ------- A-NATURAL SOURCE: main -------
; /* master ACE input _int */
;
;
; main:			/input interrupt on mother board ACE
; 	/
;
; /* bc = in (DATA); */
;
; 	in; MUDATA; c = a; b = 0;	/* pass the char to C */
;
; 	in; MUSTATUS; a & ERROR; jz L100;	/* if (in(status) & ERROR) */
; 		/
; 		call _ttyerro;		/* ttyerror (tty); */
; 		sp => af;		/* restore stack */
; 		ret;			/* return; */
; 		/
;
; 	L100:
;
; 	bc => sp;
;
; 	call _ttyin;	/ttyin(c, tty)
;
; 	sp => af => af;			/* adjust stack */
; 	ret;
; 	/
;
main:
	; de = tty
	in	a,(MUDATA)
	ld	c,a
	ld	b,0		; bc = char
	in	a,(MUSTATUS)
	and	ERROR
	jp	z,L100
	ld	h,d
	ld	l,e		; hl = tty (arg0)
	call	_ttyerror
	ret
L100:
	push	de		; push tty (arg1)
	ld	l,c
	ld	h,b		; hl = char (arg0)
	call	_ttyin
	pop	af		; drop tty
	ret

; ------- A-NATURAL SOURCE: maout -------
; maout:	/* trans. buffer empty on Master ACE */
; 	/
;
; /* if (!ready) return; */
;
; 	in; MUSTATUS; a & EMPTY; jz endint	/* if (!ready) return; */
;
; 	bc = hl;
; 	a = *(hl = MODE[1] + bc);	/* a = tty->MODE[1];	*/
;
; 	a & SHAKE; jnz maout2 /* if (!(mode & SHAKE)) */
; 		/
; 		call _ttyout;	 /* send a _char */
; 		sp => af;
; 		ret;
; 		/
;
; 	maout2: /* we have to do handshaking */
; 	in; MMSTATUS; a & CLR2SND; jz maout1; /* if (a & CLEAR) */
; 		call _ttyout;
; 		sp => af;
; 		ret;
; 		/
;
; 	maout1:
; 	/else		Not clear to send - disable
; 		/
; 		a = ODSABLE; out; MUABLE
; 		sp => af;
; 		ret;
; 		/
; 	/
;
maout:
	; de = tty
	in	a,(MUSTATUS)
	and	EMPTY
	jp	z,endint
	ld	hl,MODE+1
	add	hl,de		; hl = &tty->mode[1]
	ld	a,(hl)
	and	SHAKE
	jp	nz,maout2
	ld	h,d
	ld	l,e		; hl = tty (arg0)
	call	_ttyout
	ret
maout2:
	in	a,(MMSTATUS)
	and	CLR2SND
	jp	z,maout1
	ld	h,d
	ld	l,e		; hl = tty (arg0)
	call	_ttyout
	ret
maout1:
	ld	a,ODSABLE
	out	(MUABLE),a
	ret

; ------- A-NATURAL SOURCE: mashake -------
; mashake: /* Master ACE status change */
; 	/
; 	call _mumint;		/* mumint (tty); */
; 	sp => af;
; 	ret;
; 	/
;
mashake:
	ld	h,d
	ld	l,e		; hl = tty (arg0)
	call	_mumint
	ret

; ------- A-NATURAL SOURCE: endint -------
; endint:
; 	/
; 	sp => af	/throw away argument (tty)
; 	return;
; 	/
;
endint:
	ret

; ------- A-NATURAL SOURCE: _ace_start -------
; /* mstart (tty) struct tty *tty; */
; /* start the output on this device */
; /* the que is supposed to be emptied by interrupts */
;
; _ace_start:      /* called when a char is put into output que */
;
; 	call select;
;
; 	a = ace :: 0; jz dstart		/* if (ace) */
; 		/
; 		mstart1 = (a = miobase + UABLE);
; 		a = OENABLE; out; mstart1: UABLE /* enable ints on ACE */
; 		jmp _ei;		/balance di in select
; 		/
;
; 	/* else	 - parallel printer */
; 	dstart:
; 		/
;
; 		/* we want to clear the bit in the PICs IMR */
; 		/* that corresponds to line 6 */
;
; 		/* out (OCW1, in (OCW1) & 0277); */
;
; 		a = miobase + OCW1;
; 		mstart2 = a; mstart3 = a;
;
; 		in; mstart2: OCW1
;
; 		a & 0277;		/* clear bit 6 */
;
; 		out; mstart3: OCW1
;
; 		jmp _ei;
; 		/
;
_ace_start:
	call	select
	ld	a,(ace)
	cp	0
	jp	z,dstart
	ld	a,(miobase)
	inc	a
	ld	(mstart1),a
	ld	a,OENABLE
	.defb	0xD3
mstart1:
	.defb	UABLE
	jp	_ei
dstart:
	ld	a,(miobase)
	add	a,OCW1
	ld	(mstart2),a
	ld	(mstart3),a
	.defb	0xDB
mstart2:
	.defb	OCW1
	and	0xBF
	.defb	0xD3
mstart3:
	.defb	OCW1
	jp	_ei

; ------- A-NATURAL SOURCE: _ace_stop -------
; /* mstop (tty) struct tty *tty; */
;
; _ace_stop:			/called when a que emptys or esc is hit
; 	/		/mstop(tty, empty && closed);
; 	call select;
;
; 	a = ace :: 0; jz dstop	/* if (ace) */
; 		/
; 		mstop1 = (a = miobase + UABLE);
; 		a = *(hl = 4 + sp)
; 		a | a
; 		a = ODSABLE	/disable output, enable input and modem
; 		jz turnoff
; 			/
; 			a = IODSABLE	/if closed, disable all
; 			/
;
; 		turnoff: out; mstop1: UABLE
;
; 		a = *(hl = 4 + sp) | a; jz _ei;	/* if (cold) */
; 			/
; 			/* clear all modem stat lines */
; 			/* Drop data terminal ready. */
; 			/* Hang up the modem. */
; 			/* out (base + MODEM, 0); */
;
; 			L68 = (a = miobase + MCNTRL);
; 			a = 0; out; L68: MCNTRL;
; 			/
;
; 		jmp _ei;		/balance di in select
; 		/
;
; 	/* else parallel port */
; 	dstop:
; 		/
;
; 		/* we want to set the bit in the PICs IMR */
; 		/* that corresponds to line 6 */
; 		/* to disable further _ints */
; 		/* out (OCW1, in (OCW1) | 0100); */
;
;
; 		a = miobase + OCW1 -> mstop2 -> mstop3;
;
; 		in; mstop2: OCW1;
;
; 		a | 0100;	/* set bit 6, disable _para _ints */
;
; 		out; mstop3: OCW1;
;
; 		jmp _ei;
; 		/
;
_ace_stop:
	call	select		; tty (arg0) in hl
	ld	a,(ace)
	cp	0
	jp	z,dstop
	ld	a,(miobase)
	inc	a
	ld	(mstop1),a
	ld	hl,2
	add	hl,sp
	ld	a,(hl)
	or	a
	ld	a,ODSABLE
	jp	z,turnoff
	ld	a,IODSABLE
turnoff:
	.defb	0xD3
mstop1:
	.defb	UABLE
	ld	hl,2
	add	hl,sp
	ld	a,(hl)
	or	a
	jp	z,_ei
	ld	a,(miobase)
	add	a,MCNTRL
	ld	(L68),a
	ld	a,0
	.defb	0xD3
L68:
	.defb	MCNTRL
	jp	_ei
dstop:
	ld	a,(miobase)
	add	a,OCW1
	ld	(mstop2),a
	ld	(mstop3),a
	.defb	0xDB
mstop2:
	.defb	OCW1
	or	0x40
	.defb	0xD3
mstop3:
	.defb	OCW1
	jp	_ei

; ------- A-NATURAL SOURCE: _ace_putc -------
; /* send a _char to the Mult I/O */
;
; _ace_putc:			/called at interrupt time to print a char
; 	/
; 	call select;
;
; 	mputc3 = (a = miobase);
;
; 	a = *(hl = 4 + sp);	/* a = character; */
;
; 	out; mputc3: UDATA
; 	jmp _ei;		/balance di in select
; 	/
;
_ace_putc:
	call	select		; tty (arg0) in hl
	ld	a,(miobase)
	ld	(mputc3),a
	ld	hl,2
	add	hl,sp		; hl = &c (arg1, at sp+2)
	ld	a,(hl)
	.defb	0xD3
mputc3:
	.defb	UDATA
	jp	_ei

; ------- A-NATURAL SOURCE: _ace_set -------
; /* mset (tty) struct tty *tty; */
; /* Set the baud rate on serial ports */
; /* Do a restore on parallel ports  */
;
; _ace_set:			/called during stty system call and at open */
; 	/
; 	call select;
; 	a = ace | a;	/* if (ace % 4 == 0) */
; 		/
; 		jz _ei;		/* return; */
; 		/
;
; 	mset2 = (a = miobase + LOBAUD);
; 	mset3 = (a = miobase + HIBAUD);
;
; 	a = miobase + REGSET -> mset1 -> mset4;
;
; 	mset5 = (a = miobase + MCNTRL);
;
; 	a = BAUDREG; out; mset1: REGSET
;
; 	c = (a = *bc & 15 + a); /* offset into baudrate table */
; 	b = 0;
;
; 	hl = &_rates + bc;  /* baudrate table */
;
; 	a = *hl; out; mset2: LOBAUD; /* low byte of constant */
;
; 	a = *(hl +1); out; mset3: HIBAUD   /* high byte */
;
; 	a = NORMAL; out; mset4: REGSET /*reset ace _reg _func to data/status */
;
; 	a = CONNECT; out; mset5: MCNTRL
;
; 	jmp _ei;		/balance di in select
; 	/
;
;
; /* Functions of "select" */
; /* the tty pointer is returned in bc */
; /* miobase is set */
; /* ace is set */
; /* the proper group on the proper Mio board is selected */
;
; 	/* struct tty *select (); */
;
_ace_set:
	push	bc		; save bc (callee-saved)
	push	hl		; save tty (for the baud-rate read below)
	call	select		; tty (arg0) in hl
	pop	hl		; hl = tty
	ld	a,(ace)
	or	a
	jp	z,mset_exit
	ld	a,(miobase)
	add	a,LOBAUD
	ld	(mset2),a
	ld	a,(miobase)
	inc	a
	ld	(mset3),a
	ld	a,(miobase)
	add	a,REGSET
	ld	(mset1),a
	ld	(mset4),a
	ld	a,(miobase)
	add	a,MCNTRL
	ld	(mset5),a
	ld	a,BAUDREG
	.defb	0xD3
mset1:
	.defb	REGSET
	ld	a,(hl)		; a = tty->baud
	and	15
	add	a,a
	ld	c,a
	ld	b,0
	ld	hl,_rates
	add	hl,bc
	ld	a,(hl)
	.defb	0xD3
mset2:
	.defb	LOBAUD
	inc	hl
	ld	a,(hl)
	.defb	0xD3
mset3:
	.defb	HIBAUD
	ld	a,NORMAL
	.defb	0xD3
mset4:
	.defb	REGSET
	ld	a,CONNECT
	.defb	0xD3
mset5:
	.defb	MCNTRL
mset_exit:
	pop	bc		; restore bc
	jp	_ei

; ------- A-NATURAL SOURCE: select -------
; select:
; 	/
; 	call _di
;
; 	hl = 4 + sp	/hl gets addr of tty pointer
; 	bc =^ hl	/bc = tty pointer
;
; 	hl = DEV + bc	/HL = ADDR OF MINOR DEV NUMBER IN TTY STRUCTURE
;
; /* the three ACEs are minors 4-6 and port groups 1-3 */
;
; 	ace = (a = *hl & 0x0f) - TTY_ACE0 + 1;
; 	miobase = MBASE;		/* one Wunderbus, 0x48 */
;
; 	select1 = (a + SELECT);
;
; 	a = ace | PENABLE; out; select1: SELECT;
;
; 	return;		/* return tty; (in bc) */
; 	/
;
select:
	push	bc		; preserve bc (callee-saved)
	call	_di
	ld	c,l
	ld	b,h		; bc = tty (arg0 arrives in hl)
	ld	hl,DEV
	add	hl,bc
	ld	a,(hl)
	and	0x0F		; the port minor
	sub	TTY_ACE0-1	; 4-6 -> ace 1-3 (0 is the parallel port)
	ld	(ace),a
	ld	a,MBASE		; the master Wunderbus base
	ld	(miobase),a
	add	a,SELECT
	ld	(select1),a
	ld	a,(ace)
	or	PENABLE		; group select: PENABLE | 1-3
	.defb	0xD3
select1:
	.defb	SELECT
	pop	bc		; restore bc
	ret

; ------- A-NATURAL SOURCE: _rates -------
; /The constants needed by the uart divider circuits to achieve the
; /stty rate requests 0 to _15
; _rates:
; 	&96	/ 0 =  1200 baud
; 	&2304	/ 1 =	 50
; 	&1536	/ 2 =	 75
; 	&1047	/ 3 =	110
; 	&857	/ 4 =	134.5
; 	&768	/ 5 =	150
; 	&576	/ 6 =	200
; 	&384	/ 7 =	300
; 	&192	/ 8 =	600
; 	&96	/ 9 =  1200
; 	&64	/10 =  1800
; 	&48	/11 =  2400
; 	&24	/12 =  4800
; 	&12	/13 =  9600
; 	&6	/14 = 19200
; 	&3	/15 = 38400
;
_rates:
	.defw	96	/ 0 =  1200 baud
	.defw	2304	/ 1 =	 50
	.defw	1536	/ 2 =	 75
	.defw	1047	/ 3 =	110
	.defw	857	/ 4 =	134.5
	.defw	768	/ 5 =	150
	.defw	576	/ 6 =	200
	.defw	384	/ 7 =	300
	.defw	192	/ 8 =	600
	.defw	96	/ 9 =  1200
	.defw	64	/10 =  1800
	.defw	48	/11 =  2400
	.defw	24	/12 =  4800
	.defw	12	/13 =  9600
	.defw	6	/14 = 19200
	.defw	3	/15 = 38400

; console(c) - send a character to the console the monitor found.  The
; monitor built its driver into on-board ram at condrv (mon500.s findcon),
; and the kernel reaches it exactly as the level 1 rom does: the character
; in C, a call to condrv.  The driver polls for ready, so a newline needs
; only its return sent first.
;
; The driver wants the character in C, so BC is spoken for; a C caller
; may have a register variable there (ccc keeps them in BC) and the
; callee-saves rule it back.  pr keeps its argument pointer in BC, and
; without this every number it prints after the first character of its
; format is read from wherever C happened to be.
;
condrv=0x134		; the monitor's console driver, conram + 2

_console:
	ld	a,l		; a = c (arg0 in hl)
	cp	'\n'
	jp	nz,conso	; not newline: output it
	push	bc		; save bc (callee-saved)
	ld	c,'\r'		; a return ahead of a newline
	call	condrv		; the driver sends the char in C
	pop	bc
conso:	push	bc		; save bc (callee-saved)
	ld	c,l		; the character, where the driver wants it
	call	condrv		; send it, and return to console's caller
	pop	bc
	ret
