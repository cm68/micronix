.globl	bss_seg, data_seg

	.text
	ld	hl,data_seg
	ld	hl,bss_seg
	ret
	nop

	.bss
bss_seg:
	.ds	20

	.data
data_seg:
	.ds	20
