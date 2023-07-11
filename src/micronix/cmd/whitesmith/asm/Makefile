#
# native Makefile for a-natural assembler
#
# src/cmd/whitesmith/asm
#
# as80: the assembler handles 8080 assembler code with anat syntax
# dis80: disassembles Whitesmiths object code
# anat: translates anat syntax to strandard 8080 assembler code
#
# Changed: <2023-07-10 17:22:17 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all: anat as80 dis80

ASOBJS = amain.o amake.o asm.o auxx.o

as80: $(ASOBJS)
	cc -o as80 $(ASOBJS) -lws

ANATOBJS = tab.o tmain.o tmake.o trn.o

anat: $(ANATOBJS)
	cc -o anat $(ANATOBJS) -lws

dis80: dis80.c
	cc -o dis80 dis80.c -lws

$(ANATOBJS) $(ASOBJS) : $*.c
	cc -c $*.c

clean:
	rm -f *.o

clobber: clean
	rm -f dis80 anat as80
