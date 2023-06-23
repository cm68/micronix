#
# makefile to generate the binary patcher and to patch the hitech c
# compiler, assembler, linker and tools to micronix
#
# hitech/Makefile
#
# Changed: <2022-09-13 17:07:40 curt>

DEBUG = -v 0x71
VERBOSE = -v
HITECH = cpp p1 cgen optim link libr zas
HITECH = cpp p1
SIM = ../../src/usersim/sim -d ../../filesystem
PATCHFILE = havesrc.pat

CFLAGS = -Wno-implicit-int

DEST = ../../filesystem/hitech
INSTALL= $(DEST)/cpp $(DEST)/p1

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

$(DEST)/p1: p1
	cp $< $@

$(DEST)/cpp: cpp
	cp $< $@

$(DEST)/cgen: cgen
	cp $< $@

install: $(HITECH)
	mkdir -p $(DEST)
	cp $(HITECH) $(DEST)
	
clobber: clean
	rm -f $(HITECH)

clean:
	rm -rf *.o bpatch objdump *.dSYM *.dis *.sym asmtext.* ctext.*

testcpp: $(DEST)/cpp
	$(SIM) $(DEBUG) /hitech/cpp -Dmicronix -Dz80 -I/include/ /usr/src/cmd/cc/cc.c /usr/src/cmd/cc/cc.i

testp1: $(DEST)/p1
	$(SIM) $(DEBUG) /hitech/p1 /usr/src/cmd/cc/cc.i /usr/src/cmd/cc/cc.1

testcgen: $(DEST)/cgen
	$(SIM) $(DEBUG) /hitech/cgen /usr/src/cmd/cc/cc.1 /usr/src/cmd/cc/cc.2

test: testcpp testp1 testcgen
	
objscan: objscan.c
	cc -g -o objscan objscan.c

p1: PATCHFILE = nosrc.pat
