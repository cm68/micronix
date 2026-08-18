#
# cross build for yacc and liby.a
#
# cmd/yacc/GNUmakefile
#
# yacc is the parser generator.  liby.a is its runtime, linked into the
# parser yacc emits, not into yacc itself - libmai.c holds the parser's
# main and libpar.c holds yyparse, so the wildcard would link them into
# yacc as well.  OBJS is spelled out and liby.a gets its own rules, the
# way cmd/diff does for diffh.
#
# dextern and files are the two headers the six y?.c translation units
# share - no .h suffix, so they are in SRCS by hand, with the parser
# templates (yaccopar, yaccepar, yaccrpar) and conv.py.  The templates
# are copied verbatim into the parser yacc generates, so they are built
# for ccc too; conv.py is the script that did the Whitesmiths-to-ccc
# conversion and stays for anyone who has to redo it.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= yacc
CLASS	= cmd

OBJS	= y0.o y1.o y2.o y3.o y4.o y5.o
TARGETS	= liby.a

include $(DEPTH)/GNUmakefile.inc

SRCS += dextern files yaccopar yaccepar yaccrpar macros conv.py

LIBOBJS	= libmai.o libpar.o libzac.o libzer.o libzin.o

liby.a: $(LIBOBJS)
	rm -f $@
	$(LIB) cr $@ $(LIBOBJS)

install: $(TARGET) $(TARGETS)
	$(RECURSE)
	mkdir -p $(INSTDIR)
	rm -f $(INSTDIR)/$(INSTNAME)
	cp $(TARGET) $(INSTDIR)/$(INSTNAME)
	mkdir -p $(LIBINSTDIR)
	rm -f $(LIBINSTDIR)/liby.a
	cp liby.a $(LIBINSTDIR)/liby.a
	mkdir -p $(ROOT)/usr/lib
	cp yaccopar yaccepar yaccrpar $(ROOT)/usr/lib/
	$(INSTALLMAN)

clean:
	$(RECURSE)
	rm -f $(OBJS) $(LIBOBJS) liby.a $(DOCFILES)
