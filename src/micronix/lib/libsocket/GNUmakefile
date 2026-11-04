#
# libsocket - BSD sockets over the network card
#
# lib/libsocket/GNUmakefile
#
# THE CROSS BUILD.  The compiler, its flags and its include path come
# from GNUmakefile.inc at the top of the tree; this file says only what
# is particular to libsocket.
#
# The library is not part of libc on purpose: it is the BSD face over
# one device, it is only linked by the programs that use sockets, and
# everything it can keep out of the kernel it keeps here.
#
# vim: tabstop=4 shiftwidth=4 expandtab:
#

DEPTH	= ../..
DIR	= libsocket
CLASS	= lib
TARGET	= libsocket.a

MANSEC	 = 3
MANSRCS	= socket.3

CSRCS	= socket.c services.c

include $(DEPTH)/GNUmakefile.inc
