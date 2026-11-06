-- rom2k.vhd - 2 KB monitor ROM, altsyncram with a MIF image.
--
-- Derived from the S100Computers Components/RAM/InternalRam2K.vhd
-- wrapper.  The image is mon500.mif (or mon447.mif), generated from
-- the monitor binary by tools/bin2mif.py.  The write port is tied off
-- in mpz80_top; this is a ROM.

library ieee;
use ieee.std_logic_1164.all;

library altera_mf;
use altera_mf.all;

entity rom2k is
	port(
		address		: in  std_logic_vector(10 downto 0);
		clock		: in  std_logic;
		data		: in  std_logic_vector(7 downto 0);
		wren		: in  std_logic;
		q			: out std_logic_vector(7 downto 0));
end rom2k;

architecture syn of rom2k is

	signal sub_wire0 : std_logic_vector(7 downto 0);

	component altsyncram
	generic(
		clock_enable_input_a		: string;
		clock_enable_output_a		: string;
		init_file					: string;
		init_file_layout			: string;
		intended_device_family		: string;
		lpm_hint					: string;
		lpm_type					: string;
		numwords_a					: natural;
		operation_mode				: string;
		outdata_aclr_a				: string;
		outdata_reg_a				: string;
		power_up_uninitialized		: string;
		widthad_a					: natural;
		width_a						: natural;
		width_byteena_a				: natural);
	port(
		address_a	: in  std_logic_vector(10 downto 0);
		clock0		: in  std_logic;
		data_a		: in  std_logic_vector(7 downto 0);
		wren_a		: in  std_logic;
		q_a			: out std_logic_vector(7 downto 0));
	end component;

begin

	q <= sub_wire0;

	altsyncram_component : altsyncram
	generic map(
		clock_enable_input_a	=> "BYPASS",
		clock_enable_output_a	=> "BYPASS",
		init_file				=> "mon500.mif",
		init_file_layout		=> "PORT_A",
		intended_device_family	=> "Cyclone IV E",
		lpm_hint				=> "ENABLE_RUNTIME_MOD=NO",
		lpm_type				=> "altsyncram",
		numwords_a				=> 2048,
		operation_mode			=> "SINGLE_PORT",
		outdata_aclr_a			=> "NONE",
		outdata_reg_a			=> "UNREGISTERED",
		power_up_uninitialized	=> "FALSE",
		widthad_a				=> 11,
		width_a					=> 8,
		width_byteena_a			=> 1)
	port map(
		address_a	=> address,
		clock0		=> clock,
		data_a		=> data,
		wren_a		=> wren,
		q_a			=> sub_wire0);

end syn;
