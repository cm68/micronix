all:	pwd

pwd : pwd.c
	cc -o pwd pwd.c

install: pwd /old/pwd
	cp pwd /bin/pwd

/old/pwd:
	cp /bin/pwd /old/pwd

clean: ; rm pwd *.o
