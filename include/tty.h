       /*
	* These parameters control the size of the tty structure
	* input and output queues.
	*/
#define CSIZE	900
#define TTYHOG	200		/* Queue size in bytes (not more that 255) */
#define LOWATER	 16		/* Queue low water mark	 */
#define HIWATER (TTYHOG - 16)	/* Queue high water mark */

       /*
	* Special characters
	*/
#define BACKGRND 02		/* cntrl-b background signal */
#define EOT	004		/* End Of Transmission (cntrl D) */
#define XON	021		/* sent to input devices to restart input */
#define XOFF	023		/* sent by printers to hold up output */
#define STOP	033		/* Escape */
#define QUIT	034		/* Quit (cntrl \) */
#define BSLASH	0134		/* Back slash \ */
#define RUB	0177		/* Rubout or Delete */
#define HIGHBIT 0200		/* non-ascii */


	/*
	 * c - list element
	 *	2 bytes of pointer, 14 bytes of data
	 */

struct cblock
	{
	struct cblock *next;
	char block[14];
	};

       /*
	* Terminal circular queue
	*/
struct que
	{
	UCHAR		count;
	char	       *first;
	char	       *last;
	};

       /*
	* Terminal control structure.
	* The first 6 bytes are used by stty/gtty.
	* The dev and state members are used in mio.s.
	*/
struct tty
	{
	UCHAR		ispeed;		/* input baud rate */
	UCHAR		ospeed;		/* output baud rate */
	UCHAR		erase;		/* erase character */
	UCHAR		kill;		/* kill character */
	UINT		mode;		/* see below */
	UCHAR		state;		/* see below */
	UCHAR		col;		/* printhead waiting */
	UINT		dev;		/* device number, tty + 8 */
	UCHAR		line;		/* first line after a return is 0 */
	int	      (*start)();	/* enable output interrupt */
	int	      (*stop)();	/* disable output interrupt */
	int	      (*put)();		/* print a character */
	int	      (*set)();		/* set new baud rate */
	UCHAR		nextc;		/* for tab and newline expansion */
	UCHAR		nbreak;
	struct que	rawque;		/* raw input queue */
	struct que	cokque;		/* cooked input queue */
	struct que	outque;		/* output queue */
	UCHAR		count;		/* see below */
	UCHAR		mstate;		/* modem related state */
	};

       /*
	* Mode bits
	*/
#define SHAKE	0100000		/* Use RS-232 clear-to-send line */
#define ALL8	040000		/* Keep all 8 bits of input data */
#define CBREAK	020000		/* raw with rub, quit, start, stop */
#define MORE	010000		/* pause after 23 lines of output */
#define RAW	040		/* Raw input mode */
#define MAPCR	020		/* cr->lf mapping */
#define ECHO	010		/* Echo input */
#define OLDTTY	004		/* Map uppercase->lower, etc. */
#define XTABS	002		/* Expand tabs via spaces */

       /*
	* State bits
	*/
#define LOSTOP	001		/* low level output stopped (esc) */
#define HOSLEEP 002		/* high level output sleeping */
#define HISLEEP 004		/* high level input sleeping */
#define STOPIN	010		/* send an XOFF at next opportunity */
#define INSTOP	020		/* an XOFF has been sent */
#define STARTIN 040		/* send an XON at next opportunity */
#define OPEN	0100		/* tty is open. See _mstop in mio.s */
#define ERROR	0200		/* input error - wrong baud rate */
#define WOPEN	0400		/* waiting for open */

	/*
	 * Modem control related state bits
	 */

#define WOPEN	001		/* waiting for CARRIER DETECT */
#define CD	002		/* CARRIER DETECT is on */
#define DTR	004		/* assert DATA TERMINAL READY */
#define DIALER	040		/* don't wait for carrier */
