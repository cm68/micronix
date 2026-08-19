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
	.extern	intrupt, _mwint, _djint, slint, m1int, m2int
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
; int2:	call intrupt; &slint		/ slave Mult I/O (s)
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
vectors:
	jp	int0
	.defb	0
	jp	int1
	.defb	0
	jp	int2
	.defb	0
	jp	int3
	.defb	0
	jp	int4
	.defb	0
	jp	int5
	.defb	0
	jp	int6
	.defb	0
	jp	int7
	.defb	0
int0:
	call	intrupt
	.defw	_mwint
int1:
	call	intrupt
	.defw	_djint
int2:
	call	intrupt
	.defw	slint
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
