;
; The slave 8259's interrupt vector table, parked in the u page (where the
; page has room) rather than the full resident text.  Three entries - the
; slave's IR0..IR2, which are VI3..VI5 - each jumping through intrupt to a
; stub that dispatches line 3, 4 or 5.
;
; The table is copied to a 32-byte boundary at boot by slave_pic_init
; (sys/cus.c), exactly the way minit (sys/inits.s) relocates the master's
; vectors, so it does not need to be aligned here: the 32 bytes of slack
; above it are the copy's target.
;
	.data			; leaf code, parked in the u page (see user.c)
	.globl	_slave_vectors
	.extern	intrupt, _ovlntr

	.defw	0,0,0,0		; 32 bytes of slack, the copy's target
	.defw	0,0,0,0
	.defw	0,0,0,0
	.defw	0,0,0,0

_slave_vectors:
	.defb	0xC3		; jp sint0 -- explicit, so asz won't relax jp to jr
	.defw	sint0
	.defb	0
	.defb	0xC3		; jp sint1
	.defw	sint1
	.defb	0
	.defb	0xC3		; jp sint2
	.defw	sint2
	.defb	0

sint0:
	call	intrupt
	.defw	_ovlint3
sint1:
	call	intrupt
	.defw	_ovlint4
sint2:
	call	intrupt
	.defw	_ovlint5

; ovlint3/4/5: dispatch the slave's line 3, 4 or 5 through the resident
; ovlntr (sys/ovl.c).  intrupt already saved the registers and does the
; dismiss, so these are a tail jump - load the line and go.
_ovlint3:
	ld	hl,3
	jp	_ovlntr
_ovlint4:
	ld	hl,4
	jp	_ovlntr
_ovlint5:
	ld	hl,5
	jp	_ovlntr
