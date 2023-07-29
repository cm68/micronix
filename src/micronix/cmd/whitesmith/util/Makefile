#
# native Makefile for whitesmith utils
#
# src/cmd/whitesmith/util/Makefile
#
# hex
# lib
# lord
# rel
#
# Changed: <2023-07-28 19:51:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

INCLUDE = -I/usr/src/cmd/whitesmith/include

all: hex lib lord rel

hex: hex.c
	cc $(INCLUDE) -o hex hex.c -lWS

lib: lib.c
	cc $(INCLUDE) -o lib lib.c -lWS -lWS ../lib/libws/itob.o

lord: lord.c
	cc $(INCLUDE) -o lord lord.c -lWS

rel: rel.c
	cc $(INCLUDE) -o rel rel.c -lWS

clean:
	rm -f *.o

clobber: clean
	rm -f hex lib lord rel
