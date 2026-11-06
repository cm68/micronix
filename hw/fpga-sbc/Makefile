QUARTUS ?= quartus_sh
PROJECT  = fpga-sbc
SOF      = output_files/$(PROJECT).sof
RTL      = $(wildcard rtl/*.v rtl/*.vhd rtl/t80/*.vhd)
INIT     = $(wildcard *.hex *.mif)

.PHONY: all clean program

all: $(SOF)

$(SOF): $(RTL) $(INIT) $(PROJECT).qsf $(PROJECT).qpf $(PROJECT).sdc
	$(QUARTUS) --flow compile $(PROJECT)
	@ls -l $@

program: $(SOF)
	quartus_pgm -c USB-Blaster -m jtag -o "p;$(SOF)"

clean:
	rm -rf db incremental_db output_files $(PROJECT).sof mon500.mif
