#
# cross build for make
#
# cmd/make/GNUmakefile
#
# make is the v7 make (the descendant of PWB make), ported to
# micronix.  gram.y is the yacc grammar and y.tab.c the parser yacc
# emits from it; y.tab.c is committed because the cross build has no
# yacc it can run before this directory builds, and it is compiled as
# y.tab.o.  defs.h is the shared header, renamed from v7's suffixless
# "defs" to this tree's .h convention; every .c pulls it in with
#  #include "defs.h".
#
# gram.y has no suffix the wildcard catches, so it is in SRCS by hand
# for update to ship it (defs.h is caught by the *.h wildcard).
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= make
CLASS	= cmd

OBJS	= main.o doname.o misc.o files.o dosys.o y.tab.o ident.o

include $(DEPTH)/GNUmakefile.inc

SRCS	+= gram.y
