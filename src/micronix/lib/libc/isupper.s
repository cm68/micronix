	global	_isupper

	psect	text
_isupper:
	.abi	_isupper,3	;arg8/ret8: c in A, the answer in A
	cp	'A'
	jr	c,nix
	cp	'Z'+1
	jr	nc,nix
	ld	a,1		;yes
	ret
nix:	xor	a		;no
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
