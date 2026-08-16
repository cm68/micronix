/*
 * lexread.c - Lexeme stream reader for cc1
 *
 * Reads preprocessed binary token stream from .x files produced by cpp.
 * Token values from cpp match pass1's token.h directly (no translation).
 *
 * Binary format:
 *   Simple tokens: single byte (values match token.h)
 *   Keywords:      single byte (128-160, match token.h directly)
 *   SYM (20):      20 + 1-byte length + name bytes
 *   NUMBER (21):   21 + 4-byte little-endian value
 *   STRING (22):   22 + 2-byte LE length + bytes
 *   LNUMBER (25):  25 + 4-byte little-endian value
 *   LABEL (112):   112 + 1-byte length + name bytes
 *   LINENO (116):  116 + 2-byte LE line + 1-byte len + filename
 *   NEWLINE (117): 117 (line increment by 1)
 *   ASMSTR (118):  118 + 2-byte LE length + bytes
 */
#include <stdlib.h>
#include <string.h>
#include "p1core.h"
#include "p1name.h"
#include "p1lex.h"
/*
 * two constants are all fcntl.h and unistd.h were bought for, and
 * on the 64K machine a header's macro and intern load is charged
 * to every file that names it - see the footprint gate.
 */
#ifdef CCC
#define O_RDONLY 0
#define SEEK_SET 0
extern int open();
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef DEBUG
unsigned long lexTokenCount = 0;  /* Token counter for debugging */
#endif

/* CPP special token values (not simple pass-through) */
#define CPP_LNUMBER 25
#define CPP_SYMID   26
#define CPP_LABELID 27
#define CPP_AMPER   35      /* CPP uses AMPER, pass1 uses AND */
#define CPP_TIMES   42      /* CPP uses TIMES, pass1 uses STAR */
#define CPP_LINENO  116
#define CPP_NEWLINE 117
#define CPP_ASMSTR  118

/* Token lookahead - visible to parser */
struct token cur, next;

/* String buffer for literals */
char strbuf[STRBUFSIZE];

/* Globals for error reporting */
char *filename;
int lineno = 0;
static char filenameBuf[256];

/* Input buffer */
static int lexFd = -1;
static unsigned char lexBuf[512];
static int lexPos = 0;
static int lexValid = 0;
/*
 * Where lexBuf[0] sits in the file, so a position can be named.  The
 * stream is read a block at a time and two tokens ahead, so "where we
 * are" and "where the token in hand began" are different places -
 * curStart and nextStart carry the second, which is the one anything
 * wanting to come back here needs.
 */
static long lexBase = 0;
long curStart = 0, nextStart = 0;

/* Keyword tokens (128-160) now pass through directly from cpp */

/*
 * Read a byte from the lexeme stream
 * Returns E_O_F (0) on end of file
 */
unsigned char
readByte(void)
{
	if (lexPos >= lexValid) {
		lexBase += lexValid;
		lexValid = read(lexFd, lexBuf, sizeof(lexBuf));
		lexPos = 0;
		if (lexValid <= 0)
			return E_O_F;
	}
	return lexBuf[lexPos++];
}

/*
 * Read N bytes into buffer.
 *
 * The count is a byte because every record that carries one says it
 * in a byte: names and labels are counted in one, strings and
 * filenames are capped under 256.  Inline assembly is the one
 * record with a 16-bit count, and its case chunks the copy rather
 * than making every other caller pay 16-bit loop arithmetic - the
 * int counter this replaces was compared signed, a word at a time
 * with an overflow fixup, once around a loop whose body is one
 * call.  A byte counted down needs no compare at all: dec sets Z
 * itself, and in B it is the top half of djnz.
 */
void
readBytes(char *buf, unsigned char n)
{
	unsigned char i, m;

	m = n;
	for (i = 0; i != m; i++)
		buf[i] = readByte();
}

/*
 * Read 2-byte little-endian value
 */
int
readLE2(void)
{
	unsigned char lo = readByte();
	unsigned char hi = readByte();
	return lo | (hi << 8);
}

/*
 * Read 4-byte little-endian value into next.v.numeric
 *
 * Placed rather than shifted, and delivered rather than returned.
 * The obvious loop -
 *
 *	val |= ((unsigned long)readByte()) << (i * 8);
 *
 * - costs a call to the variable long shift helper and another to
 * the long or on every pass.  Writing the bytes where they go costs
 * the four reads, which are the only part that was ever necessary.
 *
 * It used to write them into a local union and return the long, but
 * a long return travels in HL:DE and both callers immediately
 * stored it in next.v.numeric - four register loads and four stores
 * to move a value that was already sitting in memory.  Longs are
 * expensive on this machine and rare in the source, so a function
 * that produces one delivers it to memory and returns nothing.
 *
 * The byte order is not an assumption being smuggled in, but it is
 * no longer little-endian: the stream carries a long laid out the way
 * this machine lays one down, high word first, so the four bytes go
 * where they fall and cpp's emit4 puts them there by hand on any host
 * that disagrees.  It was little-endian by definition once, and the
 * two were the same thing until the layout moved - see QLONG.md and
 * NUXI.  The name is kept because the stream is still little-endian
 * within each word, which is the part that never moved.
 */
void
readLE4(void)
{
#ifdef CCC
	next.v.b[0] = readByte();
	next.v.b[1] = readByte();
	next.v.b[2] = readByte();
	next.v.b[3] = readByte();
#else
	unsigned char b[4];

	b[0] = readByte();
	b[1] = readByte();
	b[2] = readByte();
	b[3] = readByte();
	next.v.numeric = (long)(((unsigned long)b[1] << 24) |
	    ((unsigned long)b[0] << 16) | ((unsigned long)b[3] << 8) |
	    (unsigned long)b[2]);
#endif
}

/*
 * Spell an id, for the output streams and for gripes.  Two rotating
 * buffers so one fdprintf can hold a pair of names.
 */
char *
nameOf(unsigned short id)
{
	static char nbuf[2][16];
	static unsigned char flip;
	char *b;

	b = nbuf[flip ^= 1];
	if (id >= SYNTH)
		fmtstr(b, "str%d", id - SYNTH);
	else
		fmtstr(b, "@%d", id);
	return b;
}

/*
 * Free token resources
 */
void
freeToken(struct token *t)
{
	if (t->type == ASM) {
		if (t->v.str) {
			free(t->v.str);
			t->v.str = NULL;
		}
	}
}

/*
 * Read next token into 'next'
 *
 * Most token values pass through directly since cpp and pass1 now
 * use the same values. Only special tokens need handling.
 */
void
readNextToken(void)
{
	unsigned char c;
	int len;
	char *s;

	nextStart = lexBase + lexPos;
again:
	c = readByte();
	next.v.numeric = 0;

	switch (c) {
	case E_O_F:
		next.type = E_O_F;
		return;
	/* Line tracking - transparent to parser */
	case CPP_LINENO:
		lineno = readLE2();
		len = readByte();
		if (len > 0 && len < sizeof(filenameBuf)) {
			readBytes(filenameBuf, len);
			filenameBuf[len] = '\0';
			filename = filenameBuf;
		}
		goto again;

	case CPP_NEWLINE:
		lineno++;
		goto again;

	/* CPP uses AMPER (35) for &, pass1 uses AND (47) */
	case CPP_AMPER:
		next.type = AND;
		break;

	/* CPP uses TIMES (42) for *, pass1 uses STAR (36) */
	case CPP_TIMES:
		next.type = STAR;
		break;

	/*
	 * The id forms: the identifier IS the 2-byte id and its
	 * spelling lives in the .n sidecar, for c1 and the driver.
	 * Nothing here allocates and nothing later compares strings.
	 * The counted-string SYM/LABEL records went with the flag that
	 * chose them; a stream carrying one is not ours.
	 */
	case CPP_SYMID:
	case CPP_LABELID:
		next.type = (c == CPP_SYMID) ? SYM : LABEL;
		next.v.id = readLE2();
		break;

	case SYM:
		fatal(ER_WTF);

	/* Numbers - have 4-byte value.  LNUMBER stays LNUMBER: it is the
	 * only record that the source said L, and without it a constant
	 * is sized by how big it happens to be. */
	case INUMBER:
	case NUMBER:
		next.type = c;
		readLE4();
		break;

	case CPP_LNUMBER:
		next.type = LNUMBER;
		readLE4();
		break;

	/* String - 2-byte length + bytes */
	case STRING:
		len = readLE2();
		if (len >= STRBUFSIZE - 1) {
			next.type = E_O_F;
			return;
		}
		/* Counted string format: first byte is length */
		strbuf[0] = len;
		readBytes(strbuf + 1, len);
		next.type = STRING;
		next.v.str = strbuf;
		break;

	case LABEL:
		fatal(ER_WTF);

	/* Inline assembly - 2-byte length + bytes */
	case CPP_ASMSTR:
		/*
		 * The length field is two bytes but the value never exceeds
		 * 255: cpp splits asm text at line boundaries so that every
		 * counted record in the stream fits the byte counter
		 * readBytes runs on.  Not guarded - the buffer is sized from
		 * the full value, so a stream that breaks the contract reads
		 * as garbage tokens, not as a memory overrun, and a 16-bit
		 * compare here would tax every asm record for a stream no
		 * in-tree producer writes.
		 */
		len = readLE2();
		s = galloc(len + 1);
		readBytes(s, len);
		s[len] = '\0';
		next.type = ASM;
		next.v.str = s;
		break;

	default:
		/* All other tokens pass through directly */
		next.type = c;
		break;
	}
}

/*
 * Shift the lookahead down.  A structure assignment is what this
 * wants to be, and the compiler does not have those - they are on the
 * list of things this tree may not use, precisely so that it can
 * compile itself.  So the fields go across one at a time, the union
 * through its widest member, which covers every other.
 */
void
shifttok(void)
{
	cur.type = next.type;
	cur.v.numeric = next.v.numeric;
	curStart = nextStart;
}

/*
 * The offset the token in hand began at, and a seek back to one.
 * lexSeek re-primes, so on return cur is the token that started there
 * and next is the one after it, exactly as when the mark was taken.
 */
long
lexTell(void)
{
	return curStart;
}

void
lexSeek(long off)
{
	freeToken(&cur);
	freeToken(&next);
	lseek(lexFd, off, SEEK_SET);
	lexBase = off;
	lexPos = 0;
	lexValid = 0;
	readNextToken();
	shifttok();
	readNextToken();
}

/*
 * The scores cpp put in the .n sidecar: one bit per id, set when the
 * stream mentions that name exactly once.  A name mentioned once is
 * mentioned only where it is declared, so nothing refers to it and a
 * declaration of it need not be remembered - see declaration(), which
 * is where the entry is dropped instead of kept.
 *
 * Nothing here is required.  If the sidecar is missing or short (cpp
 * run without -j, a hand-made .x), the map stays empty, idOnce says
 * no to everything, and c0 keeps every name as it always did.
 */
static unsigned char *scoreMap;
static unsigned short scoreN;

static void
loadScores(char *fn)
{
	char nf[64];
	unsigned char b[2];
	int fd, nb, n;

	n = strlen(fn);
	if (n < 3 || n >= (int)sizeof(nf))
		return;
	strcpy(nf, fn);
	if (nf[n - 2] != '.')		/* base.x -> base.n */
		return;
	nf[n - 1] = 'n';

	fd = open(nf, O_RDONLY);
	if (fd < 0)
		return;
	if (read(fd, (char *)b, 2) != 2) {
		close(fd);
		return;
	}
	n = b[0] | (b[1] << 8);
	nb = (n + 7) / 8;
	if (n > 0) {
		scoreMap = (unsigned char *)galloc(nb);
		lseek(fd, (long)(2 + 2 * n), SEEK_SET);
		if (read(fd, (char *)scoreMap, nb) == nb)
			scoreN = n;
		else
			scoreMap = 0;	/* short file: trust nothing */
	}
	close(fd);
}

/*
 * Is this name mentioned exactly once in the whole stream?
 */
int
idOnce(unsigned short id)
{
	if (!scoreMap || id == 0 || id > scoreN)
		return 0;
	id--;
	return (scoreMap[id >> 3] >> (id & 7)) & 1;
}

/*
 * Open lexeme file and prime the token stream
 */
void
lexOpen(char *fn)
{
	loadScores(fn);
	lexFd = open(fn, O_RDONLY);
	if (lexFd < 0) {
		char buf[80], *p;
		p = fmtstr(buf, "cannot open lexeme file: %s\n", fn);
		write(2, buf, p - buf);
		exit(1);
	}

	filename = "(unknown)";
	lineno = 0;

	/* Prime the token stream - need two reads */
	readNextToken();     /* Fill next */
	/* Shift to cur, fill next */
	shifttok();
	readNextToken();
}

/*
 * Close lexeme file
 */
void
lexClose(void)
{
	if (lexFd >= 0) {
		close(lexFd);
		lexFd = -1;
	}
}

/*
 * Rewind lexeme file to start for phase 2
 */
void
lexRewind(void)
{
	/* Reset line tracking */
	filename = "(unknown)";
	lineno = 0;

	/* the start of the file is just a position like any other */
	lexSeek(0);
}

/*
 * Get next token - shifts next into cur, reads new next
 */
void
gettoken(void)
{
#ifdef DEBUG
	lexTokenCount++;
#endif
	/* Free old cur if it had allocated memory */
	freeToken(&cur);

	/* Shift next to cur */
	shifttok();

	/* Read new next */
	readNextToken();
}

/*
 * Check if current token matches and consume it if so
 */
char
match(token_t t)
{
	if (cur.type == t) {
		gettoken();
		return 1;
	}
	return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
