#
# The host build of a compiler pass.
#
# libexec/host.mk
#
# The passes are built twice from the one set of sources.  The
# framework beside this builds them WITH ccc, FOR the Z80, which is
# the compiler that runs on micronix.  This builds them with the
# machine's own cc, to run here, which is the cross compiler
# everything else in the tree is built by - including the Z80 build
# of these same sources.
#
# That circle is why this file exists.  The sources used to live in
# ~/src/ccc and were reached from here by symlink; the host build was
# over there and this tree only ever cross built.  With the sources
# forked into this tree there is nowhere else for the host build to
# be, and a tree that cannot build its own compiler cannot be checked
# out and used.
#
# What a directory has to say for itself:
#
#	HOSTPROG   what the host binary is called when installed -
#		   pass0, c0, c1, peep, asz, mxld, astpp
#	HOSTSRCS   the .c files, if the wildcard is wrong (c1 has to
#		   spell them: mkruleidx.c is a host program of its
#		   own and does not belong in the pass)
#	HOSTBIN    bin instead of libexec, for a program a person runs
#
# and then "include $(DEPTH)/libexec/host.mk".
#
# vim: tabstop=8 shiftwidth=8 noexpandtab:
#

#
# -m32 because the compiler's own pointers are the width its ints
# are, and it is checked against the Z80 build for identical output;
# a 64-bit build of these sources is a different program.
#
# gnu89 because that is the C these sources are written in - they
# have to compile with ccc as well, which has no later dialect in it.
#
# -I../../lib/libccc for fmtlong and libutil, which every pass calls,
# and -I../cpp for lexeme.h: the token numbers cpp emits are the ones
# the passes consume, and a compiler that must also build on CP/M
# cannot spell a directory in an #include.
#
HOSTCC		?= gcc
HOSTDEBUG	?= -ggdb3 -O0
HOSTDEFS	?= -DDEBUG
HOSTWARNS	?= -Wall -std=gnu89 -Werror=declaration-after-statement
HOSTCFLAGS	?= -m32 $(HOSTDEBUG) $(HOSTDEFS) $(HOSTWARNS) \
		   -I. -I$(DEPTH)/lib/libccc -I$(DEPTH)/libexec/cpp
HOSTLDFLAGS	?= -m32 $(HOSTDEBUG)

#
# Where the host compiler lands: the top of the tree, in bin/ for the
# driver and libexec/ for the passes.  Not /usr/local, and not beside
# the Z80 binary either, which has the same name and would be
# overwritten by whichever build ran last.  The driver works out where
# everything is from its own argv[0] - bin/../libexec for the passes,
# bin/../lib for the runtime - so the tree moves anywhere intact.
#
HOSTDIR		?= $(DEPTH)/../..
HOSTBIN		?= libexec
HOSTINSTDIR	= $(HOSTDIR)/$(HOSTBIN)

HOSTSRCS	?= $(CSRCS)
HOSTOBJS	= $(HOSTSRCS:.c=.ho)
HOSTLIBCCC	= $(DEPTH)/lib/libccc/libccc-host.a

#
# .ho and not .o: the Z80 object of the same source is .o and the two
# are not interchangeable, and a build that mixed them would link and
# then behave in ways that take an afternoon to explain.
#
%.ho: %.c
	$(HOSTCC) $(HOSTCFLAGS) -c $< -o $@

#
# dbgtags.c and debug.h, where a pass has the script that writes them.
#
# They are the -DDEBUG tracing: the name of every op and every verbose
# bit, which is what -v prints and what turns a "no rule" refusal into
# a tree you can read.  Generated and not checked in, because they are
# a transcription of the sources beside them and drift the moment an
# op is added.
#
DBGSCRIPT := $(wildcard makedebug.sh)
ifneq ($(DBGSCRIPT),)
DBGFILES = dbgtags.c debug.h

dbgtags.c debug.h: makedebug.sh $(HOSTSRCS)
	./makedebug.sh

$(HOSTOBJS): $(DBGFILES)
endif

.PHONY: host hostinstall hostclean

host: $(HOSTINSTDIR)/$(HOSTPROG)

$(HOSTINSTDIR)/$(HOSTPROG): $(HOSTOBJS) $(HOSTLIBCCC)
	@mkdir -p $(HOSTINSTDIR)
	$(HOSTCC) $(HOSTLDFLAGS) -o $@ $(HOSTOBJS) $(HOSTLIBCCC)

$(HOSTLIBCCC):
	$(MAKE) -C $(DEPTH)/lib/libccc libccc-host.a

hostinstall: host

hostclean:
	rm -f $(HOSTOBJS) $(DBGFILES) $(HOSTINSTDIR)/$(HOSTPROG)
