;
; sys/locore.s was sys/uhdr.s
; Changed: <2026-08-24 07:46:36 curt>
;
; this object file is loaded at a lower address than everthing else
; in particular, the loader places this in memory at 0x1000
; and jumps to it in task 1

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
; _usrtop := &0xf000
; _memtop := &0xf000
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
_usrtop=0xF000
_memtop=0xF000
	.globl	_trapstack, _oldstack, _cmask, _ctask, _mask, _rst1
	.globl	_trapad, _status, _wtask, _usrtop, _memtop, _trapvec
	.globl	_hlt, _xinit, _map0, _map1, _image0, _image1
	.extern	_plist, _start

;
; this is the kernel program entry point, and it expects to be at 0x1000
;

	.defb	0xC3		; jump opcode
_trapvec:
	.defw	boot
	
	.defw	_plist 		; the process list so ps can find it.

boot:
	ld	sp,0x1000	; task 1 has ram here.
	jp	_start		; enter the kernel start trampoline

;
; /Halt intruction for use in _trapc
;
_hlt:
	halt
	ret

;
; the interrupt vector table used by the i8259 in 4 byte mode
; this must be aligned on a 32 byte boundary, and this depends
; on this segment being loaded at an absolute address
;
	.align	32
vector:
	.ds	32

;
; this is the initial program that gets copied out to process 1
; it simply executes /etc/init
;

_xinit::
	rst	8		; SYS := rst1  (RST 1 -> vector 0x08)
	.defb	11		; EXEC := 11  (syscall number)
	.defw	initprog
	.defw	iargs
	ret

initprog:
	.defb	"/etc/init", 0

iargs:
	.defw	iarg0		; argv
	.defw	0

iarg0:
	.defb	"init", 0	; argv[0]
