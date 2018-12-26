AS = zmac
HEXFILES = mon375.hex mon447.hex

all: $(HEXFILES)

clean:
	rm -f $(HEXFILES)

mon375.hex: mon375.s
	$(AS) -o mon375.hex mon375.s
	objcopy --remove-section .sec1

mon447.hex: mon447.s
	$(AS) -o mon447.hex mon447.s

