;
; =====================================================================
; sys/intrpt.s  --  Z80 translation of sys/intrpt.anat (asz dialect)
; =====================================================================
;
; ------- A-NATURAL SOURCE: declarations -------
; /*
;  * Interrupt dispatch table
;  * Extra space allows us to move the vectors to a 32-byte
;  * boundry, as required by the interrupt controller.
;  * Most of the interrupt catchers are in mio.s.
;  *
;  * sys/intrpt.s
;  * Changed: <2021-12-24 06:08:17 curt>
;  */
;
; public	vectors
;
; 	&0; &0; &0; &0;
; 	&0; &0; &0; &0;
; 	&0; &0; &0; &0;
; 	&0; &0; &0; &0;
;
	.globl	vectors
	.extern	intrupt, _ovlint0, _ovlint1, _ovlint2, m1int, m2int
	.extern	m3int, m0int, clkint
	.defw	0,0,0,0
	.defw	0,0,0,0
	.defw	0,0,0,0
	.defw	0,0,0,0

; ------- A-NATURAL SOURCE: vectors -------
; vectors:
; 	jmp int0; 0
; 	jmp int1; 0
; 	jmp int2; 0
; 	jmp int3; 0
; 	jmp int4; 0
; 	jmp int5; 0
; 	jmp int6; 0
; 	jmp int7; 0
;
; int0:	call intrupt; &_mwint		/see below
; int1:	call intrupt; &_djint		/floppy disk interrupt
; int2:	call intrupt; &_ideint		/ide hard disk (was: slave Mult I/O)
; int3:	call intrupt; &m1int		/ Master ACE 1
; int4:	call intrupt; &m2int		/ Master ACE 2
; int5:	call intrupt; &m3int		/ Master ACE 3
; int6:	call intrupt; &m0int		/ Master parallel port
; int7:	call intrupt; &clkint		/ clock int
;
; /hint:					/hard disk interrupts
; 	/call _mwint			/mw.c
;        /call _hdint			/wn.s
; 	/ret
;
; int2 is a shared line: the IDE card's drive and an NCR 5380 host adapter
; (sys/ncr.c) are both jumpered to VI2, and neither is the only thing on
; the bus that drives it.  The line is level triggered and the 8259 is
; told the interrupt is over once, when intrupt returns, so both cards
; have to be serviced inside that one entry - which is the dispatcher's
; business below and not this file's: ovlntr (sys/ovl.c) holds two
; handlers per line and calls each with its own module mapped (ovlcall).
; sys/ide.c's ideint() and sys/ncr.c's ncrint() each read their own card's
; registers, which is what takes their own line down, and each returns at
; once when its card has nothing in flight.  A driver that used to chain
; the two by hand (sys/scsi.c's scsii2int) is gone with the module split.
;
; The lines that belong to a disk controller no longer name the driver's
; handler.  mwint, djint, ideint and ncrint are in modules, so the kernel
; has no address for them at link time; each line hands intrupt a
; dispatcher in sys/ovl.c instead, and the modules that claimed the line
; with intrset have their handlers called from there.  A line nothing has
; claimed returns without doing anything.  int3 through int7 are the
; resident ACE, parallel and clock handlers and are unchanged.
;
vectors:
	.defb	0xC3		; jp int0 -- explicit, so asz won't relax jp to jr
	.defw	int0
	.defb	0
	.defb	0xC3
	.defw	int1
	.defb	0
	.defb	0xC3
	.defw	int2
	.defb	0
	.defb	0xC3
	.defw	int3
	.defb	0
	.defb	0xC3
	.defw	int4
	.defb	0
	.defb	0xC3
	.defw	int5
	.defb	0
	.defb	0xC3
	.defw	int6
	.defb	0
	.defb	0xC3
	.defw	int7
	.defb	0
int0:
	call	intrupt
	.defw	_ovlint0
int1:
	call	intrupt
	.defw	_ovlint1
int2:
	call	intrupt
	.defw	_ovlint2
int3:
	call	intrupt
	.defw	m1int
int4:
	call	intrupt
	.defw	m2int
int5:
	call	intrupt
	.defw	m3int
int6:
	call	intrupt
	.defw	m0int
int7:
	call	intrupt
	.defw	clkint
