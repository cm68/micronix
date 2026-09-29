#
# assemble the monitor roms and generate list and symbols for simulator
#
# hwsim/d1/roms/Makefile
#
# Changed: <2023-07-10 14:47:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

OBJ = mon375.bin mon447.bin mon500.bin multIO.bin
SYM = mon375.sym mon447.sym mon500.sym multIO.sym
SRC = mon375.s mon447.s mon500.s multIO.s

all: $(OBJ) $(SYM)

install: $(OBJ) $(SYM)
	cp $(OBJ) $(SYM) ..

%.bin: %.cim
	dd if=$< of=$@ bs=1024 skip=2 count=2
	#dd if=$< of=$@ bs=1024 skip=2 count=2 seek=2

# the selector is a flat 4k image at 0xf000 - no on-board data below it,
# so the cim is already the code and just needs padding to the full 4k
multIO.bin: multIO.cim
	cp multIO.cim $@
	truncate -s 4096 $@

%.lst %.cim: %.s
	../../../tools/zmac --oo lst,cim --od . -n $<

%.sym: %.lst
	cat $< | awk ' \
		/^Symbol Table/ { k=1 ; next } \
		/=/ { next } \
		(k == 1) { k++; next } \
		(k == 2) { printf "0x%04x %s\n", strtonum("0x" $$2), $$1; next } \
		' > $@

clean:
	rm -f *.cim *.lst *.bin *.sym

clobber: clean
