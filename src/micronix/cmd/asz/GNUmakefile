#
# cross Makefile for asz assemble
#
# usr/src/cmd/asz/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CLASS = cmd
DIR = asz
CFILES = main.c asm.c
HFILES = isr.h asm.h
SRCS = $(CFILES) $(HFILES)
TARGET = asz
GENERATED = 

include ../GNUmakefile.inc

cross: $(CFILES) $(HFILES)
	cc -fdiagnostics-color=always -g -o asz $(CFILES) 2>&1

test: cross
	cd tests ; make test
