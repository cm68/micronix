// mio_pic.v - the MultIO/Wunderbus interrupt-controller pair.
//
// Two 8259A instances cover every wb/mio wiring in sim.json.
//
// sys/pic_init.c is the authority for the real machine:
//
//   master IR0..IR1   S-100 VI0..VI1
//   master IR2        the slave's INT (the notional VI2 cascade line)
//   master IR3..IR5   on-board UART1..UART3
//   master IR6        on-board / spare
//   master IR7        on-board clock (uPD1990 heartbeat)
//   slave  IR0..IR1   S-100 VI3..VI4
//   slave  IR2        the dropped VI5 / IDE position
//   slave  IR3..IR7   the second MultIO's on-board functions
//
// The four VI lines the SBC can route therefore feed master IR0..IR1
// (VI0..VI1) and the slave's IR0..IR1 (VI3..VI4).  VI2 is not a bus
// line: it is the cascade between the two MultIOs, internal to this
// FPGA.  The `onboard` vector carries the FPGA's own interrupt sources:
//
//   onboard[2:0]   master IR3..5   UART1..3
//   onboard[3]     master IR7      clock / uPD1990 heartbeat
//   onboard[4]     master IR6      spare
//   onboard[5]     slave  IR1      spare (VI4 is wired)
//   onboard[6]     slave  IR2      the dropped VI5 / IDE position
//   onboard[11:7]  slave  IR3..7   second-board functions
//
// CASCADE=1 is the Micronix profile for the two-MultIO machine: the
// slave CAS-vectors through master IR2.  CASCADE=0 models a Wunderbus
// master, which has no cascade hardware; the IR2 stub then polls the
// slave.  `slave_icw2 != 0` distinguishes the CAS-programmed slave from
// the polled one, exactly as the simulator does.
//
// Ports (sys/pic_init.c): master PIC 0x4C/0x4D, selected by 0x4F =
// PENABLE (group 0); slave PIC 0x44/0x45, selected by a write to 0x47 =
// SPICONLY.  Master group 0 also holds 0x49 (switch/open bus), 0x4A
// (uPD1990) and 0x48 (daisy); groups 1..3 put an ACE at 0x48..0x4E.
// The top decodes the 0x49/0x4A registers and the ACEs.

`default_nettype none

module mio_pic #(
    parameter CASCADE = 1,
    parameter SLAVE_PRESENT = 1
) (
    input  wire        clk,
    input  wire        reset_n,

    // CPU I/O
    input  wire [7:0]  addr,
    input  wire        iorq,
    input  wire        rd_n,
    input  wire        wr_n,
    input  wire [7:0]  din,
    output wire [7:0]  dout,
    output wire        sel,         // this board's ports are selected
    output wire [1:0]  group_o,     // the 0x4F group latch

    // four S-100 VI lines and the FPGA's on-board interrupt sources
    input  wire [3:0]  vi,
    input  wire [11:0] onboard,

    // interrupt output and acknowledge
    output wire        int_req,
    input  wire        inta,
    output wire [7:0]  inta_byte,
    output wire [7:0]  vec_low_o, vec_high_o
);

    reg [1:0] group;
    reg       slave_sel;        // the second board's 0x47 select

    wire master_cs = iorq && (addr[7:1] == 7'h26) && (group == 2'b00);
    wire slave_board_cs = iorq && (addr == 8'h47);
    wire slave_cs  = iorq && SLAVE_PRESENT && slave_sel &&
                     (addr[7:1] == 7'h22);

    wire master_int, slave_int;
    wire [7:0] master_dout, slave_dout;
    wire [7:0] master_byte, slave_byte;
    wire [3:0] master_svc, slave_svc;
    wire [7:0] slave_icw2;
    wire [1:0] master_ista;

    wire [7:0] master_ir = {
        onboard[3],                                  // IR7 clock
        onboard[4],                                  // IR6 spare
        onboard[2], onboard[1], onboard[0],          // IR5..3 UART3..1
        SLAVE_PRESENT ? slave_int : vi[2],           // IR2 cascade (notional VI2)
        vi[1], vi[0]                                 // IR1..0 VI1..0
    };
    wire [7:0] slave_ir = {
        onboard[11], onboard[10], onboard[9],
        onboard[8], onboard[7],                      // IR7..3 on-board
        onboard[6],                                  // IR2 dropped VI5/IDE
        vi[3], vi[2]                                 // IR1..0 VI4, VI3
    };

    wire delegate = CASCADE && SLAVE_PRESENT &&
                    (master_svc == 4'd2) && (slave_icw2 != 8'h00);

    // The first INTA byte uses the current delegate decision; the second
    // and third use the decision latched at the first byte, so the
    // master's clearing of IR2 cannot drop the slave mid-sequence.
    reg delegate_l;
    wire dsel = (master_ista == 2'd0) ? delegate : delegate_l;

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n)
            delegate_l <= 1'b0;
        else if (master_ista == 2'd0)
            delegate_l <= delegate;
    end

    wire [7:0] master_vec_low, master_vec_high;
    wire [7:0] slave_vec_low, slave_vec_high;

    i8259 master (
        .clk(clk), .reset_n(reset_n),
        .cs(master_cs), .a0(addr[0]), .rd_n(rd_n), .wr_n(wr_n),
        .din(din), .dout(master_dout),
        .ir(master_ir),
        .int_req(master_int),
        .inta(inta), .skip(dsel),
        .inta_byte(master_byte),
        .service_level(master_svc),
        .icw2_o(),
        .inta_state_o(master_ista),
        .vec_low_o(master_vec_low), .vec_high_o(master_vec_high)
    );

    i8259 slave (
        .clk(clk), .reset_n(reset_n),
        .cs(slave_cs), .a0(addr[0]), .rd_n(rd_n), .wr_n(wr_n),
        .din(din), .dout(slave_dout),
        .ir(slave_ir),
        .int_req(slave_int),
        .inta(inta && dsel), .skip(dsel),
        .inta_byte(slave_byte),
        .service_level(slave_svc),
        .icw2_o(slave_icw2),
        .inta_state_o(),
        .vec_low_o(slave_vec_low), .vec_high_o(slave_vec_high)
    );

    assign dout = slave_cs ? slave_dout : (master_cs ? master_dout : 8'h00);
    assign sel  = master_cs || slave_cs || slave_board_cs ||
                  (iorq && (addr == 8'h4F));
    assign group_o = group;
    assign int_req = master_int;
    assign inta_byte = dsel ? slave_byte : master_byte;
    // The game in im0_call needs the same bytes the normal INTA path
    // sees: the slave's vector when the master delegates on IR2.
    assign vec_low_o  = dsel ? slave_vec_low  : master_vec_low;
    assign vec_high_o = dsel ? slave_vec_high : master_vec_high;

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            group <= 2'b00;
            slave_sel <= 1'b0;
        end else begin
            if (iorq && !wr_n && (addr == 8'h4F))
                group <= din[1:0];
            // The second MultIO's own group select.  sys/pic_init.c
            // writes SPICONLY (0x08) here before touching 0x44/0x45;
            // the board only needs the write, so any value selects it.
            if (iorq && !wr_n && (addr == 8'h47))
                slave_sel <= 1'b1;
        end
    end

endmodule

`default_nettype wire
