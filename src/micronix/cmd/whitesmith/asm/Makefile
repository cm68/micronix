#
# native Makefile for a-natural assembler
#
# src/cmd/whitesmith/asm
#
# as80: the assembler handles 8080 assembler code with anat syntax
# dis80: disassembles Whitesmiths object code
# anat: translates anat syntax to strandard 8080 assembler code
#
# Changed: <2023-07-28 19:48:58 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

INCLUDE = -I/usr/src/cmd/whitesmith/include

all: anat as80 dis80

ASOBJS = amain.o amake.o asm.o auxx.o

as80: $(ASOBJS)
	cc $(INCLUDE) -o as80 $(ASOBJS) -lws

ANATOBJS = tab.o tmain.o tmake.o trn.o

anat: $(ANATOBJS)
	cc -o anat $(ANATOBJS) -lws

dis80: dis80.c
	cc $(INCLUDE) -o dis80 dis80.c -lws

$(ANATOBJS) $(ASOBJS) : $*.c
	cc $(INCLUDE) -c $*.c

clean:
	rm -f *.o

clobber: clean
	rm -f dis80 anat as80
