.globl	bss_seg, data_seg, text_seg

	.text
	ld	hl,text_seg
	ld	hl,data_seg
text_seg:
	ld	hl,bss_seg
	ret
	nop

	.bss
bss_seg:
	.ds	20

	.data
data_seg:
	.ds	20
