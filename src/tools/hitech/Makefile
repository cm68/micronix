# build hitech c object file tools
#
# tools/hitech/Makefile
# Changed: <2021-12-23 16:22:24 curt>

CFLAGS = -Wno-implicit-int

%: %.c
	cc -g $(CFLAGS) -o $@ $<

all: bpatch

bpatch: bpatch.c

#	cc -g -o bpatch bpatch.c

objdump: objdump.c
	cc -g -o objdump objdump.c

%: %.COM hitech.pat bpatch
	./bpatch $<

#%.COM: %.c
#	zxc $< -lc
#	mv $*.com $*.COM

%.dis: %.COM hitech.pat bpatch
	./bpatch $<
	../disas $< >$@

%.COM: %.c
	zxcc CPP.COM --DCPM --DHI_TECH_C --Dz80 --Ic: $< $*.i
	zxcc P1.COM $*.i CTMP2
	zxcc CGEN.COM CTMP2 CTMP3
	zxcc OPTIM.COM CTMP3 CTMP4
	zxcc ZAS.COM --N --$.obj CTMP4
	zxcc LINK.COM --Z --Ptext=0,data,bss --C100H --O$*.COM CRTCPM.OBJ $*.OBJ LIBc.LIB LIBC.LIB

clean:
	rm -rf *.o bpatch objdump hitech *.dSYM *.dis *.sym
