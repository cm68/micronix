all:	cbios.prn

cbios.asm:	CBIOS.A CBIOS.B
	cat CBIOS.A CBIOS.B | tnylpo-convert -z -u - -c cbios.asm 

cbios.prn:	cbios.asm
	tnylpo /usr/local/lib/cpm/bin80/mac cbios
