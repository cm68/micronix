IVERILOG ?= iverilog
VVP      ?= vvp

.PHONY: all check clean

all: check

check:
	$(IVERILOG) -g2005 -o u1.vvp u1.v
	$(IVERILOG) -g2005 -o u2.vvp u2.v

clean:
	rm -f u1.vvp u2.vvp
