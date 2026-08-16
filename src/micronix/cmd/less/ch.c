/*
 * Low level character input from the input file.
 * We use these special purpose routines which optimize moving
 * both forward and backward from the current read pointer.
 *
 * cmd/less/ch.c
 *
 * THE POOL IS PARALLEL SCALAR ARRAYS, and the original's circular
 * list of kilobyte structs is gone.  Not by preference - by
 * casualty count.  The struct spelling was ported four times over:
 * chained member stores, then plain temporaries, then counted
 * walks, then the loops hoisted into their own small functions,
 * and each version worked until an unrelated edit moved the stack
 * frame around it and c1 built some member access wrong again -
 * a chain pointer into the middle of the pool, a search that
 * matched address 1, an init stride of four bytes where sizeof
 * said a thousand.  Every one of those failures needed a kilobyte
 * struct: the big member offsets and the pointer-to-pointer stores
 * are what the generator keeps getting wrong, and no local
 * spelling of them stayed compiled correctly.
 *
 * What is left uses only shapes that have never broken: word
 * arrays indexed by a small int, and one flat character array
 * walked by a char pointer.  A buffer is a slot number.  Its data
 * is pool_data + slot * BUFSIZ, its block number is pool_block[i],
 * and recency is a clock stamp in pool_age[i] instead of a list -
 * the LRU victim is the smallest stamp, found by scanning at most
 * sixteen words, which on this machine costs less than the pointer
 * relinking it replaces ever did.
 */

#include "less.h"

public int file = -1;	/* File descriptor of the input file */

#define BUFSIZ	1024

/*
 * The pool.  Sixteen slots is sixteen kilobytes of bss; the b
 * command on a pipe can reach back that far and no farther, which
 * is why POOLBUFS wants to be as big as the image can afford.
 */
#define	POOLBUFS	16
#define	NOBLOCK		((unsigned)-1)

static char	pool_data[POOLBUFS * BUFSIZ];
static unsigned	pool_block[POOLBUFS];	/* which block, NOBLOCK = empty */
static unsigned	pool_age[POOLBUFS];	/* recency stamp, bigger = newer */
static unsigned	ch_clock;		/* the stamp source */

public int nbufs;

/*
 * The macro ch_get() checks these instead of chasing a chain head:
 * the data of the slot that satisfied the last request, and the
 * block it held.  Both are plain scalars on purpose.
 */
static char	*ch_dp;			/* data of the last-used slot */
static unsigned	ch_cblock = NOBLOCK;	/* block in that slot */

extern int clean_data;
extern int ispipe;
extern int sigs;

#if LOGFILE
extern int logfile;
#endif

/*
 * Current position in file.
 * Stored as a block number and an offset into the block.
 *
 * A word of block number addresses 64 megabytes of file; the
 * original carried a long, and long members were among the
 * casualties described above.
 */
static unsigned ch_block;
static int ch_offset;

/*
 * Length of file, needed if input is a pipe.
 */
static POSITION ch_fsize;

/*
 * Largest block number read if input is standard input (a pipe).
 */
static int last_piped_block;

#define	ch_get()	((ch_cblock == ch_block) ? \
				ch_dp[ch_offset] : fch_get())

static int fch_get();

/*
 * The data address of a slot: a word multiply and an add, in a
 * function small enough that the generator has never missed it.
 */
	static char *
ch_data(slot)
	int slot;
{
	return (pool_data + (unsigned)slot * BUFSIZ);
}

/*
 * Which slot holds a block; -1 if none does.
 */
	static int
ch_slot(want)
	unsigned want;
{
	register int i;

	for (i = 0;  i < nbufs;  i++)
		if (pool_block[i] == want)
			return (i);
	return (-1);
}

/*
 * The least recently used slot: the smallest clock stamp.
 */
	static int
ch_victim()
{
	register int i;
	register int v;
	unsigned best;

	v = 0;
	best = pool_age[0];
	for (i = 1;  i < nbufs;  i++)
		if (pool_age[i] < best)
		{
			best = pool_age[i];
			v = i;
		}
	return (v);
}

/*
 * Get the character pointed to by the read pointer,
 * reading its block into a pool slot if no slot holds it.
 */
	static int
fch_get()
{
	register char *dp;
	int slot;
	int n;
	int end;
	POSITION pos;

	slot = ch_slot(ch_block);
	if (slot >= 0)
	{
		dp = ch_data(slot);
		goto found;
	}

	/*
	 * Block is not in a buffer.
	 * Take the least recently used slot
	 * and read the desired block into it.
	 */
	slot = ch_victim();
	pool_block[slot] = ch_block;
	pos = (POSITION)ch_block * BUFSIZ;
	if (ispipe)
	{
		/*
		 * The block requested should be one more than
		 * the last block read.
		 */
		if ((int)ch_block != ++last_piped_block)
		{
			/*
			 * A pipe cannot be reseeked, so a block that has
			 * left the pool is gone; this is where a b that
			 * reached back past POOLBUFS worth of input ends
			 * up.  The original called it "should not happen"
			 * and printed the two block numbers through a
			 * sprintf; the message is static now - the numbers
			 * never told the user anything the sentence does
			 * not, and the sprintf came out unformatted in
			 * this frame anyway.
			 */
			error("cannot go back that far in a pipe");
			quit();
		}
	} else
		lseek(file, pos, 0);

	/*
	 * Read the block.  This may take several reads if the input
	 * is coming from standard input, due to the nature of pipes.
	 */
	dp = ch_data(slot);
	end = 0;
	while ((n = read(file, dp + end, BUFSIZ-end)) > 0)
		if ((end += n) >= BUFSIZ)
			break;

	if (n < 0)
	{
		error("read error");
		quit();
	}

#if LOGFILE
	/*
	 * If we have a log file, write this block to it.
	 */
	if (logfile >= 0 && end > 0)
		write(logfile, dp, end);
#endif

	/*
	 * Set an EOF marker in the buffered data itself.
	 * Then ensure the data is "clean": there are no
	 * extra EOF chars in the data and that the "meta"
	 * bit (the 0200 bit) is reset in each char.
	 */
	if (end < BUFSIZ)
	{
		ch_fsize = pos + end;
		dp[end] = EOF;
	}

	if (!clean_data)
		while (--end >= 0)
		{
			dp[end] &= 0177;
			if (dp[end] == EOF)
				dp[end] = '@';
		}

    found:
	pool_age[slot] = ++ch_clock;
	ch_dp = dp;
	ch_cblock = ch_block;
	return (dp[ch_offset]);
}

#if LOGFILE
/*
 * Close the logfile.
 * If we haven't read all of standard input into it, do that now.
 */
	public void
end_logfile()
{
	static int tried;

	if (logfile < 0)
		return;
	if (!tried && ch_fsize == NULL_POSITION)
	{
		tried = 1;
		lower_left();
		clear_eol();
		so_enter();
		puts("finishing logfile... (interrupt to abort)");
		so_exit();
		flush();
		while (sigs == 0 && ch_forw_get() != EOF)
			;
	}
	close(logfile);
	logfile = -1;
}
#endif

/*
 * Determine if a specific block is currently in one of the buffers.
 */
	static int
buffered(block)
	unsigned block;
{
	return (ch_slot(block) >= 0);
}

/*
 * Seek to a specified position in the file.
 * Return 0 if successful, non-zero if can't seek there.
 */
	public int
ch_seek(pos)
	register POSITION pos;
{
	unsigned new_block;

	new_block = pos / BUFSIZ;
	if (!ispipe || (int)new_block == last_piped_block + 1 || buffered(new_block))
	{
		/*
		 * Set read pointer.
		 */
		ch_block = new_block;
		ch_offset = pos % BUFSIZ;
		return (0);
	}
	return (1);
}

/*
 * Seek to the end of the file.
 */
	public int
ch_end_seek()
{
	if (ispipe)
	{
		/*
		 * Do it the slow way: read till end of data.
		 */
		while (ch_forw_get() != EOF)
			;
	} else
	{
		(void) ch_seek((POSITION)(lseek(file, (off_t)0, 2)));
	}
	return (0);
}

/*
 * Seek to the beginning of the file, or as close to it as we can get.
 * We may not be able to seek there if input is a pipe and the
 * beginning of the pipe is no longer buffered.
 */
	public int
ch_beg_seek()
{
	register int i;
	unsigned low;

	/*
	 * Try a plain ch_seek first.
	 */
	if (ch_seek((POSITION)0) == 0)
		return (0);

	/*
	 * Can't get to position 0.
	 * Look for the buffered block closest to position 0.
	 */
	low = NOBLOCK;
	for (i = 0;  i < nbufs;  i++)
		if (pool_block[i] < low)
			low = pool_block[i];
	if (low == NOBLOCK)
		return (1);
	ch_block = low;
	ch_offset = 0;
	return (0);
}

/*
 * Return the length of the file, if known.
 */
	public POSITION
ch_length()
{
	if (ispipe)
		return (ch_fsize);
	return ((POSITION)(lseek(file, (off_t)0, 2)));
}

/*
 * Return the current position in the file.
 */
	public POSITION
ch_tell()
{
	return ((POSITION)ch_block * BUFSIZ + ch_offset);
}

/*
 * Get the current char and post-increment the read pointer.
 */
	public int
ch_forw_get()
{
	register int c;

	c = ch_get();
	if (c != EOF && ++ch_offset >= BUFSIZ)
	{
		ch_offset = 0;
		ch_block ++;
	}
	return (c);
}

/*
 * Pre-decrement the read pointer and get the new current char.
 */
	public int
ch_back_get()
{
	register int c;

	if (--ch_offset < 0)
	{
		if (ch_block == 0 || (ispipe && !buffered(ch_block-1)))
		{
			ch_offset = 0;
			return (EOF);
		}
		ch_offset = BUFSIZ - 1;
		ch_block--;
	}
	c = ch_get();
	return (c);
}

/*
 * Initialize the buffer pool to all empty.
 * Caller suggests that we use want_nbufs buffers.
 */
	public void
ch_init(want_nbufs)
	int want_nbufs;
{
	register int i;

	/*
	 * The pool is static; all there is to "allocate" is
	 * deciding how much of it to mark empty.
	 */
	if (want_nbufs > POOLBUFS)
		want_nbufs = POOLBUFS;
	if (nbufs < want_nbufs)
		nbufs = want_nbufs;

	for (i = 0;  i < nbufs;  i++)
	{
		pool_block[i] = NOBLOCK;
		pool_age[i] = 0;
	}
	ch_clock = 0;
	ch_cblock = NOBLOCK;
	last_piped_block = -1;
	ch_fsize = NULL_POSITION;
	(void) ch_seek((POSITION)0);
}
