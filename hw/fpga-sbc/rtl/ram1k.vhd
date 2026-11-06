-- ram1k.vhd - 1 KB local page RAM, altsyncram single port.
--
-- Derived from the S100Computers Components/RAM/InternalRam1K.vhd
-- megafunction wrapper.  The device family is Cyclone IV E and the
-- output is unregistered so the T80 sees the byte in the same cycle.
-- The MPZ80's 1 KB local RAM is read/write at full CPU speed.

library ieee;
use ieee.std_logic_1164.all;

library altera_mf;
use altera_mf.all;

entity ram1k is
	port(
		address		: in  std_logic_vector(9 downto 0);
		clock		: in  std_logic;
		data		: in  std_logic_vector(7 downto 0);
		wren		: in  std_logic;
		q			: out std_logic_vector(7 downto 0));
end ram1k;

architecture syn of ram1k is

	signal sub_wire0 : std_logic_vector(7 downto 0);

	component altsyncram
	generic(
		clock_enable_input_a		: string;
		clock_enable_output_a		: string;
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
		address_a	: in  std_logic_vector(9 downto 0);
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
		intended_device_family	=> "Cyclone IV E",
		lpm_hint				=> "ENABLE_RUNTIME_MOD=NO",
		lpm_type				=> "altsyncram",
		numwords_a				=> 1024,
		operation_mode			=> "SINGLE_PORT",
		outdata_aclr_a			=> "NONE",
		outdata_reg_a			=> "UNREGISTERED",
		power_up_uninitialized	=> "FALSE",
		widthad_a				=> 10,
		width_a					=> 8,
		width_byteena_a			=> 1)
	port map(
		address_a	=> address,
		clock0		=> clock,
		data_a		=> data,
		wren_a		=> wren,
		q_a			=> sub_wire0);

end syn;
