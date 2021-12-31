#
# makefile to generate the binary patcher and to patch the hitech c
# compiler, assembler, linker and tools to micronix
# Changed: <2021-12-30 18:27:49 curt>
#

DEBUG = -v 0x71
VERBOSE = -v
HITECH = cpp p1 cgen optim link libr zas
HITECH = cpp p1
SIM = ../../src/usersim/sim -d ../../filesystem
PATCHFILE = havesrc.pat

CFLAGS = -Wno-implicit-int


all: bin $(HITECH)
	cp $(HITECH) bin

bin:
	mkdir -p bin

bpatch: bpatch.c
	cc -g $(CFLAGS) -o $@ $<

%: %.com bpatch $(PATCHFILE)
	./bpatch $(VERBOSE) -p $(PATCHFILE) $<
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
	rm -rf *.o bpatch objdump *.dSYM *.dis *.sym asmtext.* ctext.*

test: install
	$(SIM) $(DEBUG) /hitech/cpp -Dmicronix -Dz80 -I/include/ /usr/src/cmd/cc/cc.c /usr/src/cmd/cc/cc.i
	$(SIM) $(DEBUG) /hitech/p1 /usr/src/cmd/cc/cc.i /usr/src/cmd/cc/cc.1
	
objscan: objscan.c
	cc -g -o objscan objscan.c

p1: PATCHFILE = nosrc.pat
