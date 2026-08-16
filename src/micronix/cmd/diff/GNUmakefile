#
# cross build for diff and diffh
#
# cmd/diff/GNUmakefile
#
# diffh is the halfhearted diff that diff's -h execs.  It is a second
# program built in this directory and is not part of diff, so OBJS is
# spelled out - the wildcard would link diffh.o into diff - and diffh
# gets its own rules.  The framework's all and clobber already know
# TARGETS; install and clean are redefined below to carry diffh too.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= diff
CLASS	= cmd

OBJS	= diff.o diffdir.o diffreg.o
TARGETS	= diffh

include $(DEPTH)/GNUmakefile.inc

diffh: diffh.o
	$(CCC) $(CCFLAGS) -o $@ diffh.o $(LIBS) $(TREELIBS)

install: $(TARGET) $(TARGETS)
	$(RECURSE)
	mkdir -p $(INSTDIR)
	rm -f $(INSTDIR)/$(INSTNAME)
	cp $(TARGET) $(INSTDIR)/$(INSTNAME)
	rm -f $(INSTDIR)/diffh
	cp diffh $(INSTDIR)/diffh
	$(INSTALLMAN)

clean:
	$(RECURSE)
	rm -f $(OBJS) diffh.o $(DOCFILES)
