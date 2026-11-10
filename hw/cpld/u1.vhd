-- u1.vhd - Z180 MPZ80-compatible sequencer + translation CPLD
--
-- VHDL-93 translation of u1.v.  ATF1508AS-10, 84-pin PLCC.  This is RTL,
-- not a fitted design.  Pin names match hw/cpld/pins_u1.csv; package pin
-- numbers are assigned by the fitter/board.
--
-- The external map RAMs (CY7C199-15PC) and flash are not modelled here;
-- this module drives their address/control pins and reads only the four
-- permission bits from map RAM B.
--
-- Trap model: the 15-byte sequence at 0xbf0 is fetched from the on-board
-- flash.  The CPU's address is ignored for the 15 forced reads; the
-- external 74AHCT161/163 counter supplies the flash offset.  A user HALT
-- is replaced with 0x00 on its own M1, and the 15-read window starts on
-- the next read.
--
-- The board contract is ../z180-cpu.md.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity u1_sequencer is
    generic (
        -- EMU_FAULT_STATUS = true reports every user memory fault as
        -- 0xf7, matching mpz80.c and the kernel's fault().  false gives
        -- the manual's 0x3e/0x7e/0xbe/0xfe forms.
        EMU_FAULT_STATUS : boolean := true
    );
    port (
        clk     : in std_logic;
        reset_n : in std_logic;

        -- Z180 address (only the decoded bits enter this device)
        a0  : in std_logic;
        a1  : in std_logic;
        a2  : in std_logic;
        a3  : in std_logic;
        a12 : in std_logic;
        a13 : in std_logic;
        a14 : in std_logic;
        a15 : in std_logic;

        -- local decode from the external 74AHCT138 (active low)
        sel_ram_n  : in std_logic;
        sel_regs_n : in std_logic;
        sel_map_n  : in std_logic;
        sel_rom_n  : in std_logic;
        sel_fpu_n  : in std_logic;

        -- Z180 data bus (registers and local device traffic)
        d : inout std_logic_vector(7 downto 0);

        -- Z180 bus control
        m1_n   : in std_logic;
        mreq_n : in std_logic;
        iorq_n : in std_logic;
        rd_n   : in std_logic;
        wr_n   : in std_logic;
        halt_n : in std_logic;

        -- map RAM B attribute nibble: {unused, RI10, execute, read}
        pte_attr : in std_logic_vector(3 downto 0);

        -- from U2
        bus_grant : in std_logic;
        stop_i    : in std_logic;
        aux_i     : in std_logic;
        int_req   : in std_logic;

        -- from the A23-A20 = F comparator
        flash_phys : in std_logic;

        -- task nibble to the external map address mux
        task_o : out std_logic_vector(3 downto 0);

        -- local/trap state to U2
        local_o : out std_logic;
        trap_o  : out std_logic;

        -- map RAM and address-driver control
        mapram_oe_n    : out std_logic_vector(1 downto 0);  -- {B,A}
        map_we         : out std_logic;
        addr12_23_oe_n : out std_logic;

        -- flash trap-window counter and address mux
        flash_cnt_load_n : out std_logic;
        flash_cnt_clr_n  : out std_logic;
        flash_cnt_clk    : out std_logic;
        flash_half       : out std_logic;
        flash_mux        : out std_logic;  -- 1 = trap counter selected

        -- flash and local SRAM
        flash_ce_n : out std_logic;
        flash_oe_n : out std_logic;
        sram_oe_n  : out std_logic;
        sram_we_n  : out std_logic;

        -- trap data buffer / 0x00 substitution
        trapbuf_oe_n : out std_logic;
        trapdata_sel : out std_logic;  -- 1 = flash, 0 = 0x00

        -- interrupt request allowed through to the Z180 /INT0
        int_to_cpu : out std_logic
    );
end entity u1_sequencer;

architecture rtl of u1_sequencer is

    ----------------------------------------------------------------------
    -- Bus decode
    ----------------------------------------------------------------------
    signal m1   : std_logic;
    signal mreq : std_logic;
    signal iorq : std_logic;
    signal rd   : std_logic;
    signal wr   : std_logic;

    signal mem       : std_logic;
    signal m1_fetch   : std_logic;
    signal memread  : std_logic;
    signal memwrite : std_logic;
    signal ioread   : std_logic;
    signal iowrite  : std_logic;

    signal addr_lo : std_logic_vector(3 downto 0);
    signal page    : std_logic_vector(3 downto 0);

    signal local     : std_logic;
    signal local_mem : std_logic;
    signal local_reg  : std_logic;
    signal local_ram  : std_logic;
    signal local_rom  : std_logic;
    signal local_fpu  : std_logic;

    signal reg_400 : std_logic;
    signal reg_401 : std_logic;
    signal reg_402 : std_logic;
    signal reg_403 : std_logic;

    signal rd_reg : std_logic;
    signal wr_reg : std_logic;

    signal task_write     : std_logic;
    signal mask_write     : std_logic;
    signal trap_addr_read : std_logic;
    signal keyb_read      : std_logic;
    signal status_read    : std_logic;

    signal map_write : std_logic;
    signal mapped    : std_logic;

    ----------------------------------------------------------------------
    -- Trap-window state (declared early: the decode and every control
    -- output below are qualified by it)
    ----------------------------------------------------------------------
    signal trapcount  : unsigned(3 downto 0);
    signal trap_win : std_logic;
    signal trap_arm  : std_logic;
    signal flash_half_r : std_logic;

    ----------------------------------------------------------------------
    -- Task register and 8-M1 switch delay
    --
    -- Low nibble is the task/map-table selector.  The high nibble is
    -- latched context and is not an address field.
    ----------------------------------------------------------------------
    signal taskreg   : unsigned(3 downto 0);
    signal next_taskreg  : unsigned(3 downto 0);
    signal delay : unsigned(3 downto 0);
    signal task_hi   : unsigned(3 downto 0);

    signal task_eff : unsigned(3 downto 0);
    signal supervisor : std_logic;
    signal user  : std_logic;

    ----------------------------------------------------------------------
    -- Mask and status
    ----------------------------------------------------------------------
    signal maskreg   : std_logic_vector(7 downto 0);
    signal trapstat : std_logic_vector(7 downto 0);

    -- Common MPZ80 contract definitions.  The same names and values
    -- are in the Verilog twin and the FPGA sequencer; change one,
    -- change all.  Manual Tables 1-1, 4-9 and 4-10.
    constant ATTR_READ   : integer := 0;
    constant ATTR_EXEC   : integer := 1;
    constant ATTR_RI10   : integer := 2;
    constant ATTR_UNUSED : integer := 3;

    constant MASK_STOP : integer := 0;
    constant MASK_AUX  : integer := 1;
    constant MASK_TINT : integer := 2;
    constant MASK_HALT : integer := 4;
    constant MASK_SINT : integer := 5;
    constant MASK_IO   : integer := 6;
    constant MASK_ZIO  : integer := 7;

    constant TS_RESET    : std_logic_vector(7 downto 0) := x"BF";
    constant TS_HALT     : std_logic_vector(7 downto 0) := x"BB";
    constant TS_INT      : std_logic_vector(7 downto 0) := x"B7";
    constant TS_IN       : std_logic_vector(7 downto 0) := x"3C";
    constant TS_OUT      : std_logic_vector(7 downto 0) := x"BC";
    constant TS_FAULT    : std_logic_vector(7 downto 0) := x"F7";
    constant TS_FAULT_RD : std_logic_vector(7 downto 0) := x"3E";
    constant TS_FAULT_WR : std_logic_vector(7 downto 0) := x"BE";
    constant TS_GROW_RD  : std_logic_vector(7 downto 0) := x"7E";
    constant TS_GROW_WR  : std_logic_vector(7 downto 0) := x"FE";
    constant TS_STOP_RD  : std_logic_vector(7 downto 0) := x"2F";
    constant TS_STOP_WR  : std_logic_vector(7 downto 0) := x"AF";
    constant TS_AUX_RD   : std_logic_vector(7 downto 0) := x"1F";
    constant TS_AUX_WR   : std_logic_vector(7 downto 0) := x"9F";

    constant ST_VOID : std_logic_vector(7 downto 0) := x"01";
    constant ST_IORQ : std_logic_vector(7 downto 0) := x"02";
    constant ST_HALT : std_logic_vector(7 downto 0) := x"04";
    constant ST_INT  : std_logic_vector(7 downto 0) := x"08";
    constant ST_STOP : std_logic_vector(7 downto 0) := x"10";
    constant ST_AUX  : std_logic_vector(7 downto 0) := x"20";
    constant ST_RI10 : std_logic_vector(7 downto 0) := x"40";
    constant ST_READ : std_logic_vector(7 downto 0) := x"80";

    constant ST_RESET : std_logic_vector(7 downto 0) := TS_RESET;

    signal mask_stop_dis : std_logic;
    signal mask_aux_en   : std_logic;
    signal mask_tint_dis : std_logic;
    signal mask_halt_dis : std_logic;
    signal mask_sint_dis : std_logic;
    signal mask_io_dis   : std_logic;

    ----------------------------------------------------------------------
    -- Prefix tracking, halt substitution and trap sources
    ----------------------------------------------------------------------
    signal prefix    : std_logic;
    signal is_prefix : std_logic;

    signal halt_event : std_logic;
    signal io_event   : std_logic;
    signal int_event  : std_logic;
    signal stop_event : std_logic;
    signal aux_event  : std_logic;

    ----------------------------------------------------------------------
    -- Permission check and deferred memory fault
    ----------------------------------------------------------------------
    signal perm_read  : std_logic;
    signal perm_exec  : std_logic;
    signal perm_write : std_logic;
    signal perm_ok    : std_logic;
    signal attr_grow  : std_logic;
    signal perm_fault : std_logic;
    signal grow       : std_logic;
    signal mem_fault  : std_logic;

    signal fault_pending  : std_logic;
    signal fault_is_write : std_logic;
    signal fault_grow     : std_logic;
    signal fault_trapreg  : std_logic_vector(7 downto 0);

    signal fault_event : std_logic;

    ----------------------------------------------------------------------
    -- Trap address shift register: {current page, previous page}
    ----------------------------------------------------------------------
    signal trapreg   : std_logic_vector(7 downto 0);
    signal trapreg_hold : std_logic_vector(7 downto 0);

    signal trap_shift : std_logic;

    ----------------------------------------------------------------------
    -- Trap window, flash counter and data buffer
    ----------------------------------------------------------------------
    signal trap_read       : std_logic;
    signal trap_start_read : std_logic;
    signal trap_event      : std_logic;
    signal trapstat_next     : std_logic_vector(7 downto 0);

    ----------------------------------------------------------------------
    -- Data bus
    ----------------------------------------------------------------------
    signal d_out : std_logic_vector(7 downto 0);
    signal d_oe  : std_logic;

begin

    ----------------------------------------------------------------------
    -- Bus decode
    ----------------------------------------------------------------------
    m1   <= not m1_n;
    mreq <= not mreq_n;
    iorq <= not iorq_n;
    rd   <= not rd_n;
    wr   <= not wr_n;

    mem       <= mreq and (not iorq);
    m1_fetch   <= m1 and mem and rd;
    memread  <= mem and rd;
    memwrite <= mem and wr;
    ioread   <= iorq and rd;
    iowrite  <= iorq and wr;

    addr_lo <= a3 & a2 & a1 & a0;
    page    <= a15 & a14 & a13 & a12;

    -- The 74AHCT138 is enabled by SEG0 & TASK0; any asserted select means
    -- the access is the on-board page.  The FPU select may instead be a
    -- second 1K RAM bank.
    -- The external 74AHCT138 must be enabled only for memory cycles; the
    -- mem term here is the second line of defence against an I/O address
    -- accidentally matching the local page.
    local <= mem and
                  (not (sel_ram_n and sel_regs_n and sel_map_n and
                        sel_rom_n and sel_fpu_n));
    local_mem  <= local or (flash_phys and mem);

    local_reg <= mem and (not sel_regs_n);
    local_ram <= mem and (not sel_ram_n);
    local_rom <= mem and (not sel_rom_n);
    local_fpu <= mem and (not sel_fpu_n);

    reg_400 <= '1' when (local_reg = '1' and addr_lo = "0000") else '0';
    reg_401 <= '1' when (local_reg = '1' and addr_lo = "0001") else '0';
    reg_402 <= '1' when (local_reg = '1' and addr_lo = "0010") else '0';
    reg_403 <= '1' when (local_reg = '1' and addr_lo = "0011") else '0';

    rd_reg <= local_reg and rd and (not trap_win);
    wr_reg <= local_reg and wr and (not trap_win);

    task_write     <= wr_reg and reg_402;
    mask_write     <= wr_reg and reg_403;
    trap_addr_read <= rd_reg and reg_400;
    keyb_read      <= rd_reg and reg_401;
    status_read    <= rd_reg and reg_403;

    map_write <= mem and (not sel_map_n) and wr and (not trap_win);

    -- A normal S-100 memory cycle: not local, not the flash region, not
    -- inside the forced trap window.
    mapped <= mem and (not local_mem) and (not trap_win);

    ----------------------------------------------------------------------
    -- Task register and 8-M1 switch delay
    --
    -- On the final M1 the new task is already visible to the map mux and
    -- permission check, exactly as mpz80.c applies set_taskreg() before
    -- decoding the address.
    ----------------------------------------------------------------------
    task_eff <= next_taskreg when (m1_fetch = '1' and delay = 1)
                else taskreg;

    supervisor <= '1' when task_eff = 0 else '0';
    user  <= not supervisor;

    task_o <= std_logic_vector(task_eff);

    ----------------------------------------------------------------------
    -- Mask and status
    ----------------------------------------------------------------------
    -- SINT follows the emulator/kernel: mask bit 5 high blocks supervisor
    -- interrupts; low allows them.  (The manual's wording is the reverse.)
    mask_stop_dis <= maskreg(MASK_STOP);
    mask_aux_en   <= maskreg(MASK_AUX);
    mask_tint_dis <= maskreg(MASK_TINT);
    mask_halt_dis <= maskreg(MASK_HALT);
    mask_sint_dis <= maskreg(MASK_SINT);
    mask_io_dis   <= maskreg(MASK_IO);

    ----------------------------------------------------------------------
    -- Prefix tracking, halt substitution and trap sources
    ----------------------------------------------------------------------
    is_prefix <= '1' when (d = x"CB") or (d = x"DD") or
                          (d = x"ED") or (d = x"FD")
                 else '0';

    halt_event <= '1' when (m1_fetch = '1' and prefix = '0' and d = x"76" and
                            mask_halt_dis = '1' and trap_win = '0' and
                            trap_arm = '0')
                  else '0';

    io_event <= '1' when (iorq = '1' and m1 = '0' and user = '1' and
                          mask_io_dis = '1' and trap_win = '0' and
                          trap_arm = '0')
                else '0';

    int_event <= '1' when (int_req = '1' and user = '1' and
                           mask_tint_dis = '1' and trap_win = '0' and
                           trap_arm = '0')
                 else '0';

    stop_event <= '1' when (stop_i = '1' and mask_stop_dis = '0' and
                            trap_win = '0' and trap_arm = '0')
                  else '0';

    aux_event <= '1' when (aux_i = '1' and mask_aux_en = '1' and
                           trap_win = '0' and trap_arm = '0')
                 else '0';

    ----------------------------------------------------------------------
    -- Permission check and deferred memory fault
    ----------------------------------------------------------------------
    -- Manual Table 1-1 and Tables 4-9/4-10: Read-Only (01) may
    -- execute and read, Execute-Only (10) may execute, Full (11) may
    -- do everything.  So execute is any nonzero attribute, read is
    -- bit 0 and write needs both bits.
    perm_read  <= pte_attr(ATTR_READ);
    perm_exec  <= pte_attr(ATTR_READ) or pte_attr(ATTR_EXEC);
    perm_write <= pte_attr(ATTR_READ) and pte_attr(ATTR_EXEC);

    perm_ok <= perm_exec when m1_fetch = '1' else
               perm_write when memwrite = '1' else
               perm_read;

    attr_grow <= pte_attr(ATTR_RI10);

    perm_fault <= mapped and user and (not perm_ok);
    grow       <= mapped and user and perm_ok and attr_grow;

    mem_fault <= perm_fault or grow;

    fault_event <= '1' when (fault_pending = '1' and m1_fetch = '1' and
                             trap_win = '0' and trap_arm = '0')
                   else '0';

    ----------------------------------------------------------------------
    -- Trap address shift register: {current page, previous page}
    ----------------------------------------------------------------------
    trap_shift <= mapped and (memread or memwrite) and
                  (not trap_win);

    ----------------------------------------------------------------------
    -- Trap window, flash counter and data buffer
    ----------------------------------------------------------------------
    trap_read <= memread;          -- opcode and operand fetches count

    -- The faulting M1 is the first forced read; int/stop/aux/reset arm the
    -- window for the next read; the halt's own M1 is substituted with 0x00
    -- and the window starts after it.
    trap_start_read <= trap_read and
                       (fault_event or (trap_arm and (not halt_event)));

    trap_event <= halt_event or io_event or int_event or stop_event or
                  aux_event or fault_event;

    ----------------------------------------------------------------------
    -- Status encoding, highest-priority source first
    ----------------------------------------------------------------------
    status_encode : process (stop_event, aux_event, int_event, halt_event,
                             io_event, fault_event, rd, ioread,
                             fault_is_write, fault_grow)
    begin
        trapstat_next <= ST_RESET;
        if stop_event = '1' then
            if rd = '1' then
                trapstat_next <= TS_STOP_RD;
            else
                trapstat_next <= TS_STOP_WR;
            end if;
        elsif aux_event = '1' then
            if rd = '1' then
                trapstat_next <= TS_AUX_RD;
            else
                trapstat_next <= TS_AUX_WR;
            end if;
        elsif int_event = '1' then
            trapstat_next <= TS_INT;
        elsif halt_event = '1' then
            trapstat_next <= TS_HALT;
        elsif io_event = '1' then
            if ioread = '1' then
                trapstat_next <= TS_IN;
            else
                trapstat_next <= TS_OUT;
            end if;
        elsif fault_event = '1' then
            if EMU_FAULT_STATUS then
                trapstat_next <= TS_FAULT;
            elsif fault_is_write = '1' then
                if fault_grow = '1' then
                    trapstat_next <= TS_GROW_WR;
                else
                    trapstat_next <= TS_FAULT_WR;
                end if;
            else
                if fault_grow = '1' then
                    trapstat_next <= TS_GROW_RD;
                else
                    trapstat_next <= TS_FAULT_RD;
                end if;
            end if;
        end if;
    end process status_encode;

    ----------------------------------------------------------------------
    -- Interrupt gating (U2 supplies the raw S-100 request)
    ----------------------------------------------------------------------
    int_to_cpu <= int_req and (not trap_win) and (not trap_arm) and
                  ((user and (not mask_tint_dis)) or
                   (supervisor and (not mask_sint_dis)));

    ----------------------------------------------------------------------
    -- Data bus
    ----------------------------------------------------------------------
    data_bus : process (trap_addr_read, keyb_read, status_read,
                        trapreg_hold, trapstat)
    begin
        d_oe  <= '0';
        d_out <= x"FF";
        if trap_addr_read = '1' then
            d_oe  <= '1';
            d_out <= trapreg_hold;
        elsif keyb_read = '1' then
            d_oe  <= '1';
            d_out <= x"FF";
        elsif status_read = '1' then
            d_oe  <= '1';
            d_out <= trapstat;
        end if;
    end process data_bus;

    d <= d_out when d_oe = '1' else (others => 'Z');

    ----------------------------------------------------------------------
    -- Sequential state
    ----------------------------------------------------------------------
    sequencer : process (clk, reset_n)
    begin
        if reset_n = '0' then
            taskreg          <= (others => '0');
            next_taskreg         <= (others => '0');
            delay        <= (others => '0');
            task_hi          <= (others => '0');
            maskreg          <= (others => '0');
            trapstat        <= ST_RESET;
            prefix            <= '0';
            trapreg     <= (others => '0');
            trapreg_hold   <= (others => '0');
            trapcount        <= (others => '0');
            trap_win       <= '0';
            trap_arm        <= '1';   -- reset window starts at first read
            flash_half_r      <= '0';   -- lower 1K reset half
            fault_pending <= '0';
            fault_is_write    <= '0';
            fault_grow        <= '0';
            fault_trapreg        <= (others => '0');
        elsif rising_edge(clk) then

            -- Prefix: one prefix byte, then the next M1 is the opcode.
            if m1_fetch = '1' then
                prefix <= is_prefix;
            elsif memread = '1' then
                prefix <= '0';
            end if;

            -- Task switch delay.  The new task is selected on the final
            -- M1, and task_eff already reflects that for this fetch.
            if m1_fetch = '1' and delay /= 0 then
                if delay = 1 then
                    taskreg   <= next_taskreg;
                    delay <= (others => '0');
                else
                    delay <= delay - 1;
                end if;
            end if;

            -- Trap address shift, and latch on a faulting reference.
            if trap_shift = '1' then
                trapreg <= page & trapreg(7 downto 4);
            end if;

            if mem_fault = '1' then
                fault_pending <= '1';
                fault_is_write    <= memwrite;
                fault_grow        <= attr_grow;
                fault_trapreg        <= page & trapreg(7 downto 4);
            end if;

            if trap_event = '1' then
                trapstat <= trapstat_next;
                taskreg   <= (others => '0');
                next_taskreg  <= (others => '0');
                delay <= (others => '0');
                trap_arm <= '1';
            end if;

            if fault_event = '1' then
                trapreg_hold   <= fault_trapreg;
                fault_pending <= '0';
            elsif trap_event = '1' then
                if trap_shift = '1' then
                    trapreg_hold <= page & trapreg(7 downto 4);
                else
                    trapreg_hold <= trapreg;
                end if;
            end if;

            -- Window counter and first-read start.
            if trap_win = '1' then
                if trap_read = '1' then
                    trapcount <= trapcount - 1;
                    if trapcount = 1 then
                        trap_win <= '0';
                    end if;
                end if;
            elsif trap_start_read = '1' then
                trap_win <= '1';
                trapcount  <= to_unsigned(15, 4);
                trap_arm  <= '0';
            end if;

            -- Register writes.
            if task_write = '1' then
                next_taskreg    <= unsigned(d(3 downto 0));
                task_hi     <= unsigned(d(7 downto 4));
                delay   <= to_unsigned(8, 4);
                flash_half_r <= '1';   -- upper monitor half
            end if;

            if mask_write = '1' then
                maskreg <= d;
            end if;

            -- Any non-reset trap selects the upper monitor half.
            if trap_event = '1' then
                flash_half_r <= '1';
            end if;
        end if;
    end process sequencer;

    ----------------------------------------------------------------------
    -- Output control
    ----------------------------------------------------------------------
    local_o <= local or (flash_phys and mem);

    trap_o <= trap_win or halt_event or mem_fault or
              fault_pending or trap_arm;

    mapram_oe_n <= "00" when mapped = '1' else "11";

    map_we <= map_write;

    addr12_23_oe_n <= not (mapped and (not bus_grant));

    flash_cnt_load_n <= not trap_start_read;
    flash_cnt_clr_n  <= reset_n;
    flash_cnt_clk    <= trap_read and trap_win;

    flash_half <= flash_half_r;
    flash_mux  <= trap_win or trap_start_read;

    flash_ce_n <= not (local_rom or flash_phys or trap_win or
                       trap_start_read or halt_event);
    flash_oe_n <= not ((local_rom or flash_phys or trap_win or
                        trap_start_read or halt_event) and memread);

    sram_oe_n <= not (local_ram and memread);
    sram_we_n <= not (local_ram and memwrite);

    trapbuf_oe_n <= not (trap_win or trap_start_read or halt_event);
    trapdata_sel <= trap_win or trap_start_read;

end architecture rtl;
