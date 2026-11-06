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

        -- map RAM B permission nibble: {unused, RI10, execute, read}
        perm : in std_logic_vector(3 downto 0);

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
    signal m1_read   : std_logic;
    signal mem_read  : std_logic;
    signal mem_write : std_logic;
    signal io_read   : std_logic;
    signal io_write  : std_logic;

    signal addr_lo : std_logic_vector(3 downto 0);
    signal page    : std_logic_vector(3 downto 0);

    signal local_page : std_logic;
    signal local      : std_logic;
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
    signal trap_count  : unsigned(3 downto 0);
    signal trap_active : std_logic;
    signal trap_armed  : std_logic;
    signal flash_half_r : std_logic;

    ----------------------------------------------------------------------
    -- Task register and 8-M1 switch delay
    --
    -- Low nibble is the task/map-table selector.  The high nibble is
    -- latched context and is not an address field.
    ----------------------------------------------------------------------
    signal task_cur   : unsigned(3 downto 0);
    signal task_next  : unsigned(3 downto 0);
    signal task_delay : unsigned(3 downto 0);
    signal task_ctx   : unsigned(3 downto 0);

    signal eff_task : unsigned(3 downto 0);
    signal is_super : std_logic;
    signal is_user  : std_logic;

    ----------------------------------------------------------------------
    -- Mask and status
    ----------------------------------------------------------------------
    signal mask_reg   : std_logic_vector(7 downto 0);
    signal status_reg : std_logic_vector(7 downto 0);

    constant ST_VOID : std_logic_vector(7 downto 0) := x"01";
    constant ST_IORQ : std_logic_vector(7 downto 0) := x"02";
    constant ST_HALT : std_logic_vector(7 downto 0) := x"04";
    constant ST_INT  : std_logic_vector(7 downto 0) := x"08";
    constant ST_STOP : std_logic_vector(7 downto 0) := x"10";
    constant ST_AUX  : std_logic_vector(7 downto 0) := x"20";
    constant ST_RI10 : std_logic_vector(7 downto 0) := x"40";
    constant ST_READ : std_logic_vector(7 downto 0) := x"80";

    constant ST_RESET : std_logic_vector(7 downto 0) :=
        ST_VOID or ST_IORQ or ST_HALT or ST_INT or
        ST_STOP or ST_AUX or ST_READ;

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
    signal perm_read_ok  : std_logic;
    signal perm_exec_ok  : std_logic;
    signal perm_write_ok : std_logic;

    signal access_ok     : std_logic;
    signal ri10          : std_logic;
    signal prot_fault    : std_logic;
    signal grow_fault    : std_logic;
    signal mem_fault_now : std_logic;

    signal mem_fault_pending : std_logic;
    signal fault_is_write    : std_logic;
    signal fault_ri10        : std_logic;
    signal fault_page        : std_logic_vector(7 downto 0);

    signal fault_event : std_logic;

    ----------------------------------------------------------------------
    -- Trap address shift register: {current page, previous page}
    ----------------------------------------------------------------------
    signal trap_addr_reg   : std_logic_vector(7 downto 0);
    signal trap_addr_latch : std_logic_vector(7 downto 0);

    signal trap_shift : std_logic;

    ----------------------------------------------------------------------
    -- Trap window, flash counter and data buffer
    ----------------------------------------------------------------------
    signal trap_read       : std_logic;
    signal trap_start_read : std_logic;
    signal trap_event      : std_logic;
    signal trap_status     : std_logic_vector(7 downto 0);

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
    m1_read   <= m1 and mem and rd;
    mem_read  <= mem and rd;
    mem_write <= mem and wr;
    io_read   <= iorq and rd;
    io_write  <= iorq and wr;

    addr_lo <= a3 & a2 & a1 & a0;
    page    <= a15 & a14 & a13 & a12;

    -- The 74AHCT138 is enabled by SEG0 & TASK0; any asserted select means
    -- the access is the on-board page.  The FPU select may instead be a
    -- second 1K RAM bank.
    -- The external 74AHCT138 must be enabled only for memory cycles; the
    -- mem term here is the second line of defence against an I/O address
    -- accidentally matching the local page.
    local_page <= mem and
                  (not (sel_ram_n and sel_regs_n and sel_map_n and
                        sel_rom_n and sel_fpu_n));
    local      <= local_page or (flash_phys and mem);

    local_reg <= mem and (not sel_regs_n);
    local_ram <= mem and (not sel_ram_n);
    local_rom <= mem and (not sel_rom_n);
    local_fpu <= mem and (not sel_fpu_n);

    reg_400 <= '1' when (local_reg = '1' and addr_lo = "0000") else '0';
    reg_401 <= '1' when (local_reg = '1' and addr_lo = "0001") else '0';
    reg_402 <= '1' when (local_reg = '1' and addr_lo = "0010") else '0';
    reg_403 <= '1' when (local_reg = '1' and addr_lo = "0011") else '0';

    rd_reg <= local_reg and rd and (not trap_active);
    wr_reg <= local_reg and wr and (not trap_active);

    task_write     <= wr_reg and reg_402;
    mask_write     <= wr_reg and reg_403;
    trap_addr_read <= rd_reg and reg_400;
    keyb_read      <= rd_reg and reg_401;
    status_read    <= rd_reg and reg_403;

    map_write <= mem and (not sel_map_n) and wr and (not trap_active);

    -- A normal S-100 memory cycle: not local, not the flash region, not
    -- inside the forced trap window.
    mapped <= mem and (not local) and (not trap_active);

    ----------------------------------------------------------------------
    -- Task register and 8-M1 switch delay
    --
    -- On the final M1 the new task is already visible to the map mux and
    -- permission check, exactly as mpz80.c applies set_taskreg() before
    -- decoding the address.
    ----------------------------------------------------------------------
    eff_task <= task_next when (m1_read = '1' and task_delay = 1)
                else task_cur;

    is_super <= '1' when eff_task = 0 else '0';
    is_user  <= not is_super;

    task_o <= std_logic_vector(eff_task);

    ----------------------------------------------------------------------
    -- Mask and status
    ----------------------------------------------------------------------
    -- SINT follows the emulator/kernel: mask bit 5 high blocks supervisor
    -- interrupts; low allows them.  (The manual's wording is the reverse.)
    mask_stop_dis <= mask_reg(0);
    mask_aux_en   <= mask_reg(1);
    mask_tint_dis <= mask_reg(2);
    mask_halt_dis <= mask_reg(4);
    mask_sint_dis <= mask_reg(5);
    mask_io_dis   <= mask_reg(6);

    ----------------------------------------------------------------------
    -- Prefix tracking, halt substitution and trap sources
    ----------------------------------------------------------------------
    is_prefix <= '1' when (d = x"CB") or (d = x"DD") or
                          (d = x"ED") or (d = x"FD")
                 else '0';

    halt_event <= '1' when (m1_read = '1' and prefix = '0' and d = x"76" and
                            mask_halt_dis = '1' and trap_active = '0' and
                            trap_armed = '0')
                  else '0';

    io_event <= '1' when (iorq = '1' and m1 = '0' and is_user = '1' and
                          mask_io_dis = '1' and trap_active = '0' and
                          trap_armed = '0')
                else '0';

    int_event <= '1' when (int_req = '1' and is_user = '1' and
                           mask_tint_dis = '1' and trap_active = '0' and
                           trap_armed = '0')
                 else '0';

    stop_event <= '1' when (stop_i = '1' and mask_stop_dis = '0' and
                            trap_active = '0' and trap_armed = '0')
                  else '0';

    aux_event <= '1' when (aux_i = '1' and mask_aux_en = '1' and
                           trap_active = '0' and trap_armed = '0')
                 else '0';

    ----------------------------------------------------------------------
    -- Permission check and deferred memory fault
    ----------------------------------------------------------------------
    perm_read_ok  <= perm(0);
    perm_exec_ok  <= perm(0) or perm(1);   -- manual: r/o is executable
    perm_write_ok <= perm(0) and perm(1);

    access_ok <= perm_exec_ok when m1_read = '1' else
                 perm_write_ok when mem_write = '1' else
                 perm_read_ok;

    ri10 <= perm(2);

    prot_fault <= mapped and is_user and (not access_ok);
    grow_fault <= mapped and is_user and access_ok and ri10;

    mem_fault_now <= prot_fault or grow_fault;

    fault_event <= '1' when (mem_fault_pending = '1' and m1_read = '1' and
                             trap_active = '0' and trap_armed = '0')
                   else '0';

    ----------------------------------------------------------------------
    -- Trap address shift register: {current page, previous page}
    ----------------------------------------------------------------------
    trap_shift <= mapped and (mem_read or mem_write) and
                  (not trap_active);

    ----------------------------------------------------------------------
    -- Trap window, flash counter and data buffer
    ----------------------------------------------------------------------
    trap_read <= mem_read;          -- opcode and operand fetches count

    -- The faulting M1 is the first forced read; int/stop/aux/reset arm the
    -- window for the next read; the halt's own M1 is substituted with 0x00
    -- and the window starts after it.
    trap_start_read <= trap_read and
                       (fault_event or (trap_armed and (not halt_event)));

    trap_event <= halt_event or io_event or int_event or stop_event or
                  aux_event or fault_event;

    ----------------------------------------------------------------------
    -- Status encoding, highest-priority source first
    ----------------------------------------------------------------------
    status_encode : process (stop_event, aux_event, int_event, halt_event,
                             io_event, fault_event, rd, io_read,
                             fault_is_write, fault_ri10)
    begin
        trap_status <= ST_RESET;
        if stop_event = '1' then
            if rd = '1' then
                trap_status <= x"2F";
            else
                trap_status <= x"AF";
            end if;
        elsif aux_event = '1' then
            if rd = '1' then
                trap_status <= x"1F";
            else
                trap_status <= x"9F";
            end if;
        elsif int_event = '1' then
            trap_status <= x"B7";
        elsif halt_event = '1' then
            trap_status <= x"BB";
        elsif io_event = '1' then
            if io_read = '1' then
                trap_status <= x"3C";
            else
                trap_status <= x"BC";
            end if;
        elsif fault_event = '1' then
            if EMU_FAULT_STATUS then
                trap_status <= x"F7";
            elsif fault_is_write = '1' then
                if fault_ri10 = '1' then
                    trap_status <= x"FE";
                else
                    trap_status <= x"BE";
                end if;
            else
                if fault_ri10 = '1' then
                    trap_status <= x"7E";
                else
                    trap_status <= x"3E";
                end if;
            end if;
        end if;
    end process status_encode;

    ----------------------------------------------------------------------
    -- Interrupt gating (U2 supplies the raw S-100 request)
    ----------------------------------------------------------------------
    int_to_cpu <= int_req and (not trap_active) and (not trap_armed) and
                  ((is_user and (not mask_tint_dis)) or
                   (is_super and (not mask_sint_dis)));

    ----------------------------------------------------------------------
    -- Data bus
    ----------------------------------------------------------------------
    data_bus : process (trap_addr_read, keyb_read, status_read,
                        trap_addr_latch, status_reg)
    begin
        d_oe  <= '0';
        d_out <= x"FF";
        if trap_addr_read = '1' then
            d_oe  <= '1';
            d_out <= trap_addr_latch;
        elsif keyb_read = '1' then
            d_oe  <= '1';
            d_out <= x"FF";
        elsif status_read = '1' then
            d_oe  <= '1';
            d_out <= status_reg;
        end if;
    end process data_bus;

    d <= d_out when d_oe = '1' else (others => 'Z');

    ----------------------------------------------------------------------
    -- Sequential state
    ----------------------------------------------------------------------
    sequencer : process (clk, reset_n)
    begin
        if reset_n = '0' then
            task_cur          <= (others => '0');
            task_next         <= (others => '0');
            task_delay        <= (others => '0');
            task_ctx          <= (others => '0');
            mask_reg          <= (others => '0');
            status_reg        <= ST_RESET;
            prefix            <= '0';
            trap_addr_reg     <= (others => '0');
            trap_addr_latch   <= (others => '0');
            trap_count        <= (others => '0');
            trap_active       <= '0';
            trap_armed        <= '1';   -- reset window starts at first read
            flash_half_r      <= '0';   -- lower 1K reset half
            mem_fault_pending <= '0';
            fault_is_write    <= '0';
            fault_ri10        <= '0';
            fault_page        <= (others => '0');
        elsif rising_edge(clk) then

            -- Prefix: one prefix byte, then the next M1 is the opcode.
            if m1_read = '1' then
                prefix <= is_prefix;
            elsif mem_read = '1' then
                prefix <= '0';
            end if;

            -- Task switch delay.  The new task is selected on the final
            -- M1, and eff_task already reflects that for this fetch.
            if m1_read = '1' and task_delay /= 0 then
                if task_delay = 1 then
                    task_cur   <= task_next;
                    task_delay <= (others => '0');
                else
                    task_delay <= task_delay - 1;
                end if;
            end if;

            -- Trap address shift, and latch on a faulting reference.
            if trap_shift = '1' then
                trap_addr_reg <= page & trap_addr_reg(7 downto 4);
            end if;

            if mem_fault_now = '1' then
                mem_fault_pending <= '1';
                fault_is_write    <= mem_write;
                fault_ri10        <= ri10;
                fault_page        <= page & trap_addr_reg(7 downto 4);
            end if;

            if trap_event = '1' then
                status_reg <= trap_status;
                task_cur   <= (others => '0');
                task_next  <= (others => '0');
                task_delay <= (others => '0');
                trap_armed <= '1';
            end if;

            if fault_event = '1' then
                trap_addr_latch   <= fault_page;
                mem_fault_pending <= '0';
            elsif trap_event = '1' then
                if trap_shift = '1' then
                    trap_addr_latch <= page & trap_addr_reg(7 downto 4);
                else
                    trap_addr_latch <= trap_addr_reg;
                end if;
            end if;

            -- Window counter and first-read start.
            if trap_active = '1' then
                if trap_read = '1' then
                    trap_count <= trap_count - 1;
                    if trap_count = 1 then
                        trap_active <= '0';
                    end if;
                end if;
            elsif trap_start_read = '1' then
                trap_active <= '1';
                trap_count  <= to_unsigned(15, 4);
                trap_armed  <= '0';
            end if;

            -- Register writes.
            if task_write = '1' then
                task_next    <= unsigned(d(3 downto 0));
                task_ctx     <= unsigned(d(7 downto 4));
                task_delay   <= to_unsigned(8, 4);
                flash_half_r <= '1';   -- upper monitor half
            end if;

            if mask_write = '1' then
                mask_reg <= d;
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
    local_o <= local_page or (flash_phys and mem);

    trap_o <= trap_active or halt_event or mem_fault_now or
              mem_fault_pending or trap_armed;

    mapram_oe_n <= "00" when mapped = '1' else "11";

    map_we <= map_write;

    addr12_23_oe_n <= not (mapped and (not bus_grant));

    flash_cnt_load_n <= not trap_start_read;
    flash_cnt_clr_n  <= reset_n;
    flash_cnt_clk    <= trap_read and trap_active;

    flash_half <= flash_half_r;
    flash_mux  <= trap_active or trap_start_read;

    flash_ce_n <= not (local_rom or flash_phys or trap_active or
                       trap_start_read or halt_event);
    flash_oe_n <= not ((local_rom or flash_phys or trap_active or
                        trap_start_read or halt_event) and mem_read);

    sram_oe_n <= not (local_ram and mem_read);
    sram_we_n <= not (local_ram and mem_write);

    trapbuf_oe_n <= not (trap_active or trap_start_read or halt_event);
    trapdata_sel <= trap_active or trap_start_read;

end architecture rtl;
