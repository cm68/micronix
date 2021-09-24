#
# makefile to generate the binary patcher and to patch the hitech c
# compiler, assembler, linker and tools to micronix
#

DEBUG = -v 0x71
VERBOSE = -v
HITECH = cpp p1 cgen optim link libr zas
HITECH = cpp p1

CFLAGS = -Wno-implicit-int

all: $(HITECH) bin
	cp $(HITECH) bin

bin:
	mkdir bin

bpatch: bpatch.c
	cc -g $(CFLAGS) -o $@ $<

%: %.com hitech.pat bpatch
	./bpatch $(VERBOSE) -p hitech.pat $<
	cp $* ../../filesystem/hitech

%.dis: %.com hitech.pat bpatch
	./bpatch -p hitech.pat $<
	../tools/disas $< >$<.dis
	../tools/disas $* >$*.dis 

install: $(HITECH)
	mkdir -p ../../filesystem/hitech
	cp $(HITECH) ../../filesystem/hitech
	
clobber: clean
	rm -f $(HITECH)

clean:
	rm -rf *.o bpatch objdump *.dSYM *.dis *.sym asmtext.*

test: install
	../../sim -d ../../filesystem $(DEBUG) /hitech/cpp -Dmicronix -Dz80 -I/include/ /usr/src/cmd/cc/cc.c /usr/src/cmd/cc/cc.i
	../../sim -d ../../filesystem $(DEBUG) /hitech/p1 /usr/src/cmd/cc/cc.i /usr/src/cmd/cc/cc.1
	
objscan: objscan.c
	cc -g -o objscan objscan.c
