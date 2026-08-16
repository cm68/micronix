	psect	text
	global	iregset, iregstore, asalsub, asllsub, alsub

asalsub:
asllsub:
	call	iregset
	call	alsub
	jp	iregstore

; vim: tabstop=4 shiftwidth=4 noexpandtab:
