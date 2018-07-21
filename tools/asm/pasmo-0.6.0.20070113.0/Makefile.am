# Makefile.am for Pasmo

srcdir = @srcdir@
VPATH = @srcdir@

bin_PROGRAMS = pasmo

pasmo_SOURCES = \
	asm.h \
	asm.cpp \
	asmfile.h \
	asmfile.cpp \
	asmimpl.h \
	asmimpl.cpp \
	asmimplparse.cpp \
	asmreal.h \
	asmreal.cpp \
	codeaux.h \
	codeaux.cpp \
	cpc.h \
	cpc.cpp \
	local.h \
	local.cpp \
	machine.h \
	machine.cpp \
	macro.h \
	macro.cpp \
	macroframe.h \
	macroframe.cpp \
	module.h \
	module.cpp \
	nullstream.h \
	nullstream.cpp \
	parser.yxx \
	parseraux.h \
	parseraux.cpp \
	parsertypes.h \
	pasmo.cpp \
	pasmoimpl.h \
	pasmotypes.h \
	pasmotypes.cpp \
	relfile.h \
	relfile.cpp \
	segment.h \
	segment.cpp \
	simpleinst.h \
	simpleinst.cpp \
	spectrum.h \
	spectrum.cpp \
	tap.h \
	tap.cpp \
	token.h \
	token.cpp \
	trace.h \
	trace.cpp \
	tzx.h \
	tzx.cpp \
	value.h \
	value.cpp \
	var.h \
	var.cpp

BUILT_SOURCES = parser.h parser.cxx

AM_YFLAGS = -d

tgz: dist
	cp $(distdir).tar.gz $(distdir).tgz

rpm: dist
	rpm -ta $(distdir).tar.gz

#*************************************************

EXTRA_DIST = \
	bootstrap autogen.sh \
	do_conf \
	do_confnodeb \
	do_conf0 \
	do_confmingw \
	pasmodoc.html


ACLOCAL_AMFLAGS = -I .

# End of Makefile.am for Pasmo
