IVERILOG ?= iverilog
GHDL     ?= ghdl
VHDLWORK ?= ghdlwork

.PHONY: all vhdl check clean

all: vhdl
	@if command -v $(IVERILOG) >/dev/null 2>&1; then \
		$(MAKE) check; \
	else \
		echo "$(IVERILOG) not installed; skipping Verilog check"; \
	fi

vhdl:
	mkdir -p $(VHDLWORK)
	$(GHDL) -a --std=93 --workdir=$(VHDLWORK) u1.vhd u2.vhd
	$(GHDL) -e --std=93 --workdir=$(VHDLWORK) u1_sequencer
	$(GHDL) -e --std=93 --workdir=$(VHDLWORK) u2_bus

check:
	$(IVERILOG) -g2005 -o u1.vvp u1.v
	$(IVERILOG) -g2005 -o u2.vvp u2.v

clean:
	rm -f u1.vvp u2.vvp
	rm -rf $(VHDLWORK)
