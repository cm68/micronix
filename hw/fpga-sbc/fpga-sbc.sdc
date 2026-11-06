create_clock -name CLK_50 -period 20.000 [get_ports CLK_50]
derive_pll_clocks
derive_clock_uncertainty

# The USB UART runs at 50 MHz and the 8250 front end at 4 MHz.  The
# tx/rx strobes and the busy/ready levels are stable for many cycles at
# both rates; they are a handshake, not a synchronous path.
set_false_path -from [get_clocks CLK_50] -to [get_clocks {*clk[0]}]
set_false_path -from [get_clocks {*clk[0]}] -to [get_clocks CLK_50]

# S-100 inputs are asynchronous to the FPGA clock.
set_false_path -from [get_ports {FPGA_IN_DI[*] DIP[*]}]
set_false_path -from [get_ports {FPGA_IN_RDY FPGA_IN_XRDY FPGA_IN_INT_n FPGA_IN_pHOLD_n FPGA_IN_BOARD_RESET_n}]
