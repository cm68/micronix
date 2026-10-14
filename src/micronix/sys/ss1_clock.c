/*
 * The CompuPro System Support 1 clock: the OKI MSM5832 calendar and the
 * 8253 interval-timer heartbeat.
 *
 * sys/ss1_clock.c
 *
 * The SS1 carries a different clock from the Wunderbus/MultIO: an OKI
 * MSM5832 real-time clock - thirteen BCD digits, second through year,
 * addressed a digit at a time through a command port and a data port -
 * and an Intel 8253 whose counter 0 is the heartbeat, clocked at 2 MHz
 * and wired to the slave 8259's IR1.  This is the SS1 counterpart of
 * wb_clock.c; the two provide the same three entry points - rtcinit,
 * rtcwrite, clockinit - and only one links.
 *
 * The MSM5832 HAS a year, where the uPD1990 does not, so rtcinit reads
 * the year from the chip and its base argument (the superblock timestamp
 * wb_clock.c borrows a year from) is unused.
 */
#include <types.h>
#include <sys/sys.h>

#define SS1_RTC_CMD   0x5a    /* command: digit select + Hold/Write/Read */
#define SS1_RTC_DATA  0x5b    /* data: one 4-bit BCD digit */

#define RTC_HOLD    0x40        /* bit 6: freeze the counters across the access */
#define RTC_WRITE   0x20        /* bit 5: write the data port into the digit */
#define RTC_READ    0x10        /* bit 4: read the digit out the data port */

extern int in(), out();
extern int di(), ei();
extern long seconds;
extern long bcd2unix();         /* unbcd.s, BCD -> unix */
extern void unixtobcd();        /* unixtobcd.s, unix -> BCD */

/*
 * The MSM5832's thirteen digits, in order: second ones, second tens,
 * minute ones, minute tens, hour ones, hour tens (with an AM/PM bit),
 * weekday, day ones, day tens, month ones, month tens, year ones, year
 * tens.  Read one: Hold|Read|digit to the command port, then the data
 * port's low nibble.  Write one: Hold|Write|digit, then the nibble.
 */
static int
rdigit(int d)
{
    out(SS1_RTC_CMD, RTC_HOLD | RTC_READ | d);
    return in(SS1_RTC_DATA) & 0x0f;
}

static void
wdigit(int d, int v)
{
    out(SS1_RTC_CMD, RTC_HOLD | RTC_WRITE | d);
    out(SS1_RTC_DATA, v & 0x0f);
}

/*
 * Read the calendar into b[6] as packed BCD, the shape bcd2unix takes:
 * year (2 digits), month (1-based), day, hour, minute, second.  The
 * hour-tens digit's high bit is the AM/PM/24-hour flag, so only its low
 * two bits are the tens.
 */
static void
rtcget(UINT8 *b)
{
    b[0] = (rdigit(12) << 4) | rdigit(11);          /* year */
    b[1] = (rdigit(10) << 4) | rdigit(9);           /* month, 1-based */
    b[2] = (rdigit(8)  << 4) | rdigit(7);           /* day */
    b[3] = ((rdigit(5) & 0x03) << 4) | rdigit(4);   /* hour, 24h */
    b[4] = (rdigit(3)  << 4) | rdigit(2);           /* minute */
    b[5] = (rdigit(1)  << 4) | rdigit(0);           /* second */
}

/*
 * Write b[6] (packed BCD) back into the calendar.  The two second digits
 * are read-only in the chip, so their writes are ignored; the weekday is
 * not kept; the hour-tens gets the 24-hour flag bit.
 */
static void
rtcset(UINT8 *b)
{
    wdigit(0,  b[5] & 0x0f);            /* second */
    wdigit(1,  b[5] >> 4);
    wdigit(2,  b[4] & 0x0f);            /* minute */
    wdigit(3,  b[4] >> 4);
    wdigit(4,  b[3] & 0x0f);            /* hour */
    wdigit(5,  (b[3] >> 4) | 0x08);     /* hour tens, 24-hour mode */
    wdigit(6,  0);                      /* weekday */
    wdigit(7,  b[2] & 0x0f);            /* day */
    wdigit(8,  b[2] >> 4);
    wdigit(9,  b[1] & 0x0f);            /* month */
    wdigit(10, b[1] >> 4);
    wdigit(11, b[0] & 0x0f);            /* year */
    wdigit(12, b[0] >> 4);
}

/*
 * Set the system time from the calendar.  base is unused: the year comes
 * from the chip, so there is no rollover year to derive from it.
 */
void
rtcinit(long base)
{
    UINT8 b[6];
    long t;

    rtcget(b);
    t = bcd2unix(b);
    di();
    seconds = t;
    ei();
}

/*
 * Write the system time to the calendar.
 */
void
rtcwrite(long t)
{
    UINT8 b[6];

    unixtobcd(t, b);
    rtcset(b);
}
