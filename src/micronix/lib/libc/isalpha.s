	global	_isalpha

	psect	text
_isalpha:
	.abi	_isalpha,3	;arg8/ret8: c in A, the answer in A
	cp	'A'
	jr	c,nix
	cp	'z'+1
	jr	nc,nix
	cp	'a'
	cp	'Z'+1
	jr	c,yes
	cp	'a'
	jr	c,nix
yes:
	ld	a,1		;yes
	ret
nix:	xor	a		;no
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
