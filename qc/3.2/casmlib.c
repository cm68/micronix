/********************************************************/
/*							*/
/*	 Q/C Assembly Language Library Functions	*/
/*			 (Z80)				*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			03/21/84			*/
/********************************************************/
/*
 *	Z80 port I/O functions
 *
 *	contributed by Kirk Bailey
 *	slightly modified by Jim Colvin
 */
/* Read a byte from specified input port */
in()
	{
#asm
;	in(port_address) -- Returns the value of the specified input
;		port.
;
	PUSH	BC		;Save frame pointer
	LD	HL,4		;Compute address of "port_address"
	ADD	HL,SP
	LD	C,(HL)		;Get "port_address"
	IN	A,(C)		;Read port
	LD	L,A
	LD	H,0		;Zero extend byte read
	OR	H		;Set zero flag per result
	POP	BC		;Restore frame pointer
#endasm
	}
/* Write a byte to specified output port */
out()
	{
#asm
;	out(data,port_address) -- Writes "data" to the specified output
;		port.  Returns a copy of the data being written out.
;
	PUSH	BC		;Save frame pointer
	LD	HL,4		;Compute address of "port_address"
	ADD	HL,SP
	LD	C,(HL)		;Get "port_address"
	INC	HL		;Compute address of "data"
	INC	HL
	LD	A,(HL)		;Get "data"
	OUT	(C),A		;Write "data" to port
	LD	L,A
	LD	H,0		;Zero extend byte read
	OR	H		;Set zero flag per result
	POP	BC		;Restore frame pointer
#endasm
	}
/* UNIX style non-local goto */
setjmp()
	{
#asm
;setjmp(env)
;jmp_buf env;
	PUSH	BC	;save frame ptr
	LD	HL,8	;get value of SP to restore
	ADD	HL,SP
	LD	C,L	;save it
	LD	B,H
	DEC	HL
	LD	D,(HL)	;get contents of cell above SP
	DEC	HL
	LD	E,(HL)
	DEC	HL
	LD	A,(HL)	;get addr of env
	DEC	HL
	LD	L,(HL)
	LD	H,A
	LD	(HL),C	;save SP value to restore
	INC	HL
	LD	(HL),B
	INC	HL
	LD	(HL),E	;save cell above SP
	INC	HL
	LD	(HL),D
	POP	BC	;restore frame ptr
	POP	DE	;get RET addr
	PUSH	DE
	INC	HL
	LD	(HL),E	;save RET addr
	INC	HL
	LD	(HL),D
	INC	HL
	LD	(HL),C	;save frame pointer
	INC	HL
	LD	(HL),B
	XOR	A	;set Z flag
	LD	L,A	;load return value
	LD	H,A
#endasm
	}
longjmp()
	{
#asm
;longjmp(env, retval)
;jmp_buf env;
	POP	BC	;throw away RET address
	POP	BC	;get return value
	POP	HL	; and address of env
	LD	E,(HL)	;retrieve value of SP from env
	INC	HL
	LD	D,(HL)
	EX	DE,HL
	LD	SP,HL	;reset stack pointer
	EX	DE,HL
	INC	HL
	LD	E,(HL)	;reset cell above SP
	INC	HL
	LD	D,(HL)
	PUSH	DE
	PUSH	DE	;fake the arg to setjmp which caller will remove
	INC	HL
	LD	E,(HL)	;restore RET address
	INC	HL
	LD	D,(HL)
	PUSH	DE
	PUSH	BC	;save return value for later
	INC	HL	;retrieve frame pointer
	LD	C,(HL)
	INC	HL
	LD	B,(HL)
	PUSH	BC
	POP	IX	;restore IX
	POP	HL	;retrieve return value
	LD	A,H	;set Z flag
	OR	L
#endasm
	}
/* Do CP/M system call and return what CP/M puts in HL */
bdos()
	{
#asm
;bdos(c, de)
;int c; (c -> C)
;int de; (de -> DE)
	PUSH	BC	;save frame ptr
	LD	HL,4
	ADD	HL,SP
	LD	E,(HL)	;load de
	INC	HL
	LD	D,(HL)
	INC	HL
	LD	C,(HL)	;load function number (c)
	CALL	5H
	POP	BC	;restore frame ptr
	PUSH	BC	;restore IX
	POP	IX
	LD	A,L	;don't assume A is set to L
	OR	H	;set Z flag per result
#endasm
	}
/* Do CP/M system call like bdos but return A zero-extended into HL */
bdos1()
	{
#asm
;bdos1(c, de)
	PUSH	BC
	LD	HL,4
	ADD	HL,SP
	LD	E,(HL)
	INC	HL
	LD	D,(HL)
	INC	HL
	LD	C,(HL)
	CALL	5H
	POP	BC
	PUSH	BC
	POP	IX
	LD	L,A
	LD	H,0
	OR	H
#endasm
	}
/* Make an MP/M system call and return HL and A */
mpm()
	{
#asm
;mpm(c, de, a)
;int c; (c -> C)
;int de; (de -> DE)
;int *a; (*a <- A zero-extended)
	PUSH	BC	;save calling routine's stack frame ptr
	LD	HL,4
	ADD	HL,SP
	LD	E,(HL)	;get address of a
	INC	HL
	LD	D,(HL)
	PUSH	DE	;save for later
	INC	HL
	LD	E,(HL)	;load de
	INC	HL
	LD	C,(HL)
	INC	HL
	LD	C,(HL)	;load function number (c)
	CALL	5H
	POP	DE	;retrieve address of a
	EX	DE,HL	;save return value
	LD	(HL),A
	INC	HL
	LD	(HL),0
	EX	DE,HL
	POP	BC	;restore frame ptr
	PUSH	BC	;restore IX
	POP	IX
	LD	A,L
	OR	H	;set Z flag per result
#endasm
	}
	/* array of ... */
#define T_FUNC		11		/* function returning