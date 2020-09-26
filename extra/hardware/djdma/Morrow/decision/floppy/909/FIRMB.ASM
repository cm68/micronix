*****************************************************************
*								*
* Firmware for Disk Jockey Model B.				*
*								*
*****************************************************************

	title	'*** Disk Jockey Model B Firmware ***'

ORIGIN	EQU	0F800H

	ORG	ORIGIN

RAM	EQU	ORIGIN+400h
IO	EQU	ORIGIN+3f8h
UDATA	EQU	IO
DREG	EQU	IO+1
USTAT	EQU	IO+1
DCMD	EQU	IO+2
DSTAT	EQU	IO+2
CSTALL	EQU	IO+3
CMDREG	EQU	IO+4
CSTAT	EQU	IO+4
TRKREG	EQU	IO+5
SECREG	EQU	IO+6
DATREG	EQU	IO+7

LIGHT	EQU	1
HEAD	EQU	1
DENSITY	EQU	1
INTRQ	EQU	4
ISTAT	EQU	4
TZERO	EQU	4
LOAD	EQU	4
ULOAD	EQU	6
OSTAT	EQU	10Q
DSIDE	EQU	10Q
NOLITE	EQU	11Q
DCRINT	EQU	11Q
HCMD	EQU	11Q
INDEX	EQU	20Q
WINDXD	EQU	22Q
SKCMD	EQU	30Q
RINDXD	EQU	32Q
SVCMD	EQU	35Q
WPROT	EQU	100Q
ACCESS	EQU	100Q
RSTBIT	EQU	200Q
READY	EQU	200Q
RDCMD	EQU	210Q	
WRCMD	EQU	250Q
STBITS	EQU	300Q
RACMD	EQU	304Q
CLRCMD	EQU	320Q

DBOOT	JMP	BOOT
TERMIN	JMP	CIN
TRMOUT	JMP	COUT
TKZERO	JMP	HOME
TRKSET	JMP	SEEK
SETSEC	JMP	SECSET
SETDMA	JMP	DMA
DREAD	JMP	READ
DWRITE	JMP	WRITE
SELDRV	JMP	DRIVE
TPANIC	JMP	CPAN
TSTAT	JMP	TMSTAT
DMAST	JMP	DMSTAT
STATUS	JMP	DISKST
DSKERR	JMP	LERROR
SETDEN	JMP	DENFIX
SETSID	JMP	SIDEFX


	DS	66Q

BOOT	LXI	SP,TRACK+1	;initialize SP
	CALL	TIMOUT		;poc/reset timeout
	LXI	H,1
	PUSH	H		;track 0, sector 1
	MVI	L,DCRINT	;set up the
	PUSH	H		;    side select
	MVI	H,377Q		;    and initial
	PUSH	H		;    drive
	PUSH	H		;    parameters
	PUSH	H
	PUSH	H
	LXI	H,10Q		;initialize
	PUSH	H		;    tzflag & cdisk
	MVI	L,176Q		;initialize
	PUSH	H		;    disk & drvsel
	MVI	L,10Q		;initialize
	PUSH	H		;    hdflag & dsflag
	MVI	H,30Q		;initialize
	PUSH	H		;    timer constant
	MVI	A,177Q		;start 1791
	STA	DREG
	MVI	A,CLRCMD	;1791 reset
	STA	CMDREG
LDHEAD
	XRA	A		;load the head
	CALL	HDCHK		;    and test for
	JNC	DOOROK		;    drive ready
	MVI	A,LIGHT		;turn on the
	STA	DCREG		;    error LED
	CALL	TIMOUT		;timeout to
	JMP	LDHEAD		;    close drive door
DOOROK
	MVI	A,NOLITE	;turn off the
	STA	DCREG		;    error LED
	CALL	MEASUR		;head load time
	POP	B		;adjust the stack
	LXI	B,RAM+300H	;DMA addr
	PUSH	B		;initialize
	PUSH	D		;    dmaadr & timer
	LXI	H,0		;initialize
	PUSH	H		;    intrfg & ramins
	NOP			;debug instruction
	PUSH	B		;boot address
	MVI	B,12		;number of retrys
LDLOOP
	PUSH	B		;save the retry no.
	CALL	READ		;read boot sector
	POP	B		;restor retry no.
	RNC			;successful read?
	DCR	B		;no - count down
	JNZ	LDLOOP		;    and try again
LERROR
	MVI	C,11Q
	LXI	D,0a2c3h
LELOOP
	DCX	D
	MOV	A,D
	ORA	E
	JNZ	LELOOP
	MVI	A,10Q		;blink
	XRA	C		;    the LED at
	MOV	C,A		;    top of the
	STA	DCMD		;    circuit board
	JMP	LERROR+2

COUT
	LDA	USTAT		;get UART status
	ANI	OSTAT		;output ready mask
	JNZ	COUT		;test buffer empty
	MOV	A,C		;character data
	CMA			;negative logic bus
	STA	UDATA		;send data to UART
	CMA			;make positive
	RET

CIN
	LDA	USTAT		;get UART status
	ANI	ISTAT		;input ready mask
	JNZ	CIN		;wait for input
	LDA	UDATA		;get the character
	CMA			;adjust for negative bus
	ANI	177Q		;trim to 7 bits
	RET

CPAN
	LDA	USTAT		;get UART status
	ANI	ISTAT		;input ready mask
	RNZ			;test for data
	CALL	CIN		;get character
	CMP	C		;test for panic chtr
	RET
TMSTAT
	LDA	USTAT		;get UART status
	ANI	ISTAT		;input ready mask
	RET

DISKST
	LXI	H,TRKREG	;most recent
	MOV	C,M		;    track to C
	INX	H		;most recent
	MOV	B,M		;    sector to B
	LDA	DCREG		;get current
	CMA			;    density in
	ANI	1		;    the msb
	RRC			;    position
	MOV	D,A		;save in D
	LDA	SIDE		;put the
	RLC			;    most recent
	RLC			;    side select
	RLC			;    in bit positin
	ORA	D		;    6 and merge
	MOV	D,A		;save in D
	LDA	DSFLAG		;get the
	XRI	DSIDE		;    most recent
	RAL			;    double sided
	RAL			;    status and place
	ADD	D		;    in bit position
	MOV	D,A		;    5 and merge
	LDA	SECLEN		;get the
	RAL			;    sector length
	RAL			;    code bits in
	ORA	D		;    positions 2 & 3
	MOV	D,A		;    and merge
	LDA	CDISK		;get the current
	ADD	D		;    disk no. in bit
	RET			;    positions 0 & 1

DMSTAT
	PUSH	H		;save the HL pair
	LHLD	DMAADR		;move the
	MOV	B,H		;    DMA address to
	MOV	C,L		;    the BC pair
	POP	H		;recover HL
	RET

DRIVE
	MOV	A,C		;drive select
	ANI	3		;    values must be
	STA	DISK		;    between zero
	RET			;    and three

DMA
	LXI	H,-RAM		;test the
	DAD	B		;    DMA address
	JC	DMASET		;    for conflict
	LXI	H,8-ORIGIN
	DAD	B		;    with the I/O
	JNC	DMASET		;    on the DJ/2D
	MVI	A,20Q		;    controller
	RET
DMASET
	MOV	H,B		;store the
	MOV	L,C		;    BC pair
	SHLD	DMAADR
	RET

HOME
	CALL	HDLOAD		;load the head
	RC			;not ready error
	CALL	HENTRY		;move the head
	PUSH	PSW		;save status
	SBB	A		;update the
	STA	TRACK		;    track
	STA	TRKREG		;    registers
	XRA	A		;set the not
	STA	TZFLAG		;    verified flag
	JMP	LEAVE+1		;unload the head
HENTRY
	XRA	A		;set the force
	STA	HDFLAG		;    verify flag
	LXI	H,0		;timeout constant
	MVI	A,HCMD		;move the head
	CALL	CENTRY		;to track 0
	ANI	TZERO		;track zero bit
	RNZ
	STC			;error flag
	RET

SECSET
	XRA	A		;test for
	ORA	C		;    zero value
	STC			;error flag
	RZ			;error return
	ANI	37Q		;trim & clear cry
	STA	SECTOR
	RET

SEEK
	MOV	A,C		;test for
	CPI	77		;    track
	CMC			;    too large
	RC
	STA	TRACK
	RET

ISSUE	STA	ECOUNT+1	;update count
	CALL	MEASUR		;find index
	MVI	C,1		;Start with sector 1
ISLOOP
	MOV	A,C		;Initilize the
	STA	SECREG		;    sector register
	LDA	SECTOR		;Test for
	CMP	C		;    target sector
	RZ			
	MVI	A,RDCMD		;do a fake
	CALL	COMAND		;    read command
	JC	PLEAVE		;Abort on error
	INR	C		;Increment sector number
	JMP	ISLOOP

COMNDP	STA	CMDREG		;Start operation
	MOV	C,B		;Initilize block count
	LXI	D,DATREG	;Data register
	LHLD	DMAADR		;transfer address
	RET

WRITE
	CALL	PREP		;prepare for write
	JC	LEAVE		;abort operation
WRENTRY
	MVI	A,WRCMD		;write sector cmd
	CALL	COMNDP
WRLOOP
	MOV	A,M		;load 1st byte of data
	INX	H
	STAX	D		;write 1st byte of data
	MOV	A,M		;load 2nd byte of data
	INX	H
	STAX	D		;write 2nd byte of data
	MOV	A,M		;load 3rd byte of data
	INX	H
	STAX	D		;write 3rd byte of data
	DCR	c		;reduce block count
	MOV	A,M
	INX	H
	STAX	D
	JNZ	WRLOOP		;write next 4 bytes
	LXI	H,WRENTRY
	JMP	CBUSY

READ
	CALL	PREP		;prepare for read
	JC	LEAVE		;abort operation
RDENTRY
	MVI	a,RDCMD
	CALL	COMNDP
RDLOOP
	LDAX	D		;read 1st byte
	MOV	M,A		;store 1st byte
	INX	H
	LDAX	D		;read 2nd byte
	MOV	M,A		;store 2nd byte
	INX	H
	LDAX	D		;read 3rd byte
	MOV	M,A		;store 3rd byte
	INX	H
	DCR	c		;reduce block count
	LDAX	D		;read 4th byte
	MOV	M,A		;store 4th byte
	INX	H
	JNZ	RDLOOP		;read next 4 bytes
	LXI	H,RDENTRY

CBUSY
	PUSH	H		;Save return
	LXI	H,CSTAT		;Wait for 1791
	CALL	BUSY		;     to finish command
	ANI	137Q		;Error mask
	JZ	LEAVE-1		;No error
	CPI	20Q		;Premature interupt
	JNZ	PLEAVE		;Other type of error
	LDA	ECOUNT		;decrement error count
	DCR	A		;     by one
	JM	STEST		;Hard interupt error
	STA	ECOUNT		;Update count
	RET
STEST
	LDA	ECOUNT+1	;Decrement error
	DCR	A
	JP	ISSUE
	MVI	A,20Q
PLEAVE
	STC			;error flag
	POP	H

LEAVE
	PUSH	PSW		;save the status
	LDA	DCREG		;control bits
	XRI	LOAD		;toggle the
	STA	DCMD		;    head load bit
	LDA	DRVSEL		;enable access to
	STA	DREG		;    the data register
	POP	PSW		;recover the status
	RET

PREP
	CALL	HDLOAD		;load the head
	RC			;test for drive ready
	LDA	TRKREG		;get old track
	INR	A		;test for head
	CZ	HENTRY		;    not calibrated
	RC			;seek error?
	LXI	H,TRKREG	;old track
	LDA	TRACK		;new track
	CMP	M		;test for head motion
	INX	H		;advance to the
	INX	H		;    data register
	MOV	M,A		;save new track
	MOV	A,C		;turn off data reg
	STA	DREG		;    access control bit
	JZ	TVERFY		;test for seek
	XRA	A		;force a read
	STA	HDFLAG		;    header operation
	LDA	DSTAT		;get the double
	ANI	DSIDE		;    sided flag
	STA	DSFLAG		;save for status
	RAR			;shift for
	RAR			;    3/6 ms step
	RAR			;    rate constant
	ADI	SKCMD		;do a
	LXI	H,0		;    seek
	CALL	CENTRY		;    operation
	JC	SERROR		;seek error?
TVERFY
	LDA	HDFLAG		;get the force
	ORA	A		;    verify hdr flag
	JNZ	CHKSEC		;no seek & head OK
	MVI	B,2		;verify retry count
SLOOP
	MVI	A,SVCMD		;do a verify
	CALL	COMAND		;    command
	ANI	231Q		;error bit mask
	MOV	D,A		;save
	JZ	RDHDR		;no error
	LDA	DCREG		;denisty control
	XRI	DENSITY		;flip the density
	STA	DCREG		;update and
	STA	DCMD		;    change density
	DCR	B		;decrement retry
	JNZ	SLOOP		;    count & test
	MOV	A,D
SERROR
	STC
	PUSH	PSW
	CALL	HENTRY
	POP	PSW
	RET
RDHDR
	MVI	B,12Q		;number of retrys
RHLOOP
	LXI	D,DATREG	;Data register
	LXI	H,TRACK+1	;Data pointer
	MVI	A,RACMD		;Start read header command
	STA	CMDREG		
RHL1
	LDAX	D		;get disk data 0
	MOV	M,A		;store in mem
	INR	L		;advance pointer
	JNZ	RHL1		;test end of page
	LXI	H,CSTAT		;wait for 1791
	CALL	BUSY		;    to finish cmd
	ORA	A		;test for errors
	JZ	CHKSEC		;transfer OK?
	DCR	B		;no - test for
	JNZ	RHLOOP		;    hard error
	JMP	SERROR		;recalibrate
CHKSEC
	LDA	SECLEN		;get the sector
	MOV	C,A		;    size and setup
	MVI	B,0		;    the table offset
	LXI	H,STABLE	;sector table
	DAD	B		;sector size pntr
	LDA	SECTOR		;get the sector
	MOV	B,A		;    and save in B
	ADD	M		;compare w/table
	MVI	A,20Q		;error flag
	RC			;error return
	MOV	A,B		;initialize 1791
	STA	SECREG		;    sector register
	MVI	A,40Q		;128 byte sector
	LXi	h,505h
	SHLD	ECOUNT
SZLOOP
	DCR	C		;reduce size count
	MOV	B,A		;sector size to B
	RM			;return on minus
	RAL			;double the count
	ORA	A		;clear the carry
	JMP	SZLOOP

STABLE
	DB	345Q		;26 sector diskettes
	DB	345Q		;26 sector diskettes
	DB	360Q		;15 sector diskettes
	DB	367Q		;8 sector diskettes
HDLOAD
	LXI	H,DISK		;new drv ptr
	MOV	C,M		;save new drv in C
	INX	H		;current drv ptr
	MOV	E,M		;save old drv in E
	MOV	M,C		;update current drv
	INX	H		;home cmd flag
	MOV	A,E		;test for
	CMP	C		;    drive change
	MOV	A,M		;head load mask
	MVI	M,HEAD		;update the mask
	JZ	HDCHK		;no drive change?
	INX	H		;addr of drive table
	PUSH	H		;save table addr
	MVI	D,0		;set up the
	MOV	B,D		;    offset address
	DAD	D		;calculate the
	DAD	D		;    parameter addr
	LDA	DCREG		;save the
	MOV	M,A		;density status
	INX	H		;track pointer
	LXI	D,TRKREG	;1791 trk reg
	LDAX	D		;get current track
	MOV	M,A		;save in the table
	POP	H		;beginning of table
	DAD	B		;new drive
	DAD	B		;    table pointer
	MOV	A,M		;get density status
	STA	DCREG		;update DCREG
	INX	H		;get the old
	MOV	A,M		;    track number
	STAX	D		;    and update 1791
	MVI	A,177Q		;drive select bits
DSROT
	RLC			;rotate to
	DCR	C		;    select the
	JP	DSROT		;    proper drive
	ANI	177Q		;set the run bit
	STA	DRVSEL		;save in drv reg
	XRA	A		;force a head load
HDCHK
	LXI	H,DSTAT		;test for
	ANA	M		;    head loaded
	STA	HDFLAG		;save the head
	PUSH	PSW		;    loaded status
	LDA	DRVSEL		;get current drive
	MOV	C,A		;save
	LDA	SIDE		;get current side
	CMA			;    and merge
	ANA	C		;    with drive select
	STA	DREG		;select drive & side
	XRI	ACCESS		;toggle access bit
	MOV	C,A		;save for PREP routine
	LDA	DCREG		;den & head cntl bits
	MOV	B,A		;save
	LDA	TRACK		;get the new track
	SUI	1		;force single
	SBB	A		;    density
	DCR	A		;    if track = 0
	CMA			;compliment
	ORA	B		;merge w/control bits
	MOV	M,A		;load head & set density
	POP	PSW		;head load status
	JNZ	RDYCHK		;conditionally
	PUSH	H		;    wait for head
	LHLD	TIMER		;    load time out
TLOOP
	DCX	H		;count down
	MOV	A,H		;    40 ms for
	ORA	L		;    head load
	JNZ	TLOOP		;    time out
	POP	H
RDYCHK
	MOV	A,M		;test for
	ANI	READY		;    drive ready
	RNZ
UNLOAD
	LDA	DCREG		;force a
	ORI	ULOAD		;    head
	MOV	M,A		;    unload
	MVI	A,READY		;set drive
	STC			;    not ready
	RET			;    error flag

COMAND
	LHLD	TIMER		;get index count
	DAD	H		;    and multiply
	DAD	H		;    by four
CENTRY
	XCHG			;save in D-E pair
	LXI	H,CSTAT		;issue command
	MOV	M,A		;    to the 1791
NBUSY
	MOV	A,M		;wait
	RAR			;    for the
	JNC	NBUSY		;    busy flag
BUSY
	MOV	A,M		;test for
	RAR			;    device busy
	MOV	A,M		;restore status
	RNC			;return if not busy
	JMP	PATCH+3		;jump around patch
PATCH
	JMP	HDLOAD		;patch for old ATE

	DCX	D		;test for
	MOV	A,D		;    two disk
	ORA	E		;    revolutions
	JNZ	BUSY		;47 machine cycles
	MOV	E,M		;get error code
	PUSH	H		;save cmd address
	INX	H		;track register
	MOV	D,M		;save present track
	LDA	DRVSEL		;control bits
	XRI	RSTBIT		;reset the 1791
	STA	DREG		;    controller to
	XRI	STBITS		;    clear the
	XTHL			;    command busy
	STA	DREG		;    fault condition
	MVI	M,CLRCMD	;force interrupt
	XTHL			;restore the
	MOV	M,D		;    the track reg
	POP	H		;restore the stack
	MOV	A,E		;error code to A
	STC			;    error flag
	RET

MEASUR
	LXI	D,0		;initialize count
	LXI	H,DSTAT		;status port
	MVI	C,INDEX		;index bit flag
INDXLO
	MOV	A,M		;wait for
	ANA	C		;    index
	JZ	INDXLO		;    pulse high
INDXHI
	MOV	A,M		;wait for
	ANA	C		;    index
	JNZ	INDXHI		;    pulse low
INDXCT
	INX	D		;advance count
	XTHL			;four dummy
	XTHL			;    instructions
	XTHL			;    to lengthen
	XTHL			;    the delay
	MOV	A,M		;wait for
	ANA	C		;    the index
	JZ	INDXCT		;    to go high
	RET			;98 machine cycles
DENFIX
	MOV	A,C		;trim the
	ANI	1		;    excess bits
	CMA			;compliment and
	MOV	B,A		;    save in B
	LXI	H,DISK		;new disk ptr
	MOV	E,M		;get disk no.
	MVI	D,0		;offset addr
	INX	H		;current disk ptr
	MOV	A,M		;move to ACC
	XRA	E		;cmpr old w/new
	PUSH	PSW		;save status
	INX	H		;disk table
	INX	H		;    address
	DAD	D		;add the
	DAD	D		;    offset
	MOV	A,M		;get parameters
	ORI	1		;mask off density
	ANA	B		;set new density
	MOV	M,A		;update parameters
	POP	PSW		;test new=old?
	RNZ
	MOV	A,M		;updata CDISK
	STA	DCREG		;    also
	RET

TIMOUT
	LXI	H,0		;time-out delay
TILOOP
	DCX	H		;decrement count
	MOV	A,H		;test for delay
	ORA	L		;    count equal zero
	XTHL			;long NOP
	XTHL			;    instruction
	JNZ	TILOOP
	RET

SBEGIN	PUSH	H
	LXI	H,DSTALL
DSTALL	PCHL
	POP	H
	RET

SIDEFX
	MOV	A,C		;get the side bit
	ANI	1		;trim the excess
	RAL			;move the bit
	RAL			;    to the side
	RAL			;    select bit
	RAL			;    position
	STA	SIDE		;save side bit
	RET

PWRJMP
	NOP			;power-on
	NOP			;    jump
	NOP			;    sequence
	NOP			;    with NOP
	NOP			;    padding
	JMP	DBOOT

	DS	10Q		;I/O locations

	org	ram+3c9h

STACK	DS	31Q

ECOUNT	DW	0
TIMER	DW	1800h		;head load time out
DMAADR	DW	RAM+300H	;dma address
DSFLAG	DB	10Q
HDFLAG	DB	0		;read header flag
DRVSEL	DB	176Q		;drive select constant
DISK	DB	0		;new drive
CDISK	DB	10Q		;current disk
TZFLAG	DB	0		;home cmd indicator
DOPRAM	DB	11Q		;drive 0 parameters
DOTRK	DB	377Q		;drive 0 track no
D1PRAM	DB	11Q		;drive 1 parameters
D1TRK	DB	377Q		;drive 1 track no
D2PRAM	DB	11Q		;drive 2 parameters
D2TRK	DB	377Q		;drive 2 track no
D3PRAM	DB	11Q		;drive 3 parameters
D3TRK	DB	377Q		;drive 3 track no
DCREG	DB	11Q		;current parameters
SIDE	DB	0		;new side
SECTOR	DB	1		;new sector
TRACK	DB	0		;new track
TRKNO	DB	0		;disk
SIDENO	DB	0		;    sector
SECTNO	DB	0		;    header
SECLEN	DB	0		;    data
CRCLO	DB	0		;    buffer
CRCHI	DB	0


