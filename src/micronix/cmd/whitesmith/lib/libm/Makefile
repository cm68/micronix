#
# native Makefile for whitesmiths machine interface library
#
# src/cmd/whitesmith/lib/libm/Makefile
#
# Changed: <2023-07-28 19:08:17 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

LIB = libm.a

all: $(LIB)

install: $(LIB)
	cp $(LIB) /lib

SOBJS1 = \
	btou.o cffun.o count.o dadd.o dcmp.o ddiv.o dmul.o \
	dneg.o dsub.o dtd.o dtf.o dtl.o ent.o ents.o entx.o ftd.o idiv.o \
	ilsh.o imul.o in.o ladd.o land.o lclt.o lcmp.o lcom.o lcpy.o ldiv.o
SOBJS2 = \
	llsh.o lmul.o lneg.o lor.o lsub.o ltd.o lxor.o out.o raise.o \
	repk.o switch.o unpk.o utob.o zret.o

COBJS = \
	cfcan.o cfcon.o memerr.o stop.o

LIBOBJS = $(SOBJS1) $(SOBJS2) $(COBJS)

$(LIB): $(LIBOBJS) Makefile
	rm $(LIB) objs
	obj $(SOBJS1) > objs
	obj $(SOBJS2) >> objs
	obj $(COBJS) >> objs
	lord -s > objlist < objs
	/bin/echo 1,-+s/%/lib $(LIB) -r / > editscript
	/bin/echo w >> editscript
	/bin/echo q >> editscript
	/bin/echo "cat editscript | edit objlist" > makelib.sh
	/bin/echo "sh -v objlist" >> makelib.sh
	sh -v makelib.sh
	rm -f objs editscript makelib.sh objlist


$(SOBJS1) $(SOBJS2) : $*.s
	as -o $*.o $*.s

$(COBJS) : $*.c
	cc -c $*.c

clean:
	rm -f *.o objs editscript makelib.sh objlist

clobber: clean
	rm -f $(LIB)
