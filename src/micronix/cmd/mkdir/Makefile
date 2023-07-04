#
# makefile for mkdir
#
# micronix/cmd/mkdir/Makefile
#
# Changed: <2023-07-01 07:02:14 curt>
 
all: mkdir

mkdir: mkdir.c
	cc -o mkdir mkdir.c -lS

install: mkdir
	cp mkdir /bin/mkdir

upgrade: mkdir
	cp /bin/mkdir /old/mkdir
	cp mkdir /bin/mkdir

clean:
	rm mkdir *.o

# vim: tabstop=4 shiftwidth=4 noexpandtab:
