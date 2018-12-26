	global	_test
	psect	text
test:	jp test
	ld a,(foo)

	psect foobie
foo:  defb 99
