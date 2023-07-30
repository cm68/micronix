#
# Makefile for updating the source tree
#
# cmd/whitesmith/asm/GNUmakefile
#
# Changed: <2023-07-30 14:41:06 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH = ../../..
DIR = asm
CLASS = cmd/whitesmith
DOCSRCS = anat.mp dis80.mp as80.mp as80.ic
SRCS = $(wildcard *.c *.h)
TARGETS = as80 dis80 anat

as80: amain.o amake.o asm.o auxx.o
dis80: dis80.o
anat: tmain.o tmake.o trn.o tab.o

include $(DEPTH)/GNUmakefile.inc

