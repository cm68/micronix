.globl	bss_seg, data_seg

.text
	ld	hl,data_seg
	ld	hl,bss_seg
	ret
	nop

.data
data_seg:
	.db "foobie", 45h
	.dw	4
	.ds 30
.bss
bss_seg: .ds 20
