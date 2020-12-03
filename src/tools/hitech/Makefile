all: test.dis test

test.dis: test.com
	../disas test.com >test.dis

test: bpatch test.com hitech.pat
	./bpatch test.com

test.com:
	zxc test.c -lc

bpatch: bpatch.c
	cc -g -o bpatch bpatch.c

objdump: objdump.c
	cc -g -o objdump objdump.c

hello.com: hello.c
	zxc hello.c -lc

hello.dis: hello.com hello.sym
	../disas -f hello.sym hello.com >hello.dis

hello: bpatch hello.com hitech.pat
	./bpatch hello.com
	
clean:
	rm *.o hitpatch objdump hitech
