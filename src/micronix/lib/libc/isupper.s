	global	_isupper

	psect	text
_isupper:
	ld	a,h		;check for a char - the argument is in hl
	or	a
	jr	nz,nix
	ld	a,l
	cp	'A'
	jr	c,nix
	cp	'Z'+1
	jr	nc,nix
	ld	hl,1		;yes
	ret
nix:	ld	hl,0
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
