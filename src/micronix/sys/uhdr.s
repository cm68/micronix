;
; =====================================================================
; sys/uhdr.s  --  Z80 translation of sys/uhdr.anat (asz dialect)
; =====================================================================
;
; ------- COMPLETE ORIGINAL A-NATURAL SOURCE (every line preserved) -------
;
; /*
;  * Decision firmware references
;  *
;  * sys/uhdr.s
;  * Changed: <2022-01-04 11:29:52 curt>
;  */
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
; _trapvec:
; 	&boot
; 
; /Hook for ps (at address 0x1003)
; 	&_plist
; 
; boot:
; 	sp = &0x1000
; 	jmp _start
; 
; /Halt intruction for use in _trapc
; _hlt:
; 	hlt
; 	ret
; 
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
; name1:
; 	/
; 	"/etc/init\0";
; 	/
; 
; iargs:
; 	/
; 	&iarg0;
; 	&0;
; 	/
; 
; iarg0:
; 	/
; 	"init\0";
; 	/
;
; ------- END A-NATURAL SOURCE -------
;
; ------- Z80 TRANSLATION -------
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
	.defb	0xC3		; jump := 0303 (jp opcode)
_trapvec:
	.defw	boot
	.defw	_plist
boot:
	ld	sp,0x1000
	jp	_start
_hlt:
	halt
	ret
_xinit:
	rst	8		; SYS := rst1  (RST 1 -> vector 0x08)
	.defb	11		; EXEC := 11  (syscall number)
	.defw	name1
	.defw	iargs
	ret
name1:
	.defb	"/etc/init", 0
iargs:
	.defw	iarg0
	.defw	0
iarg0:
	.defb	"init", 0
