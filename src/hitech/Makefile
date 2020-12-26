#
# makefile to generate the binary patcher and to patch the hitech c
# compiler, assembler, linker and tools to micronix
#

HITECH = cpp p1 cgen optim link libr zas

CFLAGS = -Wno-implicit-int

all: $(HITECH)

bpatch: bpatch.c
	cc -g $(CFLAGS) -o $@ $<

%: %.com hitech.pat bpatch
	./bpatch $<

%.dis: %.COM hitech.pat bpatch
	./bpatch $<
	../disas $< >$@

install: $(HITECH)
	mkdir -p ../../filesystem/hitech
	cp $(HITECH) ../../filesystem/hitech
	
clobber:
	rm -f $(HITECH)

clean:
	rm -rf *.o bpatch objdump hitech *.dSYM *.dis *.sym asmtext.*
