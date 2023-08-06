#
# Makefile for  rebuilding cbios
#
# <path>
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:	cbios.prn

cbios.asm:	CBIOS.A CBIOS.B
	cat CBIOS.A CBIOS.B | tnylpo-convert -z -u - -c cbios.asm 

cbios.prn:	cbios.asm
	tnylpo /usr/local/lib/cpm/bin80/mac cbios
