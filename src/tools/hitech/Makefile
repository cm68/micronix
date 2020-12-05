all: bpatch

bpatch: bpatch.c
	cc -g -o bpatch bpatch.c

objdump: objdump.c
	cc -g -o objdump objdump.c

%: %.COM hitech.pat bpatch
	./bpatch $<

%.COM: %.c
	zxc $< -lc
	mv $*.com $*.COM

%.dis: %.COM hitech.pat bpatch
	./bpatch $<
	../disas $< >$@

clean:
	rm -rf *.o bpatch objdump hitech *.dSYM *.dis *.sym
