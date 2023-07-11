#
# populate our new, improved versions into /usr/include
#
# we keep the old whitesmith's includes in /include
#
# /include/Makefile
#
# Changed: <2023-07-10 14:37:49 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

all:

upgrade:
	mkdir /usr/include /usr/include/sys
	cp /include/*.h /usr/include
	cp *.h /usr/include
	cd sys ; make install

install: upgrade
