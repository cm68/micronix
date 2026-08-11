	psect	text
	global	iregset, iregstore, asalxor, asllxor, alxor

asalxor:
asllxor:
	call	iregset
	call	alxor
	jp	iregstore

; vim: tabstop=4 shiftwidth=4 noexpandtab:
