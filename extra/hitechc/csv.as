; this fragment is called to make a stack frame
; generally, this is the function prolog that happens
; right after a C function is called with stacked args

; here's what the stack looks like when we enter

;        arg1
;        arg0
;        caller's caller return address
; sp->   return to function head

;
; and when we exit, via the jp (hl), the stack and registers looks like this
;
;	  arg1
;         arg0
;         return
;         iy
; ix,sp-> ix
;
; so, arg0 is (ix+6) and (ix+7)
;
	global	csv,cret,indir, ncsv
	psect	text
csv:	pop	hl		;return address
	push	iy
	push	ix
	ld	ix,0
	add	ix,sp		;new frame pointer
	jp	(hl)

cret:	ld	sp,ix
	pop	ix
	pop	iy
	ret

indir:	jp	(hl)

;	New csv: allocates space for stack based on word following
;	call ncsv

ncsv:
	pop	hl
	push	iy
	push	ix
	ld	ix,0
	add	ix,sp
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	ex	de,hl
	add	hl,sp
	ld	sp,hl
	ex	de,hl
	jp	(hl)
