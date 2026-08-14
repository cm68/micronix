#
# Makefile for the whitesmith c preprocessor
#
# cmd/whitesmith/pp/GNUmakefile
#
# Changed: <2023-07-27 12:51:22 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

DEPTH = ../../..
DIR = pp
CLASS = cmd/whitesmith
TARGET = pp

include $(DEPTH)/GNUmakefile.inc

test: pp
	./pp -i ../include/ -d unix -o p0main.i p0main.c
