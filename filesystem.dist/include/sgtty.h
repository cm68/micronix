struct sgtty
	{
	UTINY	ispeed, ospeed;
	TEXT	erase, kill;
	UCOUNT	mode;
	};

# define B50	 1
# define B75	 2
# define B110	 3
# define B134	 4
# define B150	 5
# define B200	 6
# define B300	 7
# define B600	 8
# define B1200	 9
# define B1800	10
# define B2400	11
# define B4800	12
# define B9600	13
# define B19200	14
# define B19200 14

# define BS0	0000000
# define BS1	0100000

# define BS	0100000
# define SHAKE	0100000

# define FF0	000000
# define FF1	040000

# define FF	040000

# define CR0	000000
# define CR1	010000
# define CR2	020000
# define CR3	030000

# define CR	030000

# define TAB0	00000
# define TAB1	02000
# define TAB2	04000
# define TAB3	06000

# define TAB	06000

# define NL0	00000
# define NL1	00400
# define NL2	01000
# define NL3	01400

# define NL	01400


# define EVEN	0200


# define ODD	0100


# define RAW	040

# define CRMOD	020

# define ECHO	010

# define ULMOD	004

# define TABS	002

# define HUP	001
