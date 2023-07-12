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
# Changed: <2023-07-10 19:24:41 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all: hex lib lord rel

hex: hex.c
	cc -o hex hex.c -lws -lws

lib: lib.c
	cc -o lib lib.c -lws -lws

lord: lord.c
	cc -o lord lord.c -lws -lws

rel: rel.c
	cc -o rel rel.c -lws -lws

clean:
	rm -f *.o

clobber: clean
	rm -f hex lib lord rel
