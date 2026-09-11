;
; ------- A-NATURAL SOURCE: declarations -------
; Decision firmware references
; sys/uhdr.s
; Changed: <2026-08-17 20:01:38 curt>
;
; _trapstack := &8
; _cmask	:= &7
; _ctask	:= &6
; _mask	:= &0x403
; _oldstack := &0x1e0
; _rst1	:= &8
; _trapad := &0x400
; _status := &0x403
; _wtask	:= &0x81b
; _map0	:= &0x600
; _map1	:= &0x620
; _image0 := &0x200
; _image1 := &0x220
;
; /ram configuration
;
; _usrtop := &0xffff
; _memtop := &0xffff
;
; /Making these references public AFTER their definitions
; /seems to appease A-Natural
;
; public	_trapstack
; public	_oldstack
; public	_cmask
; public	_ctask
; public	_mask
; public	_rst1
; public	_trapad
; public	_status
; public	_wtask
; public	_usrtop
; public	_memtop
; public	_trapvec
; public	_hlt
; public	_xinit
; public	_map0
; public	_map1
; public	_image0
; public	_image1
;
; /Power-up entry _point
; /Decision firmware expects this to be at 0x1000
;
; 	jump	:= 0303
; 	jump
;
_trapstack=8
_cmask=7
_ctask=6
_mask=0x403
_oldstack=0x1E0
_rst1=8
_trapad=0x400
_status=0x403
_wtask=0x81B
_map0=0x600
_map1=0x620
_image0=0x200
_image1=0x220
_usrtop=0xFFFF
_memtop=0xFFFF
	.globl	_trapstack, _oldstack, _cmask, _ctask, _mask, _rst1
	.globl	_trapad, _status, _wtask, _usrtop, _memtop, _trapvec
	.globl	_hlt, _xinit, _map0, _map1, _image0, _image1
	.extern	_plist, _start

;
; this is the kernel program entry point, and it expects to be at 0x1000
;

	.defb	0xC3		; jump opcode

; ------- A-NATURAL SOURCE: _trapvec -------
; _trapvec:
; 	&boot
;
; /Hook for ps (at address 0x1003)
; 	&_plist
;
_trapvec:
	.defw	boot

;
; this is the address of the process list so ps can find it.
;
	.defw	_plist

;
; boot lands here
;

; ------- A-NATURAL SOURCE: boot -------
; boot:
; 	sp = &0x1000
; 	jmp _start
;
boot:
	ld	sp,0x1000	; an initial stack pointer at an odd place
	jp	_start		; enter the kernel

; ------- A-NATURAL SOURCE: _hlt -------
; /Halt intruction for use in _trapc
; _hlt:
; 	hlt
; 	ret
;
_hlt:
	halt
	ret

;
; task0 is entered by the firmware with its return address on the
; firmware stack; task0_body()'s setframe() moves SP to the kernel
; stack.  Park the firmware SP in BC (callee-saved) and hand it back
; in _retask so the ret lands in the firmware, not in u.stack.
;
	.globl	_task0, _retask
	.extern	_task0_body
_task0:
	ld	hl,0
	add	hl,sp		; hl = firmware SP
	ld	b,h
	ld	c,l		; bc = firmware SP
	call	_task0_body	; never returns - ends in retask()
	ret			; not reached

_retask:
	ld	h,b
	ld	l,c		; hl = firmware SP
	ld	sp,hl
	ret

;
; this is the initial program that gets copied out to process 1
; it simply executes /etc/init
;

; ------- A-NATURAL SOURCE: _xinit -------
; /System call to execute "init".
; /Called from start() in _trapc
; 	EXEC	:= 11
; 	SYS	:= rst1
;
; _xinit:
; 	/
; 	SYS; EXEC; &name1; &iargs;
; 	ret		/error return
; 	/
;
_xinit::
	rst	8		; SYS := rst1  (RST 1 -> vector 0x08)
	.defb	11		; EXEC := 11  (syscall number)
	.defw	initprog
	.defw	iargs
	ret

; ------- A-NATURAL SOURCE: name1 -------
; name1:
; 	/
; 	"/etc/init\0";
; 	/
;
initprog:
	.defb	"/etc/init", 0

; ------- A-NATURAL SOURCE: iargs -------
; iargs:
; 	/
; 	&iarg0;
; 	&0;
; 	/
;
iargs:
	.defw	0		; empty argv, as the reference kernel passes
