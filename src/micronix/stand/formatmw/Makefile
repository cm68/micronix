#
# micronix native standalone
#
# micronix/stand/formatmw/Makefile
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#

#
# Morrow's hard disc formatter, version 1.7 with the 1.8 proto drive
# table, ported off the Whitesmiths run time library - the WSLIB ifdefs
# in formatmw.c select the original calls back if that library returns.
#
# A CP/M program, not a micronix one, and it cannot become one: it hands
# the controller physical dma addresses, writes the channel address to
# physical 0x50, and issues out instructions.  None of that survives the
# MPZ80's map.  It runs under CP/M on the d1 simulation, which is where
# the format command's sector id table can be read off the wire instead
# of guessed at.
#
# Deliberately not in all: c1 dies on this file today, so a default
# build would stop here.  See ~/src/ccc/ARRAY_ERROR.
#
CCC = ccc

formatmw.com: formatmw.c
	$(CCC) -m cpm -o formatmw.com formatmw.c
