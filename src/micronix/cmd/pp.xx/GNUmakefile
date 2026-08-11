#
# cross build for pp, the whitesmith c preprocessor
#
# cmd/pp.xx/GNUmakefile
#
# The directory is pp.xx and the program is pp, so TARGET has to be
# said out loud.  cmd/GNUmakefile used to name this directory "pp" in
# DIRS, which is what sent the whole cmd build into a recursion.
#
# The sources are listed rather than globbed: dumplex.c is a debugging
# lexer dump that has never been linked into pp.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH	= ../..
DIR	= pp.xx
CLASS	= cmd
TARGET	= pp

CSRCS	= p0main.c p0eval.c p0lex.c p0subs.c

# not linked: a standalone dump of the lexer's token stream
NOTBUILT_C = dumplex.c

include $(DEPTH)/GNUmakefile.inc
