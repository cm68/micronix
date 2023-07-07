.globl	bss_seg, data_seg

.text
	ld	hl,data_seg
	ld	hl,bss_seg
	ret
	nop

.data
data_seg:
	.defw 2
	.defb 1,3,5

.bss
bss_seg: .ds 20
