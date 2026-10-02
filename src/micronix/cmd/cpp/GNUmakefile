#
# cross build for cpp
#
# cmd/cpp/GNUmakefile
#
# cpp is the STANDALONE C preprocessor - Reiser's v7 /lib/cpp, a
# filter from standard input to standard output - not a pass of this
# tree's compiler.  The compiler's front end is libexec/pass0, which
# ccc runs under the name pass0; this program installs as /bin/cpp.
# The two programs only share a name, and that name is pass0's
# misnomer, explained in cmd/TODO.  See the README beside this file.
#
# cpy.c is the hand-written #if parser (its header explains why); cpy.y
# is the original yacc grammar, shipped for reference, and yylex.c is
# #included by cpy.c rather than compiled on its own - so OBJS names
# cpp.o and cpy.o and not yylex.o.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= cpp
CLASS	= cmd

OBJS	= cpp.o cpy.o

include $(DEPTH)/GNUmakefile.inc

SRCS += cpy.y README

# yylex.c is pulled into cpy.o by cpy.c's #include, so a change to it
# has to rebuild cpy.o - the wildcard OBJS override above keeps make
# from also compiling yylex.c as a unit of its own.
cpy.o: yylex.c
