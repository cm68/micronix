#
# cross build for expr
#
# cmd/expr/GNUmakefile
#
# The PWB expression evaluator, ported from
# extra/pwb/spencer/sys/source/s1/expr.y.  expr.y is the yacc grammar
# and y.tab.c the parser yacc emits from it; y.tab.c is committed
# because the cross build has no yacc to run before this directory
# builds, and it is compiled as y.tab.o.  expr links ../yacc/liby.a
# for yyerror - the archive member is pulled only because it resolves
# an undefined symbol, so the yyparse and main that liby.a also
# carries do not collide with expr's own.
#
# expr.y has no suffix the wildcard catches, so it is in SRCS by hand
# for update to ship it.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= expr
CLASS	= cmd

OBJS	= y.tab.o
LIBS	= ../yacc/liby.a

include $(DEPTH)/GNUmakefile.inc

SRCS	+= expr.y
