	psect	text
	global	iregset, iregstore, asaladd, aslladd, aladd

asaladd:
aslladd:
	call	iregset
	call	aladd
	jp	iregstore

; vim: tabstop=4 shiftwidth=4 noexpandtab:
