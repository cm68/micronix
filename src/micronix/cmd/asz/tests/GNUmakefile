#
# cross Makefile for asz test
#
# usr/src/cmd/asz/test/GNUmakefile
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

TFILES = bsst.s getc.s hello.s putc.s puts.s symtest.s deftest.s
TFILES = symtest.s

test: $(TFILES)
	for i in $(TFILES) ; do \
		echo ============== $$i ============== ; \
		../asz -vvvv $$i ; \
		bname=$$(basename $$i .s) ; \
		nm80 -drv $$bname.o > $$bname.dis ; \
	done

symtest.s: gentest.sh
	bash gentest.sh >symtest.s

clean:
	rm symtest.s *.dis *.o
