/*
 * the clock is bit-banged, a 1990 clock/calendar chip.
 */
#include "sim.h"
#include "util.h"
#include <time.h>
#include <stdio.h>

#define CLK_DATA    0x01    // the data shifts in and out here
#define CLK_SHIFT   0x02    // the shift register strobe
#define CLK_CMD     0x1c    // command bits
#define     CC_SHOLD    0x00    // shift register hold
#define     CC_ENSR     0x04    // enable shift register
#define     CC_SET      0x08    // load clock from shift register
#define     CC_GET      0x0c    // read clock to shift register
#define     CC_64HZ     0x10
#define     CC_256HZ    0x14
#define     CC_2kKHZ    0x18
#define     CC_32HZ     0x1c    // really test mode
#define CLK_SETCMD  0x20    // strobe the command

#define  bcd(h,l)    ((((h) & 0xf) << 4) | ((l) & 0xf))

/*
 * the real time clock is latched this 40 bit structure
 */
static byte rtc[5] = { 0x25, 0x34, 0x12, 0x25, 0x76 };
static int rtcptr;
static byte last_wrclock;
static time_t now;
static char *clk_cmd[] = { 
    "SHOLD", "ENSR", "SET", "GET", 
    "64Hz", "256Hz", "2kHz", "32Hz"
};
static char *wclk_bits[] = { "DATA", "DSTROBE", 0, 0, 0, "CSTROBE", 0, 0 };
static int_line clk_interrupt;
int trace_1990;

void
clock_handler()
{
    set_interrupt(clk_interrupt, int_set);
}

/*
 * the bit banging works like this:
 * the program sets strobe low, with command set, 
 * then it sets it high, again with the same command,
 * then it sets it low again
 * I'm going to be lazy and only notice the high to low transitions
 */
static void
wr_clock(portaddr p, byte v)
{
    struct tm *tm;
    int rate = 0;

    if (trace & trace_1990) {
        printf("1990: write clock %02x %s %s\n", 
            v, clk_cmd[(v & CLK_CMD) >> 2], bitdef(v, wclk_bits));
    }

    // if falling edge on CSTROBE and command the same, do a command
    if ((!(v & CLK_SETCMD)) && (last_wrclock & CLK_SETCMD) &&
        ((v & CLK_CMD) == (last_wrclock & CLK_CMD))) {
        switch (v & CLK_CMD) {
        case CC_SHOLD:      // do nothing with shift register
            break;
        case CC_ENSR:       // reset shift register
            rtcptr = 0;
            break;
        case CC_SET:        // set time - no, we not going to do that
            break; 
        case CC_GET:        // read the unix time and populate the array
            now = time(&now);
            tm = localtime(&now);
            rtc[0] = bcd(tm->tm_sec / 10, tm->tm_sec % 10);
            rtc[1] = bcd(tm->tm_min / 10, tm->tm_min % 10);
            rtc[2] = bcd(tm->tm_hour / 10, tm->tm_hour % 10);
            rtc[3] = bcd(tm->tm_mday / 10, tm->tm_mday % 10);
            rtc[4] = bcd(tm->tm_mon, tm->tm_wday);
            break;
        case CC_64HZ:
            rate = 64;
            break;
        case CC_256HZ:
            rate = 256;
            break;
        case CC_2kKHZ:
            rate = 2048;
            break;
        case CC_32HZ:           // test mode in the real chip. tricky.
            rate = 32;
            break;
        }
#ifdef notdef
        if ((config_sw & CONF_SET) && (config_sw & HZ_1)) {
            rate = 1;
        }
#endif
        if (rate != 0) {
            recurring_timeout("multio_clock", rate, clock_handler, 0);
        }
    }

    /*
     * if falling edge on DSTROBE and command the same, 
     * advance the data bit pointer
     */
    if ((!(v & CLK_SHIFT)) && (last_wrclock & CLK_SHIFT) &&
        ((v & CLK_CMD) == (last_wrclock & CLK_CMD))) {
        if (++rtcptr == 40) {
            rtcptr = 0;
        }
    }
    last_wrclock = v;
}

static byte 
rd_clock(portaddr p)
{
    byte v;

    set_interrupt(clk_interrupt, int_clear);

    v = (rtc[rtcptr / 8] >> (rtcptr % 8)) & 1;
    if (trace & trace_1990) 
        printf("1990: read clock %02x\n", v);
    return v;
}

void
create_1990(portaddr p, int_line line)
{
    clk_interrupt = line;
}

void
activate_1990(byte port)
{
    register_input(port, &rd_clock);
    register_output(port, &wr_clock);
}

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))    // call before main()
void
register_1990_driver()
{
    trace_1990 = register_trace("1990");
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
