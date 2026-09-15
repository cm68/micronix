;
; End of the .bss segment.  textpad.o links last, so its .bss section
; lands after every other object's.  binit() (main.c) places the buffer
; pool here, above all data *and* all .bss - the buffer data must not
; overlap the .bss that holds function-scope statics (mw.c's mwinfo,
; mwbuf, curdrv, ...), which ccc puts after the .data blist lives in.
;
; u (see user.c) no longer needs the text padded up to a 4K boundary:
; the link now places user.rel at 0xf000 outright (see GNUmakefile), so
; u and its segpad occupy the last 4K segment and the buffer pool runs
; from _ebss up to _usrtop == _memtop == 0xf000, just below it.
;
	.bss
	.globl	_ebss
_ebss:
	.ds	0
