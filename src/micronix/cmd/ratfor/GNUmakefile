#
# cross build for ratfor
#
# cmd/ratfor/GNUmakefile
#
# ratfor is the Rational Fortran preprocessor - the v7 usr/src/cmd/ratfor,
# which turns the Ratfor in Software Tools into FORTRAN 66.  The parser
# y.tab.c is generated from r.g by yacc and committed, because the cross
# build has no yacc to run before this directory builds; r.g is the
# source of the source and y.tab.h the token table.  -Dunix sets
# CONTFLD 1 and the unix half of r.h.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= ratfor
CLASS	= cmd

include $(DEPTH)/GNUmakefile.inc

CCFLAGS += -Dunix

SRCS += r.g
