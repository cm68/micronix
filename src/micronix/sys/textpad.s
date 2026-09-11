;
; Pad the text segment up to the next 4K boundary.
;
; The DJDMA loader reads the kernel image contiguously - header, then
; text, then data - and does not honour the a.out data_base field.  So
; the data segment must begin exactly where the text ends, and the u
; struct (see user.c), which newmap() remaps per process, must sit at
; the base of a 4K segment.  u is the first data object, so the text
; has to end on a 4K boundary for the two to agree.
;
; 0x53 = 0xa000 - 0x9fad = 0x1000 - (current text size mod 0x1000).
; Keep this in sync with the link: after a build, "mxnm unix" must show
; _u at 0xa000.  If the text grows or shrinks past a boundary, bump or
; shrink this pad accordingly.
;
	.text
	.ds	0x53

;
; End of the .bss segment.  textpad.o links last, so its .bss section
; lands after every other object's.  binit() (main.c) places the buffer
; pool here, above all data *and* all .bss - the buffer data must not
; overlap the .bss that holds function-scope statics (mw.c's mwinfo,
; mwbuf, curdrv, ...), which ccc puts after the .data blist lives in.
;
	.bss
	.globl	_ebss
_ebss:
	.ds	0
