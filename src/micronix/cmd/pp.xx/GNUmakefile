#
# cross Makefile for pp preprocessor
#
# usr/src/cmd/asz/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

CLASS = cmd
DIR = pp
CFILES = p0eval.c p0lex.c p0main.c p0subs.c
HFILES = int012.h int01.h int0.h
SRCS = $(CFILES) $(HFILES)

include ../GNUmakefile.inc

TARGET = pp

cross: $(CFILES) $(HFILES)
	cc -o pp $(CFILES)
