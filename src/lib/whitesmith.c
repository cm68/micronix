/*
 * library for dealing with whitesmith's object file
 *
 * lib/whitesmith.c
 * Changed: <2021-12-23 15:45:52 curt>
 */
#include <stdio.h>
#include "../micronix/include/types.h"
#include "../micronix/include/obj.h"
#include "../include/ws.h"

/*
 * a binary output function callable from printf.
 * it returns a pointer to a static array that has enough space for
 * up to PONGS distinct byte values.  so you can write things like:
 *
 * printf("%s %s %4s %2s\n", binout(34), binout(55), binout(77)+4, binout(2)+6);
 */
char *
binout(unsigned char b)
{
#define	PONGS	4
	static char buf[9 * PONGS];
	static char pong = 0;
	unsigned char i;

	pong = (pong + 1) % PONGS;

	buf[(9 * pong) + 8] = 0;

	for (i = 0; i < 8; i++) {
		buf[(pong * 9) + i] = (b & (1 << (7 - i))) ? '1' : '0';
	}
	return (&buf[pong * 9]);
}

/*
 * whitesmith's relocation entries are variable length, so this function
 * returns a relocation entry and bumps up a pointer to consume the bytes
 * used by the reloc.
 * it returns a pointer to a static reloc structure or null if no more.
 * it's up to the caller to copy out of the static struct
 *
 * location is modified as a side-effect
 *
 * relocation control bytes take the following values:
 * 0 : 		end of relocs
 * 1 - 31 :	skip 1 - 31 bytes
 * 32 - 63 : 	skip 0 to 31 pages, plus the next byte
 * 64 :		unused
 * 68 : 	text offset
 * 72 :		data offset
 * 76 :		unused
 * 80 - 248 :	symbol table entry 0 - 42
 * 252 :	symbol table 43 and beyond read next byte
 *     0 - 127   symbol table entries 43 - 170
 *     128-256   symbol table entries 256 * (n - 128) + 175 + next byte
 */
struct ws_reloc *
readreloc(unsigned char **rp)
{
	static struct ws_reloc reloc;
	unsigned char control;

	while (1) {
		control = *(*rp)++;

		/* no more relocs */
		if (control == 0) {
			return (struct ws_reloc *)0;
		}

		/* one byte skips */
		if (control < 32) {
			location += control;
			continue;
		}

		/* two byte skips */
		if (control < 64) {
			location += 32 + (256 * (control - 32));
			location += *(*rp)++;
			continue;
		}

		/* not less than 64, must be relocation type */
		if (control & 0x03) {
			printf("low control bit set %x\n", control);
		}

		/* a relocation  - let's strip the bias and the low bits */
		control -= 64;
		control >>= 2;

		/* these are just words that add in the segment base */
		if (control == REL_TEXTOFF || control == REL_DATAOFF) {
			reloc.offset = location;
			reloc.type = control;
			location += 2;
			reloc.value = 0;
			return (&reloc);
		}

		/* control now becomes a symbol table index. make it zero-based */
		control -= 4;

		/* one byte extended */
		if (control == REL_EXTEND) {
			control = *(*rp)++;
			if (control < 128) {
				control += REL_EXTEND;
			} else {
				/* 2 byte extended */
				control = ((control - 128) * 256) + 175 + *(*rp)++;
			}
		}

		/* return a symbol reference */
		reloc.type = REL_SYMBOL;
		reloc.offset = location;
		reloc.value = control;

		location += 2;
		return (&reloc);
	}
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
