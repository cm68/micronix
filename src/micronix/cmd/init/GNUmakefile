#
# cross build for init
#
# cmd/init/GNUmakefile
#
# Everything is in GNUmakefile.inc: ccc from /usr/local, every .c in
# this directory, linked into init.
#
# init.ctl and init.dist are data, not sources, and are not shipped by
# the update rule - which is what was true before this was converted.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= init
CLASS	= cmd

# process 1 lives in /etc, not /bin
BINDIR	= /etc

#
# TARGETS is the hook for a second binary out of the same sources: "all"
# makes it and "clobber" removes it, but "install" copies $(TARGET) alone,
# so init.dbg is built every time and shipped never.  It has to be set
# before the include - "all" names it as a prerequisite, and prerequisites
# are expanded where the rule is read.
#
TARGETS	= init.dbg

include $(DEPTH)/GNUmakefile.inc

#
# init.dbg - the same init with the console left on descriptor 2.
#
# init proper cannot report anything.  closeall() closes every descriptor
# it inherited, /dev/root and /dev/swap then take 0 and 1, and nothing ever
# opens 2 - which is the one perror() writes to.  So the parent runs blind:
# a fork that fails, an /etc/ttys that will not open, a root device that is
# not there all return quietly.  -DINITDBG opens the console on 2 before
# closeall() runs and reports those.  The INITDBG comment at the top of
# init.c has the detail.
#
# It is built by "all" so that it cannot rot - an #ifdef nothing compiles
# stops compiling - and installed by nothing.  It does not go into the
# filesystem; mnix it into an image by hand when it is wanted.
#
# The objects are .dbg.o rather than .o so that they cannot be linked into
# the real init by accident.  GNU make prefers the %.dbg.o rule over the
# generic %.o for these because its stem is shorter.
#
DBGOBJS	 = $(CSRCS:%.c=%.dbg.o)
GENERATED = $(DBGOBJS)

%.dbg.o : %.c
	$(CCC) $(CCFLAGS) -DINITDBG $(INC) -c $< -o $@

init.dbg: $(DBGOBJS)
	$(CCC) $(CCFLAGS) -o $@ $(DBGOBJS) $(LIBS) $(TREELIBS)
