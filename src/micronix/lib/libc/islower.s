	global	_islower

	psect	text
_islower:
	.abi	_islower,3	;arg8/ret8: c in A, the answer in A
	cp	'a'
	jr	c,nix
	cp	'z'+1
	jr	nc,nix
	ld	a,1		;yes
	ret
nix:	xor	a		;no
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
