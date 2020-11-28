all: objdump hitpatch

objdump: objdump.c
	cc -g -o objdump objdump.c

hitech: hitech.c
	cc -g -o hitech hitech.c

hitpatch: hitpatch.c
	cc -g -o hitpatch hitpatch.c ../../lib/util.c

clean:
	rm *.o hitpatch objdump hitech
