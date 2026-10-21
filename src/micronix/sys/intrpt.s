;
; =====================================================================
; sys/intrpt.s  --  Z80 translation of sys/intrpt.anat (asz dialect)
; =====================================================================
;
; ------- A-NATURAL SOURCE: declarations -------
; /*
;  * Interrupt dispatch table.  The table itself is copied to 0x200 at
;  * boot (sys/picvec.c) and the 8259's ICW2 names that address; here it
;  * is only the source.  Most of the interrupt catchers are in mio.s.
;  *
;  * sys/intrpt.s
;  * Changed: <2021-12-24 06:08:17 curt>
;  */
;
; public	vectors
;
	.globl	_vectors
	.extern	intrupt, m1int, m2int
	.extern	m3int, clkint, _ovlntr
	.globl	_ovlint0, _ovlint1, _ovlint5, _ovlint6, _ovlint7
	.globl	int0, int1, spurious

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
; int6:	call intrupt; &_ovlint6		/ parallel printer (1 PIC) or slave cascade (2 PIC)
; int7:	call intrupt; &clkint		/ clock int
;
; /hint:					/hard disk interrupts
; 	/call _mwint			/mw.c
;        /call _hdint			/wn.s
; 	/ret
;
; int2 is the MultIO slave's cascade line on the two-PIC Wunderbus, and
; the cascade stub below polls the slave and dispatches the line it
; names.  A card names no line: the attach loop (sys/probe_init.c)
; tickles it and reads the PIC back to find its line, and ovlntr
; (sys/probe.c) holds one handler per line.  A second card that tickles
; a line already claimed is a wiring error and the attach loop panics on
; it - cards do not share a line on this kernel.
;
; The lines that belong to a disk controller no longer name the driver's
; handler.  mwint, djint, ideint and ncrint are in modules, so the kernel
; has no address for them at link time; each line hands intrupt a
; dispatcher in sys/ovl.c instead, and the modules that claimed the line
; with intrset have their handlers called from there.  A line nothing has
; claimed returns without doing anything.  int3 through int7 are the
; resident ACE, parallel and clock handlers and are unchanged.
;
_vectors:
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
	.defw	cascade
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
	.defw	spurious
int7:
	call	intrupt
	.defw	clkint

; ovlint0/1: the two master VI lines, dispatched through the resident
; ovlntr (sys/probe.c).  intrupt already saved the registers and does the
; dismiss, so these are a tail jump - load the line and go.
_ovlint0:
	ld	hl,0
	jp	_ovlntr
_ovlint1:
	ld	hl,1
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

;
; IR2 (VI2), the slave's cascade line.  The slave's INT is wired here, so
; the master vectors this and the slave has to say which of its lines
; fired.  Poll it (OCW3 poll) and hand the line to the resident dispatcher
; (_ovlntr, sys/ovl.c), which maps each module and calls its handler.
; The poll read is itself the slave's acknowledge; the master's EOI is
; intrdismiss(), so there is nothing else to dismiss.
;
; The slave sits at 0x40 and these two ports are its own - the same base
; sys/pic_init.c brings it up on and sys/pic.c dismisses it by.  The three
; have to agree: the poll read is the acknowledge, so a slave polled
; anywhere else is never acknowledged, its INT stays asserted, and the
; master's IR2 is taken again the moment intrupt returns.
;
cascade:
	ld	a,0x08		; SPICONLY: select the slave PIC
	out	(047h),a	; SSELECT
	ld	a,0x0c		; OCW3: poll command
	out	(044h),a	; SPIC0
	in	a,(044h)	; the poll word (and the acknowledge)
	ld	b,a		; keep it for the level
	and	080h		; the pending bit
	jp	z,casnone	; nothing to dispatch
	ld	a,b
	and	07h		; the slave's line, 0..2
	add	a,2		; the line number, 2..4
	ld	l,a
	ld	h,0
	call	_ovlntr		; dispatch that line's handlers
	ret

;
; The cascade fired but the slave has nothing pending (it was polled by
; something else, or the INT was already dropping).  ovlntr never ran, so
; the master's IR2 in-service bit is still set and must be dismissed here
; or the master wedges.  PENABLE (0x28) to MSELECT (0x4f) selects the
; controller group; 0x62 is the specific EOI for master IR2.
;
casnone:
	ld	a,0x28
	out	(04fh),a
	ld	a,0x62
	out	(04ch),a
	ret

;
; IR6, nothing wired here on the two-PIC build.  A spurious interrupt
; (or one from a card that never attached) is still a master line in
; service, so dismiss it before returning.
;
spurious:
	ld	a,0x28		; PENABLE to MSELECT: the controller group
	out	(04fh),a
	ld	a,0x66		; specific EOI, master IR6
	out	(04ch),a
	ret
