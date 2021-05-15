psect	text
global	_main
_main:
global	ncsv, cret, indir
call	ncsv
defw	f1
global	_printf
ld	hl,19f
push	hl
call	_printf
ld	hl,2
add	hl,sp
ld	sp,hl
l1:
jp	cret
f1	equ	0
global	_foobiebletch
_foobiebletch:
call	ncsv
defw	f5
global	_v
global	_k
ld	hl,(_k)
push	hl
ld	hl,_v+4
push	hl
ld	hl,29f
push	hl
call	_printf
ld	hl,2+2+2
add	hl,sp
ld	sp,hl
l2:
jp	cret
f5	equ	8
psect	data
19:
defb	104,101,108,108,111,44,32,119,111,114,108,100,10,0
29:
defb	37,115,32,37,100,10,0
psect	bss
_v:
	defs	39
_k:
	defs	2
global	ncsv, cret, indir
call	ncsv
defw	f1
global	_printf
ld	hl,19f
push	hl
call	_printf
