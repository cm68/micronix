*****************************************************************
*								*
* Hard Disk Boot program.					*
* For M26, M10, and M20.					*
*								*
*****************************************************************

	org	100h

revnum	equ	13
IOADDR	EQU	120Q
CONTRL	EQU	IOADDR
STATUS	EQU	IOADDR
DATA	EQU	IOADDR+3
FUNCTN	EQU	IOADDR+2
COMMD	EQU	IOADDR+1
DREAD	EQU	1
SECTOR	EQU	1
OPDONE	EQU	2
COMPLT	EQU	4
HEADER	EQU	10Q
DRENBL	EQU	5
DSKRUN	EQU	7
READY	EQU	40Q
SYSTEM	EQU	200Q
STEPO	EQU	370Q
DRIVEA	EQU	374Q

	jmp	start

	ds	509		;Room for the boot

START	MVI	A,DRIVEA	;select
	OUT	FUNCTN		;    drive A
	MVI	A,DRENBL	;turn on drive
	OUT	CONTRL		;    command register
RLOOP	IN	STATUS		;test for
	ANI	READY		;drive A ready
	JNZ	RLOOP
	MVI	A,DSKRUN	;enable the
	OUT	CONTRL		;    controller
WAITZ	IN	STATUS		;test for heads
	RAR			;    at track zero
	JNC	SDONE
	MVI	A,STEPO		;execute
	OUT	FUNCTN		;    the
	MVI	A,DRIVEA	;    step out
	OUT	FUNCTN		;    command
WAITC	IN	STATUS		;wait for
	ANI	COMPLT		;    the seek
	JZ	WAITC		;    to complete
	JMP	WAITZ

SDONE	IN	STATUS		;get an image
	MOV	C,A		;    of the status reg
IWAIT1	IN	STATUS		;wait for
	SUB	C		;    the index pulse
	JZ	IWAIT1		;    to arrive
IWAIT2	IN	STATUS		;wait for the
	SUB	C		;    next index pulse
	JNZ	IWAIT2		;test for head settle
iwait3	in	status
	sub	c
	jz	iwait3

	MVI	A,HEADER	;reset the
	OUT	COMMD		;    buffer pointer
	XRA	A		;    to header area
	OUT	DATA		;head 0
	OUT	DATA		;track 0
	MVI	A,SECTOR	;1 for CP/M
	OUT	DATA		;    or 30 for drivers
	MVI	A,SYSTEM	;system key
	OUT	DATA
	MVI	A,DREAD		;issue a
	OUT	COMMD		;    read command
WAITD	IN	STATUS		;wait for command
	ANI	OPDONE		;    to complete
	JZ	WAITD
	IN	DATA		;low order byte of
	MOV	L,A		;    bootstrap address
	MOV	E,A
	IN	DATA		;high order byte of
	MOV	H,A		;    bootstrap address
	MOV	D,A
LLOOP	IN	DATA		;load
	STAX	D		;    the
	INR	E		;    bootstrap
	JNZ	LLOOP
	PCHL			;branch there
