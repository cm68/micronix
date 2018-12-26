
# define equal(a,b) cmpstr(a,b)
# define scopy(a,b) cpystr(b,a)
# define NEWLINE '\n'
# define RETURN  '\r'

# define C_OPEN		15
# define C_CLOSE	16
# define C_DELETE	19
# define C_READ		33
# define C_WRITE	34
# define C_SELECT	14
# define C_ERROR	-1
# define C_MAKE		22
# define C_CONIN	1
# define C_CONREAD	10
# define C_CONWRITE	2
# define C_PRINT	9
# define C_DMA		26
# define C_RDR		3
# define C_PUNCH	4
# define C_LIST		5
# define C_STAT		11
# define C_SEARCH	17
# define C_SIZE		35
# define C_IO		6
# define C_IOIN		-1

# define E_NODATA	1
# define E_CANTCLOSE	3
# define E_NOEXT	4
# define E_PASTEND	6


# define CONSOLE	'C'
# define LIST		'L'
# define PUNCH		'P'
# define READER		'T'
# define DISK		'D'
# define CLOSED		 (TEXT) 0

# define NFILES		16
# define ERROR		-1
# define OK		0

# define BLANK		' '
# define COLON		':'
# define DOT		'.'
# define ENDOFTEXT	((TEXT) 26)

# define RECSIZE	128
# define CHARMASK	127

struct fcb
	{
	TEXT	drive,
		
		filename [8],
		filetype [3],

		ex,

		s1,
		s2,

		rcount,

		dn [16],

		cr,

		random  [3];
	};


struct file
	{
	TINY	mode;	/* READ WRITE UPDATE */

	ULONG	offset;

	TINY 	type;	/* CONSOLE READER PUNCH LIST DISK CLOSED */

	BYTES	rsize;

	struct fcb fcb;
	};


struct file _files [NFILES];
