/*
 * crypt - the password hash
 *
 * lib/libc/crypt.c
 *
 * login's own code until now, and moved here for the reason login's
 * note gave: in the 1982 image this is a library member and login is
 * only its caller.  The addresses the code was read out of are kept,
 * and they are login's, not this file's - 0x0c37 to 0x0f07, with the
 * seed strings in login's literal pool at 0x0c17 and 0x0c28.
 *
 * This is NOT the DES crypt.  The DES one's object is in libwsc.a and
 * is nowhere in the login image; "Here is is !?!" is the signature of
 * the older hash, so what is below is the big-multiply one: two
 * twelve-byte key blocks, seventeen rounds of a cyclic 3x3
 * multiply-accumulate, then the two salt characters and eleven
 * filtered ones.  Two passwords that hash alike to the same string
 * still collide the same way they always did.
 *
 * The two blocks are three 4-byte words each and the rounds rewrite
 * block2 IN PLACE, word by word, so a later word of a round sees the
 * words already written in that same round - which is why the three
 * statements are sequential and not one expression.  The order was
 * read off 0x0cd8-0x0e3b, where word 0 is written back at 0x0d41
 * before word 1 reads block2+0 at 0xda7.
 *
 * key1 is sixteen bytes and the copy of the sixteen-character seed
 * writes seventeen.  That is what the binary does: the NUL lands on
 * key2[0] and key2's own copy, which follows immediately, overwrites
 * it.  It is left alone here rather than corrected.
 *
 * The answer is in a static buffer that the next call overwrites.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include	<types.h>
#include	<string.h>

/* the twelve bytes of a key block, seen as three 4-byte words */
#define W(p)    (*(long *)(p))

static char crypt_buf[16];              /* 0x608a - the answer */
static char key1[16];                   /* 0x609a */
static char key2[16];                   /* 0x60aa */

char *
crypt(pw, salt)
char *pw;
char *salt;
{
	register int i;
	register char c;
	char *o;

	strcpy(key1, "abcdefghijklmnop");   /* 0x0c50 */
	strcpy(key2, "Here is is !?!");     /* 0x0c62 */

	key1[0] = salt[0];                  /* 0x0c74 - the salt goes in ... */
	key1[1] = salt[1];                  /* 0x0c7b */
	for (i = 0; pw[i]; i++)             /* 0x0c8d - ... and the password */
		key1[i % 12] += pw[i];      /*         is added to it */

	for (i = 0; i < 17; i++) {          /* 0x0cd8 - seventeen rounds */
		W(key2 + 0) = W(key2 + 0) * W(key1 + 0)
			    + W(key2 + 4) * W(key1 + 4)
			    + W(key2 + 8) * W(key1 + 8);
		W(key2 + 4) = W(key2 + 4) * W(key1 + 0)
			    + W(key2 + 8) * W(key1 + 4)
			    + W(key2 + 0) * W(key1 + 8);
		W(key2 + 8) = W(key2 + 8) * W(key1 + 0)
			    + W(key2 + 0) * W(key1 + 4)
			    + W(key2 + 4) * W(key1 + 8);
	}

	o = crypt_buf;                      /* 0x0e3b - the output */
	*o++ = *salt++;                     /* 0x0e41 - the salt, twice */
	*o++ = *salt++;
	for (i = 0; i < 11; i++) {          /* 0x0e80 - eleven characters */
		c = key2[i];
		if (c < 0)                  /* 0x0e9b - |c| */
			c = -c;
		if (c <= 0x20)              /* 0x0ec5 - keep it printable */
			c += 0x21;
		if (c == 0x7f || c == ':')  /* 0x0ed6 - not DEL, not ':' */
			c--;
		*o++ = c;                   /* 0x0eee */
	}
	*o = 0;                             /* 0x0eb8 */
	return (crypt_buf);                 /* the base, not the cursor */
}
