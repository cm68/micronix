all:	rm

rm : rm.c
	cc -o rm rm.c -lS

install: rm
	cp rm /bin/rm

upgrade: rm
	cp /bin/rm /old/rm
	cp rm /bin/rm

clean: ; /bin/rm rm *.o
