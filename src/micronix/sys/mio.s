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
; TTSIZE4 := 152		/* X 4 */
;
; public	tty
; public	doslave
; public	doace
; public	miobase
; public	_nmio
; public	_mttys
; public	_mstart
; public	_mstop
; public	_mputc
; public	_mset
; public	_console
; public	intrupt
; public	slint
; public	m1int
; public	m2int
; public	m3int
; public	m0int
; public	clkint
;
; /* absolute I/O addresses on the Master Mult I/O */
;
;
; MBASE	:= 0x48		/base io address for master multiboard
; SLBASE	:= 0x58
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
; MOCW3	:= MBASE[4]
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
; IIR	:= 2
; MCNTRL	:= 4
; USTATUS := 5
; MSTATUS := 6
; SELECT	:= 7
; REGSET	:= 3
; OCW1	:= 5
; OCW2	:= 4
; OCW3	:= 4
; ICW1	:= 4
; ICW2	:= 5
; ICW3	:= 5
; ICW4	:= 5
;
; /*	masks and bits */
;
; IRP	:= 0100		/* interrupt _req parallel */
; IRA1	:= 010		/* _int _req ACE 1 */
; IRA2	:= 020
; IRA3	:= 040
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
; SLICW4	:= 016		/* auto EOI */
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
; /* the parallel port is diabled at the PIC int line */
;
;
; ARMMASTER  := 0103	/allow interrupts 2 - 7 (ttys, clock, slaves)
; ARMSLAVE:= 0307		/allow 3-5	(ACEs only)
; GETIRR	:= 10
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
TTSIZE4=152
	.globl	tty, doslave, doace, miobase, _nmio, _mstart, _mstop
	.globl	_mputc, _mset, _console, intrupt, slint, m1int, m2int
	.globl	m3int, m0int, clkint
	.extern	_di, _ei, dicount, _mttys, _ppint, _ttyin, _ttyout
	.extern	_mumint, _ttyerror, _clock
MBASE=0x48
SLBASE=0x58
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
IIR=2
REGSET=3
MCNTRL=4
USTATUS=5
MSTATUS=6
SELECT=7
OCW1=5
OCW2=4
OCW3=4
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
IRP=0x40
IRA1=0x08
IRA2=0x10
IRA3=0x20
PENABLE=0x28
ENDINT=0xA0
MACE1=0x29
MACE2=0x2A
MACE3=0x2B
GETIRR=10
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
; 	a = PENABLE; out; MSELECT;	/* EOI cycle */
; 	a = ENDINT;  out; ICNTRL;
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
	ld	a,PENABLE
	out	(MSELECT),a
	ld	a,ENDINT
	out	(ICNTRL),a
	pop	af
	pop	bc
	pop	de
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

; ------- A-NATURAL SOURCE: _nmio -------
; 	_nmio:	 0
;
_nmio:
	.defb	0

; ------- A-NATURAL SOURCE: slave -------
; 	slave:	 0
;
slave:
	.defb	0

; ------- A-NATURAL SOURCE: m0int -------
; m0int:	/* print wheel ready on Master */
; 	bc = &_mttys => sp; call _ppint; sp => bc;
; 	return;
;
m0int:
	ld	bc,_mttys
	push	bc
	call	_ppint
	pop	bc
	ret

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
	jp	maint

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
	push	hl
	in	a,(MIIR)
	cp	ININT
	jp	z,main
	cp	OUTINT
	jp	z,maout
	cp	MODINT
	jp	z,mashake
	pop	af
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
	in	a,(MUDATA)
	ld	c,a
	ld	b,0
	in	a,(MUSTATUS)
	and	ERROR
	jp	z,L100
	call	_ttyerror
	pop	af
	ret
L100:
	push	bc
	call	_ttyin
	pop	af
	pop	af
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
	in	a,(MUSTATUS)
	and	EMPTY
	jp	z,endint
	ld	c,l
	ld	b,h
	ld	hl,MODE+1
	add	hl,bc
	ld	a,(hl)
	and	SHAKE
	jp	nz,maout2
	call	_ttyout
	pop	af
	ret
maout2:
	in	a,(MMSTATUS)
	and	CLR2SND
	jp	z,maout1
	call	_ttyout
	pop	af
	ret
maout1:
	ld	a,ODSABLE
	out	(MUABLE),a
	pop	af
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
	call	_mumint
	pop	af
	ret

; ------- A-NATURAL SOURCE: tty -------
; /*	in; MMSTATUS; a & CLR2SND; jz endint /* if (stat & CLEAR ) */
; /*		/
; /*		call _cstart;	/* cstart(tty); */
; /*		/
; /*
; /*	jmp endint;
;
;
;
;
;
; /* slint - slave Mult I/O _int */
;
; tty: &0;
;
tty:
	.defw	0

; ------- A-NATURAL SOURCE: slint -------
; slint:
; 	/
;
; /*	for (slave = 0; slave < nmio; slave++)	*/
; /*		doslave ();			*/
; /*	Service all the _ints on each of the slaves */
;
; 	bc = &_mttys; hl = TTSIZE4 + bc -> tty;		/* tty = &mttys[4]; */
; 	a = SLBASE -> miobase;				/* base = SLBASE; */
; 	slave = (a ^ a);	/* slave = 0; */
;
; 	slint2:
;
; 	a = slave;
; 	hl = &_nmio;
; 	a :: *hl; rz;
; 		/
; 		call doslave;
; 		slave = (a = slave + 1);	/* slave++; */
; 		miobase = (a = miobase + 0x10); /* miobase += 0x10; */
; 		jmp slint2;
; 		/
; 	/
;
slint:
	ld	bc,_mttys
	ld	hl,TTSIZE4
	add	hl,bc
	ld	(tty),hl
	ld	a,SLBASE
	ld	(miobase),a
	xor	a
	ld	(slave),a
slint2:
	ld	a,(slave)
	ld	hl,_nmio
	cp	(hl)
	ret	z
	call	doslave
	ld	a,(slave)
	inc	a
	ld	(slave),a
	ld	a,(miobase)
	add	a,0x10
	ld	(miobase),a
	jp	slint2

; ------- A-NATURAL SOURCE: irr -------
; /* doslave - service the slave mio whose base I/O address */
; /* is given in miobase */
; /* bc contains a pointer to the _app tty structure */
; /* update bc */
;
; irr: 0
;
irr:
	.defb	0

; ------- A-NATURAL SOURCE: doslave -------
; doslave:
; 	/
;
; /* we want to read the _int _req _reg on the current PIC */
;
;
; 	a = miobase + SELECT -> slave0
; 	a = miobase + OCW3 -> slave1 -> slave2
;
; 	a = PENABLE; out; slave0: SELECT;	/* select _grp 0 */
;
; 	a = GETIRR; out; slave1: OCW3		/* ask for _int _req _reg */
;
; 	in; slave2: OCW3; a-> irr;	/* save _int _req _reg */
;
; 	slave3:
;
; /* now irr contains high bits for active _ints */
;
; /* check the parallel port */
;
; 	a = irr & IRP; jz slave4		/* if (c & DAISY) */
; 		/
; 		hl = tty => sp; call _ppint; sp => af; /* ppint (tty); */
; 		/
;
; 	slave4:
;
; 	hl = tty + (bc = TTSIZE) -> tty;	/* tty++ */
;
; 	a = irr & IRA1; jz slave5	/* ACE # 1 */
; 		/
; 		h = MACE1; call doace;
; 		/
;
; 	slave5:
;
; 	hl = tty + (bc = TTSIZE) -> tty;	/* tty++ */
; 	a = irr & IRA2; jz slave6	/* ACE #2 */
; 		/
; 		h = MACE2; call doace;
; 		/
;
; 	slave6:
;
; 	hl = tty + (bc = TTSIZE) -> tty;	/* tty++ */
;
; 	a = irr & IRA3; jz slave7	/* ACE #3 */
; 		/
; 		h = MACE3; call doace;
; 		/
;
; 	slave7:
;
; 	hl = tty + (bc = TTSIZE) -> tty;	/* tty++ */
;
; 	return;
; 	/
;
doslave:
	ld	a,(miobase)
	add	a,SELECT
	ld	(slave0),a
	ld	a,(miobase)
	add	a,OCW3
	ld	(slave1),a
	ld	(slave2),a
	ld	a,PENABLE
	.defb	0xD3
slave0:
	.defb	SELECT
	ld	a,GETIRR
	.defb	0xD3
slave1:
	.defb	OCW3
	.defb	0xDB
slave2:
	.defb	OCW3
	ld	(irr),a
slave3:
	ld	a,(irr)
	and	IRP
	jp	z,slave4
	ld	hl,(tty)
	push	hl
	call	_ppint
	pop	af
slave4:
	ld	hl,(tty)
	ld	bc,TTSIZE
	add	hl,bc
	ld	(tty),hl
	ld	a,(irr)
	and	IRA1
	jp	z,slave5
	ld	h,MACE1
	call	doace
slave5:
	ld	hl,(tty)
	ld	bc,TTSIZE
	add	hl,bc
	ld	(tty),hl
	ld	a,(irr)
	and	IRA2
	jp	z,slave6
	ld	h,MACE2
	call	doace
slave6:
	ld	hl,(tty)
	ld	bc,TTSIZE
	add	hl,bc
	ld	(tty),hl
	ld	a,(irr)
	and	IRA3
	jp	z,slave7
	ld	h,MACE3
	call	doace
slave7:
	ld	hl,(tty)
	ld	bc,TTSIZE
	add	hl,bc
	ld	(tty),hl
	ret

; ------- A-NATURAL SOURCE: doace -------
; /* service one ACE on a slave board */
; /* h contains the ace # */
;
; doace:
; 	a = miobase + SELECT -> ace1;
; 	a = h; out; ace1: SELECT     /* select proper group */
;
; 	a = miobase + IIR -> ace2;
; 	in; ace2: IIR;			/* read _int _id _reg */
;
; 	a :: OUTINT;  jnz ace3;
; 		/
; 		a = miobase + USTATUS -> ace9;	/* if (!ready) return; */
; 		in; ace9: USTATUS; a & EMPTY; rz;
;
; 		hl = tty -> bc;		/* bc = tty; */
; 		a = *(hl = MODE[1] + bc) & SHAKE; jz ace4;
; 			/
; 			ace5 = (a = miobase + MSTATUS);
;
; 			in; ace5: MSTATUS; a & CLR2SND; jnz ace4;
; 				/
; 				ace51 = (a = miobase + UABLE);
; 				a = ODSABLE; out; ace51: UABLE;
; 				return;
; 				/
; 			/
;
; 		ace4:
; 		hl = tty => sp; call _ttyout; sp => af;
; 		return;
; 		/
; ace3:
; 	a :: MODINT; jnz ace6;
; 		/
; 		hl = tty => sp; call _mumint;	/* mumint (tty); */
; 		sp => af;
; 		ret;
; 		/
;
;
;
;
; /*		ace7 = (a = miobase + MSTATUS);
;  *		in; ace7: MSTATUS; a & CLR2SND; rz;
;  *			/
;  *			hl = tty => sp; call _cstart; sp => af;
;  *			/
;  *		return;
;  *		/
;  */
;
;
; 	ace6:
; 	a :: ININT; rnz
; 		/
; 		ace8 = (a = miobase); /* + UDATA */
; 		in; ace8: UDATA; b = 0; c = a;
;
; 		hl = tty => sp;
; 		bc => sp;
;
; 		call _ttyin;		/* ttyin (c, tty); */
; 		sp => af => af;
;
; 		return;
; 		/
; 	/
;
doace:
	ld	a,(miobase)
	add	a,SELECT
	ld	(ace1),a
	ld	a,h
	.defb	0xD3
ace1:
	.defb	SELECT
	ld	a,(miobase)
	add	a,IIR
	ld	(ace2),a
	.defb	0xDB
ace2:
	.defb	IIR
	cp	OUTINT
	jp	nz,ace3
	ld	a,(miobase)
	add	a,USTATUS
	ld	(ace9),a
	.defb	0xDB
ace9:
	.defb	USTATUS
	and	EMPTY
	ret	z
	ld	hl,(tty)
	ld	c,l
	ld	b,h
	ld	hl,MODE+1
	add	hl,bc
	ld	a,(hl)
	and	SHAKE
	jp	z,ace4
	ld	a,(miobase)
	add	a,MSTATUS
	ld	(ace5),a
	.defb	0xDB
ace5:
	.defb	MSTATUS
	and	CLR2SND
	jp	nz,ace4
	ld	a,(miobase)
	inc	a
	ld	(ace51),a
	ld	a,ODSABLE
	.defb	0xD3
ace51:
	.defb	UABLE
	ret
ace4:
	ld	hl,(tty)
	push	hl
	call	_ttyout
	pop	af
	ret
ace3:
	cp	MODINT
	jp	nz,ace6
	ld	hl,(tty)
	push	hl
	call	_mumint
	pop	af
	ret
ace6:
	cp	ININT
	ret	nz
	ld	a,(miobase)
	ld	(ace8),a
	.defb	0xDB
ace8:
	.defb	UDATA
	ld	b,0
	ld	c,a
	ld	hl,(tty)
	push	hl
	push	bc
	call	_ttyin
	pop	af
	pop	af
	ret

; ------- A-NATURAL SOURCE: endint -------
; endint:
; 	/
; 	sp => af	/throw away argument (tty)
; 	return;
; 	/
;
endint:
	pop	af
	ret

; ------- A-NATURAL SOURCE: _mstart -------
; /* mstart (tty) struct tty *tty; */
; /* start the output on this device */
; /* the que is supposed to be emptied by interrupts */
;
; _mstart:      /* called when a char is put into output que */
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
_mstart:
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

; ------- A-NATURAL SOURCE: _mstop -------
; /* mstop (tty) struct tty *tty; */
;
; _mstop:			/called when a que emptys or esc is hit
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
_mstop:
	call	select
	ld	a,(ace)
	cp	0
	jp	z,dstop
	ld	a,(miobase)
	inc	a
	ld	(mstop1),a
	ld	hl,4
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
	ld	hl,4
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

; ------- A-NATURAL SOURCE: _mputc -------
; /* send a _char to the Mult I/O */
;
; _mputc:			/called at interrupt time to print a char
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
_mputc:
	call	select
	ld	a,(miobase)
	ld	(mputc3),a
	ld	hl,4
	add	hl,sp
	ld	a,(hl)
	.defb	0xD3
mputc3:
	.defb	UDATA
	jp	_ei

; ------- A-NATURAL SOURCE: _mset -------
; /* mset (tty) struct tty *tty; */
; /* Set the baud rate on serial ports */
; /* Do a restore on parallel ports  */
;
; _mset:			/called during stty system call and at open */
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
_mset:
	call	select
	ld	a,(ace)
	or	a
	jp	z,_ei
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
	ld	a,(bc)
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
; /* find mult I/O board base address */
;
; /* miobase = 0x48 + 4 * (minor & 014); */
;
; 	miobase	= (a = *hl & 014 + a + a + MBASE);
;
; 	select1 = (a + SELECT);
; 	ace = (a = *hl & 3);	/* ace = minor & 3; */
;
; 	a | PENABLE; out; select1: SELECT;
;
; 	return;		/* return tty; (in bc) */
; 	/
;
select:
	call	_di
	ld	hl,4
	add	hl,sp
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	ld	hl,DEV
	add	hl,bc
	ld	a,(hl)
	and	0x0C		; 014
	add	a,a
	add	a,a
	add	a,MBASE
	ld	(miobase),a
	add	a,SELECT
	ld	(select1),a
	ld	a,(hl)
	and	3
	ld	(ace),a
	or	PENABLE
	.defb	0xD3
select1:
	.defb	SELECT
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

; ------- A-NATURAL SOURCE: _console -------
; /console(c)
;
; /Send a character directly to the multiboard serial port 1
;
; public	_console
;
; _console:
; 	/
; 	sp => hl => bc
; 	sp <= bc <= hl	/bc = character
; 	a = c :: '\n'	/check for newline
; 	sp <= bc	/save original character
; 	c = '\r'	/insert a return before a newline
; 	cz spout
; 	sp => bc	/get back original character
; 	/
;
_console:
	pop	hl
	pop	bc
	push	bc
	push	hl
	ld	a,c
	cp	'\n'
	push	bc
	ld	c,'\r'
	call	z,spout
	pop	bc

; ------- A-NATURAL SOURCE: spout -------
; /* fall into ... */
;
; spout:			/put out the character in reg c
; 	/
; 	sp <= bc	/pass character
; 	bc = TTSIZE;
; 	hl = &_mttys + bc => sp;
; 	call conout	/conout(&_mttys[1], c)
; 	sp => af => af
; 	return;
; 	/
;
spout:
	push	bc
	ld	bc,TTSIZE
	ld	hl,_mttys
	add	hl,bc
	push	hl
	call	conout
	pop	af
	pop	af
	ret

; ------- A-NATURAL SOURCE: conout -------
; conout:
; 	/
; 	call select;
;
; 	a = miobase -> c3 + USTATUS -> c2;
;
; /*
;  * wait til ready
;  */
;
; 	c1: in; c2: USTATUS; a & EMPTY;
; 		/
; 		jz c1;
; 		/
;
; 	a = *(hl = 4 + sp);
;
; 	out; c3: UDATA;
; 	jmp _ei;		/balance di in select
; 	/
;
conout:
	call	select
	ld	a,(miobase)
	ld	(c3),a
	add	a,USTATUS
	ld	(c2),a
c1:
	.defb	0xDB
c2:
	.defb	USTATUS
	and	EMPTY
	jp	z,c1
	ld	hl,4
	add	hl,sp
	ld	a,(hl)
	.defb	0xD3
c3:
	.defb	UDATA
	jp	_ei
