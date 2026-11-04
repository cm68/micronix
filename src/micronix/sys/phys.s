;
; The banked physical-memory trampoline for /dev/mem (sys/memdev.c).
;
; The S-100 address space is 24 bits, but a program sees 1MB at a time:
; the map register names an 8-bit 4K page and the task register's high
; nibble selects the 1MB bank (A20-A23).  Writing the task register to
; change the bank also moves the whole address space, so the code that
; does it cannot run from mapped memory - it would unmount itself.
;
; The CPU board's own page at 0000h-0fffh is not mapped: in supervisor
; mode it is served from on-board RAM and registers whatever the bank
; is (mpz80.c: local = super_flag && addr < 0x1000).  That is why the
; monitor's level 0 copy loop runs from ramhome and banks to 15 to read
; the level 1 rom, bouncing 256 bytes at a time through the buffer at
; 0032h (mon500.s, ramcode/bank).  This is the same trick after boot:
; the kernel installs _physcode at 0032h and calls it there.
;
; The low page's 0032h-0131h is the monitor's bounce buffer and is dead
; once the kernel is running; the kernel's own low block starts at 0260h
; (uhdr.s) and the freed tail above it is left alone.
;
;	void phinstall();		install _physcode at 0032h, once
;	void physxfer(u8 *p);		call at 0032h, p as:
;	  p[0..1] the offset in the 4K page, little endian
;	  p[2]    the count, 1..128
;	  p[3]    the task register value (bank << 4)
;	  p[4]    the page number within the bank
;	  p[5]    direction: 0 physical to bounce, 1 bounce to physical
;
; The trampoline's bounce buffer is 0090h-010fh, inside the monitor's
; dead 0032h-0131h block (mon500.s's 256-byte bounce).  Every access in
; the banked window
; is to the on-board page: the code, the bounce, and the map and task
; registers.  Interrupts are off across it, and segment 1 is put back to
; page 1 - the identity the kernel runs with, and what memrw restores -
; before the bank returns to 0.

	.globl	_physcode, _physend, _physxfer

MAP0=0x0600		;the map registers, task 0
TASK=0x0402		;the task register: task | bank << 4
PHYSLOW=0x0032		;where the code runs, in on-board ram
PHYSHOP=0x0090		;the bounce buffer, 128 bytes

_physxfer=PHYSLOW

;	The trampoline itself.  No internal labels are needed: the two
;	branches are relative and every other address is absolute, so the
;	bytes run the same wherever they land.
;
_physcode:
	push	bc		;callee-saved
	push	de
	push	hl
	ld	a,(hl)		;offset low
	ld	e,a
	inc	hl
	ld	a,(hl)		;offset high (0..15)
	ld	d,a
	inc	hl
	ld	a,(hl)		;count
	ld	c,a
	ld	b,0
	inc	hl
	ld	a,(hl)		;task register: bank << 4
	push	af
	inc	hl
	ld	a,(hl)		;page within the bank
	push	af
	inc	hl
	ld	a,(hl)		;direction
	or	a
	jr	nz,1f
	ld	a,d		;read: physical -> bounce
	or	0x10
	ld	h,a
	ld	l,e
	ld	de,PHYSHOP
	jr	2f
1:
	ld	a,d		;write: bounce -> physical
	or	0x10
	ld	d,a
	ld	hl,PHYSHOP
2:
	di
	pop	af		;page
	ld	(MAP0+2),a	;segment 1 -> the page, in the new bank
	pop	af		;task and bank
	ld	(TASK),a
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop			;the task register's eight-cycle swap
	ldir
	ld	a,1		;segment 1 back to page 1, identity
	ld	(MAP0+2),a
	xor	a
	ld	(TASK),a	;task 0, bank 0
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop			;the pops below touch the banked stack
	ei
	pop	hl
	pop	de
	pop	bc
	ret
_physend:

;
; vim: tabstop=8 shiftwidth=8 noexpandtab:
;
