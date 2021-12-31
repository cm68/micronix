/*
 * Interrupt dispatch table
 * Extra space allows us to move the vectors to a 32-byte
 * boundry, as required by the interrupt controller.
 * Most of the interrupt catchers are in mio.s.
 *
 * sys/intrpt.s
 * Changed: <2021-12-24 06:08:17 curt>
 */

public	vectors

	&0; &0; &0; &0;
	&0; &0; &0; &0;
	&0; &0; &0; &0;
	&0; &0; &0; &0;

vectors:
	jmp int0; 0
	jmp int1; 0
	jmp int2; 0
	jmp int3; 0
	jmp int4; 0
	jmp int5; 0
	jmp int6; 0
	jmp int7; 0

int0:	call intrupt; &_mwint		/see below
int1:	call intrupt; &_djint		/floppy disk interrupt
int2:	call intrupt; &slint		/ slave Mult I/O (s)
int3:	call intrupt; &m1int		/ Master ACE 1
int4:	call intrupt; &m2int		/ Master ACE 2
int5:	call intrupt; &m3int		/ Master ACE 3
int6:	call intrupt; &m0int		/ Master parallel port
int7:	call intrupt; &clkint		/ clock int

/hint:					/hard disk interrupts
	/call _mwint			/mw.c
       /call _hdint			/wn.s
	/ret
