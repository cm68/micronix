-- t80_wrap.vhd - the T80s core with the MPZ80 bus settings.
--
-- The core is Daniel Wallner's T80 (see rtl/t80/, BSD-style licence).
-- T80s is the synchronous wrapper; Mode=1 is the fast Z80 timing,
-- T2Write=1 puts WR_n in T2, and IOWait=0 gives the single-cycle I/O
-- the MPZ80 monitor expects.  The clock is the 4.000 MHz MPZ80 clock.

library ieee;
use ieee.std_logic_1164.all;

entity t80_wrap is
	port(
		reset_n		: in  std_logic;
		clk			: in  std_logic;
		wait_n		: in  std_logic;
		int_n		: in  std_logic;
		nmi_n		: in  std_logic;
		busrq_n		: in  std_logic;
		m1_n		: out std_logic;
		mreq_n		: out std_logic;
		iorq_n		: out std_logic;
		rd_n		: out std_logic;
		wr_n		: out std_logic;
		rfsh_n		: out std_logic;
		halt_n		: out std_logic;
		busak_n		: out std_logic;
		a			: out std_logic_vector(15 downto 0);
		di			: in  std_logic_vector(7 downto 0);
		do			: out std_logic_vector(7 downto 0));
end t80_wrap;

architecture rtl of t80_wrap is
begin

	u0 : entity work.t80s
		generic map(
			Mode	=> 1,
			T2Write	=> 1,
			IOWait	=> 0)
		port map(
			reset_n	=> reset_n,
			clk_n	=> clk,
			wait_n	=> wait_n,
			int_n	=> int_n,
			nmi_n	=> nmi_n,
			busrq_n	=> busrq_n,
			m1_n	=> m1_n,
			mreq_n	=> mreq_n,
			iorq_n	=> iorq_n,
			rd_n	=> rd_n,
			wr_n	=> wr_n,
			rfsh_n	=> rfsh_n,
			halt_n	=> halt_n,
			busak_n	=> busak_n,
			a		=> a,
			di		=> di,
			do		=> do);

end rtl;
