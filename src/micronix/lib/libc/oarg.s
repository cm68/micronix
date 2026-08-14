;	out() with its argument inline after the call.
;
;	"out(str)" with a constant string is the most repeated call in
;	the code generator - 182 sites between rewrite and parseast -
;	and each one spelled ld hl / push / call / two inc sp: nine
;	bytes to say five.  The peephole rewrites the shape to
;
;		call oarg
;		.dw  str
;
;	and this fetches the argument through the return address, the
;	same trick swtab uses for its table.  _out is the application's
;	own symbol; the linker only pulls this member when the peephole
;	has planted a reference, so programs without an out() never see
;	it.
	psect	text
	global	oarg, _out

oarg:
	pop	hl		; -> the inline argument
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	push	hl		; the real return address
	ex	de,hl		; the argument, in hl
	jp	_out		; whose own ret goes straight back

; vim: tabstop=4 shiftwidth=4 noexpandtab:
