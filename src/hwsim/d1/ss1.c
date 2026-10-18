/*
 * ss1.c
 *
 * The CompuPro System Support 1 board.  It is a different interrupt
 * controller from the Wunderbus/MultIO: a pair of cascaded 8259As (the
 * slave is CAS-cascaded, so it drives its own vector, unlike the MultIO's
 * polled slave), a 2 MHz 8253 interval timer, an OKI MSM5832 real-time
 * clock, and a 2651 UART.  All sixteen ports sit at the board's own
 * base, which is a jumper block: 0x70 here, a change from CompuPro's
 * default 0x50.
 *
 * 0x50 is where the HDCA's four registers and the HD-DMA's two live, so
 * at the default the cards were claiming six ports between them and the
 * last one to start won - the SS1, being last in the link.  Both disk
 * controllers are silent losers of that, and nothing said so.  The base
 * is a jumper on the real board: whatever it is set to, mon500's s1*
 * equates, sys/ss1pic.c and sys/ss1_clock.c's addresses and this file
 * have to say the same thing.
 *
 * The 8259 chip model is shared with the MultIO (pic.c); what differs is
 * the port decode (direct A0, four adjacent ports) and the cascade (the
 * slave's INT is the master's IR7).  The interrupt wiring is the standard
 * SS1 profile: master IR0..IR6 = VI0..VI6, master IR7 = slave, slave
 * IR0 = VI7, IR1..IR3 = timers 0..2, IR4..IR5 = the 9511 SVRQ/END,
 * IR6 = 2651 TxRDY, IR7 = 2651 RxRDY.
 *
 * Only one interrupt controller is active per machine; which one is the
 * -w int driver.  When this board is not it, the ports are not wired and
 * nothing folds the INT line, so it sits dormant beside the MultIO.
 */

#include "sim.h"
#include "config.h"
#include "hwsim.h"
#include "util.h"
#include "pic.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

extern int terminal_fd_in;
extern int terminal_fd_out;

#ifndef NODEBUG
int trace_ss1;
#endif

#define SS1_BASE    0x70        /* the jumper: not CompuPro's default 0x50 */

/* where the jumper is: the compiled value unless the machine file moves it */
static int ss1_base = SS1_BASE;

/*
 * Register offsets from the base (see the manual's I/O port map).
 */
#define SS1_M8259_L  0x00        // master 8259, A0=0
#define SS1_M8259_H  0x01        // master 8259, A0=1
#define SS1_S8259_L  0x02        // slave 8259, A0=0
#define SS1_S8259_H  0x03        // slave 8259, A0=1
#define SS1_8253_TC0 0x04        // timer 0 count
#define SS1_8253_TC1 0x05        // timer 1 count
#define SS1_8253_TC2 0x06        // timer 2 count
#define SS1_8253_CTL 0x07        // timer control word
#define SS1_RTC_CMD   0x0a       // MSM5832 command
#define SS1_RTC_DATA  0x0b       // MSM5832 data
#define SS1_UART_DATA 0x0c       // 2651 data
#define SS1_UART_STAT 0x0d       // 2651 status
#define SS1_UART_MODE 0x0e       // 2651 mode registers
#define SS1_UART_CMD  0x0f       // 2651 command

/*
 * The slave 8259's eight inputs, per the manual's wiring table.  IR0 is
 * VI7; the rest are on-board sources.
 */
#define SS1_IR_VI7      0
#define SS1_IR_TIMER0   1
#define SS1_IR_TIMER1   2
#define SS1_IR_TIMER2   3
#define SS1_IR_FPU0     4
#define SS1_IR_FPU1     5
#define SS1_IR_UART_TX  6
#define SS1_IR_UART_RX  7

#define SS1_MASTER_CASCADE_IR 7   // the slave's INT is the master's IR7

/*
 * The two 8259s and the interrupt-source names for the dumps.
 */
static struct pic ss1_pic[2];       // [0] master, [1] slave
static char *ss1_master_bits[] = { "vi0", "vi1", "vi2", "vi3", "vi4",
    "vi5", "vi6", "cascade" };
static char *ss1_slave_bits[] = { "vi7", "timer0", "timer1", "timer2",
    "fpu0", "fpu1", "uart_tx", "uart_rx" };

/*
 * The three-byte interrupt-ack vector, the same 0xcd / lo / hi protocol
 * the MultIO's 8259 uses and the CPU's Z80 interrupt cycle expects.
 */
static unsigned char ss1_vector[3];
#define IV_FILLED   0
#define IV_1SENT    1
#define IV_2SENT    2
#define IV_EMPTY    3
#define IV_INVALID  4
static int ss1_ivecstate = IV_INVALID;
static char *ss1_ivs_name[] = { "filled", "1", "2", "empty", "invalid" };
static struct pic *ss1_iv_pic;          // which PIC the ack vector came from
static unsigned char ss1_iv_isrbit;     // the in-service bit to set

/*
 * The 8253 interval timer.  Each counter is a 16 bit down-counter at
 * 2 MHz; reaching zero pulses OUT, which is wired to the slave's
 * IR1..IR3.  Mode 0 is a one-shot, modes 2 and 3 are periodic.
 */
struct ss1_timer {
    unsigned short count;       // the reload count
    unsigned short load;        // the count being loaded, lsb first
    int nload;                  // how many count bytes have been written
    int mode;
    int rl;
};
static struct ss1_timer ss1_timer[3];

#define T8253_SC    0xc0
#define T8253_RL    0x30
#define T8253_MODE  0x0e
#define T8253_BCD   0x01

/*
 * The MSM5832 clock.  The command port carries the digit select in the
 * low nibble and Hold/Write/Read in bits 6/5/4; the data port is a 4 bit
 * BCD digit.  Like the MultIO's clock the time handed out is UTC.
 */
#define RTC_HOLD    0x40
#define RTC_WRITE   0x20
#define RTC_READ    0x10
static byte ss1_rtc_digit;

/*
 * The 2651 UART.  TxRDY and RxRDY are hard-wired to the slave's IR6 and
 * IR7, and are level-asserted; the line is edged on each change of cause
 * so the 8259 re-latches (see multio_set_inti for the same dance).  The
 * character path is an xterm, opened when the console-xterm config bit
 * (0x100) is set; without it the uart has no data path.
 */
static int ss1_uart_infd = -1;      // the xterm's read end
static int ss1_uart_outfd = -1;     // the xterm's write end
static byte ss1_uart_rx;            // the received character
static int ss1_uart_rx_valid;       // a character is waiting (RxRDY)
static int ss1_uart_txbusy;         // a character is shifting out
static int ss1_uart_txen;           // transmitter enabled (cmd bit 0)
static int ss1_uart_txrdy_prev = -1;
static int ss1_uart_rxrdy_prev = -1;

static void ss1_slave_ir(int ir, int value);

/*
 * Edge the two UART interrupt lines to their current cause: TxRDY when
 * the transmitter is idle and enabled, RxRDY when a character is waiting.
 * Each change de-asserts then re-asserts, so a level-triggered 8259 sees
 * the edge and re-latches the request.
 */
static void
ss1_uart_set_lines(void)
{
    int txrdy = ss1_uart_txen && !ss1_uart_txbusy;
    int rxrdy = ss1_uart_rx_valid;

    if (txrdy != ss1_uart_txrdy_prev) {
        ss1_uart_txrdy_prev = txrdy;
        if (txrdy) {
            ss1_slave_ir(SS1_IR_UART_TX, 0);
            ss1_slave_ir(SS1_IR_UART_TX, 1);
        } else {
            ss1_slave_ir(SS1_IR_UART_TX, 0);
        }
    }
    if (rxrdy != ss1_uart_rxrdy_prev) {
        ss1_uart_rxrdy_prev = rxrdy;
        if (rxrdy) {
            ss1_slave_ir(SS1_IR_UART_RX, 0);
            ss1_slave_ir(SS1_IR_UART_RX, 1);
        } else {
            ss1_slave_ir(SS1_IR_UART_RX, 0);
        }
    }
}

static void ss1_set_int_line(void);

/*
 * Drive one of the slave's on-board inputs (timer, fpu, uart).
 */
static void
ss1_slave_ir(int ir, int value)
{
    pic_request(&ss1_pic[1], ir, value);
    ss1_set_int_line();
}

/*
 * Fold the two PICs onto the S-100 INT line.  The slave's INT is the
 * master's cascade line, so ask the slave first, fold its answer into the
 * master's IR7, then let the master's resolver cover both.
 */
static void
ss1_set_int_line(void)
{
    int line;

    if (!wiring_active("ss1"))
        return;

    line = (pic_serviceable(&ss1_pic[1]) >= 0) ? 1 : 0;
    if (line)
        ss1_pic[0].irr |= (1 << SS1_MASTER_CASCADE_IR);
    else
        ss1_pic[0].irr &= ~(1 << SS1_MASTER_CASCADE_IR);

    line = (pic_serviceable(&ss1_pic[0]) >= 0) ? 1 : 0;
    trace(trace_ss1, "ss1: %s line\n", line ? "set" : "clear");
    (*int_change)(line);
}

/*
 * The S-100 VI lines changed: master IR0..IR6 are VI0..VI6, the slave's
 * IR0 is VI7.
 */
void
ss1_vi_change(unsigned char new)
{
    byte mask = vi_lines ^ new;
    int i;

    trace(trace_ss1, "ss1: vi_changed lines:%x new:%x diff:%x\n",
        vi_lines, new, mask);
    if (mask == 0)
        return;
    vi_lines = new;

    for (i = 0; i < 7; i++) {
        byte bit = 1 << i;

        if (mask & bit)
            pic_request(&ss1_pic[0], i, (new & bit) != 0);
    }
    if (mask & 0x80)
        pic_request(&ss1_pic[1], SS1_IR_VI7, (new & 0x80) != 0);

    ss1_set_int_line();
}

/*
 * Interrupt acknowledge: return one byte of the three-byte vector.  Only
 * the master owns the acknowledge; when its cascade line (IR7) is the
 * request, the slave is CAS-cascaded and drives its own vector, and the
 * in-service bit lands on whichever PIC supplied the vector.
 */
unsigned char
ss1_intack(void)
{
    int level;
    byte mask;
    int vecaddr;
    struct pic *p;
    unsigned char i;

    trace(trace_ss1, "ss1_intack: ivecstate=%d %s\n",
        ss1_ivecstate, ss1_ivs_name[ss1_ivecstate]);

    if (ss1_ivecstate == IV_EMPTY) {
        level = pic_serviceable(&ss1_pic[0]);

        if (level == SS1_MASTER_CASCADE_IR) {
            ss1_pic[0].irr &= ~(1 << SS1_MASTER_CASCADE_IR);
            p = &ss1_pic[1];
            level = pic_serviceable(p);
        } else {
            p = &ss1_pic[0];
        }

        if (level < 0) {
            l("ss1: lose: no unmasked request found!\n");
            ss1_ivecstate = IV_INVALID;
            ss1_iv_isrbit = 0;
        } else {
            mask = 1 << level;
            ss1_iv_pic = p;
            ss1_iv_isrbit = mask;
            /* a level-triggered 8259 drops the request on acknowledge;
             * the line's own edges will re-raise it */
            p->irr &= ~mask;
            vecaddr = pic_vector(p, level);
            ss1_vector[0] = 0xcd;
            ss1_vector[1] = vecaddr & 0xff;
            ss1_vector[2] = (vecaddr >> 8) & 0xff;
            ss1_ivecstate = IV_FILLED;
            trace(trace_ss1, "ss1: vector %d (%s) %04x\n",
                level, p->irbits[level], vecaddr);
        }
    }

    switch (ss1_ivecstate) {
    case IV_INVALID:
        ss1_ivecstate = IV_EMPTY;
        i = 0xff;
        break;
    case IV_FILLED:
        ss1_ivecstate++;
        i = ss1_vector[0];
        break;
    case IV_1SENT:
        ss1_ivecstate++;
        i = ss1_vector[1];
        break;
    case IV_2SENT:
        ss1_ivecstate = IV_EMPTY;
        i = ss1_vector[2];
        break;
    default:
        l("ss1: bad ivecstate %d\n", ss1_ivecstate);
        i = 0xff;
        ss1_iv_isrbit = 0;
        ss1_ivecstate = IV_EMPTY;
        break;
    }

    /* after the last byte, set the in-service bit on the acked PIC */
    if (ss1_ivecstate == IV_EMPTY) {
        if (ss1_iv_pic && ss1_iv_isrbit) {
            struct pic *q = ss1_iv_pic;

            if (q->auto_eoi) {
                if (q->rotate) {
                    int lvl;

                    for (lvl = 0; lvl < 8; lvl++)
                        if (ss1_iv_isrbit & (1 << lvl))
                            break;
                    q->priority = (lvl + 1) % 8;
                }
            } else {
                q->isr |= ss1_iv_isrbit;
            }
        }
        ss1_set_int_line();
    }
    return i;
}

/*
 * A 8253 timer reached zero.  Timer i is the slave's IR(1+i).  For the
 * periodic modes the timeout re-arms below; for mode 0 it does not.
 */
static void
ss1_timer_fire(int i)
{
    ss1_slave_ir(SS1_IR_TIMER0 + i, 1);
}

/*
 * Arm one timer once its count is fully loaded.  At 2 MHz a count of N
 * is N/2 microseconds; the periodic modes repeat, mode 0 fires once.
 */
static void
ss1_timer_arm(int i)
{
    struct ss1_timer *t = &ss1_timer[i];
    unsigned long count = t->count;

    cancel_time_out(ss1_timer_fire, i);  /* a reprogrammed timer replaces the old */

    if (count == 0)
        count = 0x10000;        /* a load of zero counts down from 65536 */

    if (t->mode == 0) {
        time_out("ss1_timer", count / 2, ss1_timer_fire, i);
    } else {
        int hz = 2000000L / count;

        if (hz <= 0)
            hz = 1;
        recurring_time_out("ss1_timer", hz, ss1_timer_fire, i);
    }
}

/*
 * The 8253 control word selects a counter and programs its read/load and
 * mode.  A control write resets that counter's partial load.
 */
static void
wr_8253_ctl(portaddr p, byte v)
{
    int sel = (v & T8253_SC) >> 6;
    struct ss1_timer *t = &ss1_timer[sel];

    t->mode = (v & T8253_MODE) >> 1;
    t->rl = (v & T8253_RL) >> 4;
    t->nload = 0;
    t->load = 0;
    if (v & T8253_BCD)
        l("ss1: timer %d BCD mode not supported\n", sel);

    trace(trace_ss1, "ss1: write 8253 ctl %02x -> timer %d mode %d rl %d\n",
        v, sel, t->mode, t->rl);
}

/*
 * A count write.  RL says whether it is one byte or two; when the count
 * is complete the timer is armed.
 */
static void
wr_8253_count(portaddr p, byte v)
{
    int sel = (p & 0x0f) - SS1_8253_TC0;   // 0x54, 0x55, 0x56 -> 0, 1, 2
    struct ss1_timer *t = &ss1_timer[sel];

    if (t->rl == 1) {           // lsb only
        t->count = v;
        t->nload = 2;           // complete
    } else if (t->rl == 2) {    // msb only
        t->count = v << 8;
        t->nload = 2;
    } else {                    // lsb then msb
        if (t->nload == 0) {
            t->load = v;
            t->nload = 1;
        } else {
            t->count = t->load | (v << 8);
            t->nload = 2;
        }
    }

    if (t->nload == 2)
        ss1_timer_arm(sel);

    trace(trace_ss1, "ss1: write 8253 count %d %02x\n", sel, v);
}

/*
 * The MSM5832 command port: digit select in the low nibble, Hold/Write/
 * Read in the high.  Only reads are modelled - the guest's clock is
 * seeded from s_time, so writing the digits is a no-op here.
 */
static void
wr_rtc_cmd(portaddr p, byte v)
{
    ss1_rtc_digit = v & 0x0f;
    trace(trace_ss1, "ss1: write rtc cmd %02x digit %d%s%s%s\n", v,
        ss1_rtc_digit, (v & RTC_HOLD) ? " hold" : "",
        (v & RTC_WRITE) ? " write" : "", (v & RTC_READ) ? " read" : "");
}

/*
 * Read one BCD digit of the UTC time.  The MSM5832 has thirteen digits:
 * seconds, minutes, hours, weekday, day, month, year - each as a ones
 * digit then a tens digit.
 */
static byte
rd_rtc_data(portaddr p)
{
    time_t now = time(0);
    struct tm *tm = gmtime(&now);
    int val;

    switch (ss1_rtc_digit) {
    case 0:  val = tm->tm_sec % 10; break;
    case 1:  val = tm->tm_sec / 10; break;
    case 2:  val = tm->tm_min % 10; break;
    case 3:  val = tm->tm_min / 10; break;
    case 4:  val = tm->tm_hour % 10; break;
    case 5:  val = tm->tm_hour / 10; break;
    case 6:  val = tm->tm_wday; break;
    case 7:  val = tm->tm_mday % 10; break;
    case 8:  val = tm->tm_mday / 10; break;
    case 9:  val = (tm->tm_mon + 1) % 10; break;
    case 10: val = (tm->tm_mon + 1) / 10; break;
    case 11: val = ((tm->tm_year + 1900) % 100) % 10; break;
    case 12: val = ((tm->tm_year + 1900) % 100) / 10; break;
    default: val = 0; break;
    }

    trace(trace_ss1, "ss1: read rtc digit %d = %d\n", ss1_rtc_digit, val);
    return val & 0x0f;
}

static void
wr_rtc_data(portaddr p, byte v)
{
    trace(trace_ss1, "ss1: write rtc data %02x\n", v);
}

/*
 * The 2651 status: TxRDY when the transmitter is idle and enabled, RxRDY
 * when a character is waiting, TxEMT always.
 */
static byte
rd_uart_stat(portaddr p)
{
    byte v = 0;

    if (ss1_uart_txen && !ss1_uart_txbusy)
        v |= 0x01;              // TxRDY
    if (ss1_uart_rx_valid)
        v |= 0x02;              // RxRDY
    v |= 0x04;                  // TxEMT
    trace(trace_ss1, "ss1: read uart status %02x\n", v);
    return v;
}

static void
wr_uart_cmd(portaddr p, byte v)
{
    ss1_uart_txen = v & 0x01;   // bit 0 = TxEN
    trace(trace_ss1, "ss1: write uart cmd %02x\n", v);
    ss1_uart_set_lines();
}

static void
wr_uart_data(portaddr p, byte v)
{
    /*
     * Sending on the 2651 claims the console, exactly as transmitting on
     * the multio's uart 0 does.  The SS1 is the wired controller on this
     * machine, so the claim normally changes nothing; it is here for the
     * same reason the multio's is - so that whichever character path the
     * bios actually uses is the one that owns the shared input.
     */
    console_claim(CONS_SS1);

    if (ss1_uart_outfd != -1)
        write(ss1_uart_outfd, &v, 1);
    /*
     * The 2651 is the console whenever the SS1 is the wired controller,
     * so its output is console output and belongs in the -L log the same
     * way the MultIO's ACE 0 does.  It is logged whether or not the xterm
     * was asked for, which is what makes an ss1 machine drivable and
     * legible headlessly.
     */
    console_log(v);
    ss1_uart_txbusy = 1;        // shifting out: TxRDY drops
    ss1_uart_set_lines();
}

static byte
rd_uart_data(portaddr p)
{
    byte v = ss1_uart_rx;

    if (ss1_uart_rx_valid) {
        ss1_uart_rx_valid = 0;  // consumed: RxRDY drops
        ss1_uart_set_lines();
    } else {
        v = 0;
    }
    return v;
}

/*
 * The periodic work: complete a transmit (which raises TxRDY and edges
 * its line) and pull a character from the xterm (which raises RxRDY).
 */
int
ss1_poll(void)
{
    int bytes;

    if (ss1_uart_txbusy) {
        ss1_uart_txbusy = 0;
        ss1_uart_set_lines();
    }

    /*
     * Only the device that owns the console may take a character out of
     * it.  Uart 0 on the multio and the floppy controller's serial port
     * read the same descriptor, and the SS1 may share it too; whichever
     * of them read first would leave the others waiting forever for a
     * keystroke that had already been delivered somewhere else.
     */
    if (console_owner != CONS_SS1)
        return 0;

    if (ss1_uart_infd != -1 && !ss1_uart_rx_valid) {
        if (ioctl(ss1_uart_infd, FIONREAD, &bytes) == 0 && bytes > 0) {
            if (read(ss1_uart_infd, &ss1_uart_rx, 1) == 1) {
                ss1_uart_rx_valid = 1;
                ss1_uart_set_lines();
            }
        }
    }
    return 0;
}

/*
 * The SIGUSR2 interrupt-controller dump.
 */
void
ss1_pic_dump(void)
{
    struct pic *m = &ss1_pic[0];
    struct pic *s = &ss1_pic[1];

    printf("\n== SS1 interrupt controller (8259) ==\n");
    printf("  master imr=%02x irr=%02x isr=%02x priority=%d\n",
        m->imr, m->irr, m->isr, m->priority);
    printf("  slave  imr=%02x irr=%02x isr=%02x priority=%d\n",
        s->imr, s->irr, s->isr, s->priority);
    printf("  ivec=%s iv_isrbit=%02x\n", ss1_ivs_name[ss1_ivecstate],
        ss1_iv_isrbit);
}

/*
 * The four 8259 register ports, decoded directly by A0.  These are thin
 * wrappers because register_input/output pass only the port and value;
 * the chip model takes the struct pic.
 */
static byte
rd_ss1_m8259_l(portaddr p) { return pic_read_cmd(&ss1_pic[0]); }
static byte
rd_ss1_m8259_h(portaddr p) { return pic_read_data(&ss1_pic[0]); }
static byte
rd_ss1_s8259_l(portaddr p) { return pic_read_cmd(&ss1_pic[1]); }
static byte
rd_ss1_s8259_h(portaddr p) { return pic_read_data(&ss1_pic[1]); }
static void
wr_ss1_m8259_l(portaddr p, byte v) { pic_write_cmd(&ss1_pic[0], v); }
static void
wr_ss1_m8259_h(portaddr p, byte v) { pic_write_data(&ss1_pic[0], v); }
static void
wr_ss1_s8259_l(portaddr p, byte v) { pic_write_cmd(&ss1_pic[1], v); }
static void
wr_ss1_s8259_h(portaddr p, byte v) { pic_write_data(&ss1_pic[1], v); }

static int
ss1_init(void)
{
    /*
     * Where the board is strapped.  0x70 is where ours is; 0x50 is
     * CompuPro's default and is also where the HDCA lives, which is why
     * moving it here and getting told about it is the point.
     */
    ss1_base = conf_board_hex("ss1", 0, "base", SS1_BASE);

    pic_init(&ss1_pic[0]);
    pic_init(&ss1_pic[1]);
    ss1_pic[0].name = "ss1 master";
    ss1_pic[1].name = "ss1 slave";
    ss1_pic[0].irbits = ss1_master_bits;
    ss1_pic[1].irbits = ss1_slave_bits;
    ss1_pic[0].line_changed = ss1_set_int_line;
    ss1_pic[1].line_changed = ss1_set_int_line;
#ifndef NODEBUG
    ss1_pic[0].trace_flag = trace_ss1;
    ss1_pic[1].trace_flag = trace_ss1;
#endif

    /* the board drives nothing when it is not the interrupt controller */
    if (!wiring_active("ss1"))
        return 0;

    register_input(ss1_base + SS1_M8259_L, &rd_ss1_m8259_l);
    register_input(ss1_base + SS1_M8259_H, &rd_ss1_m8259_h);
    register_input(ss1_base + SS1_S8259_L, &rd_ss1_s8259_l);
    register_input(ss1_base + SS1_S8259_H, &rd_ss1_s8259_h);
    register_output(ss1_base + SS1_M8259_L, &wr_ss1_m8259_l);
    register_output(ss1_base + SS1_M8259_H, &wr_ss1_m8259_h);
    register_output(ss1_base + SS1_S8259_L, &wr_ss1_s8259_l);
    register_output(ss1_base + SS1_S8259_H, &wr_ss1_s8259_h);

    register_output(ss1_base + SS1_8253_TC0, &wr_8253_count);
    register_output(ss1_base + SS1_8253_TC1, &wr_8253_count);
    register_output(ss1_base + SS1_8253_TC2, &wr_8253_count);
    register_output(ss1_base + SS1_8253_CTL, &wr_8253_ctl);

    register_output(ss1_base + SS1_RTC_CMD, &wr_rtc_cmd);
    register_input(ss1_base + SS1_RTC_DATA, &rd_rtc_data);
    register_output(ss1_base + SS1_RTC_DATA, &wr_rtc_data);

    register_input(ss1_base + SS1_UART_DATA, &rd_uart_data);
    register_input(ss1_base + SS1_UART_STAT, &rd_uart_stat);
    register_output(ss1_base + SS1_UART_DATA, &wr_uart_data);
    register_output(ss1_base + SS1_UART_CMD, &wr_uart_cmd);

    /* the console uart gets an xterm when the config asks for one */
    if ((config_sw & CONF_SET) && (config_sw & 0x100))
        open_terminal("ss1", 0, &ss1_uart_infd, &ss1_uart_outfd, 0, 0);

    /*
     * This is the machine's console, so publish it the way the multio
     * does: terminal_fd_in/out are the common character path that the
     * other drivers share, and the SS1 owns the input from reset.  The
     * ownership is set directly, not claimed, so an ss1 machine starts
     * quiet - there has been no change to report.
     */
    terminal_fd_in = ss1_uart_infd;
    terminal_fd_out = ss1_uart_outfd;
    console_owner = CONS_SS1;

    vi_change = &ss1_vi_change;
    get_intack = &ss1_intack;

    return 0;
}

static int
ss1_setup(void)
{
#ifndef NODEBUG
    trace_ss1 = register_trace("ss1");
#endif
    return 0;
}

static void
ss1_usage(void)
{
    printf("\tss1: CompuPro System Support 1 interrupt controller\n");
    printf("\tconfig switch values:\n");
    printf("\t0x0100 - the 2651 console in an xterm\n");
}

struct driver ss1_driver = {
    "ss1",
    &ss1_usage,
    &ss1_setup,
    &ss1_init,
    &ss1_poll
};

__attribute__((constructor))
void
register_ss1_driver(void)
{
    register_driver(&ss1_driver);
}
