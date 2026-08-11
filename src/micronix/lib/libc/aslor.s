	psect	text
	global	iregset, iregstore, asalor, asllor, alor

asalor:
asllor:
	call	iregset
	call	alor
	jp	iregstore

; vim: tabstop=4 shiftwidth=4 noexpandtab:
