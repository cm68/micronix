#
# cross build for the internet family headers
#
# include/netinet/GNUmakefile
#
# <netinet/in.h> and what it carries.  The address structures are in
# sys/socket.h beside the calls; this directory exists so a source
# written against the BSD includes finds what it expects, and so the
# native build's /usr/include has it.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

# The tree's own filesystem - see GNUmakefile.inc.  Not a home
# directory: CURDIR here is src/micronix/include/netinet, so
# ../../../.. is the top.
#
ROOT	?= $(abspath $(CURDIR)/../../../../filesystem)
DEST	 = $(ROOT)/usr/src/include/netinet
INCDIR	 = $(ROOT)/usr/include/netinet
NOWARN	 = --warning=no-ignore-newer

HDRS	   = in.h
MKFILE	 = $(wildcard ?akefile)

.PHONY: all update install install-man install-man-host clean clobber doc

all:

netinet.tar: $(HDRS) $(MKFILE)
	tar -f netinet.tar -c $(MKFILE) $(HDRS)

update: netinet.tar
	mkdir -p $(DEST)
	tar -f netinet.tar -C $(DEST) --keep-newer-files -x $(NOWARN)

install:
	mkdir -p $(INCDIR)
	cp $(HDRS) $(INCDIR)

install-man:

install-man-host:

clean:

clobber: clean
	rm -f netinet.tar

doc:
