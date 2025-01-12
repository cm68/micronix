#
# cross Makefile for asz test
#
# usr/src/cmd/asz/test/GNUmakefile
#
# Changed: <2023-09-14 15:51:08 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

TFILES = bsst.s getc.s hello.s putc.s puts.s symtest.s deftest.s \
	symtest.s

test: $(TFILES)
	for i in $(TFILES) ; do \
		echo ============== $$i ============== ; \
		../asz $$i ; \
		bname=$$(basename $$i .s) ; \
		nm80 -drv $$bname.o > $$bname.dis ; \
	done

symtest.s: gentest.sh
	bash gentest.sh >symtest.s

clean:
	rm symtest.s *.dis *.o
