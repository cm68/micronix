;
; The SS1's two 8259 vector tables and their dispatch stubs.
;
; sys/ss1pic.s
;
; Each table is 32 bytes of slack followed by 8 entries of "jp <stub>"
; (one byte of pad per entry).  pic_init (sys/ss1pic.c) copies the 32
; table bytes down to the slack's 32-byte boundary and names that in the
; ICW1/ICW2 vector bits, exactly as minit did the master's (sys/inits.s).
;
; The master vectors IR0-6 (VI0-6) to the line stubs and IR7 (the
; cascade) to nothing: with CAS the slave supplies its own vector, so the
; master's cascade entry is never reached.  The slave vectors IR0 (VI7)
; to the line stub and IR1-7 (timers, FPU, UART) to badint until that
; board bring-up is written.
;
	.globl	_ss1_master_vec, _ss1_slave_vec
	.extern	_ovlntr

	.defw	0,0,0,0		; 32 bytes of slack, the master copy's target
	.defw	0,0,0,0
	.defw	0,0,0,0
	.defw	0,0,0,0

_ss1_master_vec:
	.defb	0xC3		; jp ovlint0 -- explicit, so asz won't relax jp to jr
	.defw	_ovlint0
	.defb	0
	.defb	0xC3
	.defw	_ovlint1
	.defb	0
	.defb	0xC3
	.defw	_ovlint2
	.defb	0
	.defb	0xC3
	.defw	_ovlint3
	.defb	0
	.defb	0xC3
	.defw	_ovlint4
	.defb	0
	.defb	0xC3
	.defw	_ovlint5
	.defb	0
	.defb	0xC3
	.defw	_ovlint6
	.defb	0
	.defb	0xC3		; IR7: the cascade, never vectored
	.defw	badint
	.defb	0

	.defw	0,0,0,0		; 32 bytes of slack, the slave copy's target
	.defw	0,0,0,0
	.defw	0,0,0,0
	.defw	0,0,0,0

_ss1_slave_vec:
	.defb	0xC3		; jp ovlint7 -- the slave's IR0 is VI7
	.defw	_ovlint7
	.defb	0
	.defb	0xC3		; IR1-7: timers, FPU, UART - stubs until written
	.defw	badint
	.defb	0
	.defb	0xC3
	.defw	badint
	.defb	0
	.defb	0xC3
	.defw	badint
	.defb	0
	.defb	0xC3
	.defw	badint
	.defb	0
	.defb	0xC3
	.defw	badint
	.defb	0
	.defb	0xC3
	.defw	badint
	.defb	0
	.defb	0xC3
	.defw	badint
	.defb	0

; The line stubs: load the line and hand it to the resident dispatcher
; (sys/probe.c).  intrupt already saved the registers and does the
; dismiss, so these are a tail jump.
_ovlint0:
	ld	hl,0
	jp	_ovlntr
_ovlint1:
	ld	hl,1
	jp	_ovlntr
_ovlint2:
	ld	hl,2
	jp	_ovlntr
_ovlint3:
	ld	hl,3
	jp	_ovlntr
_ovlint4:
	ld	hl,4
	jp	_ovlntr
_ovlint5:
	ld	hl,5
	jp	_ovlntr
_ovlint6:
	ld	hl,6
	jp	_ovlntr
_ovlint7:
	ld	hl,7
	jp	_ovlntr

badint:
	ret
