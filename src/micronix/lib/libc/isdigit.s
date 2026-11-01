	global	_isdigit, _isdig

	psect	text
_isdigit:
_isdig:
	.abi	_isdigit,3	;arg8/ret8: c in A, the answer in A
	.abi	_isdig,3
	cp	'0'
	jr	c,nix
	cp	'9'+1
	jr	nc,nix
	ld	a,1		;yes
	ret
nix:	xor	a		;no
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
