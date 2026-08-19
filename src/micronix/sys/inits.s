;
; =====================================================================
; sys/inits.s  --  Z80 translation of sys/inits.anat (asz dialect)
; =====================================================================
;
; ------- A-NATURAL SOURCE: declarations -------
; /*
;  * Initialize the Wunderbuss/Multio
;  *
;  * sys/inits.s
;  * Changed: <2026-08-17 19:49:08 curt>
;  */
;
; /*
;  * Load this module in the data area above _main
;  * so that this code sits in buffer space
;  */
;
; . := .data
;
; public	_minit
; public	_coninit
;
; /*
;  * absolute I/O addresses on the Master Mult I/O
;  */
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
; /*
;  * offsets into mio I/O space
;  */
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
; NORMAL	:= 7		/uart data reg: 8 bits + 2 stop bits
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
;
; public _minit		/initialize multiboard hardware
; 	LDIR := &0xB0ED /z80 do *de++ = *hl++ while --bc != 0
;
	.data
	.globl	_minit, _coninit
	.extern	_di, _ei, _muopen, vectors, _nmio
; 
; /*
;  * port addresses of the Master Mult I/O
;  */
; MBASE := 0x48         /base io address for master multiboard
; SLBASE        := 0x58
; MUDATA        := MBASE         /port for uart data
; MLOBAUD := MBASE       /port for low byte of baud rate
; MUABLE        := MBASE[1]      /port to enable uart interrupts
; MHIBAUD := MBASE[1]    /port for high byte of baud rate
; MIIR  := MBASE[2]      /* interrupt identification _reg (master) */
; CLOCK := MBASE[2]      /clock port
; CLKCLR        := MBASE[2]      /clear clock-_latch Use base[3] for multio rev < 3
; MREGSET := MBASE[3]    /port for uart register selection
; MMCNTRL := MBASE[4]    /modem control register
; ICNTRL        := MBASE[4]      /port to interrupt controller
; ICNTRL1 := MBASE[5]    /another
; MOCW1 := MBASE[5]      /* Operation control word 1 */
; MOCW3 := MBASE[4]
; MUSTATUS := MBASE[5]   /* Master Uart(ace) status */
; MMSTATUS := MBASE[6]  /* Master Modem status */
; MSELECT := MBASE[7]   /offset to select uart or _int controller

MBASE=0x48		; base address of master mio
SLBASE=0x58		; base address of slave mio

MUDATA=MBASE+0		; uart data
MUABLE=MBASE+1		; uart interrupt enables

CLOCK=MBASE+2		; real time clock	
CLKCLR=MBASE+2

ICNTRL=MBASE+4		; interrupt controller
ICNTRL1=MBASE+5

MSELECT=MBASE+7		; uart group select 

UDATA=0
UABLE=1
ICW1=4
ICW2=5
ICW4=5
SELECT=7
OCW1=5
OCW2=4
TIMSET=0x08
TIMSTRB=0x28
CLKRATE=0x1C
CLKSTRB=0x3C
BOUND32=0xE0
ICWORD1=0x1F
ICWORD4=0x0C
SLICW4=0x0E
ENDINT=0xA0
PENABLE=0x28
PICONLY=0x08
INTSOFF=0xFF
ARMMASTER=0x43
ARMSLAVE=0xC7

; ------- A-NATURAL SOURCE: _minit -------
; _minit:			/Called from cus(). Interrupts must be _off
; 	sp <= de	/save C frame pointer
;
; 	/* clear and disable the 3 ACE's on the Master Mult I/O board */
; 	c = 3
; 	minit1:
; 		a = c; out; MSELECT	/* select ACE */
; 		in; MUDATA		/* read and discard data */
; 		a ^ a; out; MUABLE	/* disable ACE ints */
; 		c - 1; jnz minit1;	/* if (!--c) break; */
;
; 	/* set up the clock on the master Mult I/O board */
;
; 	a ^ a;	     out; MSELECT
;
; 	a = TIMSET;  out; CLOCK
; 	a = TIMSTRB; out; CLOCK
; 	a = TIMSET;  out; CLOCK
;
; 	a = CLKRATE; out; CLOCK
; 	a = CLKSTRB; out; CLOCK
; 	a = CLKRATE; out; CLOCK
;
; 	in;  CLKCLR			/clear clock latch
;
; 	hl = &vectors	/move interrupt vectors to a 32 byte boundry
; 	de = hl		/de = &vectors
; 	e = (a = e & BOUND32);	/* de &= ~31; */
;
; 	sp <= de	/save target address
; 	bc = 32		/number of bytes to copy
; 	LDIR		/z80 do *de++ = *hl++; while --bc;
; 	sp => de	/recover target address
;
; 	call _di;
; 	a = PENABLE; out; MSELECT /* select and enable the int controller */
; 	a = ICWORD1 + e; out; ICNTRL	 /* ICW1 */
; 	a = d; out; ICNTRL1    /* send rest of address (ICW2) */
; 	a = ICWORD4; out; ICNTRL1 /* ICW4 */
; 	a = INTSOFF; out; ICNTRL1 /* disable int on master pic */
; 	call findn;	/* set up slave mio's */
; 	call _ei;
;
; 	a = PENABLE; out; MSELECT	/* select PIC */
; 	a = ARMMASTER; out; ICNTRL1	/* arm PIC ints */
;
; 	sp => de	/recover C frame pointer
; 	return;
;
_minit:
	push	de
	ld	c,3
minit1:
	ld	a,c
	out	(MSELECT),a
	in	a,(MUDATA)
	xor	a
	out	(MUABLE),a
	dec	c
	jp	nz,minit1
	xor	a
	out	(MSELECT),a
	ld	a,TIMSET
	out	(CLOCK),a
	ld	a,TIMSTRB
	out	(CLOCK),a
	ld	a,TIMSET
	out	(CLOCK),a
	ld	a,CLKRATE
	out	(CLOCK),a
	ld	a,CLKSTRB
	out	(CLOCK),a
	ld	a,CLKRATE
	out	(CLOCK),a
	in	a,(CLKCLR)
	ld	hl,vectors
	ld	e,l
	ld	d,h
	ld	a,e
	and	BOUND32
	ld	e,a
	push	de
	ld	bc,32
	ldir
	pop	de
	call	_di
	ld	a,PENABLE
	out	(MSELECT),a
	ld	a,ICWORD1
	add	a,e
	out	(ICNTRL),a
	ld	a,d
	out	(ICNTRL1),a
	ld	a,ICWORD4
	out	(ICNTRL1),a
	ld	a,INTSOFF
	out	(ICNTRL1),a
	call	findn
	call	_ei
	ld	a,PENABLE
	out	(MSELECT),a
	ld	a,ARMMASTER
	out	(ICNTRL1),a
	pop	de
	ret

; ------- A-NATURAL SOURCE: _coninit -------
; /coninit()
; /Initialize the _console
; /Unfortunately, this is tied to _ciosw
;
; public	_coninit
; _coninit:
; 	bc = 0x0101	/multio tty 1 in ciosw - major 1, minor 1
; 	sp <= bc	/pass argument
; 	call _muopen	/mopen(dev)
; 	sp => af	/throw away argument
; 	return;
;
_coninit:
	ld	bc,0x0101
	push	bc
	call	_muopen
	pop	af
	ret

; ------- A-NATURAL SOURCE: findn -------
; /* find the number of slave boards and */
; /* initialize each of them */
;
; findn:
;
; 	b = 0;
; 	find1 = (a = 0x58);
;
;
; find2:
; 	find0 = (a = find1 + 7);
; 	a = 0; out; find0: SELECT;	/* group zero */
;
; 	in; find1: 0
; 	a :: 0xff; jz find3	/* if (in (BASE) == 0xFF) break; */
;
;
; /* yes, there is a slave mio board here */
;
; /* so initialize it */
;
; /* set up the output port numbers for all the out insructions */
;
;
; a = find1 -> find10;				/* UDATA */
; a = find1 + UABLE -> find11;
; a = find1 + ICW1 -> find5 -> find17;
; a = find1 + ICW2 -> find6->find7->find14;
; a = find1 + SELECT -> find4->find9->find16;
; a = find1 + 2 -> find15;
;
; a = 0; out; find16: SELECT;
; in; find15: CLKCLR;		/* clear the clock latch */
;
;
; /* clear and disable each of the 3 ACE's on this board */
;
;
; c = 3			/* for (c = 3; c; c--) */
; find12:
; 	/
; 	a = c; out; find9: SELECT;	/* select _grp */
; 	in; find10: UDATA		/* read and discard a _char */
; 	a ^ a; out; find11: UABLE;	/* no ACE _ints */
; 	c - 1; jnz find12;		/* if (!--c) break; */
; 	/
;
;
;
;
; /* run the PIC initialization sequence */
;
;
; call _di;
;
; a = PICONLY; out; find4: SELECT		/* select _grp 0 */
; a = ICWORD1; out; find5: ICW1		/* ICW1 */
; a = 0;	     out; find6: ICW2		/* ICW2 */
; a = SLICW4; out; find7: ICW4		/* ICW4 */
; a = ARMSLAVE; out; find14: OCW1;	/* enable 4 pic _int lines */
; a = ENDINT; out; find17: OCW2;		/* reset the _int _pin */
;
; call _ei;
;
;
;
; 	b + 1;			/* nmio++; */
;
; 	find1 = (a = find1 + 0x10);	/* find1 += 0x10; (next mio) */
;
; 	jmp find2;
;
; find3:
;
; 	_nmio = (a = b);		/* nmio = b; */
; 	return;
; 	/
;
findn:
	ld	b,0
	ld	a,0x58
	ld	(find1),a
find2:
	ld	a,(find1)
	add	a,SELECT
	ld	(find0),a
	ld	a,0
	.defb	0xD3		; out (n),a  -- port patched at runtime
find0:
	.defb	SELECT
	.defb	0xDB		; in a,(n)  -- port patched at runtime
find1:
	.defb	0
	cp	0xFF
	jp	z,find3
	ld	a,(find1)
	ld	(find10),a
	ld	a,(find1)
	inc	a
	ld	(find11),a
	ld	a,(find1)
	add	a,ICW1
	ld	(find5),a
	ld	(find17),a
	ld	a,(find1)
	add	a,ICW2
	ld	(find6),a
	ld	(find7),a
	ld	(find14),a
	ld	a,(find1)
	add	a,SELECT
	ld	(find4),a
	ld	(find9),a
	ld	(find16),a
	ld	a,(find1)
	add	a,2
	ld	(find15),a
	ld	a,0
	.defb	0xD3
find16:
	.defb	SELECT
	.defb	0xDB
find15:
	.defb	CLKCLR
	ld	c,3
find12:
	ld	a,c
	.defb	0xD3
find9:
	.defb	SELECT
	.defb	0xDB
find10:
	.defb	UDATA
	xor	a
	.defb	0xD3
find11:
	.defb	UABLE
	dec	c
	jp	nz,find12
	call	_di
	ld	a,PICONLY
	.defb	0xD3
find4:
	.defb	SELECT
	ld	a,ICWORD1
	.defb	0xD3
find5:
	.defb	ICW1
	ld	a,0
	.defb	0xD3
find6:
	.defb	ICW2
	ld	a,SLICW4
	.defb	0xD3
find7:
	.defb	ICW4
	ld	a,ARMSLAVE
	.defb	0xD3
find14:
	.defb	OCW1
	ld	a,ENDINT
	.defb	0xD3
find17:
	.defb	OCW2
	call	_ei
	inc	b
	ld	a,(find1)
	add	a,0x10
	ld	(find1),a
	jp	find2
find3:
	ld	a,b
	ld	(_nmio),a
	ret
