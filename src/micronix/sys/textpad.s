;
; End of the .bss segment.  textpad.o links last, so its .bss section
; lands after every other object's.  binit() (main.c) places the buffer
; pool here, above all data *and* all .bss - the buffer data must not
; overlap the .bss that holds function-scope statics (mw.c's mwinfo,
; mwbuf, curdrv, ...), which ccc puts after the .data blist lives in.
;
; u (see user.c) no longer needs the text padded up to a 4K boundary:
; the link places upage.o at 0xf000 outright (see GNUmakefile), so u
; and its segpad occupy segment 15.  The buffer pool runs from _ebss up
; to SCRWIN (0xc000, the scratch window's page) and not to BUFWIN, the
; page below u: mem.s and swin() borrow the scratch page, and bhold()
; maps buffer data over the buffer page, so a header in either would be
; overwritten.
;
	.bss
	.globl	_blist
_blist:
	.ds	152		; 8 * sizeof(struct buf) = 8*19, the boot headers
	.globl	_ebss
_ebss:
	.ds	0
