;
; The CompuPro SS1's two 8259 vector tables.
;
; sys/ss1vec.s
;
; The SS1 has two CAS-cascaded 8259s.  The master's IR0..IR6 are VI0..VI6
; and its IR7 is the slave's cascade; the slave's IR0 is VI7 and IR1..IR7
; are the board's own timers, FPU and UART.  The two tables are copied to
; 0x200 and 0x220 at boot by the probe (sys/pic_init.c), the way vecalign
; relocates the single-PIC table.
;
; The tables are data, not code: each entry is a jp to a resident handler.
; They reuse the master's int0/int1 (sys/intrpt.s) for VI0/VI1 and the
; slave's sint0/sint1/sint2 (sys/slavevec.s) for VI2..VI4, and reach VI5,
; VI6 and VI7 through the resident ovlint5/6/7.  On the slave, IR1 is the
; 8253 heartbeat (ss1_clkint, sys/mio.s) and IR6/IR7 are the 2651's
; TxRDY/RxRDY, which drive the console tty (ss1_txint/ss1_rxint).  IR2..IR5
; are the FPU and the unimplemented uses, so they land on spurious.
;
	.data			; data only: folds into highmem.o and is reclaimed
	.globl	_ss1_master_vec, _ss1_slave_vec
	.extern	int0, int1, sint0, sint1, sint2
	.extern	_ovlint5, _ovlint6, _ovlint7, spurious, ss1_clkint
	.extern	ss1_txint, ss1_rxint

_ss1_master_vec:
	.defb	0xC3		; jp int0    (VI0, line 0)
	.defw	int0
	.defb	0
	.defb	0xC3		; jp int1    (VI1, line 1)
	.defw	int1
	.defb	0
	.defb	0xC3		; jp sint0   (VI2, line 2)
	.defw	sint0
	.defb	0
	.defb	0xC3		; jp sint1   (VI3, line 3)
	.defw	sint1
	.defb	0
	.defb	0xC3		; jp sint2   (VI4, line 4)
	.defw	sint2
	.defb	0
	.defb	0xC3		; jp ovlint5 (VI5, line 5)
	.defw	_ovlint5
	.defb	0
	.defb	0xC3		; jp ovlint6 (VI6, line 6)
	.defw	_ovlint6
	.defb	0
	.defb	0xC3		; jp spurious (IR7, the cascade: the slave vectors itself)
	.defw	spurious
	.defb	0

_ss1_slave_vec:
	.defb	0xC3		; jp ovlint7 (VI7, line 7)
	.defw	_ovlint7
	.defb	0
	.defb	0xC3		; jp ss1_clkint (IR1, the 8253 heartbeat)
	.defw	ss1_clkint
	.defb	0
	.defb	0xC3		; jp spurious (IR2, FPU/UART - not yet)
	.defw	spurious
	.defb	0
	.defb	0xC3		; jp spurious (IR3)
	.defw	spurious
	.defb	0
	.defb	0xC3		; jp spurious (IR4)
	.defw	spurious
	.defb	0
	.defb	0xC3		; jp spurious (IR5)
	.defw	spurious
	.defb	0
	.defb	0xC3		; jp ss1_txint (IR6, the 2651's TxRDY)
	.defw	ss1_txint
	.defb	0
	.defb	0xC3		; jp ss1_rxint (IR7, the 2651's RxRDY)
	.defw	ss1_rxint
	.defb	0
