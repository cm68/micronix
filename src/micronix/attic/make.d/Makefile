# Description file for the Make command

P = und -3 | opr -r2
T =
FILES = Makefile pwinfo pr_make.sh rc version.c defs main.c doname.c \
	misc.c files.c dosys.c\
	gram.y lex.c gcos.c
OBJECTS = version.o main.o doname.o misc.o files.o dosys.o gram.o 
LIBES= -lPW -lS
LINT = lint -p

CC= cc
CFLAGS = -O
LDFLAGS= -s
.c.o:
	$(CC) -c -Dpwb $(CFLAGS) $<

GCOSFILES = defs version.c main.c doname.c misc.c gram.c lex.c gcos.c
make:  $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBES) -o make
	size make

$(OBJECTS):  defs
gram.o: lex.c

cleanup:
	-rm *.o gram.c
	-du

install:
	@size make /bin/make
	cp make /bin/make ; rm make

printall: # Print files off line.
	pr $(FILES) | $P
	touch print

print:  $(FILES)	# print recently changed files
	pr $? | $P
	touch print

save: # Write files on Spider store.
	-nfs -ucv make $(FILES)

test:
	make -dp | grep -v TIME >1zap
	/bin/make -dp | grep -v TIME >2zap
	diff 1zap 2zap
	rm 1zap 2zap

time:  time1 time2 ;

time1 time2 :
	time make $T
	time /bin/make $T

lint :  dosys.c doname.c files.c main.c misc.c version.c gram.c
	$(LINT) dosys.c doname.c files.c main.c misc.c version.c gram.c
	rm gram.c

arch:
	ar r make.a $(FILES)

gcos: $(GCOSFILES)
	fsend -c -u sif $?
	touch gcos

vplist:
	-vp -t-4 -o2 -bR711 sh pr_make.sh -l84

# vim: tabstop=4 shiftwidth=4 noexpandtab:

