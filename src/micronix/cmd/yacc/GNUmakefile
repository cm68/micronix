
CC=zxc
O=obj
CFLAGS=
LDFLAGS = -s
INCLUDE=-I../../include/
LIBDIR = /usr/local/lib/cpm/lib80

OBJECTS = y1.$O y2.$O y3.$O y4.$O
LIBOBJS = libmai.o libpar.o libzac.o libzer.o libzin.o

#	zxlink -R -Myaccmap -Z -Ptext=0,data,bss -oyacc $(OBJECTS) $(LIBDIR)/crtcpm.obj
yacc: $(OBJECTS)
	zxlink -Myaccmap -Z -Ptext=0,data,bss -C100H -oyacc.com $(LIBDIR)/crtcpm.obj $(OBJECTS) $(LIBDIR)/libc.lib

liby.a: $(LIBOBJS)
	rm liby.a
	lib liby.a -r -v6 -v $(LIBOBJS)

install: head
	cp yacc $(YNAME)
	cp liby.a $(YLIB)
	cp yaccopar $(AUXDIR)/yaccopar

clean:
	rm -f yacc liby.a *.$O

%.$O: %.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $<

