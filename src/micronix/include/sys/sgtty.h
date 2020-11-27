/*
 * sgtty.h
 * 
 * communicate with the terminal driver
 */
struct sgtty {
    UINT8 ispeed;
    UINT8 ospeed;
    char erase;
    char kill;
    UINT mode;
};

#define B50 	 1
#define B75 	 2
#define B110	 3
#define B134	 4
#define B150	 5
#define B200	 6
#define B300	 7
#define B600	 8
#define B1200	 9
#define B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define B19200	14

/*
 * tty mode bits - note that micronix does not use most of this, and in
 * fact, appropriates some bits to do strange things like 'more'
 * see the 'Mode bits' section of tty.h
 * this may be what the substantive changes in the works between
 * micronix 1.4 and 1.6
 */
#define BS  	0100000     /* backspace delay */
#define BS0	    0000000
#define BS1 	0100000
#define SHAKE	0100000

#define FF  	0040000     /* form feed delay */
#define FF0 	0000000
#define FF1 	0040000

#define CR  	0030000     /* carriage return delay */
#define CR0 	0000000
#define CR1 	0010000
#define CR2 	0020000
#define CR3 	0030000

#define TAB 	0006000     /* tab delay */
#define TAB0	0000000
#define TAB1	0002000
#define TAB2	0004000
#define TAB3	0006000

#define NL  	0001400     /* newline delay */
#define NL0 	0000000
#define NL1 	0000400
#define NL2 	0001000
#define NL3 	0001400

#define EVEN	0000200     /* allow even parity on input */
#define ODD 	0000100     /* allow odd parity in input */

#define RAW 	0000040     /* 8 bit, 1 character read */
#define CRMOD	0000020     /* map cr->lf, send cr-lf on cr or lf */
#define ECHO	0000010     /* echo input characters */
#define ULMOD	0000004     /* map upper case to lower case on input */
#define TABS	0000002     /* print tabs as spaces */
#define HUP	    0000001     /* drop dtr on last close */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
