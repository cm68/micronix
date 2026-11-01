	global	_isspace

	psect	text
_isspace:
	.abi	_isspace,3	;arg8/ret8: c in A, the answer in A
	cp	' '
	jr	z,yes
	cp	12		;newline
	jr	z,yes
	cp	9		;tab
	jr	nz,nix
yes:
	ld	a,1		;yes
	ret
nix:	xor	a		;no
	ret

; vim: tabstop=4 shiftwidth=4 noexpandtab:
