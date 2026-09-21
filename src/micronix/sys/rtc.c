/*
 * real time clock
 *
 * sys/rtc.c
 *
 * The Master Mult I/O board carries a NEC uPD1990 calendar/clock, a 40-bit
 * bit-banged shift register (seconds, minutes, hours, day-of-month, month and
 * weekday, BCD) at port 0x4a (MBASE+2).  There is no year.  The epoch<->date
 * math lives in unbcd.s (BCD -> unix) and unixtobcd.s (unix -> BCD),
 * wrapped for the ccc long ABI by rtcglue.s.
 *
 * The 1990's month field is plain binary, 0-based (0..11), packed with the
 * weekday in the low nibble of the same byte; bcd2unix()/unixtobcd() use a
 * 1-based packed-BCD month (0x01..0x12), so this file reorders the fields.
 *
 * The write port bits: CLK_DATA (0x01) is the data line, CLK_SHIFT (0x02) the
 * DSTROBE that advances the shift register, CLK_CMD (0x1c) the 3-bit command,
 * and CLK_SETCMD (0x20) the CSTROBE that strokes a command.  A command fires on
 * the CSTROBE falling edge; a bit shifts on the DSTROBE falling edge, lsb first.
 *
 * Commands: CC_ENSR (enable the shift register), CC_SET (load the clock from the
 * shift register), CC_GET (read the clock into the shift register).
 */
#include <types.h>
#include <sys/sys.h>

#define CLOCKPORT   0x4a            /* MBASE + 2 */

#define CLK_DATA    0x01
#define CLK_SHIFT   0x02            /* DSTROBE */
#define CLK_CMD     0x1c            /* command bits */
#define CLK_SETCMD  0x20            /* CSTROBE */

#define CC_ENSR     0x04            /* enable shift register */
#define CC_SET      0x08            /* shift register -> clock */
#define CC_GET      0x0c            /* clock -> shift register */

extern int in(), out();
extern int di(), ei();
extern long seconds;
extern long bcd2unix();         /* rtcglue.s -> unbcd.s */
extern void unixtobcd();        /* rtcglue.s -> unixtobcd.s */

/* stroke a command: cmd, cmd|CSTROBE, cmd (falling edge commits) */
static void
clkcmd(int cmd)
{
    out(CLOCKPORT, cmd);
    out(CLOCKPORT, cmd | CLK_SETCMD);
    out(CLOCKPORT, cmd);
}

/* pulse DSTROBE: high then low, advancing the shift register */
static void
clkshift(void)
{
    out(CLOCKPORT, CLK_SHIFT);
    out(CLOCKPORT, 0);
}

/* read the 40-bit rtc into r[5]: sec, min, hour, day, mon+wday (bcd) */
static void
rtcget(UINT8 *r)
{
    int b;

    clkcmd(CC_ENSR);
    clkcmd(CC_GET);
    for (b = 0; b < 5; b++)
        r[b] = 0;
    for (b = 0; b < 40; b++) {
        if (in(CLOCKPORT) & 1)
            r[b / 8] |= 1 << (b % 8);
        clkshift();
    }
}

/* write the 40-bit rtc from r[5] */
static void
rtcset(UINT8 *r)
{
    int b;

    clkcmd(CC_ENSR);
    for (b = 0; b < 40; b++) {
        int bit = (r[b / 8] >> (b % 8)) & 1;

        out(CLOCKPORT, (bit ? CLK_DATA : 0) | CLK_SHIFT);
        out(CLOCKPORT, (bit ? CLK_DATA : 0));
    }
    clkcmd(CC_SET);
}

/*
 * Set the system time from the rtc, taking the year from the superblock
 * timestamp (the 1990 has no year).  The year is chosen so the resulting
 * epoch lands at or after base; a rollover since the last umount simply
 * advances the year.
 */
void
rtcinit(long base)
{
    UINT8 r[5];
    UINT8 b[6];
    long t;
    int y, m;

    rtcget(r);
    unixtobcd(base, b);             /* b[0] = year of the superblock stamp */
    y = b[0];
    m = r[4] >> 4;                  /* 1990 month, 0-based (0..11) */
    for (;;) {
        b[0] = y;
        if (m >= 9)
            b[1] = m + 7;           /* 0-based -> 1-based packed BCD */
        else
            b[1] = m + 1;
        b[2] = r[3];                /* day */
        b[3] = r[2];                /* hour */
        b[4] = r[1];                /* minute */
        b[5] = r[0];                /* second */
        t = bcd2unix(b);
        if (t >= base)
            break;
        /* the 1990's calendar is still last year's: bump the 2-digit BCD year */
        if (y == 0x99)
            y = 0x00;
        else if ((y & 0x0f) == 9)
            y = y + 7;
        else
            y = y + 1;
    }
    di();
    seconds = t;
    ei();
}

/*
 * Write the system time to the rtc (month and weekday, no year).
 */
void
rtcwrite(long t)
{
    UINT8 r[5];
    UINT8 b[6];
    int m;

    unixtobcd(t, b);                /* b = year,mon,day,hour,min,sec (BCD) */
    m = (b[1] & 0x0f) + (b[1] >= 0x10 ? 10 : 0);    /* 1..12 */
    r[0] = b[5];                    /* second */
    r[1] = b[4];                    /* minute */
    r[2] = b[3];                    /* hour */
    r[3] = b[2];                    /* day */
    r[4] = (m - 1) << 4;            /* month 0-based, high nibble; wday 0 */
    rtcset(r);
}
