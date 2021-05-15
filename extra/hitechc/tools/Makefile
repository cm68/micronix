all: objdump hello.com

objdump: objdump.c
	cc -g -o objdump objdump.c

hello.com:  hello.c
	zxcc CPP --DCPM --DHI_TECH_C --Dz80 --Ic: --I HELLO.C hello.cpp
	zxcc P1 hello.cpp hello.tre hello.tmp
	zxcc CGEN hello.tre hello.s
	zxcc OPTIM hello.s hello.os
	zxcc ZAS --N --ohello.obj hello.s
	zxcc ZAS --N --ohelloo.obj hello.os
	zxcc LINK --Z --Ptext=0,data,bss --C100H --OHELLO.COM CRTCPM.OBJ HELLO.OBJ LIBC.LIB
