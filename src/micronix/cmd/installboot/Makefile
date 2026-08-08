#
# makefile for installboot
#
# micronix/cmd/installboot/Makefile
#

all: installboot

installboot: installboot.c
	cc -I/usr/include -o installboot installboot.c -lS

install: installboot
	cp installboot /bin/installboot

upgrade: installboot
	cp /bin/installboot /old/installboot
	cp installboot /bin/installboot

clean:
	rm installboot *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
