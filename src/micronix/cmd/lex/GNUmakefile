#
# cross build for lex and liblc.a
#
# cmd/lex/GNUmakefile
#
# lex is the scanner generator.  liblc.a is the runtime linked into the
# scanner lex emits, not into lex itself - libmai.c holds the scanner's
# main and the other lib*.c hold the scanner's library routines, so the
# wildcard would link them into lex as well.  OBJS is spelled out and
# liblc.a gets its own rules, the way cmd/yacc does for liby.a.
#
# ldefs.c and once.c are shared by main.c, header.c, sub1.c, sub2.c and
# y.tab.c through #include, not compiled on their own, so they are not
# in OBJS; they ride in SRCS so update ships them.  y.tab.c is generated
# from parser.y by yacc - it is committed because the cross build has no
# yacc to run before this directory builds.  ncform is the driver
# template lex copies verbatim into the scanner it emits, installed at
# /usr/lib/lex/ncform where lex looks for it (see once.c).
#
# lex links ../yacc/liby.a for yyerror - the archive member is pulled
# only because it resolves an undefined symbol, so the yyparse and main
# that liby.a also carries do not collide with lex's own.
#
# -Dunix is what turns ldefs.c's ASCII tables on and selects the
# /usr/lib/lex/ncform path in once.c; without it neither is defined.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= lex
CLASS	= cmd

OBJS	= main.o header.o sub1.o sub2.o y.tab.o
TARGETS	= liblc.a
LIBS	= ../yacc/liby.a

include $(DEPTH)/GNUmakefile.inc

CCFLAGS += -Dunix

SRCS += parser.y ncform

LIBOBJS	= libmai.o allprint.o yyless.o yywrap.o ereject.o

liblc.a: $(LIBOBJS)
	rm -f $@
	$(LIB) cr $@ $(LIBOBJS)

install: $(TARGET) $(TARGETS)
	$(RECURSE)
	mkdir -p $(INSTDIR)
	rm -f $(INSTDIR)/$(INSTNAME)
	cp $(TARGET) $(INSTDIR)/$(INSTNAME)
	mkdir -p $(LIBINSTDIR)
	rm -f $(LIBINSTDIR)/liblc.a
	cp liblc.a $(LIBINSTDIR)/liblc.a
	mkdir -p $(ROOT)/usr/lib/lex
	cp ncform $(ROOT)/usr/lib/lex/ncform
	$(INSTALLMAN)

clean:
	$(RECURSE)
	rm -f $(OBJS) $(LIBOBJS) liblc.a $(DOCFILES)
