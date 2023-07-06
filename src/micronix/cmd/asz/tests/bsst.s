.globl	bss_seg, data_seg

.text
	ld	hl,data_seg
	ld	hl,bss_seg
	ret
	nop
.bss
	.defl byte[0x20] bss_seg
.data
	.defl byte[0x20] data_seg
