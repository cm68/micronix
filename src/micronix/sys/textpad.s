;
; End of the .bss segment.  textpad.o links last, so its .bss section
; lands after every other object's.  binit() (main.c) places the buffer
; pool here, above all data *and* all .bss - the buffer data must not
; overlap the .bss that holds function-scope statics (mw.c's mwinfo,
; mwbuf, curdrv, ...), which ccc puts after the .data blist lives in.
;
; u (see user.c) no longer needs the text padded up to a 4K boundary:
; the link places upage.o at 0xf000 outright (see GNUmakefile), so u
; and its segpad occupy segment 15 and the buffer pool runs from _ebss
; up to BUFWIN (0xe000, the base of the window's page), one page below.
;
	.bss
	.globl	_blist
_blist:
	.ds	168		; 8 * sizeof(struct buf) = 8*21, the boot headers
	.globl	_ebss
_ebss:
	.ds	0
