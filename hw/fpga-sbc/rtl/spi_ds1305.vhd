-- spi_ds1305.vhd - fixed-generic wrapper around the vendor SPI master.
--
-- One 16-bit slave, mode 0, SCLK = 4 MHz / (2*4) = 500 kHz.  The
-- DS1305 command byte plus data byte are one transfer.

library ieee;
use ieee.std_logic_1164.all;

entity spi_ds1305 is
	port (
		clk      : in  std_logic;
		reset_n  : in  std_logic;
		enable   : in  std_logic;
		tx_data  : in  std_logic_vector(15 downto 0);
		miso     : in  std_logic;
		sclk     : out std_logic;
		cs_n     : out std_logic;
		mosi     : out std_logic;
		busy     : out std_logic;
		rx_data  : out std_logic_vector(15 downto 0));
end spi_ds1305;

architecture rtl of spi_ds1305 is
	signal ss_n : std_logic_vector(0 downto 0);
begin
	u0 : entity work.spi_16bit_master
		generic map (
			slaves  => 1,
			d_width => 16)
		port map (
			clock   => clk,
			reset_n => reset_n,
			enable  => enable,
			cpol    => '0',
			cpha    => '0',
			cont    => '0',
			clk_div => 4,
			addr    => 0,
			tx_data => tx_data,
			miso    => miso,
			sclk    => sclk,
			ss_n    => ss_n,
			mosi    => mosi,
			busy    => busy,
			rx_data => rx_data);

	cs_n <= ss_n(0);
end rtl;
