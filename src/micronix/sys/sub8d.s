;
; Data for sub8.s's disable/enable leaf code.  dicount is a mutable
; counter, so it stays in ordinary kernel data and NOT the u page (the
; u page is cloned per process by fork's segcopy).  badmsg is the
; "di < 0" message that _enable/_disable pass to _pr.
;
	.globl	dicount, badmsg
	.data			; a counter and a string, not code

dicount:
	.defb	0
badmsg:
	.defb	"di < 0", 0
