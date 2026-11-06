-- u2.vhd - Z180 MPZ80-compatible S-100 bus interface CPLD
--
-- VHDL-93 translation of u2.v.  ATF1508AS-10, 84-pin PLCC.  RTL only;
-- the fitter assigns package pins.  All S-100 outputs are buffer inputs;
-- the 74ABT/74F parts drive the edge.
--
-- U2 owns the S-100 cycle, wait states, buffer enables, interrupt
-- request/acknowledge, DMA acknowledgment, the S-100 clock divider and
-- the switch-register status bits.  The trap engine and permission check
-- live in U1.
--
-- The board contract is ../z180-cpu.md.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity u2_bus is
    generic (
        -- S-100 pRDY polarity.  The IEEE-696 signal is commonly active
        -- low; verify against the memory board before changing this.
        RDY_ACTIVE_LOW : boolean := false
    );
    port (
        clk     : in std_logic;
        reset_n : in std_logic;

        -- S-100 clock divider strap: 00=/4, 01=/5, 10=/6, 11=/8
        clkdiv : in std_logic_vector(1 downto 0);

        -- Z180 bus
        mreq_n : in std_logic;
        iorq_n : in std_logic;
        rd_n   : in std_logic;
        wr_n   : in std_logic;
        m1_n   : in std_logic;
        busak_n : in std_logic;

        -- S-100 inputs (after 74AHCT244 receivers)
        s100_rdy     : in std_logic;
        s100_hold_n  : in std_logic;
        s100_reset_n : in std_logic;
        s100_poc_n   : in std_logic;
        s100_nmi_n   : in std_logic;
        s100_pint_n  : in std_logic;
        s100_vi_or_n : in std_logic;

        -- front panel
        panel_stop : in std_logic;
        panel_aux  : in std_logic;

        -- U1 inter-CPLD
        local_i     : in std_logic;
        trap_i      : in std_logic;
        int_to_cpu  : in std_logic;
        stop_o      : out std_logic;
        aux_o       : out std_logic;
        bus_grant   : out std_logic;
        int_req     : out std_logic;

        -- S-100 control buffer inputs
        pSYNC  : out std_logic;
        pSTVAL : out std_logic;
        pDBIN  : out std_logic;
        pWR    : out std_logic;
        sMEMR  : out std_logic;
        sMEMW  : out std_logic;
        sINP   : out std_logic;
        sOUT   : out std_logic;
        sM1    : out std_logic;
        sINTA  : out std_logic;
        pWAIT  : out std_logic;

        -- buffer enables (active low)
        a0_11_oe_n : out std_logic;
        do_oe_n    : out std_logic;
        di_oe_n    : out std_logic;
        ctrl_oe_n  : out std_logic;

        -- S-100 clock buffer
        s100_clk1 : out std_logic;
        s100_clk2 : out std_logic;

        -- S-100 hold acknowledge buffer input
        phlda : out std_logic;

        -- Z180 control
        z180_wait_n   : out std_logic;
        z180_nmi_n    : out std_logic;
        z180_int0_n   : out std_logic;
        z180_reset_n  : out std_logic;
        z180_busreq_n : out std_logic;

        -- switch-register buffer
        int_pend   : out std_logic;
        trap_reset : out std_logic
    );
end entity u2_bus;

architecture rtl of u2_bus is

    signal mreq : std_logic;
    signal iorq : std_logic;
    signal rd   : std_logic;
    signal wr   : std_logic;
    signal m1   : std_logic;

    signal local : std_logic;
    signal grant : std_logic;
    signal rdy   : std_logic;

    ----------------------------------------------------------------------
    -- S-100 bus state machine
    ----------------------------------------------------------------------
    constant IDLE  : unsigned(2 downto 0) := "000";
    constant ADDR  : unsigned(2 downto 0) := "001";
    constant READ  : unsigned(2 downto 0) := "010";
    constant WRITE : unsigned(2 downto 0) := "011";
    constant BUS_WAIT : unsigned(2 downto 0) := "100";
    constant DONE  : unsigned(2 downto 0) := "101";

    signal state    : unsigned(2 downto 0);
    signal is_io    : std_logic;
    signal is_write : std_logic;
    signal is_m1    : std_logic;
    signal timeout  : unsigned(3 downto 0);

    signal start     : std_logic;
    signal wait_done : std_logic;
    signal at_addr   : std_logic;

    signal cycle       : std_logic;
    signal read_cycle  : std_logic;
    signal write_cycle : std_logic;

    ----------------------------------------------------------------------
    -- S-100 clock divider
    ----------------------------------------------------------------------
    signal div_cnt  : unsigned(3 downto 0);
    signal div_n    : unsigned(3 downto 0);
    signal s100_clk : std_logic;

    -- Output ports cannot be read in VHDL-93; these carry the values
    -- that int_pend and trap_reset also need.
    signal int_req_i      : std_logic;
    signal z180_reset_i   : std_logic;

begin

    mreq <= not mreq_n;
    iorq <= not iorq_n;
    rd   <= not rd_n;
    wr   <= not wr_n;
    m1   <= not m1_n;

    local <= local_i or trap_i;
    grant <= not busak_n;

    rdy <= not s100_rdy when RDY_ACTIVE_LOW else s100_rdy;

    ----------------------------------------------------------------------
    -- S-100 bus state machine
    ----------------------------------------------------------------------
    start <= '1' when (local = '0' and grant = '0' and state = IDLE and
                       (mreq = '1' or iorq = '1'))
             else '0';

    wait_done <= '1' when (rdy = '1' or timeout = 15) else '0';

    bus_fsm : process (clk, reset_n)
    begin
        if reset_n = '0' then
            state    <= IDLE;
            is_io    <= '0';
            is_write <= '0';
            is_m1    <= '0';
            timeout  <= (others => '0');
        elsif rising_edge(clk) then
            if local = '1' or grant = '1' then
                -- A local/trap access or a DMA grant takes the S-100 bus
                -- away immediately.  Returning to IDLE releases /WAIT so
                -- the Z180 can run the local trap or the DMA master can
                -- own the bus.
                state   <= IDLE;
                timeout <= (others => '0');
            else
                case state is
                    when IDLE =>
                        timeout <= (others => '0');
                        if start = '1' then
                            is_io    <= iorq;
                            is_write <= wr;
                            is_m1    <= m1;
                            state    <= ADDR;
                        end if;

                    when ADDR =>
                        if is_write = '1' then
                            state <= WRITE;
                        else
                            state <= READ;
                        end if;

                    when READ | WRITE =>
                        if rdy = '1' then
                            state <= DONE;
                        else
                            timeout <= timeout + 1;
                            state   <= BUS_WAIT;
                        end if;

                    when BUS_WAIT =>
                        if wait_done = '1' then
                            state <= DONE;
                        else
                            timeout <= timeout + 1;
                        end if;

                    when DONE =>
                        state <= IDLE;

                    when others =>
                        state <= IDLE;
                end case;
            end if;
        end if;
    end process bus_fsm;

    cycle       <= '1' when (state /= IDLE and local = '0' and grant = '0')
                   else '0';
    read_cycle  <= cycle and (not is_write);
    write_cycle <= cycle and is_write;

    at_addr <= '1' when state = ADDR else '0';

    ----------------------------------------------------------------------
    -- S-100 control outputs (buffer inputs)
    ----------------------------------------------------------------------
    pSYNC  <= '1' when state = ADDR else '0';
    pSTVAL <= '1' when state = ADDR else '0';
    pDBIN  <= read_cycle;
    pWR    <= write_cycle;

    -- S-100 status/control lines sMEMR, sMEMW/sWO, sINP, sOUT, sM1 and
    -- sINTA are active low; pSYNC/pSTVAL/pDBIN/pWR are active high.
    sMEMR <= not (read_cycle and (not is_io));
    sMEMW <= not (write_cycle and (not is_io));
    sINP  <= not (read_cycle and is_io);
    sOUT  <= not (write_cycle and is_io);

    sM1   <= not m1;
    sINTA <= not (m1 and iorq);

    -- S-100 pWAIT is active low; the Z180 /WAIT is held while the bus
    -- cycle is in progress.
    pWAIT       <= '1' when state /= BUS_WAIT else '0';
    z180_wait_n <= '1' when state = IDLE else '0';

    ----------------------------------------------------------------------
    -- Buffer enables.  All are forced off for a local/trap cycle, while
    -- the bus is granted to a DMA master, and for a non-S-100 cycle.
    ----------------------------------------------------------------------
    a0_11_oe_n <= not cycle;
    do_oe_n    <= not (write_cycle and (not at_addr));
    di_oe_n    <= not (read_cycle and (not at_addr));
    ctrl_oe_n  <= not cycle;

    ----------------------------------------------------------------------
    -- Interrupts, reset and DMA
    ----------------------------------------------------------------------
    stop_o <= panel_stop;
    aux_o  <= panel_aux;

    int_req_i <= not (s100_pint_n and s100_vi_or_n);
    int_req   <= int_req_i;
    int_pend  <= int_req_i;

    z180_int0_n <= not int_to_cpu;
    z180_nmi_n  <= s100_nmi_n;

    z180_reset_i <= reset_n and s100_reset_n and s100_poc_n;
    z180_reset_n <= z180_reset_i;
    trap_reset   <= not z180_reset_i;

    z180_busreq_n <= s100_hold_n;
    phlda         <= not busak_n;
    bus_grant     <= not busak_n;

    ----------------------------------------------------------------------
    -- S-100 clock divider: 10 MHz /5 -> 2 MHz with the default strap.
    -- The two outputs feed the clock buffer; clk2 is the complement.
    ----------------------------------------------------------------------
    div_strap : process (clkdiv)
    begin
        case clkdiv is
            when "00" => div_n <= to_unsigned(4, 4);
            when "01" => div_n <= to_unsigned(5, 4);
            when "10" => div_n <= to_unsigned(6, 4);
            when others => div_n <= to_unsigned(8, 4);
        end case;
    end process div_strap;

    clock_div : process (clk, reset_n)
    begin
        if reset_n = '0' then
            div_cnt  <= (others => '0');
            s100_clk <= '0';
        elsif rising_edge(clk) then
            if div_cnt >= (div_n - 1) then
                div_cnt <= (others => '0');
            else
                div_cnt <= div_cnt + 1;
            end if;

            if div_cnt < shift_right(div_n, 1) then
                s100_clk <= '1';
            else
                s100_clk <= '0';
            end if;
        end if;
    end process clock_div;

    s100_clk1 <= s100_clk;
    s100_clk2 <= not s100_clk;

end architecture rtl;
