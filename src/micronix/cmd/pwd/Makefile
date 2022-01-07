all:	pwd

pwd : pwd.c
	cc -o pwd pwd.c

install: pwd
	cp pwd /bin/pwd

upgrade: pwd
	cp /bin/pwd /old/pwd
	cp pwd /bin/pwd

clean: ; rm pwd *.o
