/*
 * unlz4 - decompress the LZ4 frame format
 *
 * cmd/unlz4/unlz4.c
 *
 * The reverse of lz4: reads one LZ4 frame (magic, FLG, BD, optional
 * content-size/dictionary, header checksum, blocks, end mark) and writes
 * the original bytes.  The frame is streamed block by block; the only
 * state is a history ring of the last LZ4_WINDOW output bytes, which is
 * all a match can reach back into.  Offsets beyond that are rejected,
 * matching the cap the compressor imposes - see lz4.c.  The header
 * checksum is read and discarded, not verified; the magic number is the
 * guard against misreading a non-LZ4 stream.
 *
 * Dual build, same as lz4.c: ccc for the guest (unlz4), the host cc for
 * bin/mxunlz4, from this one source.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define LZ4_WINDOW	16384
#define LZ4_MASK	(LZ4_WINDOW - 1)
#define LZ4_MAXBLK	0x400000L	/* the format's 4 MB block ceiling */
#define MINMATCH	4
#define IOBSIZE		4096

static unsigned char	hist[LZ4_WINDOW];	/* last LZ4_WINDOW output bytes */
static unsigned char	ibuf[IOBSIZE];
static int		ipos, ilen;
static unsigned char	obuf[IOBSIZE];
static int		on;
static long		outpos;		/* total bytes written, drives the ring */
static long		blockleft;	/* compressed bytes left in this block */
static int		ifd = 0, ofd = 1;

static void
die(const char *m)
{
	(void)write(2, m, strlen(m));
	(void)write(2, "\n", 1);
	exit(1);
}

static void
flushout(void)
{
	if (on > 0) {
		if (write(ofd, (char *)obuf, on) != on)
			die("unlz4: write error");
		on = 0;
	}
}

/*
 * One byte from the input, buffered.  Returns -1 at end of input.
 */
static int
rdbyte(void)
{
	if (ipos >= ilen) {
		ilen = read(ifd, (char *)ibuf, IOBSIZE);
		if (ilen < 0)
			die("unlz4: read error");
		if (ilen == 0)
			return -1;
		ipos = 0;
	}
	return ibuf[ipos++];
}

/*
 * One byte from the current block.  Stops at the block boundary, which
 * is how the block decoder knows its block has ended; bytes beyond are
 * left in ibuf for the next block header to pick up.
 */
static int
getc_block(void)
{
	if (blockleft <= 0)
		return -1;
	blockleft--;
	return rdbyte();
}

static void
skip(int n)
{
	while (n-- > 0)
		if (rdbyte() < 0)
			die("unlz4: truncated input");
}

/*
 * One output byte: into the history ring (for future matches) and out
 * through the output buffer.
 */
static void
putout(int c)
{
	hist[outpos & LZ4_MASK] = (unsigned char)c;
	outpos++;
	if (on >= IOBSIZE)
		flushout();
	obuf[on++] = (unsigned char)c;
}

/*
 * A match of len bytes at offset: byte-by-byte, because the source may
 * overlap the destination (offset 1 repeats the last byte).  Every byte
 * produced also lands in the ring, so overlap reads what was just written.
 */
static void
copymatch(unsigned offset, int len)
{
	int k;

	for (k = 0; k < len; k++) {
		long src = outpos - (long)offset;
		putout((int)hist[src & LZ4_MASK]);
	}
}

/*
 * One compressed block, whose compressed bytes are the next blockleft of
 * the input.  The final sequence of a block is literals alone; probing
 * for the offset and finding the block exhausted is how that is told
 * from a match.
 */
static void
decodeblock(void)
{
	for (;;) {
		int token = getc_block();
		int litlen, matchlen, b;
		unsigned offset;

		if (token < 0)
			break;

		litlen = token >> 4;
		if (litlen == 15) {
			do {
				b = getc_block();
				if (b < 0)
					die("unlz4: truncated input");
				litlen += b;
			} while (b == 255);
		}
		while (litlen-- > 0) {
			b = getc_block();
			if (b < 0)
				die("unlz4: truncated input");
			putout(b);
		}

		b = getc_block();
		if (b < 0)
			break;			/* literals only: end of block */
		offset = (unsigned)b;
		b = getc_block();
		if (b < 0)
			die("unlz4: truncated input");
		offset |= (unsigned)b << 8;
		if (offset == 0)
			die("unlz4: bad offset");
		if (offset > LZ4_MASK)
			die("unlz4: offset too large");

		matchlen = (token & 0x0F) + MINMATCH;
		if ((token & 0x0F) == 15) {
			do {
				b = getc_block();
				if (b < 0)
					die("unlz4: truncated input");
				matchlen += b;
			} while (b == 255);
		}
		copymatch(offset, matchlen);
	}
}

static void
decompress(void)
{
	int b0, b1, b2, b3, flg;
	unsigned long sz;

	b0 = rdbyte(); b1 = rdbyte(); b2 = rdbyte(); b3 = rdbyte();
	if (b0 != 0x04 || b1 != 0x22 || b2 != 0x4D || b3 != 0x18)
		die("unlz4: not an lz4 frame");
	flg = rdbyte();
	(void)rdbyte();				/* BD, unused */

	if (flg & 0x08)
		skip(8);			/* content size */
	if (flg & 0x01)
		skip(4);			/* dictionary id */
	(void)rdbyte();				/* header checksum, unverified */

	for (;;) {
		sz = (unsigned long)rdbyte();
		sz |= (unsigned long)rdbyte() << 8;
		sz |= (unsigned long)rdbyte() << 16;
		sz |= (unsigned long)rdbyte() << 24;
		if (sz == 0)
			break;			/* end mark */

		blockleft = (long)(sz & 0x7FFFFFFFL);
		if (blockleft > LZ4_MAXBLK)
			die("unlz4: block too large");

		if (sz & 0x80000000L) {
			/* stored raw */
			while (blockleft > 0) {
				int c = getc_block();
				if (c < 0)
					die("unlz4: truncated input");
				putout(c);
			}
		} else {
			decodeblock();
		}

		if (flg & 0x10)
			skip(4);		/* block checksum */
	}
	if (flg & 0x04)
		skip(4);			/* content checksum */
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	char *in, *out;

	in = "-";
	out = "-";
	if (argc > 3)
		die("unlz4: usage: unlz4 [infile [outfile]]");
	if (argc >= 2)
		in = argv[1];
	if (argc >= 3)
		out = argv[2];

	if (strcmp(in, "-") != 0) {
		ifd = open(in, 0);
		if (ifd < 0)
			die("unlz4: cannot open input");
	}
	if (strcmp(out, "-") != 0) {
		ofd = creat(out, 0666);
		if (ofd < 0)
			die("unlz4: cannot create output");
	}

	decompress();
	flushout();
	exit(0);
}
