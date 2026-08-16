	psect	text
	global	iregset, iregstore, asaland, aslland, aland

asaland:
aslland:
	call	iregset
	call	aland
	jp	iregstore

; vim: tabstop=4 shiftwidth=4 noexpandtab:
