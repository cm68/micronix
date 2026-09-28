/*
 * lz4 - compress a byte stream into the LZ4 frame format
 *
 * cmd/lz4/lz4.c
 *
 * Writes a single LZ4 frame (the format `lz4 file` produces, not the
 * raw block format): a 7-byte header, then a sequence of independent
 * blocks, then a 4-byte end mark.  A frame, unlike a raw block, does
 * not need its whole input buffered: each block is self-contained and
 * may end in literals, so the input is compressed block by block with
 * bounded memory.
 *
 * The header is the minimal legal frame: FLG 0x40 (version 01,
 * independent blocks, no content size, no checksums, no dictionary) and
 * BD 0x04 (64 KB maximum block; the blocks here are LZ4_BLOCK, well
 * under that).  The one-byte header checksum is xxHash-32 of the six
 * descriptor bytes, shifted right 8 - that is what makes `lz4 -d`
 * accept the result.  mxlz4 (the host build of this same source) writes
 * the same format, so a stream made here is readable by unlz4, by
 * mxunlz4, and by the standard lz4 tool, unchanged.
 *
 * The one deliberate narrowing, inherited from the block format this
 * sits on: a match offset never exceeds LZ4_WINDOW-1 (16383).  Standard
 * LZ4 allows 65535; this caps it so the decoder's history ring fits a
 * 64K address space.  A standard decoder reads these small offsets
 * without complaint; our decoder rejects larger ones, which is the only
 * direction in which the two do not fully interoperate.
 *
 * Dual build: compiled by ccc for the guest (lz4), and by the host cc
 * for bin/mxlz4.  The body is plain C plus read/write/memcpy/memset -
 * no floating point.  The guest's int is 16 bits, so 32-bit quantities
 * are carried in unsigned long and assembled byte by byte.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <types.h>

#define LZ4_BLOCK	16384		/* input per block; power of two */
#define LZ4_WINDOW	16384		/* max match offset; power of two */
#define LZ4_MASK	(LZ4_WINDOW - 1)
#define LZ4_HSIZE	4096		/* hash table entries; power of two */
#define LZ4_HMASK	(LZ4_HSIZE - 1)
#define LZ4_CBLK	(LZ4_BLOCK + LZ4_BLOCK/8 + 64)	/* worst case */
#define MINMATCH	4
#define MFLIMIT		12		/* a match may not START this near end */
#define LASTLIT		5		/* a match may not END this near end */
#define IOBSIZE		4096

static unsigned char	src[LZ4_BLOCK];		/* one input block */
static unsigned char	cblk[LZ4_CBLK];	/* one compressed block */
static unsigned short	hash[LZ4_HSIZE];
static unsigned char	obuf[IOBSIZE];		/* frame output buffer */
static int		on;			/* bytes pending in obuf */
static int		cpos;			/* bytes written into cblk */
static int		ifd = 0, ofd = 1;

static void
die(const char *m)
{
	(void)write(2, m, strlen(m));
	(void)write(2, "\n", 1);
	exit(1);
}

/* ---- frame output (obuf) ---- */

static void
flushout(void)
{
	if (on > 0) {
		if (write(ofd, (char *)obuf, on) != on)
			die("lz4: write error");
		on = 0;
	}
}

static void
oputb(int c)
{
	if (on >= IOBSIZE)
		flushout();
	obuf[on++] = (unsigned char)c;
}

static void
oputn(const unsigned char *p, int len)
{
	while (len > 0) {
		int room = IOBSIZE - on;
		int c = len < room ? len : room;

		memcpy(obuf + on, p, c);
		on += c;
		p += c;
		len -= c;
		if (on >= IOBSIZE)
			flushout();
	}
}

static void
oput32(unsigned long v)
{
	oputb((int)(v & 0xFF));
	oputb((int)((v >> 8) & 0xFF));
	oputb((int)((v >> 16) & 0xFF));
	oputb((int)((v >> 24) & 0xFF));
}

/* ---- block output (cblk) ---- */

static void
eputb(int c)
{
	cblk[cpos++] = (unsigned char)c;
}

static void
eputn(const unsigned char *p, int len)
{
	memcpy(cblk + cpos, p, len);
	cpos += len;
}

/*
 * xxHash-32, seed form, for inputs under 16 bytes - which is every LZ4
 * frame header.  All arithmetic is in UINT32, which types.h makes
 * exactly 32 bits on both the 16-bit guest (unsigned long) and the
 * 64-bit host (unsigned int), so no masking is needed.  The constants
 * carry an L suffix rather than U: this compiler rejects the unsigned
 * suffix, and the signed long's bit pattern converts to UINT32 cleanly.
 */
static UINT32
xxh32(const unsigned char *p, int len, UINT32 seed)
{
	UINT32 h, v;

	h = seed + (UINT32)0x165667B1L;			/* prime 5 */
	h += (UINT32)len;
	while (len >= 4) {
		v = (UINT32)p[0] | ((UINT32)p[1] << 8) |
		    ((UINT32)p[2] << 16) | ((UINT32)p[3] << 24);
		h += v * (UINT32)0xC2B2AE3DL;		/* prime 3 */
		h = (((h << 17) | (h >> 15)) * (UINT32)0x27D4EB2FL);
		p += 4;
		len -= 4;
	}
	while (len > 0) {
		h += (UINT32)p[0] * (UINT32)0x165667B1L;	/* prime 5 */
		h = (((h << 11) | (h >> 21)) * (UINT32)0x9E3779B1L);
		p += 1;
		len -= 1;
	}
	h ^= h >> 15;
	h *= (UINT32)0x85EBCA77L;			/* prime 2 */
	h ^= h >> 13;
	h *= (UINT32)0xC2B2AE3DL;			/* prime 3 */
	h ^= h >> 16;
	return h;
}

/*
 * Hash four bytes into a table index.  A 16-bit friendly mix; the
 * distribution matters only to the ratio, never to correctness.
 */
static unsigned
hash4(const unsigned char *p)
{
	unsigned h;

	h = (unsigned)p[0] | ((unsigned)p[1] << 8);
	h ^= h << 6;
	h = (h >> 3) ^ ((unsigned)p[2] | ((unsigned)p[3] << 8));
	h ^= h << 5;
	return h & LZ4_HMASK;
}

/*
 * One sequence into cblk: litlen literals, then - unless matchlen is 0,
 * which marks the block's final literals-only sequence - a match of
 * matchlen bytes at offset, little-endian.
 */
static void
emitseq(const unsigned char *lit, int litlen, int matchlen, unsigned offset)
{
	int token, ml;

	token = (litlen >= 15 ? 15 : litlen) << 4;
	ml = 0;
	if (matchlen > 0) {
		ml = matchlen - MINMATCH;
		token |= (ml >= 15 ? 15 : ml);
	}
	eputb(token);

	if (litlen >= 15) {
		int r = litlen - 15;
		while (r >= 255) {
			eputb(255);
			r -= 255;
		}
		eputb(r);
	}
	eputn(lit, litlen);

	if (matchlen > 0) {
		eputb((int)(offset & 0xFF));
		eputb((int)((offset >> 8) & 0xFF));
		if (ml >= 15) {
			int r = ml - 15;
			while (r >= 255) {
				eputb(255);
				r -= 255;
			}
			eputb(r);
		}
	}
}

/*
 * Compress src[0..n) into cblk, returning the compressed length.  The
 * hash table is cleared each block: blocks are independent, so a match
 * may not reach into an earlier block.
 */
static int
compressblock(int n)
{
	int ip = 0, anchor = 0;
	int mflimit = n - MFLIMIT;
	int matchlimit = n - LASTLIT;

	cpos = 0;
	memset(hash, 0, sizeof(hash));

	while (ip < n) {
		if (ip <= mflimit) {
			unsigned h = hash4(src + ip);
			unsigned hpos = hash[h];
			unsigned diff;

			hash[h] = (unsigned short)ip;
			diff = (unsigned)(ip - (int)hpos);
			if (diff >= 1 && diff <= LZ4_MASK &&
			    src[ip] == src[ip - (int)diff] &&
			    src[ip + 1] == src[ip + 1 - (int)diff] &&
			    src[ip + 2] == src[ip + 2 - (int)diff] &&
			    src[ip + 3] == src[ip + 3 - (int)diff]) {
				int ml = MINMATCH;
				int k;

				while (ip + ml < matchlimit &&
				    src[ip + ml] == src[ip + ml - (int)diff])
					ml++;
				emitseq(src + anchor, ip - anchor, ml, diff);
				for (k = ip + 1; k + MINMATCH <= ip + ml; k++)
					hash[hash4(src + k)] =
					    (unsigned short)k;
				ip += ml;
				anchor = ip;
				continue;
			}
		}
		ip++;
	}
	emitseq(src + anchor, n - anchor, 0, 0);
	return cpos;
}

/*
 * Read one block (up to LZ4_BLOCK bytes) into src.  A short read is not
 * the end by itself - stdin may be a pipe - so keep reading until the
 * block is full or read() returns 0.
 */
static int
readblock(void)
{
	int got = 0;

	while (got < LZ4_BLOCK) {
		int n = read(ifd, (char *)(src + got), LZ4_BLOCK - got);

		if (n < 0)
			die("lz4: read error");
		if (n == 0)
			break;
		got += n;
	}
	return got;
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	char *in, *out;
	unsigned char hdr[7];
	unsigned long hc;

	in = "-";
	out = "-";
	if (argc > 3)
		die("lz4: usage: lz4 [infile [outfile]]");
	if (argc >= 2)
		in = argv[1];
	if (argc >= 3)
		out = argv[2];

	if (strcmp(in, "-") != 0) {
		ifd = open(in, 0);
		if (ifd < 0)
			die("lz4: cannot open input");
	}
	if (strcmp(out, "-") != 0) {
		ofd = creat(out, 0666);
		if (ofd < 0)
			die("lz4: cannot create output");
	}

	/* frame header: magic, FLG, BD, then the header checksum */
	hdr[0] = 0x04;
	hdr[1] = 0x22;
	hdr[2] = 0x4D;
	hdr[3] = 0x18;
	hdr[4] = 0x40;			/* FLG: version 01, independent, bare */
	hdr[5] = 0x40;			/* BD: block size id 4 (64 KB) in bits 6-4 */
	hc = xxh32(hdr + 4, 2, 0);	/* checksum covers FLG and BD, not magic */
	hdr[6] = (unsigned char)((hc >> 8) & 0xFF);
	oputn(hdr, 7);

	for (;;) {
		int n = readblock();
		int csize;

		if (n == 0)
			break;
		csize = compressblock(n);
		if (csize < n) {
			oput32((unsigned long)csize);
			oputn(cblk, csize);
		} else {
			/* compression did not shrink it: store raw */
			oput32((unsigned long)n | 0x80000000L);
			oputn(src, n);
		}
	}

	oput32(0);			/* end mark */
	flushout();
	exit(0);
}
