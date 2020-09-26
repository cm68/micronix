
unix:	low.o io.o high.o
unix:;	link -tb0x1000 -r -o unix low.o io.o -lm.a high.o

low.o:	uhdr.o access.o	balloc.o cio.o create.o	exec.o fio.o fork.o
low.o:	ialloc.o iget.o	imap.o link.o user.o malloc.o mem.o mount.o
low.o:	name.o open.o queue.o rdwri.o reg.o sig.o sleep.o sub8.o
low.o:	subc.o swap.o sys1.o sys2.o system.o time.o trap.o tty.o uio.o
	link -tb0x1000 -d -o low.o uhdr.o access.o balloc.o cio.o create.o exec.o fio.o fork.o ialloc.o iget.o imap.o link.o user.o malloc.o mem.o mount.o name.o open.o queue.o rdwri.o reg.o sig.o sleep.o sub8.o subc.o swap.o sys1.o sys2.o system.o time.o trap.o tty.o uio.o 
io.o:	con.o dj.o memdev.o intrpt.o mio.o multio.o mw.o
	link -tb0x1000 -d -o io.o con.o dj.o memdev.o intrpt.o mio.o multio.o mw.o

high.o:	main.o cus.o inits.o
	link -tb0x1000 -d -o high.o main.o cus.o inits.o

main.o:	main.c buf.h con.h file.h inode.h mount.h proc.h sys.h
	cc -c -x0 main.c

access.o: access.c inode.h mount.h proc.h sys.h
	cc -c access.c

balloc.o: balloc.c buf.h proc.h	sup.h sys.h
	cc -c balloc.c

cio.o: cio.c con.h file.h inode.h proc.h sys.h
	cc -c cio.c

con.o: con.c con.h proc.h sys.h
	cc -cz con.c

create.o: create.c inode.h proc.h sys.h
	cc -c create.c

cus.o:	cus.c sys.h
	cc -c -x0 cus.c

exec.o:	exec.c buf.h inode.h obj.h proc.h sys.h
	cc -c exec.c

fio.o: fio.c buf.h con.h inode.h proc.h	sys.h
	cc -c fio.c

fork.o:	fork.c con.h file.h inode.h proc.h sys.h
	cc -c fork.c

ialloc.o: ialloc.c buf.h inode.h proc.h	sup.h sys.h
	cc -c ialloc.c

iget.o:	iget.c buf.h inode.h mount.h proc.h sys.h
	cc -c iget.c

imap.o:	imap.c buf.h inode.h proc.h sys.h
	cc -c imap.c

inits.o:	inits.s
	as inits.s

intrpt.o: intrpt.s
	as intrpt.s

dj.o: dj.c buf.h con.h proc.h sys.h
	cc -c dj.c

link.o:	link.c inode.h proc.h sys.h
	cc -c link.c

# lock.o:	lock.c inode.h proc.h sys.h lock.h file.h
# 	cc -c lock.c

malloc.o: malloc.c buf.h con.h sup.h sys.h proc.h
	cc -c malloc.c

mem.o: mem.s
	as mem.s

memdev.o: memdev.c proc.h sys.h
	cc -c memdev.c

mio.o: mio.s
	as mio.s

mount.o: mount.c buf.h inode.h mount.h proc.h sup.h sys.h
	cc -c mount.c

multio.o: multio.c proc.h sys.h	tty.h
	cc -cz multio.c

mw.o: mw.c buf.h con.h proc.h sys.h
	cc -c mw.c

name.o:	name.c buf.h inode.h proc.h sys.h
	cc -c name.c

open.o:	open.c con.h file.h inode.h proc.h sys.h 
	cc -c open.c

queue.o: queue.c proc.h	sys.h tty.h
	cc -c queue.c

rdwri.o: rdwri.c file.h	inode.h	proc.h sys.h
	cc -c rdwri.c

reg.o:	reg.c proc.h sys.h
	cc -c reg.c

sig.o: sig.c proc.h sys.h tty.h
	cc -cz sig.c

sleep.o: sleep.c proc.h	sys.h
	cc -c sleep.c

sub8.o:	sub8.s
	as sub8.s

subc.o:	subc.c proc.h sys.h
	cc -c subc.c

swap.o:	swap.c buf.h con.h proc.h sys.h
	cc -c swap.c

sys1.o:	sys1.c file.h inode.h proc.h sys.h
	cc -c sys1.c

sys2.o:	sys2.c file.h inode.h proc.h sys.h
	cc -c sys2.c

system.o: system.c proc.h sys.h
	cc -c system.c

time.o:	time.c proc.h sys.h
	cc -c time.c

trap.o:	trap.c proc.h sys.h
	cc -c trap.c

tty.o: tty.c proc.h sys.h tty.h
	cc -cz tty.c

uhdr.o:	uhdr.s
	as uhdr.s

uio.o: uio.c buf.h con.h proc.h sup.h sys.h
	cc -c uio.c

user.o:	user.c proc.h sys.h
	cc -c -x7 user.c

clean:
	rm *.o unix
