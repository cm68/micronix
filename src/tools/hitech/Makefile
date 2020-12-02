test: bpatch
	./bpatch C.COM
	cp C ../../../filesystem
	
all: bpatch objdump

bpatch: bpatch.c
	cc -g -o bpatch bpatch.c

objdump: objdump.c
	cc -g -o objdump objdump.c

hello.com: hello.c
	zxc hello.c -lc

hello.dis: hello.com
	../disas hello.com >hello.dis

hello: bpatch hello.com hitech.pat
	./bpatch hello.com
	
clean:
	rm *.o hitpatch objdump hitech
