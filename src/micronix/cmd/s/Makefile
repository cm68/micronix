#
# makefile for s
#
# micronix/cmd/s/Makefile
#
# Changed: <2023-06-23 01:07:31 curt>
#
# S is too big to build with whitesmith's c, because the code quality
# is just not good enough.
#

OBJEXT = o
#OBJEXT = obj

OBJECTS = address.$(OBJEXT) adjust.$(OBJEXT) bman.$(OBJEXT) \
	buffer.$(OBJEXT) commands.$(OBJEXT) keyboard.$(OBJEXT) \
	lib.$(OBJEXT) operator.$(OBJEXT) s.$(OBJEXT) screen.$(OBJEXT) \
	sman.$(OBJEXT) yank.$(OBJEXT)

s: $(OBJECTS)
	cc -v -o s $(OBJECTS) -lS

$(OBJECTS) : $*.c
	cc -c $*.c

#s: $(OBJECTS)
#	zxcc -o s $(OBJECTS)

#%.obj : %.c
#	zxc -c $<

install: s
	cp s /bin/s

clean : ; rm *.$(OBJEXT)

update :  ; cd ../.. ; make update

# vim: tabstop=4 shiftwidth=4 noexpandtab:
